/// @file FeedSubtableWriter.cc
///
/// @copyright (c) 2012 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "ingestpipeline/mssink/FeedSubtableWriter.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include <askap/AskapUtil.h>

// boost includes
#include <boost/noncopyable.hpp>

// casacore includes
#include "casacore/ms/MeasurementSets/MSColumns.h"

ASKAP_LOGGER(logger, ".FeedSubtableWriter");

namespace askap {
namespace cp {
namespace ingest {

/// @brief constructor
FeedSubtableWriter::FeedSubtableWriter() : itsNumberOfAntennas(-1), itsUpdateCounter(0u),
       itsStartRowForLastUpdate(0u), itsStartTimeForLastUpdate(0.), itsOffsetsChanged(false)
{
}

/// @brief define antenna
/// @details For simplicity only support case of consecutive antenna indices starting from zero, although
/// MS standard supports any sparse configuration. This can be updated later, if found necessary.
/// @param[in] antennaId zero-based index of an antenna included in the configuration
void FeedSubtableWriter::defineAntenna(int antennaId)
{
   ASKAPASSERT(antennaId >= 0);
   ASKAPCHECK(itsUpdateCounter == 0u, "Attempted to define antenna after FEED subtable has already been written in some form - this is not supported");
   if (itsNumberOfAntennas < 0) {
       ASKAPCHECK(antennaId == 0, "Expect that antnena with Id=0 will be added first");
       itsNumberOfAntennas = 1;
   } else {
       ++itsNumberOfAntennas;
       ASKAPCHECK(antennaId + 1 == itsNumberOfAntennas, "Sparse antenna indices are not supported");
   }
}

/// @brief define offsets
/// @details This method sets up offsets for each beam. It has to be called before the first attempt to
/// write subtable. If offsets have been setup before, the new values are checked against stored offsets. 
/// The table is only updated if the values have changed.
/// @param[in] offsets 2 x nBeam matrix with offsets in radians for each beam
/// @note Different phase centre for different polarisations is not supported
void FeedSubtableWriter::defineOffsets(const casa::Matrix<casa::Double> &offsets)
{
   ASKAPCHECK(!itsOffsetsChanged, "Attempted to set the new beam offsets while the previous ones were not written yet");
   ASKAPASSERT(offsets.nrow() == 2u);
   if (itsUpdateCounter == 0u) {
       ASKAPDEBUGASSERT(itsOffsets.nelements() == 0u);
       itsOffsets = offsets;
       itsOffsetsChanged = true;
   } else {
       // check that offsets changed
       ASKAPDEBUGASSERT(itsOffsets.nrow() == 2u);
       if (offsets.ncolumn() != itsOffsets.ncolumn()) {
           itsOffsetsChanged = true;
       } else {
           for (casa::Matrix<casa::Double>::const_contiter ci1 = offsets.cbegin(), ci2 = itsOffsets.cbegin(); ci1 != offsets.cend(); ++ci1,++ci2) {
                ASKAPDEBUGASSERT(ci2 != itsOffsets.cend());
                if (casa::abs(*ci1 - *ci2) > offsetTolerance()) {
                    itsOffsetsChanged = true;
                    break;
                }
           }
       }
       // update cached offsets if they have been changed
       if (itsOffsetsChanged) {
           // the following ensures we are not tripped by reference semantics of casa arrays
           itsOffsets.reference(offsets.copy());
       }
   }
}

/// @brief define offsets
/// @details This method sets up offsets for each beam. It has to be called before the first attempt to
/// write subtable. If offsets have been setup before, the new values are checked against stored offsets. 
/// The table is only updated if the values have changed. This version of the method extracts the required
/// information from the FeedConfig class.
/// @param[in] feedCfg feed configuration class
/// @note Different phase centre for different polarisations is not supported
void FeedSubtableWriter::defineOffsets(const FeedConfig &feedCfg)
{
   ASKAPCHECK(!itsOffsetsChanged, "Attempted to set the new beam offsets while the previous ones were not written yet");
   const casa::uInt nBeams = feedCfg.nFeeds();
   if (itsUpdateCounter == 0u) {
       // initialise buffer
       itsOffsets.resize(2u, nBeams);
   }
   
   ASKAPASSERT(itsOffsets.ncolumn() == nBeams);
   ASKAPDEBUGASSERT(itsOffsets.nrow() == 2u);
   for (casa::uInt beam = 0; beam < nBeams; ++beam) {
        const double x = feedCfg.offsetX(beam).getValue("rad");
        const double y = feedCfg.offsetY(beam).getValue("rad");

        if (!itsOffsetsChanged) {
            if (itsUpdateCounter > 0u) {
                // check that the offsets changed
                if (casa::abs(itsOffsets(0, beam) - x) > offsetTolerance() || casa::abs(itsOffsets(1, beam) - y) > offsetTolerance()) {
                    itsOffsetsChanged = true;
                }
            } else {
                itsOffsetsChanged = true;
            }
        }

        if (itsOffsetsChanged) {
            itsOffsets(0, beam) = x;
            itsOffsets(1, beam) = y;
        }
   }
}
   

/// @brief write information into subtable if necessary
/// @details 
/// @param[in] ms reference to the measurement set
/// @param[in] time current time centroid (seconds)
/// @param[in] interval current interval/exposure (seconds)
void FeedSubtableWriter::write(casa::MeasurementSet &ms, double time, double interval)
{
   ASKAPCHECK(itsNumberOfAntennas > 0, "Number of antennas have to be setup before call to FeedSubtableWriter::write");
   // for convenience we write validity times with some time in reserve to avoid the need updating the record on
   // every correlator cycle which would be bad from performance point of view 
   const double maxObsDurationInSeconds = 48. * 3600.; 
   if (!itsOffsetsChanged)  {
       ASKAPASSERT(itsUpdateCounter > 0);
       // reuse existing records in the feed table - nothing to write
       // but check that assumption about the duration of observation still holds
       // (note, this behaviour can be improved if we want to)
       ASKAPCHECK(time < itsStartTimeForLastUpdate + maxObsDurationInSeconds + 0.5 * interval, "Current code only supports observations up to "<<maxObsDurationInSeconds/3600.<<" hours long");
       return;
   }

   ASKAPLOG_DEBUG_STR(logger, "Update number "<<itsUpdateCounter + 1<<" of the FEED table has been triggered, start row = "<<itsStartRowForLastUpdate);

   // Add to the Feed table
   casa::MSColumns msc(ms);
   casa::MSFeedColumns& feedc = msc.feed();
   const casa::uInt startRow = feedc.nrow();
 
   const double validityStartTime = time - interval * 0.5;
   // by default, the entry is valid essentially forever
   double validityCentroid = 0.;
   double validityDuration = 1e30;

   // correct time of the previous records, if necessary
   if (itsUpdateCounter > 0) {
       // the second update triggers transition to time-dependent table
       validityCentroid = validityStartTime + 0.5*maxObsDurationInSeconds;
       validityDuration = maxObsDurationInSeconds;

       ASKAPDEBUGASSERT(itsStartRowForLastUpdate < startRow);
       const double lastUpdateValidityDuration = validityStartTime - itsStartTimeForLastUpdate;
       for (casa::uInt row = itsStartRowForLastUpdate; row < startRow; ++row) {
            if (itsUpdateCounter == 1) {
                feedc.time().put(row, itsStartTimeForLastUpdate + 0.5 * lastUpdateValidityDuration);
            }
            feedc.interval().put(row, lastUpdateValidityDuration);
       }
   }

   // now the actual update
  
   const casa::uInt nBeams = itsOffsets.ncolumn();
   ASKAPDEBUGASSERT(itsOffsets.nrow() == 2u);
   ms.feed().addRow(nBeams * itsNumberOfAntennas);

   for (casa::uInt ant = 0, row = startRow; ant < static_cast<casa::uInt>(itsNumberOfAntennas); ++ant) {
        for (casa::uInt beam = 0; beam < nBeams; ++beam,++row) {
              feedc.antennaId().put(row, ant);
              feedc.feedId().put(row, beam);
              feedc.spectralWindowId().put(row, -1);
              feedc.beamId().put(row, 0);
              feedc.numReceptors().put(row, 2);

              // Feed position
              const casa::Vector<double> feedXYZ(3, 0.0);
              feedc.position().put(row, feedXYZ);

              // Beam offset
              casa::Matrix<double> beamOffset(2, 2);
              beamOffset(0, 0) = itsOffsets(0,beam);
              beamOffset(1, 0) = itsOffsets(1,beam);
              beamOffset(0, 1) = itsOffsets(0,beam);
              beamOffset(1, 1) = itsOffsets(1,beam);
              feedc.beamOffset().put(row, beamOffset);

              // Polarisation type - only support XY
              casa::Vector<casa::String> feedPol(2);
              feedPol(0) = "X";
              feedPol(1) = "Y";
              feedc.polarizationType().put(row, feedPol);

              // Polarisation response - assume perfect at the moment
              casa::Matrix<casa::Complex> polResp(2, 2);
              polResp = casa::Complex(0.0, 0.0);
              polResp(1, 1) = casa::Complex(1.0, 0.0);
              polResp(0, 0) = casa::Complex(1.0, 0.0);
              feedc.polResponse().put(row, polResp);

              // Receptor angle
              casa::Vector<double> feedAngle(2, 0.0);
              feedc.receptorAngle().put(row, feedAngle);

              // Time
              feedc.time().put(row, validityCentroid);

              // Interval 
              feedc.interval().put(row, validityDuration);
        }
   };

   // Post-conditions
   ASKAPCHECK(feedc.nrow() == (startRow + nBeams *  itsNumberOfAntennas), "Unexpected feed row count");
   itsStartTimeForLastUpdate = validityStartTime;
   itsStartRowForLastUpdate = startRow;
   ++itsUpdateCounter;
   itsOffsetsChanged = false;
}


};
};
};


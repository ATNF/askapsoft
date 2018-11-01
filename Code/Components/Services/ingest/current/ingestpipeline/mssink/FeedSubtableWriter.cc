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

// ASKAPsoft includes
#include "ingestpipeline/mssink/FeedSubtableWriter.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include <askap/AskapUtil.h>

// boost includes
#include <boost/noncopyable.hpp>

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
       ASKAPCHECK(antennaId + 1 == itsNumberOfAntennas, "Sparse antenna indices are not supported");
       ++itsNumberOfAntennas;
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
   const double offsetTolerance = 1e-13;
   const casa::uInt nBeams = feedCfg.nFeeds();
   if (itsUpdateCounter == 0u) {
       // initialise buffer
       itsOffsets.resize(2u, nBeams);
   }

   for (casa::uInt beam = 0; beam < nBeams; ++beam) {
        const double x = feedCfg.offsetX(beam).getValue("rad");
        const double y = feedCfg.offsetY(beam).getValue("rad");

        if (!itsOffsetsChanged) {
            if (itsUpdateCounter > 0u) {
                // check that the offsets changed
                if (casa::abs(itsOffsets(0, beam) - x) > offsetTolerance || casa::abs(itsOffsets(1, beam) - y) > offsetTolerance) {
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
}


};
};
};


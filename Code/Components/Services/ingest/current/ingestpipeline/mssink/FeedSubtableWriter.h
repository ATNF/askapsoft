/// @file FeedSubtableWriter.h
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

#ifndef ASKAP_CP_INGEST_FEEDSUBTABLEWRITER_H
#define ASKAP_CP_INGEST_FEEDSUBTABLEWRITER_H

// ASKAPsoft includes
#include "configuration/FeedConfig.h"
#include "casacore/casa/aips.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/casa/Arrays/Matrix.h"

// boost includes
#include <boost/noncopyable.hpp>

namespace askap {
namespace cp {
namespace ingest {

/// @brief Write details to feed subtable
/// @details To support phase tracking per beam we have to write and update FEED subtable 
/// per integration. Static solution had it written in its entirety during initialisation stage.
/// This class encapsulates all logic required. The supported case is that beam footprint is the same
/// for all antnenas, although TOS implementation by necesity sets its differently per antenna and 
/// distributes metadata that way too. The metadata receiver is implementing cross-check that all 
/// antennas have consistent setup. Here we assume it has already been done.
class FeedSubtableWriter : public boost::noncopyable {
public:

   /// @brief constructor
   FeedSubtableWriter();

   /// @brief define antenna
   /// @details For simplicity only support case of consecutive antenna indices starting from zero, although
   /// MS standard supports any sparse configuration. This can be updated later, if found necessary.
   /// @param[in] antennaId zero-based index of an antenna included in the configuration
   void defineAntenna(int antennaId);

   /// @brief define offsets
   /// @details This method sets up offsets for each beam. It has to be called before the first attempt to
   /// write subtable. If offsets have been setup before, the new values are checked against stored offsets. 
   /// The table is only updated if the values have changed.
   /// @param[in] offsets 2 x nBeam matrix with offsets in radians for each beam
   /// @note Different phase centre for different polarisations is not supported
   void defineOffsets(const casacore::Matrix<casacore::Double> &offsets);

   /// @brief define offsets
   /// @details This method sets up offsets for each beam. It has to be called before the first attempt to
   /// write subtable. If offsets have been setup before, the new values are checked against stored offsets. 
   /// The table is only updated if the values have changed. This version of the method extracts the required
   /// information from the FeedConfig class.
   /// @param[in] feedCfg feed configuration class
   /// @note Different phase centre for different polarisations is not supported
   void defineOffsets(const FeedConfig &feedCfg);

   /// @brief write information into subtable if necessary
   /// @details 
   /// @param[in] ms reference to the measurement set
   /// @param[in] time current time centroid (seconds)
   /// @param[in] interval current interval/exposure (seconds)
   void write(casacore::MeasurementSet &ms, double time, double interval);

   /// @brief obtain the number of updates to FEED table so far
   /// @return the number of updates
   inline casacore::uInt updateCounter() const { return itsUpdateCounter; }

protected:
   /// @brief tolerance to consider offset changed
   /// @return tolerance in radians used to compare offsets
   inline static double offsetTolerance() { return 1e-13;}

private:
   
   /// @brief number of antennas setup
   /// @note Negative value means antennas have not been setup yet
   int itsNumberOfAntennas;

   /// @brief counter for the number of updates to the subtable                  
   /// @details The logic is different depending on whether we have time-dependent or time-independent subtable.
   /// This counter allows to implement transition between these two situations and provide additional cross-checks
   /// @note Zero means subtable has not been written yet
   casacore::uInt itsUpdateCounter;

   /// @brief start row on the last update
   /// @details this is used to modify validity time for records corresponding to the last update.
   casacore::uInt itsStartRowForLastUpdate;

   /// @brief start time for which the last update is valid
   /// @details The MS standard uses time centroid and interval to locate records. So we have update values each 
   /// integration. This field is used in combination with the new time for an update to recompute centroid.
   /// @note This field is important when time-depenent table is written (i.e. second update and after that).
   /// Time is UTC in seconds since 0 MJD (matching the definition of the main table time column)
   casacore::Double itsStartTimeForLastUpdate;

   /// @brief matrix with 2 x nBeam offsets in radians
   casacore::Matrix<casacore::Double> itsOffsets;

   /// @brief flag that offsets have been changed and need to be updated
   bool itsOffsetsChanged;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_FEEDSUBTABLEWRITER_H


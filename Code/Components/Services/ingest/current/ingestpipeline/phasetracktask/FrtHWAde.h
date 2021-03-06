/// @file FrtHWAde.h
///
/// @copyright (c) 2010 CSIRO
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

#ifndef ASKAP_CP_INGEST_FRTHWADE_H
#define ASKAP_CP_INGEST_FRTHWADE_H

// System includes
#include <vector>

// ASKAPsoft includes
#include "cpcommon/VisChunk.h"
#include "Common/ParameterSet.h"

// Local package includes
#include "ingestpipeline/phasetracktask/IFrtApproach.h"
#include "ingestpipeline/phasetracktask/FrtCommunicator.h"
#include "ingestpipeline/phasetracktask/FrtMetadataSource.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

// casa includes
#include <casacore/casa/Arrays/Matrix.h>

// boost
#include <boost/shared_ptr.hpp>

namespace askap {
namespace cp {
namespace ingest {

/// @brief fringe rotation method using ADE HW fringe rotator
/// @details A number of different approaches to fringe rotation are possible (i.e. with/without DRx,
/// with/without hw-rotator with more or with less correction in the software. It seems convenient
/// to represent all different approaches by a hierarchy of classes and get the task itself
/// responsible for just delay and rate calculation (as accurate as possible, approximations and caching
/// are done in implementations of IFrtApproach interface)
class FrtHWAde : virtual public IFrtApproach {
    public:

        /// @brief Constructor.
        /// @param[in] parset the configuration parameter set.
        /// @param[in] config configuration
        FrtHWAde(const LOFAR::ParameterSet& parset, const Configuration& config);

        /// Process a VisChunk.
        ///
        /// This method is called once for each correlator integration.
        ///
        /// @param[in] chunk    a shared pointer to a VisChunk object. The
        ///             VisChunk contains all the visibilities and associated
        ///             metadata for a single correlator integration. This method
        ///             is expected to correct visibilities in this VisChunk
        ///             as required (some methods may not need to do any correction at all)
        /// @param[in] delays matrix with delays for all antennas (rows) and beams (columns) in seconds
        /// @param[in] rates matrix with phase rates for all antennas (rows) and
        ///                  beams (columns) in radians per second
        /// @param[in] effLO effective LO frequency in Hz
        /// @note this interface stems from BETA, in particular effLO doesn't fit well with ADE
        virtual void process(const askap::cp::common::VisChunk::ShPtr& chunk,
                             const casa::Matrix<double> &delays, const casa::Matrix<double> &rates, const double effLO);

    private:
        /// @brief communicator with the python part executing OSL scripts
        FrtCommunicator itsFrtComm;

        /// @brief tolerance on the FR delay setting
        /// @details The FR delay is updated when the required value goes outside the tolerance.
        int itsDelayTolerance;

        /// @brief tolerance on the FR phase rate setting
        /// @details The FR phase rate is updated when the required value goes outside the tolerance
        int itsFRPhaseRateTolerance;

        /// @brief index of an antenna used as a reference
        casa::uInt itsRefAntIndex;

        /// @brief buffer for times, used for debugging only
        std::vector<double> itsTm;

        /// @brief previous scan number, used for debugging only
        casa::uInt itsPrevScanId;

        /// @brief phase accumulator to get phases per antenna accumulated since the last FR update
        std::vector<double> itsPhases;

        /// @brief time offset fudge factor to account for the fact that FR is updated at a different time w.r.t. correlator data stream (see #5736)
        int32_t itsUpdateTimeOffset;

        /// @brief frequency offset 
        /// @details between frequency in GUI and central frequency of correlated bandwidth (in Hz)
        double itsFreqOffset;

        /// @brief number of helper threads for the phase application
        size_t itsNumHelperThreads;
        
};

} // namespace ingest
} // namespace cp
} // namespace askap

#endif // #ifndef ASKAP_CP_INGEST_FRTHWADE_H

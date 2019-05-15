/// @file FrtSwDelays.h
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

#ifndef ASKAP_CP_INGEST_FRTSWDELAYS_H
#define ASKAP_CP_INGEST_FRTSWDELAYS_H

// ASKAPsoft includes
#include "cpcommon/VisChunk.h"
#include "Common/ParameterSet.h"

// Local package includes
#include "ingestpipeline/phasetracktask/IFrtApproach.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

// casa includes
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/Vector.h>

// boost
#include <boost/shared_ptr.hpp>

namespace askap {
namespace cp {
namespace ingest {

/// @brief simplest fringe rotation method, essentially just a proof of concept
/// @details A number of different approaches to fringe rotation are possible 
/// (i.e. with/without DRx, with/without hw-rotator with more or with less correction in 
/// software. It seems convenient to represent all different approaches by a hierarchy of
/// classes and get the task itself responsible for just delay and rate calculation 
/// (as accurate as possible, approximations and caching are done in implementations of 
/// this interface)
///
/// This particular class implements fringe rotation entirely in software. It is suitable
/// for small baselines and intended for early ADE commissioning. We will not use this class
/// in the production system
class FrtSwDelays : virtual public IFrtApproach {
    public:

        /// @brief Constructor.
        /// @param[in] parset the configuration parameter set.
        /// @param[in] config configuration
        FrtSwDelays(const LOFAR::ParameterSet& parset, const Configuration& config);

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
        ///                  beams (columns) in radians per second. Not used in this class.
        /// @param[in] effLO effective LO frequency in Hz. Note, this is BETA-specific. It is probably
        /// better to derive this from chunk if necessary.
        virtual void process(const askap::cp::common::VisChunk::ShPtr& chunk,
                             const casacore::Matrix<double> &delays, const casacore::Matrix<double> &rates, const double effLO);

    private:

        /// @brief index of an antenna used as a reference
        casacore::uInt itsRefAntIndex;
};

} // namespace ingest
} // namespace cp
} // namespace askap

#endif // #ifndef ASKAP_CP_INGEST_FRTSWDELAYS_H

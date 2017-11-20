/// @file ShadowFlagTask.h
///
/// @copyright (c) 2013 CSIRO
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

#ifndef ASKAP_CP_INGEST_SHADOWFLAGTASK_H
#define ASKAP_CP_INGEST_SHADOWFLAGTASK_H

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "cpcommon/VisChunk.h"

// std includes
#include <set>
#include <vector>
#include <string>

// Local package includes
#include "ingestpipeline/ITask.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

namespace askap {
namespace cp {
namespace ingest {

/// @brief task to flag shadowed antennas on the fly
/// @details This task assesses which antennas are shadowed by other antennas and flags corresponding baselines.
/// @note Only those antennas which are present in VisChunk (i.e. those ingest is aware of) are checked as
/// potential blockers. This task uses uvw's which have to be computed earlier in the chain
class ShadowFlagTask : public askap::cp::ingest::ITask {
    public:

        /// @brief Constructor
        /// @param[in] parset the configuration parameter set.
        /// @param[in] config configuration
        ShadowFlagTask(const LOFAR::ParameterSet& parset,
                     const Configuration& config);

        /// @brief destructor
        ~ShadowFlagTask();

        /// @brief Flag data in the specified VisChunk if necessary.
        /// @details This method applies static scaling factors to correct for FFB ripple
        ///
        /// @param[in,out] chunk  the instance of VisChunk for which the
        ///                       scaling factors will be applied.
        virtual void process(askap::cp::common::VisChunk::ShPtr& chunk);

    private:

        /// @brief set of shadowed antennas
        std::set<casa::uInt> itsShadowedAntennas;

        /// @brief dish diameter in metres for shadowing calculations
        /// @note Effective size is probably larger than geometric as antennas
        /// sense each other a bit earlier. Besides, depending on the configuration of
        /// ingest pipeline beam offsets may or may not be taken into account. Adding a
        /// metre ensures that off-axis beams are not in a different regime to boresight.
        double itsDishDiameter;

        /// @brief if true, only monitor for shadowing but don't flag
        bool itsDryRun;

        /// @brief antenna names to translate indices
        /// @details Used only for reporting
        std::vector<std::string> itsAntennaNames;

}; // ShadowFlagTask class

} // ingest
} // cp
} // askap

#endif // #ifndef ASKAP_CP_INGEST_SHADOWFLAGTASK_H


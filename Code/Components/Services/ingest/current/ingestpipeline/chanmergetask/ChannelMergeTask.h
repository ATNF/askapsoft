/// @file ChannelSelTask.h
///
/// Merge of channel space handled by adjacent ranks
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

#ifndef ASKAP_CP_INGEST_CHANNELMERGETASK_H
#define ASKAP_CP_INGEST_CHANNELMERGETASK_H

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "casa/aips.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

// I am not very happy to have MPI includes here, we may abstract this interaction
// eventually. This task is specific for the parallel case, so there is no reason to
// hide MPI for now.
#include <mpi.h>


namespace askap {
namespace cp {
namespace ingest {

/// @brief Task to merge chunks handled by adjacent ranks.
/// @details This task reduces the number of parallel streams handing the data
/// by merging visibility and flag cubes. Split in frequency is assumed.
///
/// This task requires a configuration entry in the parset passed to the
/// constructor. This configuration entry specifies how many adjacent ranks
/// are aggregated together into a single stream (handled by the first rank of 
/// the group). For example:
/// @verbatim
///    ranks2merge        = 12
/// @endverbatim
/// The above results in 12 chunks handled by consecutive ranks to be merged. The
/// total number of processes should then be an integral multiple of 12.
class ChannelMergeTask : public askap::cp::ingest::ITask {
    public:
        /// @brief Constructor.
        /// @param[in] parset   the parameter set used to configure this task.
        /// @param[in] config   configuration
        ChannelMergeTask(const LOFAR::ParameterSet& parset, const Configuration& config);

        /// @brief Destructor.
        virtual ~ChannelMergeTask();

        /// @brief Merges chunks.
        ///
        /// @param[in,out] chunk the instance of VisChunk to work with
        ///             This method manipulates the VisChunk
        ///             instance which is passed in, hence the value of the
        ///             pointer will be unchanged for the rank which continues
        ///             to handle the data (merged stream). The shared pointer
        ///             is reset for other ranks.
        virtual void process(askap::cp::common::VisChunk::ShPtr chunk);

        /// @brief should this task be executed for inactive ranks?
        /// @details If a particular rank is inactive, process method is
        /// not called unless this method returns true. Possible use cases:
        ///   - Splitting the datastream expanding parallelism, i.e
        ///     inactive rank(s) become active after this task.
        ///   - Need for collective operations 
        /// @return true, if process method should be called even if
        /// this rank is inactive (i.e. uninitialised chunk pointer
        /// will be passed to process method).
        /// @note Currently, always return true, but throw an exception if
        /// any input data stream is inactive (full implementation would involve
        /// setting up MPI communicators dynamcally, rather than in the constructor)
        virtual bool isAlwaysActive() const;

    private:

        /// @brief First channel to select
        int itsRanksToMerge;

        /// @brief MPI communicator for the group this rank belongs to
        MPI_Comm itsCommunicator;
};

}
}
}

#endif

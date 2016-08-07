/// @file BeamScatterTask.h
///
/// Scatter beams between parallel ranks. Note, this task is written for
/// experiments. This is not how the ingest pipeline was designed to operate.
/// Most likely, this approach will not scale to full ASKAP, but may be handy
/// for early science.
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

#ifndef ASKAP_CP_INGEST_BEAMSCATTERTASK_H
#define ASKAP_CP_INGEST_BEAMSCATTERTASK_H

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "casacore/casa/aips.h"
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

/// @brief Task to scatter beams
/// @details This task increases the number of parallel streams handling data 
/// by scattering beams to different streams. For simplicity, only one stream
/// is allowed to be active prior to this task and this rank will continue to
/// be active past this task (if this approach is proven to be worthwhile, 
/// we would need to rework the whole visibility corner turn and merge gather in
/// frequency with scatter in beams.
///
/// This task requires a configuration entry in the parset passed to the
/// constructor. This configuration entry specifies how many streams will exist
/// after this task. For example:
/// @verbatim
///    nstreams   = 6
/// @endverbatim
/// The above results in 6 parallel streams handling roughly 1/6 of the beam space
/// each. Obviously, total number of ranks should be equal or more than the
/// value of this parameter.
class BeamScatterTask : public askap::cp::ingest::ITask {
    public:
        /// @brief Constructor.
        /// @param[in] parset   the parameter set used to configure this task.
        /// @param[in] config   configuration
        BeamScatterTask(const LOFAR::ParameterSet& parset, const Configuration& config);

        /// @brief Destructor.
        virtual ~BeamScatterTask();

        /// @brief Splits chunks.
        ///
        /// @param[in,out] chunk the instance of VisChunk to work with
        ///             This method manipulates the VisChunk
        ///             instance which is passed in, hence the value of the
        ///             pointer may be changed 
        virtual void process(askap::cp::common::VisChunk::ShPtr& chunk);

        /// @brief should this task be executed for inactive ranks?
        /// @details If a particular rank is inactive, process method is
        /// not called unless this method returns true. Possible use cases:
        ///   - Splitting the datastream expanding parallelism, i.e
        ///     inactive rank(s) become active after this task.
        ///   - Need for collective operations 
        /// @return true, if process method should be called even if
        /// this rank is inactive (i.e. uninitialised chunk pointer
        /// will be passed to process method).
        /// @note Currently, return true before the first call and then as needed
        /// given the state of the input streams (i.e. it assumes that activity/inactivity
        /// state doesn't change throughout the observation).
        virtual bool isAlwaysActive() const;

    private:

        /// @brief local rank in the group
        /// @details Returns the rank against the local communicator, i.e.
        /// the process number in the group of processes contributing to the
        /// single output stream.
        /// @return rank against itsCommunicator
        int localRank() const;

        /// @brief Number of streams to create
        int itsNStreams;

        /// @brief MPI communicator for the group used for collectives
        MPI_Comm itsCommunicator;
};

}
}
}

#endif

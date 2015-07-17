/// @file ChannelMergeTask.cc
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

// Include own header file first
#include "ChannelMergeTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casa/aips.h"
#include "casa/Arrays/Vector.h"
#include "casa/Arrays/Cube.h"
#include "cpcommon/VisChunk.h"

// boost includes
#include "boost/shared_array.hpp"

// Local package includes
#include "configuration/Configuration.h"

ASKAP_LOGGER(logger, ".ChannelMergeTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

ChannelMergeTask::ChannelMergeTask(const LOFAR::ParameterSet& parset,
        const Configuration& config) :
    itsRanksToMerge(static_cast<int>(parset.getUint32("ranks2merge")))
{
    ASKAPLOG_DEBUG_STR(logger, "Constructor");
    ASKAPCHECK(config.nprocs() > 1,
            "This task is intended to be used in parallel mode only");
    ASKAPLOG_INFO_STR(logger, "Will aggregate data handled by "<<itsRanksToMerge<<
            " consecutive ranks");
    ASKAPCHECK(itsRanksToMerge > 1, "Number of aggregated data chunks should be more than 1!");
    ASKAPCHECK(config.nprocs() % itsRanksToMerge == 0, "Total number of MPI ranks ("<<config.nprocs()<<
               ") should be an integral multiple of selected number of ranks to merge ("<<
               itsRanksToMerge<<")");
    // just do ascending order in original ranks for local group ranks
    const int response = MPI_Comm_split(MPI_COMM_WORLD, config.rank() / itsRanksToMerge, 
               config.rank(), &itsCommunicator);
    ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_split = "<<response);
    // consistency check
    checkRanksToMerge();
}

ChannelMergeTask::~ChannelMergeTask()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
}

void ChannelMergeTask::process(VisChunk::ShPtr& chunk)
{
    checkChunkForConsistency(chunk);
    if (localRank() > 0) {
        // these ranks just send VisChunks they handle to the master (rank 0)
        sendVisChunk(chunk);
        // reset chunk as this rank now becomes inactive
        chunk.reset();
    } else {
        // this is the master process which receives the data
        receiveVisChunks(chunk);
    }
}

/// @brief receive chunks in the rank 0 process
/// @details This method implements the part of the process method
/// which is intended to be executed in rank 0 (the master process)
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::receiveVisChunks(askap::cp::common::VisChunk::ShPtr chunk) const
{
    const casa::uInt nChanOriginal = chunk->nChannel();
    // new frequency vector, visibilities and flags
    casa::Vector<casa::Double> newFreq(nChanOriginal);
    casa::Cube<casa::Complex> newVis(chunk->nRow(), nChanOriginal * itsRanksToMerge, 
                                     chunk->nPol(), casa::Complex(0.,0.));
    casa::Cube<casa::Bool> newFlag(chunk->nRow(), nChanOriginal * itsRanksToMerge, 
                                     chunk->nPol(), true);
    // receive times from all ranks to ensure consistency
    // (older data will not be copied and therefore will be flagged)
    
    // update the chunk
    chunk->resize(newVis, newFlag, newFreq);
}

/// @brief send chunks to the rank 0 process
/// @details This method implements the part of the process method
/// which is intended to be executed in ranks [1..itsRanksToMerge-1].
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::sendVisChunk(askap::cp::common::VisChunk::ShPtr chunk) const
{
}

/// @brief checks chunks presented to different ranks for consistency
/// @details To limit complexity, only a limited number of merging
/// options is supported. This method checks chunks for the basic consistency
/// like matching dimensions. It is intended to be executed on all ranks and
/// use collective MPI calls.
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::checkChunkForConsistency(askap::cp::common::VisChunk::ShPtr chunk) const
{
    ASKAPCHECK(chunk, "ChannelMergeTask currently does not support idle input streams (inactive ranks)");
    int sendBuf[3];
    sendBuf[0] = static_cast<int>(chunk->nRow());
    sendBuf[1] = static_cast<int>(chunk->nChannel());
    sendBuf[2] = static_cast<int>(chunk->nPol());
    boost::shared_array<int> receiveBuf(new int[3 * itsRanksToMerge]);

    const int response = MPI_Allgather((void*)&sendBuf, 3, MPI_INTEGER, (void*)receiveBuf.get(),
             3 * itsRanksToMerge, MPI_INTEGER, itsCommunicator);
    ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allgather = "<<response);

    for (int rank = 0; rank < itsRanksToMerge; ++rank) {
         ASKAPCHECK(sendBuf[0] == receiveBuf[3 * rank], "Number of rows "<<chunk->nRow()<<
                " is different from that of rank "<<rank<<" ("<<receiveBuf[3 * rank]<<")");
         ASKAPCHECK(sendBuf[1] == receiveBuf[3 * rank + 1], "Number of channels "<<chunk->nChannel()<<
                " is different from that of rank "<<rank<<" ("<<receiveBuf[3 * rank + 1]<<")");
         ASKAPCHECK(sendBuf[2] == receiveBuf[3 * rank + 2], "Number of polarisations  "<<chunk->nPol()<<
                " is different from that of rank "<<rank<<" ("<<receiveBuf[3 * rank + 2]<<")");
    }
}

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
bool ChannelMergeTask::isAlwaysActive() const
{
   return true;
}


/// @brief local rank in the group
/// @details Returns the rank against the local communicator, i.e.
/// the process number in the group of processes contributing to the
/// single output stream.
/// @return rank against itsCommunicator
int ChannelMergeTask::localRank() const
{
   int rank;
   const int response = MPI_Comm_rank(itsCommunicator, &rank);
   
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_rank = "<<response);
   return rank;
}

/// @brief checks the number of ranks to merge against number of ranks
/// @details This method obtains the number of available ranks against
/// the local communicator, i.e. the number of streams to merge and checks
/// that it is the same as itsRanksToMerge.
void ChannelMergeTask::checkRanksToMerge() const
{
   int nprocs;
   const int response = MPI_Comm_size(itsCommunicator, &nprocs);
   
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_size = "<<response);
   ASKAPASSERT(nprocs > 0);
   ASKAPCHECK(nprocs == itsRanksToMerge, "Number of ranks available through local communicator ("<<
          nprocs<<" doesn't match the chosen number of ranks to merge ("<<itsRanksToMerge<<")");
}



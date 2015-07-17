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
   // 1) create new frequency vector, visibilities and flags

   const casa::uInt nChanOriginal = chunk->nChannel();
   casa::Vector<casa::Double> newFreq(nChanOriginal);
   casa::Cube<casa::Complex> newVis(chunk->nRow(), nChanOriginal * itsRanksToMerge, 
                                    chunk->nPol(), casa::Complex(0.,0.));
   casa::Cube<casa::Bool> newFlag(chunk->nRow(), nChanOriginal * itsRanksToMerge, 
                                    chunk->nPol(), true);

   // 2) receive times from all ranks to ensure consistency
   // (older data will not be copied and therefore will be flagged)
   
   // MVEpoch is basically two doubles
   ASKAPDEBUGASSERT(itsRanksToMerge > 1);
   boost::shared_array<double> timeRecvBuf(new double[2 * itsRanksToMerge]);
   // not really necessary to set values for the master rank, but handy for consistency
   timeRecvBuf[0] = chunk->time().getDay();
   timeRecvBuf[1] = chunk->time().getDayFraction();
   const int response = MPI_Gather(MPI_IN_PLACE, 2, MPI_DOUBLE, (void*)timeRecvBuf.get(),
            2 * itsRanksToMerge, MPI_DOUBLE, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering times, response from MPI_Gather = "<<response);

   // 3) find the latest time - we ignore all chunks which are from the past
   // (could, in principle, find time corresponding to the largest portion of valid data,
   // but probably not worth it as we don't normally expect to loose any packets).

   casa::MVEpoch latestTime(chunk->time());

   // invalid chunk flag per rank, zero length array means that all chunks are valid
   // (could've stored validity flags as opposed to invalidity flags, but it makes the
   //  code a bit less readable).
   std::vector<bool> invalidFlags;

   for (int rank = 1; rank < itsRanksToMerge; ++rank) {
        const casa::MVEpoch currentTime(timeRecvBuf[2 * rank], timeRecvBuf[2 *  rank + 1]);
        if ((latestTime - currentTime).get() < 0.) {
             latestTime = currentTime;
             // this also means that there is at least one bad chunk to exclude
             invalidFlags.resize(itsRanksToMerge, true);
        }
   }
   if (invalidFlags.size() > 0) {
       ASKAPLOG_DEBUG_STR(logger, "VisChunks being merged correspond to different times, keep only the latest = "<<latestTime);
       int counter = 0;
       for (size_t rank = 0; rank < invalidFlags.size(); ++rank) {
            const casa::MVEpoch currentTime(timeRecvBuf[2 * rank], timeRecvBuf[2 *  rank + 1]);
            if (latestTime.nearAbs(currentTime)) {
                invalidFlags[rank] = false;
                ++counter;
            }
       }
       ASKAPCHECK(counter != 0, "It looks like comparison of time stamps failed due to floating point precision, this shouldn't have happened!");
       // case of counter == itsRanksToMerge is not supposed to be inside this if-statement
       ASKAPDEBUGASSERT(counter < itsRanksToMerge);
       ASKAPLOG_DEBUG_STR(logger, "      - keeping "<<counter<<" chunks out of "<<itsRanksToMerge<<
                                  " merged");
   }

   // 4) receive and merge frequency axis
   {
      boost::shared_array<double> freqRecvBuf(new double[nChanOriginal * itsRanksToMerge]);
      const int response = MPI_Gather((void*)chunk->frequency().data(), nChanOriginal, MPI_DOUBLE, 
                (void*)freqRecvBuf.get(), nChanOriginal * itsRanksToMerge, MPI_DOUBLE, 
                0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Error gathering frequencies, response from MPI_Gather = "<<response);
      
      for (int rank = 0; rank < itsRanksToMerge; ++rank) {
           // always merge frequencies, even if the data are not valid
           casa::Vector<double> thisFreq;
           thisFreq.takeStorage(casa::IPosition(1,nChanOriginal), freqRecvBuf.get() + rank * nChanOriginal, casa::SHARE);
           newFreq(casa::Slice(rank * nChanOriginal, nChanOriginal)) = thisFreq;
      }
   }

   // 5) receive and merge visibilities (each is two floats)
   {
      boost::shared_array<float> visRecvBuf(new float[2 * chunk->visibility().nelements() * itsRanksToMerge]);
      ASKAPASSERT(chunk->visibility().contiguousStorage());
      const int response = MPI_Gather((void*)chunk->visibility().data(), chunk->visibility().nelements() * 2, 
            MPI_FLOAT, (void*)visRecvBuf.get(), chunk->visibility().nelements() * 2 * itsRanksToMerge,
            MPI_FLOAT, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Error gathering visibilities, response from MPI_Gather = "<<response);
     
      // it is a bit ugly to rely on actual representation of casa::Complex, but this is done
      // to benefit from optimised MPI routines
      fillCube((casa::Complex*)visRecvBuf.get(), newVis, invalidFlags);
   }
    
   // 6) receive flags (each is casa::Bool)
   {
      ASKAPASSERT(chunk->flag().contiguousStorage());
      ASKAPDEBUGASSERT(sizeof(casa::Bool) == sizeof(int));
      boost::shared_array<int> flagRecvBuf(new int[chunk->flag().nelements() * itsRanksToMerge]);
      const int response = MPI_Gather((void*)chunk->flag().data(), chunk->flag().nelements(), 
            MPI_INTEGER, (void*)flagRecvBuf.get(), chunk->flag().nelements() * itsRanksToMerge,
            MPI_INTEGER, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Error gathering flags, response from MPI_Gather = "<<response);

      // it is a bit ugly to rely on actual representation of casa::Bool, but this is done
      // to benefit from optimised MPI routines
      fillCube((casa::Bool*)flagRecvBuf.get(), newFlag, invalidFlags);
   }

   // 7) update the chunk
   chunk->resize(newVis, newFlag, newFreq);

   // 8) check that the resulting frequency axis is contiguous
   if (newFreq.nelements() > 1) {
       const double resolution = (newFreq[newFreq.nelements() - 1] - newFreq[0]) / (newFreq.nelements() - 1);
       for (casa::uInt chan = 0; chan < newFreq.nelements(); ++chan) {
            const double expected = newFreq[0] + resolution * chan;
            // 1 kHz tolerance should be sufficient for practical purposes
            if (fabs(expected - newFreq[chan]) > 1e3) {
                ASKAPLOG_WARN_STR(logger, "Frequencies in the merged chunks seem to be non-contiguous, "<<
                "for resulting channel = "<<chan<<" got "<<newFreq[chan]/1e6<<" MHz, expected "<<
                expected / 1e6<<" MHz, estimated resolution "<<resolution / 1e3<<" kHz");
                break;
            }
       }
   }
}

/// @brief helper method to copy data from flat buffer
/// @details MPI routines work with raw pointers. This method encasulates
/// all ugliness of marrying this with casa cubes.
/// @param[in] buf contiguous memory buffer with stacked individual slices side to side
/// @param[in,out]  out output cube, number of channels is itsRanksToMerge times the number
///                 of channels in each slice (same number of rows and polarisations)
/// @param[in] invalidFlags if this vector has non-zero length, slices corresponding to 
///                 'true' are not copied
template<typename T>
void ChannelMergeTask::fillCube(const T* buf, casa::Cube<T> &out, 
                       const std::vector<bool> &invalidFlags) const
{
   ASKAPDEBUGASSERT(out.ncolumn() % itsRanksToMerge == 0);
   const casa::IPosition sliceShape(3, out.nrow(), out.ncolumn() / itsRanksToMerge, out.nplane());

   for (int rank = 0; rank < itsRanksToMerge; ++rank) {
        if (invalidFlags.size() > 0) {
            if (invalidFlags[rank]) {
                continue;
            }
        }
        casa::Cube<T> currentSlice;

        // it is a bit ugly to rely on exact representation of the casa::Cube, but
        // this is the only way to benefit from optimised MPI routines
        // const_cast is required due to the generic interface,
        // we don't actually change data using the cast pointer
        currentSlice.takeStorage(sliceShape, const_cast<T*>(buf) + 
                                 rank * sliceShape.product(), casa::SHARE);

        const casa::IPosition start(3, 0, rank * sliceShape(1), 0);
        const casa::Slicer slicer(start,sliceShape);
        ASKAPDEBUGASSERT(start(1) < out.ncolumn());
              
        out(slicer) = currentSlice;
   }
}

/// @brief send chunks to the rank 0 process
/// @details This method implements the part of the process method
/// which is intended to be executed in ranks [1..itsRanksToMerge-1].
/// @param[in] chunk the instance of VisChunk to work with
void ChannelMergeTask::sendVisChunk(askap::cp::common::VisChunk::ShPtr chunk) const
{
   // 1) send times corresponding to the current chunk 

   const double timeSendBuf[2] = {chunk->time().getDay(), chunk->time().getDayFraction()};
   int response = MPI_Gather((void*)timeSendBuf, 2, MPI_DOUBLE, NULL,
            2 * itsRanksToMerge, MPI_DOUBLE, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering times, response from MPI_Gather = "<<response);

   // 2) send frequencies corresponding to the current chunk

   ASKAPASSERT(chunk->frequency().contiguousStorage());
   response = MPI_Gather((void*)chunk->frequency().data(), chunk->nChannel(), MPI_DOUBLE, NULL,
            chunk->nChannel() * itsRanksToMerge, MPI_DOUBLE, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering frequencies, response from MPI_Gather = "<<response);
   
   // 3) send visibilities (each is two floats)

   ASKAPASSERT(chunk->visibility().contiguousStorage());
   response = MPI_Gather((void*)chunk->visibility().data(), chunk->visibility().nelements() * 2, 
            MPI_FLOAT, NULL, chunk->visibility().nelements() * 2 * itsRanksToMerge,
            MPI_FLOAT, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering visibilities, response from MPI_Gather = "<<response);

   // 4) send flags (each is Bool)

   ASKAPASSERT(chunk->flag().contiguousStorage());
   ASKAPDEBUGASSERT(sizeof(casa::Bool) == sizeof(int));
   response = MPI_Gather((void*)chunk->flag().data(), chunk->flag().nelements(), 
            MPI_INTEGER, NULL, chunk->flag().nelements() * itsRanksToMerge,
            MPI_INTEGER, 0, itsCommunicator);
   ASKAPCHECK(response == MPI_SUCCESS, "Error gathering flags, response from MPI_Gather = "<<response);
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

    const int response = MPI_Allgather((void*)sendBuf, 3, MPI_INTEGER, (void*)receiveBuf.get(),
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
    // could in principle check that antenna1, antenna2, etc are consistent but it will waste
    // the resourses
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



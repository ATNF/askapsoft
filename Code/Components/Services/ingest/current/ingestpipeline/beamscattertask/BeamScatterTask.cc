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
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>

// Include own header file first
#include "BeamScatterTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"
#include "casacore/casa/Arrays/Slicer.h"
#include "cpcommon/VisChunk.h"
#include "cpcommon/CasaBlobUtils.h"
#include "Blob/BlobAipsIO.h"
#include <Blob/BlobSTL.h>
#include "utils/CasaBlobUtils.h"
#include "ingestpipeline/MPITraitsHelper.h"

// boost includes
#include "boost/shared_array.hpp"


// std includes
#include <vector>
#include <map>
#include <algorithm>

// Local package includes
#include "configuration/Configuration.h"
#include "monitoring/MonitoringSingleton.h"

// LOFAR for communications
#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>


ASKAP_LOGGER(logger, ".BeamScatterTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;
using namespace LOFAR;

// BeamScatterTask

BeamScatterTask::BeamScatterTask(const LOFAR::ParameterSet& parset,
        const Configuration& config) :
    itsNStreams(static_cast<int>(parset.getUint32("nstreams", config.nprocs()))),
    itsCommunicator(NULL),
    itsConfig(config),
    itsStreamNumber(-1)
{
    ASKAPLOG_DEBUG_STR(logger, "Constructor");
    ASKAPCHECK(config.nprocs() > 1,
            "This task is intended to be used in parallel mode only");
    ASKAPCHECK(itsNStreams > 1, "Beam scatter task doesn't make sense for a single output data stream");
    ASKAPLOG_INFO_STR(logger, "Will split beam space into "<<itsNStreams<<" data streams");
    // we implicitly assume the following in MPI code
    ASKAPASSERT(sizeof(casa::Bool) == sizeof(char));
}

BeamScatterTask::~BeamScatterTask()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
    if (itsCommunicator != NULL) {
        const int response = MPI_Comm_free(&itsCommunicator);
        ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_free = "<<response);
    }
}

void BeamScatterTask::process(VisChunk::ShPtr& chunk)
{
   if (itsCommunicator == NULL) {
       // this is the first integration, figure out which ranks are active, cache data structure info, etc
       itsStreamNumber = countActiveRanks(static_cast<bool>(chunk));
       initialiseSplit(chunk);      
   } else {
       ASKAPASSERT(itsStreamNumber >= 0);
       // consistency check that cached values are still ok
       if (localRank() == 0) {
           ASKAPASSERT(chunk);
           ASKAPCHECK(chunk->nRow() == itsBeam.nelements(), "Number of rows changed since the first iteration, this is unexpected");
           ASKAPDEBUGASSERT(itsBeam.nelements() == itsAntenna1.nelements());
           ASKAPDEBUGASSERT(itsBeam.nelements() == itsAntenna2.nelements());
           for (casa::uInt row = 0; row<chunk->nRow(); ++row) {
                ASKAPCHECK(chunk->beam1()[row] == itsBeam[row], "Beam number mismatch for row "<<row);
                ASKAPCHECK(chunk->beam2()[row] == itsBeam[row], "Beam number mismatch for row "<<row);
                ASKAPCHECK(chunk->antenna1()[row] == itsAntenna1[row], "Antenna 1 number mismatch for row "<<row);
                ASKAPCHECK(chunk->antenna2()[row] == itsAntenna2[row], "Antenna 2 number mismatch for row "<<row);
           }
       }
   }

   if (itsStreamNumber >= 0) {
       // work only with the ranks involved in redistribution
       broadcastRIFields(chunk); // this also initialises the chunk and, thus, activates the stream if necessary
       ASKAPDEBUGASSERT(chunk);
       if (localRank() > 0) {
           // copy cached fields for slave ranks - they are of the right size
           chunk->antenna1().assign(itsAntenna1.copy());
           chunk->antenna2().assign(itsAntenna2.copy());
           chunk->beam1().assign(itsBeam.copy());
           chunk->beam2().assign(itsBeam.copy());
       }
   
       ASKAPDEBUGASSERT(chunk);
   
       
       scatterVector(chunk->beam1PA());
       scatterVector(chunk->beam2PA());
       scatterVector(chunk->phaseCentre());
       scatterVector(chunk->uvw());

       scatterCube(chunk->visibility());
       scatterCube(chunk->flag());
      

       if (localRank() == 0) {
           trimChunk(chunk, itsRowCounts[0]);
       }
   }
   if (chunk) {
       ASKAPLOG_DEBUG_STR(logger, "nRow="<<chunk->nRow()<<" shape: "<<chunk->visibility().shape());
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
/// @note Currently, return true before the first call and then as needed
/// given the state of the input streams (i.e. it assumes that activity/inactivity
/// state doesn't change throughout the observation).
bool BeamScatterTask::isAlwaysActive() const
{
   // always active before the first iteration and for streams with active output
   return (itsCommunicator == NULL) || (itsStreamNumber >= 0);
}


/// @brief local rank in the group
/// @details Returns the rank against the local communicator, i.e.
/// the process number in the group of processes contributing to the
/// single output stream.
/// @return rank against itsCommunicator
int BeamScatterTask::localRank() const
{
   int rank;
   const int response = MPI_Comm_rank(itsCommunicator, &rank);
   
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_rank = "<<response);
   return rank;
}


/// @brief helper method to initialise communication patterns
/// @details It does counting of active ranks across the whole rank space,
/// figures out whether this rank stays active. Communicator is also setup
/// as required. 
/// @param[in] isActive true if this rank has active input, false otherwise
/// @return stream number handled by this rank or -1 if it is not active.
/// @note The method uses MPI collective calls and should be executed by all ranks,
/// including inactive ones.
int BeamScatterTask::countActiveRanks(bool isActive) 
{
   ASKAPDEBUGASSERT(itsConfig.rank() < itsConfig.nprocs());
   std::vector<int> activityFlags(itsConfig.nprocs(), 0);
   if (isActive) {
       activityFlags[itsConfig.rank()] = 1;
   }
   const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)activityFlags.data(),
        activityFlags.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);

   std::vector<int> recvFlags(itsConfig.nprocs(), 0);
   if (itsConfig.receivingRank()) {
       recvFlags[itsConfig.rank()] = 1;
   }
   const int response3 = MPI_Allreduce(MPI_IN_PLACE, (void*)recvFlags.data(),
        recvFlags.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   ASKAPCHECK(response3 == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response3);

   // now recvFlags and activityFlags are consistent across all ranks - figure out the role of this particular rank
   ASKAPDEBUGASSERT(activityFlags.size() > 1);
   ASKAPDEBUGASSERT(recvFlags.size() > 1);
   ASKAPDEBUGASSERT(recvFlags.size() == activityFlags.size());
   ASKAPDEBUGASSERT(std::accumulate(recvFlags.begin(), recvFlags.end(),0) == itsConfig.nReceivingProcs());
   const size_t numActive = std::count_if(activityFlags.begin(), activityFlags.end(), std::bind2nd(std::greater<int>(),0));
   ASKAPCHECK(numActive > 0, "There seems to be no inputs to this task - this shouldn't have happened");


   // build a list of ranks which get output in the order of priority.
   // in principle, the same can be done without building such a list, but for now - quick and dirty way  
   // boost::circular_buffer would probably be better than a vector here
   std::vector<size_t> ranksHandlingOutput;
   ranksHandlingOutput.reserve(itsConfig.nprocs());

   // first add non-ingesting and inactive ranks
   for (size_t rank = 0; rank < recvFlags.size(); ++rank) {
        if ((recvFlags[rank] == 0) && (activityFlags[rank] == 0)) {
            ranksHandlingOutput.push_back(rank);
        }
   }
   const size_t numNonIngestingAndInactive = ranksHandlingOutput.size();
   
   // then ingesting and inactive ranks
   for (size_t rank = 0; rank < recvFlags.size(); ++rank) {
        if ((recvFlags[rank] > 0) && (activityFlags[rank] == 0)) {
            ranksHandlingOutput.push_back(rank);
        }
   }
   const size_t numIngestingAndInactive = ranksHandlingOutput.size() - numNonIngestingAndInactive;
   ASKAPDEBUGASSERT(static_cast<int>(ranksHandlingOutput.size()) < itsConfig.nprocs());
   ASKAPDEBUGASSERT(static_cast<int>(numIngestingAndInactive + numNonIngestingAndInactive + numActive) == itsConfig.nprocs());
   ASKAPCHECK(ranksHandlingOutput.size() > 0, "Need at least one free rank to handle the output");
   //ASKAPLOG_INFO_STR(logger, "numIngestingAndInactive = "<<numIngestingAndInactive<<" numNonIngestingAndInactive = "<<numNonIngestingAndInactive<<" numActive="<<numActive);
   ASKAPDEBUGASSERT(itsNStreams > 1);
   
   // now assign groups to each rank (one group per input)
   // do it for all ranks, just as an extra consistency check
   // (although, in principle, only group this rank belongs to matters)
   // each value is the group number or groups.size() if it is uninitialised
   // later we use groups.size() as a flag of unused rank (MPI requires non-negative number)
   std::vector<int> groups(activityFlags.size(), activityFlags.size());
   
   // loop over ranks with active input and assign groups to them and appropriate service ranks
   int currentGroup = 0;
   size_t nextAvailableServiceRank = 0;
   for (size_t rank = 0; rank < activityFlags.size(); ++rank) {
        const int currentFlag = activityFlags[rank];
        // could be either 0 or 1
        ASKAPASSERT(currentFlag < 2);
        ASKAPASSERT(currentFlag >= 0);
        
        if (currentFlag) {
            groups[rank] = currentGroup;
            // need itsNStreams - 1 service ranks (one stream is handled by current rank - may change it in the future)
            for (size_t serviceRank = 0; static_cast<int>(serviceRank) < itsNStreams - 1; ++serviceRank,++nextAvailableServiceRank) {
                 ASKAPCHECK(nextAvailableServiceRank < ranksHandlingOutput.size(), "Not enough free ranks to assign the output to (trying to assign "<<itsNStreams - 1<<" service ranks for input stream "<<currentGroup<<")");
                 const size_t currentRank = ranksHandlingOutput[nextAvailableServiceRank];
                 ASKAPDEBUGASSERT(currentRank < groups.size());
                 groups[currentRank] = currentGroup;
                 if ((nextAvailableServiceRank == numNonIngestingAndInactive) && (itsConfig.rank() == 0)) {
                     ASKAPLOG_WARN_STR(logger, "Assigning output to ingesting rank due to limited number ("<<numNonIngestingAndInactive<<") of free service ranks");
                 }
            }
            ++currentGroup;
        }
   }
   ASKAPDEBUGASSERT(currentGroup == static_cast<int>(numActive));
   ASKAPDEBUGASSERT(currentGroup > 0);

   // all elements of the groups vector should be non-negative
   ASKAPASSERT(std::count_if(groups.begin(), groups.end(), std::bind2nd(std::less<int>(), 0)) == 0);

   ASKAPDEBUGASSERT(itsConfig.rank() < static_cast<int>(groups.size()));
   ASKAPASSERT(currentGroup < static_cast<int>(groups.size()));
   const int thisRankGroup = groups[itsConfig.rank()];
   if (thisRankGroup == static_cast<int>(groups.size())) {
       ASKAPLOG_DEBUG_STR(logger,"This rank will be kept deactivated");
   } else {
       if (isActive) {
           ASKAPLOG_DEBUG_STR(logger,"This rank will be kept active and feed data for the group "<<thisRankGroup);
       } else {
           ASKAPLOG_DEBUG_STR(logger,"This rank will be activated and assigned to group "<<thisRankGroup);
       }
   }
   
   
   // now create intra-group communicator
   //ASKAPLOG_DEBUG_STR(logger,"Building intra-group communicator, group = "<<thisRankGroup);

   // just do ascending order in original ranks for local group ranks, but ensure that the rank with
   // active input is put first - there should be only one rank with input per group, so just assign zero sequence number to it
   // also put ingesting ranks to the back (as they're always assigned last) - this will give beam distribution in the order of beam number
   // in the local communicator at no extra cost, just a handy feature.
   const int seqNumber = (isActive ? 0 : itsConfig.rank() + 1) + (itsConfig.receivingRank() ? itsConfig.nprocs() + 1 : 0);
   const int response2 = MPI_Comm_split(MPI_COMM_WORLD, thisRankGroup, seqNumber, &itsCommunicator);
   ASKAPCHECK(response2 == MPI_SUCCESS, "Erroneous response from MPI_Comm_split = "<<response2);

   int thisRankStream = -1;
   if (thisRankGroup < static_cast<int>(groups.size())) {
       thisRankStream = localRank();
       ASKAPLOG_INFO_STR(logger, "This rank corresponds to stream "<<thisRankStream<<" group "<<thisRankGroup);
   } else {
       ASKAPLOG_INFO_STR(logger, "This rank will not be used");
   }

   return thisRankStream;
}

/// @brief set up split and cache buffer structure
/// @details The method uses MPI collective calls within the group each
/// rank belongs to. It initialises start and stop rows for each rank.
/// @param[in] chunk the instance of VisChunk to work with
void BeamScatterTask::initialiseSplit(const askap::cp::common::VisChunk::ShPtr& chunk)
{
  if (itsStreamNumber < 0) {
      // unused rank - do nothing
      return;
  }
  ASKAPDEBUGASSERT(itsStreamNumber == localRank());
  if (localRank() == 0) {
      // figure out parameters
      ASKAPCHECK(chunk, "First stream is supposed to have input data");
      // build map beamID: start row, end row
      std::map<casa::uInt, std::pair<casa::uInt, casa::uInt> > beamRowMap;
      for (casa::uInt row = 0; row < chunk->nRow(); ++row) {
           const casa::uInt beam = chunk->beam1()[row];
           ASKAPCHECK(chunk->beam2()[row] == beam, "Correlations between different beams are not supported (row="<<row<<")");
           const std::map<casa::uInt, std::pair<casa::uInt, casa::uInt> >::iterator it = beamRowMap.find(beam);
           if (it == beamRowMap.end()) {
               beamRowMap[beam] = std::pair<casa::uInt, casa::uInt>(row, row);
           } else {
               ASKAPCHECK(row == it->second.second + 1, "Data corresponding to beam "<<beam<<" seem to spread across non-contiguous blocks of rows. Not supported.");
               it->second.second = row;
           }
      }
      ASKAPLOG_INFO_STR(logger, "Found "<<beamRowMap.size()<<" beams in this group of data streams");
      ASKAPDEBUGASSERT(itsNStreams > 0);

      // the following logic is required because there could be gaps in beam space (data container is a sparse array)

      std::vector<std::pair<casa::uInt, casa::uInt> > streamRowMap(itsNStreams, std::pair<casa::uInt, casa::uInt>(0u, 0u));
      const casa::uInt beamsPerStream = beamRowMap.size() % itsNStreams == 0 ? beamRowMap.size() / itsNStreams : beamRowMap.size() / (itsNStreams - 1);
      std::map<casa::uInt, std::pair<casa::uInt, casa::uInt> >::const_iterator ci = beamRowMap.begin();

      casa::uInt lastRow = 0;
      for (size_t stream = 0; stream < streamRowMap.size(); ++stream) {
           std::vector<casa::uInt> handledBeams; 
           for (casa::uInt beamNo = 0; beamNo < beamsPerStream; ++beamNo) {
                if (ci != beamRowMap.end()) {
                    if (beamNo == 0) {
                        streamRowMap[stream] = ci->second;
                    } else {
                        ASKAPCHECK(streamRowMap[stream].second + 1 == ci->second.first, 
                                   "Non-contiguous set of rows detected between beams "<<beamNo - 1<<" and "<<beamNo<<" - not supported");
                        streamRowMap[stream].second = ci->second.second;
                    }
                    lastRow = ci->second.second;
                    handledBeams.push_back(ci->first);
                    ++ci;
                }
           }
           // in principle, it is possible to have this operation more flexible and deactive unused streams
           // it would make configuration easier, but I am running out of time to implement this logic 
           // (and especially mpi collective calls outside the group/ensure they're called from all ranks)
           // and test it. For now, force the user to have the right config manually.
           ASKAPCHECK(handledBeams.size() > 0, "Not enough beams in the data to populate stream "<<stream);

           ASKAPLOG_INFO_STR(logger, "Stream "<<stream<<" will handle beams: "<<handledBeams<<" rows from "<<streamRowMap[stream].first<<
                                 " to "<<streamRowMap[stream].second<<", inclusive");
      }
      ASKAPDEBUGASSERT(ci == beamRowMap.end());
      ASKAPCHECK(lastRow + 1 == chunk->nRow(), "Some rows of data seem to be missing as a result of data partioning. This shouldn't happen. lastRow="
                                               <<lastRow<<" nRow="<<chunk->nRow());

      // now do collectives, matching code on slave ranks is in the else part of the if-statement

      //1) scatter rows dealt with by this particular stream (stream number is the local rank in the intra-group communicator) 
      
      ASKAPDEBUGASSERT(sizeof(std::pair<casa::uInt, casa::uInt>) == 2 * sizeof(uint32_t));
      casa::uInt tempBuf[2] = {lastRow + 1, lastRow + 1};
      const int response = MPI_Scatter((void*)streamRowMap.data(), 2, MPI_UNSIGNED, &tempBuf, 2, MPI_UNSIGNED, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatter = "<<response);
      // consistency checks - we rely on the exect structure of the data
      ASKAPASSERT(streamRowMap.size() > 0);
      itsHandledRows = streamRowMap[0];
      ASKAPASSERT(tempBuf[0] == itsHandledRows.first);
      ASKAPASSERT(tempBuf[1] == itsHandledRows.second);

      // set up arrays of row counts and offsets for MPI collectives
      itsRowCounts.resize(itsNStreams);
      itsRowOffsets.resize(itsNStreams);
      for (size_t stream = 0; stream < streamRowMap.size(); ++stream) {
           const std::pair<casa::uInt, casa::uInt> rowsForThisStream = streamRowMap[stream];
           ASKAPASSERT(rowsForThisStream.first < rowsForThisStream.second);
           itsRowCounts[stream] = rowsForThisStream.second - rowsForThisStream.first + 1;
           itsRowOffsets[stream] = rowsForThisStream.first;
      }

      // copy fixed row-based vectors
      itsAntenna1.assign(chunk->antenna1().copy());
      itsAntenna2.assign(chunk->antenna2().copy());
      itsBeam.assign(chunk->beam1().copy());
  } else {
      // slave ranks of the same communicator
      // 1) receive rows dealt with by particular stream (stream number is the local rank in the intra-group communicator) 
      const int response = MPI_Scatter(NULL, 0, MPI_UNSIGNED, &itsHandledRows, 2, MPI_UNSIGNED, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatter = "<<response);
      ASKAPLOG_DEBUG_STR(logger, "   slave rank, handling rows from "<<itsHandledRows.first<<" to "<<itsHandledRows.second<<", inclusive");
  }
  scatterVector(itsAntenna1);
  scatterVector(itsAntenna2);
  scatterVector(itsBeam);

  // don't trim vectors on the root rank here - values are used for consistency as we cache the row numbers corresponding to the beam scatter layout
}

/// @brief helper method to scatter row-based cube
/// @details MPI routines work with raw pointers. This method encapsulates
/// all ugliness of marrying this with complex casa types.
/// It relies on exact physical representation of data. It is assumed that
/// local rank 0 is the root. 
/// @param[in,out] cube cube for both input (on local rank 0) and output
/// (on other ranks of the local communicator). It is the requirement that
/// the shape is correctly initialised before calling this method.
template<typename T>
void BeamScatterTask::scatterCube(casa::Cube<T> &cube) const
{
  // due to unfavourable layout of casa::Cube data storage, we can't use the similar approach to 
  // scatterVector. At this stage, do the job with extra copy 

  ASKAPDEBUGASSERT(itsHandledRows.second > itsHandledRows.first);
  const casa::uInt expectedNumberOfRows = itsHandledRows.second - itsHandledRows.first + 1;
  const casa::uInt elementsPerRow = cube.ncolumn() * cube.nplane();
  if (localRank() == 0) {
      ASKAPDEBUGASSERT(itsNStreams > 1);
      ASKAPASSERT(itsRowCounts[0] == static_cast<int>(expectedNumberOfRows));
      ASKAPDEBUGASSERT(cube.nelements() == elementsPerRow * cube.nrow());

      // use plain memory array as std::vector has different interface for bool and non-bool types complicating
      // working with this type in a template
      typename boost::shared_array<T> sndBuffer(new T[cube.nelements()]);

      // need to scale the number of elements and the offsets up to account for both value type and other dimensions
      // it is not worth to implement simple case as in the scatterVector because we will always have other dimensions here
      std::vector<int> tempCounts(itsRowCounts);
      std::vector<int> tempOffsets(itsRowOffsets);
      // note - scaling of offsets is the same for all iterations and could be cached, if we're desperate for more performance
      for (std::vector<int>::iterator ci1 = tempCounts.begin(), ci2 = tempOffsets.begin(); 
           ci1 != tempCounts.end(); ++ci1,++ci2) {
           ASKAPDEBUGASSERT(ci2 != tempOffsets.end());
           ASKAPASSERT(*ci1 + *ci2 <= static_cast<int>(cube.nrow()));
           // use this opportunity to copy the appropriate section, we could've done it for the whole cube at once (not sure if there
           // are any benefits in terms of performance). However, separating it this way may allow parallelisation if we are desperate for
           // extra peformance and/or replacing scatter call with individual (and perhaps asynchronous) sends, when the data for the 
           // particular rank are ready).
           casa::Slicer curStreamDataSlicer(casa::IPosition(3,*ci2, 0, 0), casa::IPosition(3, *ci1, cube.ncolumn(), cube.nplane()));
           typename casa::Cube<T> curStreamData = cube(curStreamDataSlicer);
           // scale the offset and counts with elementsPerRow, now these are the offsets in flattened array of type T
           (*ci1) *= elementsPerRow;
           (*ci2) *= elementsPerRow;
           ASKAPASSERT(static_cast<int>(curStreamData.nelements()) == *ci1);
           {
              // perform the copy and keep the order the same as for the target container - this way we don't need transpose on the receive side
              typename casa::Array<T> bufReference;
              bufReference.takeStorage(curStreamData.shape(), sndBuffer.get() + (*ci2), casa::SHARE);
              bufReference = curStreamData;
           }

           // now scale the offset and counts with size for composite types as MPI routines accept a plain C view
           (*ci1) *= MPITraitsHelper<T>::size;
           (*ci2) *= MPITraitsHelper<T>::size; 
      }
      const int response = MPI_Scatterv((void*)sndBuffer.get(), tempCounts.data(), tempOffsets.data(), MPITraitsHelper<T>::datatype(), MPI_IN_PLACE,
            static_cast<int>(expectedNumberOfRows) * MPITraitsHelper<T>::size * elementsPerRow,  MPITraitsHelper<T>::datatype(), 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatterv = "<<response);
  } else {
      ASKAPASSERT(cube.nrow() == expectedNumberOfRows);
      ASKAPASSERT(cube.contiguousStorage());
      const int response = MPI_Scatterv(NULL, NULL, NULL, MPITraitsHelper<T>::datatype(), (void*)cube.data(), 
            static_cast<int>(expectedNumberOfRows) * MPITraitsHelper<T>::size * elementsPerRow,  MPITraitsHelper<T>::datatype(), 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatterv = "<<response);
  }
}

/// @brief helper method to scatter row-based vector
/// @details MPI routines work with raw pointers. This method encapsulates
/// all ugliness of marrying this with complex casa types.
/// It relies on exact physical representation of data. It is assumed that
/// local rank 0 is the root. 
/// @param[in,out] vec vector for both input (on local rank 0) and output
/// (on other ranks of the local communicator)
template<typename T>
void  BeamScatterTask::scatterVector(casa::Vector<T> &vec) const
{
  ASKAPDEBUGASSERT(itsHandledRows.second > itsHandledRows.first);
  const casa::uInt expectedSize = itsHandledRows.second - itsHandledRows.first + 1;
  if (localRank() == 0) {
      ASKAPDEBUGASSERT(itsNStreams == static_cast<int>(itsRowCounts.size()));
      ASKAPDEBUGASSERT(itsNStreams == static_cast<int>(itsRowOffsets.size()));
      ASKAPASSERT(vec.contiguousStorage());
      typename casa::Vector<T> tempBuf(expectedSize); // for cross-check
      ASKAPASSERT(tempBuf.contiguousStorage());
      ASKAPDEBUGASSERT(itsNStreams > 1);
      ASKAPASSERT(itsRowCounts[0] == static_cast<int>(expectedSize));
      if (MPITraitsHelper<T>::size == 1) {
          // can be translated to array of native MPI types
          const int response = MPI_Scatterv((void*)vec.data(), const_cast<int*>(itsRowCounts.data()), 
                       const_cast<int*>(itsRowOffsets.data()), MPITraitsHelper<T>::datatype(), (void*)tempBuf.data(),
               static_cast<int>(expectedSize) * MPITraitsHelper<T>::size,  MPITraitsHelper<T>::datatype(), 0, itsCommunicator);
          ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatterv = "<<response);
      } else {
          // need to scale the number of elements and the offsets up
          std::vector<int> tempCounts(itsRowCounts);
          std::vector<int> tempOffsets(itsRowOffsets);
          for (std::vector<int>::iterator ci1 = tempCounts.begin(), ci2 = tempOffsets.begin(); 
               ci1 != tempCounts.end(); ++ci1,++ci2) {
               ASKAPDEBUGASSERT(ci2 != tempOffsets.end());
               (*ci1) *= MPITraitsHelper<T>::size;
               (*ci2) *= MPITraitsHelper<T>::size;
          }
          const int response = MPI_Scatterv((void*)vec.data(), tempCounts.data(), tempOffsets.data(), MPITraitsHelper<T>::datatype(), (void*)tempBuf.data(),
               static_cast<int>(expectedSize) * MPITraitsHelper<T>::size,  MPITraitsHelper<T>::datatype(), 0, itsCommunicator);
          ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatterv = "<<response);
      }
      // consistency check
      for (casa::uInt row = 0; row < expectedSize; ++row) {
           ASKAPCHECK(MPITraitsHelper<T>::equal(tempBuf[row],vec[row + itsRowOffsets[0]]), "Data mismatch detected in MPI collective");
      }
  } else {
      if (vec.size() != expectedSize) {
          vec.resize(expectedSize);
      }
      ASKAPASSERT(vec.contiguousStorage());
      const int response = MPI_Scatterv(NULL, NULL, NULL, MPITraitsHelper<T>::datatype(), (void*)vec.data(), 
            static_cast<int>(expectedSize) * MPITraitsHelper<T>::size,  MPITraitsHelper<T>::datatype(), 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Scatterv = "<<response);
  }
}

/// @brief specialisation to scatter vector of MVDirections
/// @param[in,out] vec vector for both input (on local rank 0) and output
/// (on other ranks of the local communicator)
void BeamScatterTask::scatterVector(casa::Vector<casa::MVDirection> &vec) const
{
   // quick and dirty way of scattering vector of MVDirections
   // relying on internal representation is probably too dangerous here
   casa::Vector<casa::RigidVector<casa::Double, 3> > mvdBuf(vec.nelements());
   if (localRank() == 0) {
       // pack data in the buffer
       for (casa::uInt row=0; row<vec.nelements(); ++row) {
            const casa::Vector<casa::Double> representation = vec[row].getVector();
            ASKAPDEBUGASSERT(representation.nelements() == 3);
            mvdBuf[row] = representation;
       }
   }
   scatterVector(mvdBuf);
   if (localRank() > 0) {
       // unpack the results, mvdBuf should be of the right length
       if (vec.nelements() != mvdBuf.nelements()) {
           vec.resize(mvdBuf.nelements());
       }
       for (casa::uInt row=0; row<vec.nelements(); ++row) {
            vec[row].putVector(mvdBuf[row].vector());
       }
   }
}

/// @brief broadcast row-independent fields
/// @details This method handles row-independent fields, broadcasts 
/// the content within the group and initialses the chunk for streams with
/// inactive input. 
/// @param[in] chunk the instance of VisChunk to work with
void BeamScatterTask::broadcastRIFields(askap::cp::common::VisChunk::ShPtr& chunk) const
{
  ASKAPDEBUGASSERT(itsStreamNumber >= 0);
  const int formatId = 2;
  if (localRank() == 0) {
      ASKAPDEBUGASSERT(chunk);
      // as we need to pass sizes anyway, pass also basic parameters required to initialise the chunk
      // for slave ranks.
      uint32_t buffer[5] = {0u, chunk->nRow(), chunk->nChannel(), chunk->nPol(), chunk->nAntenna()};

      // 1) encode info into blob 
      LOFAR::BlobString bs;
      bs.resize(0);
      LOFAR::BlobOBufString bob(bs);
      LOFAR::BlobOStream out(bob);
      out.putStart("RowIndependentParameters", formatId);
      out << chunk->time() << chunk->targetName() << chunk->interval() << chunk->scan() << 
             chunk->targetPointingCentre() << chunk->actualPointingCentre() <<
             chunk->actualPolAngle() << chunk->actualAzimuth() << chunk->actualElevation() <<
             chunk->onSourceFlag() << chunk->frequency() << chunk->channelWidth() <<
             chunk->stokes() << chunk->directionFrame();
      out.putEnd();
      // pass the size along with basic parameters
      buffer[0] = bs.size();

      // 2) broadcast size and general info
      const int response = MPI_Bcast(&buffer, 5, MPI_UNSIGNED, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Bcast = "<<response);
   
      // 3) send encoded message
      const int response1 = MPI_Bcast(bs.data(), bs.size(), MPI_BYTE, 0, itsCommunicator);
      ASKAPCHECK(response1 == MPI_SUCCESS, "Erroneous response from MPI_Bcast = "<<response1);
 
  } else {
      // this rank should have inactive input
      ASKAPDEBUGASSERT(!chunk);
      uint32_t buffer[5] = {0u, 0u, 0u, 0u, 0u};

      // 1) receive sizes and general info
      const int response = MPI_Bcast(&buffer, 5, MPI_UNSIGNED, 0, itsCommunicator);
      ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Bcast = "<<response);
 
      // 2) initialise chunk
      ASKAPDEBUGASSERT(itsHandledRows.second > itsHandledRows.first);
      ASKAPCHECK(itsHandledRows.second < buffer[1], "Selected row numbers for this stream exceed the number of rows available");
      const casa::uInt expectedSize = itsHandledRows.second - itsHandledRows.first + 1;
      
      ASKAPLOG_DEBUG_STR(logger, "Initialising chunk for "<<buffer[2]<<" channels, "<<
                         buffer[3]<<" polarisations and "<<buffer[4]<<" antennas, but for "<<expectedSize<<" rows");
      chunk.reset(new VisChunk(expectedSize, buffer[2], buffer[3], buffer[4]));

      //  3) receive encoded message
      LOFAR::BlobString bs;
      bs.resize(buffer[0]);

      const int response1 = MPI_Bcast(bs.data(), bs.size(), MPI_BYTE, 0, itsCommunicator);
      ASKAPCHECK(response1 == MPI_SUCCESS, "Erroneous response from MPI_Bcast = "<<response1);
 
      // 4) decode the message, populate fields in chunk
      LOFAR::BlobIBufString bib(bs);
      LOFAR::BlobIStream in(bib);
      const int version=in.getStart("RowIndependentParameters");
      ASKAPASSERT(version == formatId);
      
      in >> chunk->time() >> chunk->targetName() >> chunk->interval() >> chunk->scan() >>
            chunk->targetPointingCentre() >> chunk->actualPointingCentre() >> chunk->actualPolAngle() >>
            chunk->actualAzimuth() >> chunk->actualElevation() >> chunk->onSourceFlag() >> chunk->frequency() >>
            chunk->channelWidth() >> chunk->stokes() >> chunk->directionFrame();

      in.getEnd();

      // some consistency checks
      ASKAPASSERT(chunk->actualAzimuth().nelements() == chunk->nAntenna());
      ASKAPASSERT(chunk->actualElevation().nelements() == chunk->nAntenna());
      ASKAPASSERT(chunk->actualPolAngle().nelements() == chunk->nAntenna());
      ASKAPASSERT(chunk->actualPointingCentre().nelements() == chunk->nAntenna());
      ASKAPASSERT(chunk->targetPointingCentre().nelements() == chunk->nAntenna());
      ASKAPASSERT(chunk->onSourceFlag().nelements() == chunk->nAntenna());
      ASKAPASSERT(chunk->frequency().nelements() == chunk->nChannel());
      ASKAPASSERT(chunk->stokes().nelements() == chunk->nPol());
  }
}

/// @brief trim chunk to the given number of rows
/// @details
/// @param[in,out] chunk the instance of VisChunk to work with
/// @param[in] newNRows new number of rows
void BeamScatterTask::trimChunk(askap::cp::common::VisChunk::ShPtr& chunk, casa::uInt newNRows)
{
  // note - code largely untested, only used to study performance, i.e. scientific content is not preserved / dealt with correctly yet
  ASKAPLOG_DEBUG_STR(logger, "Trimming chunk to contain "<<newNRows<<" rows");

  // it may be worth to promote this code to a member of VisChunk - copying of row-independent fields will then be unnecessary resulting in a cleaner code
  ASKAPASSERT(chunk);
  ASKAPASSERT(newNRows < chunk->nRow());
  // casa arrays don't go not allow trimming through manipulation of metadata (and for cubes it doesn't work anyway due to data order),
  // so we have to do explicit copy of the part we are after anyway.
  askap::cp::common::VisChunk::ShPtr newChunk(new VisChunk(newNRows, chunk->nChannel(), chunk->nPol(), chunk->nAntenna()));
 
  // first copy row-independent fields - for casa arrays it will be referencing
  newChunk->time() = chunk->time();
  newChunk->targetName() = chunk->targetName();
  newChunk->interval() = chunk->interval();
  newChunk->scan() = chunk->scan();
  newChunk->targetPointingCentre().assign(chunk->targetPointingCentre());
  newChunk->actualPointingCentre().assign(chunk->actualPointingCentre());
  newChunk->actualPolAngle().assign(chunk->actualPolAngle());
  newChunk->actualAzimuth().assign(chunk->actualAzimuth());
  newChunk->actualElevation().assign(chunk->actualElevation());
  newChunk->onSourceFlag().assign(chunk->onSourceFlag());
  newChunk->frequency().assign(chunk->frequency());
  newChunk->channelWidth() = chunk->channelWidth();
  newChunk->stokes().assign(chunk->stokes());
  newChunk->directionFrame() = chunk->directionFrame();

  const casa::Slicer vecSlicer(casa::IPosition(1,0), casa::IPosition(1,newNRows));
  
  // have to copy row-dependent vectors to ensure they're contiguous
  newChunk->antenna1().assign(chunk->antenna1()(vecSlicer).copy());
  newChunk->antenna2().assign(chunk->antenna2()(vecSlicer).copy());
  newChunk->beam1().assign(chunk->beam1()(vecSlicer).copy());
  newChunk->beam2().assign(chunk->beam2()(vecSlicer).copy());
  newChunk->beam1PA().assign(chunk->beam1PA()(vecSlicer).copy());
  newChunk->beam2PA().assign(chunk->beam2PA()(vecSlicer).copy());
  newChunk->phaseCentre().assign(chunk->phaseCentre()(vecSlicer).copy());
  newChunk->uvw().assign(chunk->uvw()(vecSlicer).copy());
  
  casa::IPosition shape = chunk->visibility().shape();
  ASKAPASSERT(chunk->flag().shape() == shape);
  ASKAPASSERT(shape.nelements() == 3);
  shape[0] = newNRows;
  const casa::Slicer cubeSlicer(casa::IPosition(3,0,0,0), shape);


  // have to copy row-dependent cubes to ensure they're contiguous
  newChunk->flag().assign(chunk->flag()(cubeSlicer).copy());
  newChunk->visibility().assign(chunk->visibility()(cubeSlicer).copy());

  // consistency checks
  ASKAPDEBUGASSERT(newChunk->antenna1().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->antenna2().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->beam1().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->beam2().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->beam1PA().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->beam2PA().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->phaseCentre().nelements() == newNRows);
  ASKAPDEBUGASSERT(newChunk->uvw().nelements() == newNRows);

  ASKAPDEBUGASSERT(newChunk->nRow() == newNRows);
  ASKAPASSERT(newChunk->visibility().shape()  == shape);
  ASKAPASSERT(newChunk->flag().shape()  == shape);
  
  chunk = newChunk;
} 


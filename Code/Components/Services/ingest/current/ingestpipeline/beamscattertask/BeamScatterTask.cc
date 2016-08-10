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
#include "cpcommon/VisChunk.h"
#include "cpcommon/CasaBlobUtils.h"

// boost includes
#include "boost/shared_array.hpp"
#include "boost/static_assert.hpp"


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

// helper class to encapsulate mpi-related stuff 
template<typename T>
struct MPITraitsHelper {
   BOOST_STATIC_ASSERT_MSG(sizeof(T) == 0, 
          "Attempted a build for type without traits defined");
};

template<>
struct MPITraitsHelper<casa::uInt> {
   static MPI_Datatype datatype() { return MPI_UNSIGNED; };
   static const int size = 1;
   
   static bool equal(casa::uInt val1, casa::uInt val2) { return val1 == val2;}
};


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
       itsStreamNumber = countActiveRanks(chunk);
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

   /*
   if (chunk) {
       ASKAPLOG_DEBUG_STR(logger, "This rank has an active input");
       ASKAPCHECK(itsStreamNumber == 0, "Only the first rank of the group (i.e. first stream) is expected to have an active input");
   } else {
       ASKAPLOG_DEBUG_STR(logger, "This rank has an inactive input");
       ASKAPCHECK(itsStreamNumber != 0, "First rank of the group (i.e. first stream) is expected to have an active input");
   }
   */

   if (itsStreamNumber >= 0) {
       // work only with the ranks involved in redistribution
       broadcastRIFields(chunk); // this also initialises the chunk and activates the stream if necessary
       ASKAPDEBUGASSERT(chunk);
       if (localRank() > 0) {
           // copy cached fields
           chunk->antenna1().assign(itsAntenna1.copy());
           chunk->antenna2().assign(itsAntenna2.copy());
           chunk->beam1().assign(itsBeam.copy());
           chunk->beam2().assign(itsBeam.copy());
           // temporary to allow realistic performance measurements
           chunk->flag().set(false);
       }
   
       ASKAPDEBUGASSERT(chunk);
   
       /*
       scatterVector(chunk->beam1PA());
       scatterVector(chunk->beam2PA());
       scatterVector(chunk->uvw());
       */

       // other fields and data come here
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

   // count inactive ranks trailing each active one - the assumption is that
   // active rank always comes first. In principle, more logic can be built into this but
   // it seems unnecessary at this stage.
   ASKAPDEBUGASSERT(activityFlags.size() > 1);
   ASKAPCHECK(activityFlags[0] == 1, "Expect the zero rank to be active which doesn't seem to be the case");

   // do it for all ranks, just as an extra consistency check
   // (although, in principle, only group this rank belongs to matters)
   // each value is the group number or -1 if it is uninitialised
   // later we use groups.size() as a flag of unused rank (MPI requires non-negative number)
   std::vector<int> groups(activityFlags.size(), -1);
   // start ranks for each group
   std::map<int,int> startRanksMap;

   // start rank + the group count (the value of -2 is the flag for first iteration)
   int startRank = -2, group = -1;
   for (size_t rank = 0; rank < activityFlags.size(); ++rank) {
        const int currentFlag = activityFlags[rank];
        // could be either 0 or 1
        ASKAPASSERT(currentFlag < 2);
        ASKAPASSERT(currentFlag >= 0);
        
        if (currentFlag) {
            // next group
            ++group;
            ASKAPCHECK(static_cast<int>(rank) - startRank > 1, "There seems to be no idle streams available before rank="<<rank);
            startRank = rank;
            startRanksMap[group] = startRank;
        } 
        groups[rank] = group;
   }
   
   ASKAPCHECK(group >= 0, "BeamScatterTask has no active input streams!");
   const size_t nGroups = static_cast<size_t>(group + 1);

   ASKAPDEBUGASSERT(itsNStreams > 1);
   // all elements of the groups vector should be non-negative
   ASKAPASSERT(std::count_if(groups.begin(), groups.end(), std::bind2nd(std::less<int>(), 0)) == 0);

   ASKAPDEBUGASSERT(itsConfig.rank() < static_cast<int>(groups.size()));
   const int thisRankGroup = groups[itsConfig.rank()];

   for (size_t grp = 0; grp < nGroups; ++grp) {
        const int nRanksThisGroup = std::count_if(groups.begin(), groups.end(), std::bind2nd(std::equal_to<int>(), static_cast<int>(grp)));
        ASKAPASSERT(nRanksThisGroup > 1);
        const std::map<int,int>::const_iterator ciStart = startRanksMap.find(grp);
        ASKAPASSERT(ciStart != startRanksMap.end());
        const int startRankThisGroup = ciStart->second;
        const std::map<int,int>::const_iterator ciEnd = startRanksMap.find(grp+1);
        const int stopRankThisGroup  = (ciEnd != startRanksMap.end() ? ciEnd->second : static_cast<int>(activityFlags.size())) - 1;
        if (thisRankGroup == static_cast<int>(grp)) {
            ASKAPLOG_DEBUG_STR(logger, "This rank belongs to initial group "<<thisRankGroup<<" (ranks from "<<startRankThisGroup<<
                                       " to "<<stopRankThisGroup<<", inclusive)");
            ASKAPLOG_DEBUG_STR(logger, "    - available "<<nRanksThisGroup<<" ranks");
        }
        // consistency check
        ASKAPASSERT(nRanksThisGroup == stopRankThisGroup - startRankThisGroup + 1);
        ASKAPCHECK(itsNStreams <= nRanksThisGroup, "Number of streams requested ("<<itsNStreams<<
                   ") exceeds the number of ranks available ("<<nRanksThisGroup<<")");
        const int maxStride = (nRanksThisGroup - 1) / (itsNStreams - 1);
        ASKAPDEBUGASSERT(maxStride > 0);
        // trying to space active ranks as much as we can (in the future we can make this configurable)
        for (int rankOffset = 0; rankOffset < nRanksThisGroup; ++rankOffset) {
             const int rank = rankOffset + startRankThisGroup;
             if (rankOffset % maxStride == 0) {
                 if (thisRankGroup == static_cast<int>(grp)) {
                     if (rankOffset == 0) {
                         ASKAPLOG_DEBUG_STR(logger,"    - rank "<<rank<<" will be kept active");
                     } else {
                         ASKAPLOG_INFO_STR(logger,"    - rank "<<rank<<" will be activated");
                     }
                 }
             } else {
                 if (thisRankGroup == static_cast<int>(grp)) {
                     ASKAPLOG_DEBUG_STR(logger,"    - rank "<<rank<<" will be kept deactivated");
                 }
                 groups[rank] = static_cast<int>(groups.size());   
             }
        }
   }
   // now create intra-group communicator
   const int actualGroup = groups[itsConfig.rank()];
   //ASKAPLOG_DEBUG_STR(logger,"Building intra-group communicator, actualGroup = "<<actualGroup);

   // just do ascending order in original ranks for local group ranks
   const int response2 = MPI_Comm_split(MPI_COMM_WORLD, actualGroup, itsConfig.rank(), &itsCommunicator);
   ASKAPCHECK(response2 == MPI_SUCCESS, "Erroneous response from MPI_Comm_split = "<<response2);

   int thisRankStream = -1;
   if (actualGroup < static_cast<int>(groups.size())) {
       thisRankStream = localRank();
       ASKAPLOG_INFO_STR(logger, "This rank corresponds to stream "<<thisRankStream<<" group "<<actualGroup);
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

      //1) scatter rows dealt with by particular stream (stream number is the local rank in the intra-group communicator) 
      
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


/// @brief broadcast row-independent fields
/// @details This method handles row-independent fields, broadcasts 
/// the content within the group and initialses the chunk for streams with
/// inactive input. 
/// @param[in] chunk the instance of VisChunk to work with
void BeamScatterTask::broadcastRIFields(askap::cp::common::VisChunk::ShPtr& chunk) const
{
  ASKAPDEBUGASSERT(itsStreamNumber >= 0);
  const int formatId = 1;
  if (localRank() == 0) {
      ASKAPDEBUGASSERT(chunk);
      // as we need to pass sizes anyway, pass also basic parameters required to initialise the chunk
      // for slave ranks.
      uint32_t buffer[5] = {0u, chunk->nRow(), chunk->nChannel(), chunk->nPol(), chunk->nAntenna()};

      // 1) encode info into blob (some of it we could've done directly, but I am too bored to do it for
      //    some casa types (especially as I don't have time for testing)
      LOFAR::BlobString bs;
      bs.resize(0);
      LOFAR::BlobOBufString bob(bs);
      LOFAR::BlobOStream out(bob);
      out.putStart("RowIndependentParameters", formatId);
      out << chunk->time() << chunk->targetName() << chunk->interval() << chunk->scan() << 
             chunk->channelWidth();
      
      // todo:  chunk->targetPointingCentre() , chunk->actualPointingCentre() 
      // todo: chunk->actualPolAngle() << chunk->actualAzimuth() << chunk->actualElevation() 
      // todo: chunk->onSourceFlag()
      // todo:  chunk->frequency()
      // todo: chunk->stokes()
      // todo: chunk->directionFrame();
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
      
      // time
      casa::MVEpoch epoch;
      in >> epoch;
      chunk->time() = epoch;
  
      // targetName
      std::string targetName;
      in >> targetName;
      chunk->targetName() = targetName;

      // interval
      casa::Double interval = -1.;
      in >> interval;
      chunk->interval() = interval;

      // scan
      casa::uInt scan = 0;
      in >> scan;
      chunk->scan() = scan;
   
      // channel width
      casa::Double channelWidth = 0.;
      in >> channelWidth;
      chunk->channelWidth() = channelWidth;

      // other fields come here, we can probably refactor this code via templates

      in.getEnd();
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

  ASKAPASSERT(chunk);
  ASKAPASSERT(newNRows < chunk->nRow());
  askap::cp::common::VisChunk::ShPtr newChunk(new VisChunk(newNRows, chunk->nChannel(), chunk->nPol(), chunk->nAntenna()));
  newChunk->flag().set(false);
  
  /*
  newChunk->antenna1().resize(newNRows);
  newChunk->antenna2().resize(newNRows);
  newChunk->beam1().resize(newNRows);
  newChunk->beam2().resize(newNRows);
  newChunk->beam1PA().resize(newNRows);
  newChunk->beam2PA().resize(newNRows);
  newChunk->phaseCentre().resize(newNRows);
  newChunk->uvw().resize(newNRows);
  casa::IPosition shape = newChunk->visibility().shape();
  ASKAPASSERT(newChunk->flag().shape() == shape);
  ASKAPASSERT(shape.nelements() == 3);
  shape[0] = newNRows;
  newChunk->visibility().resize(shape);
  newChunk->flag().resize(shape);
  
  */
  chunk = newChunk;
} 


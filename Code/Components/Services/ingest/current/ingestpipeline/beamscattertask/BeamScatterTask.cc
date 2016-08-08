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

// boost includes
#include "boost/shared_array.hpp"

// std includes
#include <vector>
#include <map>
#include <algorithm>

// Local package includes
#include "configuration/Configuration.h"
#include "monitoring/MonitoringSingleton.h"

ASKAP_LOGGER(logger, ".BeamScatterTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

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
   }


   if (chunk) {
       ASKAPLOG_DEBUG_STR(logger, "This rank has an active input");
       ASKAPCHECK(itsStreamNumber == 0, "Only the first rank of the group (i.e. first stream) is expected to have an active input");
   } else {
       ASKAPLOG_DEBUG_STR(logger, "This rank has an inactive input");
       ASKAPCHECK(itsStreamNumber != 0, "First rank of the group (i.e. first stream) is expected to have an active input");
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
  }
}



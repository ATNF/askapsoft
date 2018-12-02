/// @file BufferedTask.cc
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

#include "ingestpipeline/bufferedtask/BufferedTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "ingestpipeline/TaskFactory.h"

// casacore includes
#include "casacore/casa/OS/Timer.h"


ASKAP_LOGGER(logger, ".BufferedTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

/// @param[in] parset the configuration parameter set.
/// @param[in] config configuration
BufferedTask::BufferedTask(const LOFAR::ParameterSet& parset,
                     const Configuration& config) :
    itsLossLess(parset.getBool("lossless", true)), 
    itsMaxWait(parset.getUint32("maxwait", 30)),
    itsStopRequested(false), 
    itsBuffer(parset.getUint32("size",1)), 
    itsChildActiveForAllRanks(false),
    itsFirstCycle(true)
{
   ASKAPLOG_DEBUG_STR(logger, "Constructor - buffer size: "<<itsBuffer.capacity());
   const std::string childTaskName = parset.getString("child");

   ASKAPLOG_DEBUG_STR(logger, "Wrapper around "<<childTaskName<<" task - setting up the child task");
   TaskFactory factory(config);
   itsTask = factory.createTask(config.taskByName(childTaskName));
   ASKAPCHECK(itsTask, "Failed to create task "<<childTaskName);
   itsChildActiveForAllRanks = itsTask->isAlwaysActive();
   itsRank = config.rank();
}

/// @brief destructor
BufferedTask::~BufferedTask()
{
   ASKAPLOG_DEBUG_STR(logger, "Destructor - stopping service thread");
   // Request stop of the parallel thread - it will finish the current call to process(...) of
   // the child task
   itsStopRequested = true;

   // Wait for the thread running the io_service to finish
   if (itsThread.get()) {
       itsThread->join();
   }
}

/// @brief service thread entry point
void BufferedTask::parallelThread()
{
   ASKAPDEBUGASSERT(itsTask);
   // Used for a timeout
   const long ONE_SECOND = 1000000;
   ASKAPLOG_DEBUG_STR(logger, "Running service thread in rank = "<<itsRank);

   casa::Timer timer;
   double timeToGetData = 0.;
   size_t numberOfFalseWakes = 0;
   timer.mark();

   while (!itsStopRequested) {
      boost::shared_ptr<askap::cp::common::VisChunk> chunk = itsBuffer.next(ONE_SECOND);
      timeToGetData += timer.real();
      if (chunk) {
          ASKAPLOG_DEBUG_STR(logger, "Took "<<timeToGetData<<" seconds and "<<numberOfFalseWakes<<" false wakes to get data for rank = "<<itsRank);
          numberOfFalseWakes = 0;
          timeToGetData = 0.;
          timer.mark();
 
          itsTask->process(chunk);
          ASKAPLOG_DEBUG_STR(logger, "Child task "<<itsTask->getName()<<" execution time "<<timer.real()<<" seconds for rank = "<<itsRank);
          
          if (!chunk) {
               ASKAPLOG_WARN_STR(logger, "Child task of the BufferedTask attempted to change the data distribution - not supported");
          }
      } else {
          ++numberOfFalseWakes;
      }
      timer.mark();
   }
   ASKAPLOG_DEBUG_STR(logger, "Service thread finishing in rank = "<<itsRank);
}

/// @brief Process single visibility chunk
/// @details There is no modification of the data, just internal buffers are updated.
/// If the child task updates the chunk this change is lost, except on the first 
/// iteration where the task is allowed to change data distribution by resetting
/// the shared pointer where appropriate.
///
/// @param[in,out] chunk  the instance of VisChunk for which the
///                       phase factors will be applied.
void BufferedTask::process(askap::cp::common::VisChunk::ShPtr& chunk)
{
   ASKAPASSERT(itsTask);
   if (itsFirstCycle) {
       itsFirstCycle = false;
       // on first cycle execute processing in the main thread -
       // this helps with lack of thread-safety in some casacore routines
       // and also allows us to lock in the data distribution pattern
       ASKAPLOG_DEBUG_STR(logger, "Buffered task adapter (child: "<<itsTask->getName()<<") - first cycle, processing in main thread");

       itsTask->process(chunk);
       // this flag can change after each execution of process
       itsChildActiveForAllRanks = itsTask->isAlwaysActive();
       ASKAPCHECK(!itsChildActiveForAllRanks, "tasks which are active for all ranks beyond the first cycle are not supported by the buffered adapter task!");

       if (chunk) {
           ASKAPLOG_DEBUG_STR(logger, "Buffered task adapter (child: "<<itsTask->getName()<<") - this rank will have data, starting the service thread");

           // Start the service thread
           itsThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&BufferedTask::parallelThread, this)));
       } else {
           ASKAPLOG_DEBUG_STR(logger, "Buffered task adapter (child: "<<itsTask->getName()<<") - this rank is permanently deactivated for the child");
       }
   } else {
       ASKAPLOG_DEBUG_STR(logger, "Buffered task adapter (child: "<<itsTask->getName()<<") - queuing data for processing");
       ASKAPCHECK(chunk, "BufferedTask::process is not expected to receive an empty shared pointer except on the first cycle");
       askap::cp::common::VisChunk::ShPtr chunkCopy(new askap::cp::common::VisChunk(*chunk));
       ASKAPDEBUGASSERT(chunkCopy);

       if (itsBuffer.size() < itsBuffer.capacity()) {
           // plenty of room - just add
           // we don't need to worry about race condition as we're
           // the only producer. If consumer thread takes out the 
           // item there will be even more room
           itsBuffer.add(chunkCopy);
       } else {
           // space may or may not exist
           
           const long ONE_SECOND = 1000000;
           casa::uInt attempt = 0;
           for (; attempt < itsMaxWait; ++attempt) {
                if (itsBuffer.addWhenThereIsSpace(chunkCopy, ONE_SECOND)) {
                    if (attempt) {
                        ASKAPLOG_DEBUG_STR(logger, "Successfully queued data chunk after "<<attempt<<" attempts");
                    }
                    break;
                }
           }
           if (attempt >= itsMaxWait) {
               ASKAPCHECK(!itsLossLess, "Timeout of "<<itsMaxWait<<" seconds waiting to queue data chunk for buffered processing");

               ASKAPLOG_ERROR_STR(logger, "Timeout of "<<itsMaxWait<<" seconds waiting to queue data chunk for buffered processing - some data lost");
           }
       }
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
/// @note default action is to return false, i.e. process method
/// is not called for inactive tasks.
bool BufferedTask::isAlwaysActive() const
{
   return itsChildActiveForAllRanks;
}


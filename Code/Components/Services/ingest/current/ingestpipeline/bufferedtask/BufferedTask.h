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

#ifndef ASKAP_CP_INGEST_BUFFEREDTASK_H
#define ASKAP_CP_INGEST_BUFFEREDTASK_H

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "cpcommon/VisChunk.h"
#include "utils/DelayEstimator.h"

// casa includes
#include <casacore/casa/Arrays/Matrix.h>

// boost includes
#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too
#include "ingestpipeline/sourcetask/CircularBuffer.h"

// std includes
#include <fstream>
#include <string>

namespace askap {
namespace cp {
namespace ingest {

/// @brief task for running another task in a parallel thread
/// @details This task is a wrapper around any other task known to the ingest pipeline.
/// Except for the first chunk of data which is just passed to the child task as is
/// (to allow adjustment to the actual configuration of the parallel streams within the
/// ingest pipeline), this task makes the copy of the data, buffers them and executes the
/// child task in a parallel thread. Provided the execution time plus copy overheads 
/// do not exceed the cycle time, this allows better utilisation of resources and more
/// distributed computing. The child task should obey the following conditions (otherwise,
/// ingesting will not work correctly and may even lock up):
///    * it should not modify the data
///    * it should not alter the distribution of data streams (except on the first cycle)
/// For example, MSSink or TCPSink are suitable while ChannelAvgTask or BeamScatterTask are not.
/// The code has limited ability to detect misuse, so it is largely up to an expert user to 
/// configure ingest pipeline correctly to avoid problems. This task supports  a couple of
/// different strategies deailg with the processing not keeping up: throw an exception, 
/// skip the data.
///
/// Parameters (example):
///   child = MSSink  (child task, same name as understood in tasklist)
///   lossless = true (if not allowed to skip data in the not-keeping up case)
///   size = 1 (circular buffer size)
///   maxwait = 20 (maximum waiting time in seconds for the child task to complete)
///
class BufferedTask : public askap::cp::ingest::ITask {
    public:

        /// @brief Constructor
        /// @param[in] parset the configuration parameter set.
        /// @param[in] config configuration
        BufferedTask(const LOFAR::ParameterSet& parset,
                     const Configuration& config);

        /// @brief destructor
        ~BufferedTask();

        /// @brief Process single visibility chunk
        /// @details There is no modification of the data, just internal buffers are updated.
        /// If the child task updates the chunk this change is lost, except on the first 
        /// iteration where the task is allowed to change data distribution by resetting
        /// the shared pointer where appropriate.
        ///
        /// @param[in,out] chunk  the instance of VisChunk for which the
        ///                       phase factors will be applied.
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
        /// @note default action is to return false, i.e. process method
        /// is not called for inactive tasks.
        virtual bool isAlwaysActive() const;

    protected:

    private:
        /// @brief service thread entry point
        void parallelThread();

        /// @brief child task this class wraps around
        boost::shared_ptr<askap::cp::ingest::ITask> itsTask;

        /// @brief true if the task is not allowed to loose any data in the case of not-keeping up
        /// @details i.e., it will throw an exception after some time out
        bool itsLossLess;

        /// @brief maximum waiting time in seconds for the child task to complete
        /// @details If not complete in time, and the buffer is full - either
        /// exception is raised or the new data chunk is skipped
        casa::uInt itsMaxWait;

        /// @brief service thread
        boost::shared_ptr<boost::thread> itsThread;

        /// @brief flag requesting service thread to finish
        bool itsStopRequested;

        /// @brief actual buffer for data chunks
        askap::cp::ingest::CircularBuffer<askap::cp::common::VisChunk> itsBuffer;

        /// @brief true if child task is active for all ranks
        bool itsChildActiveForAllRanks;

        /// @brief true, if this is the first cycle
        bool itsFirstCycle;

        /// @brief rank reported in log from the service thread
        int itsRank;

}; // BufferedTask class

} // ingest
} // cp
} // askap

#endif // #ifndef ASKAP_CP_INGEST_BUFFEREDTASK_H

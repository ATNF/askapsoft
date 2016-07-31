/// @file IngestPipeline.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "IngestPipeline.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <vector>
#include <iterator>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casacore/casa/OS/Timer.h"
#include "Common/ParameterSet.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "ingestpipeline/TaskFactory.h"
#include "ingestpipeline/sourcetask/MergedSource.h"
#include "ingestpipeline/sourcetask/NoMetadataSource.h"
#include "ingestpipeline/sourcetask/InterruptedException.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too
#include "monitoring/MonitoringSingleton.h"

ASKAP_LOGGER(logger, ".IngestPipeline");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

IngestPipeline::IngestPipeline(const LOFAR::ParameterSet& parset,
                               int rank, int ntasks)
    : itsConfig(parset, rank, ntasks), itsRunning(false)
{
}

IngestPipeline::~IngestPipeline()
{
}

void IngestPipeline::start(void)
{
    itsRunning = true;
    ingest();
}

void IngestPipeline::abort(void)
{
    itsRunning = false;
}

void IngestPipeline::ingest(void)
{
    // 1) Get task list from configuration
    const std::vector<TaskDesc>& tasks = itsConfig.tasks();

    // 2) Configure the Monitoring Singleton
    if (!itsConfig.monitoringConfig().registryHost().empty()) {
        MonitoringSingleton::init(itsConfig);
    }

    // 3) Create a Task Factory
    TaskFactory factory(itsConfig);

    // 4) Setup source
    if (tasks.empty()) {
        ASKAPTHROW(AskapError, "No pipeline tasks specified");
    }

    // at this stage we do not support the situation when
    // some ranks may be inactive up front. This may be necessary
    // if more parallelism is required for post-processing than
    // for receiving. But currently, source task is instantiated
    // for all ranks.
    if (tasks[0].type() == TaskDesc::MergedSource) {
        itsSource = factory.createMergedSource();
    } else if (tasks[0].type() == TaskDesc::NoMetadataSource) {
        itsSource = factory.createNoMetadataSource();
    } else {
        ASKAPTHROW(AskapError, "First task should be a Source");
    }

    // 5) Setup tasks
    for (size_t i = 1; i < tasks.size(); ++i) {
        ITask::ShPtr task = factory.createTask(tasks[i]);
        itsTasks.push_back(task);
    }

    // 6) Process correlator integrations, one at a time
    casa::Timer timer;
    while (itsRunning)  {
        try {
            timer.mark();
            bool endOfStream = ingestOne();
            if (itsConfig.rank() == 0) {
                ASKAPLOG_INFO_STR(logger, "Total cycle execution time "
                       << timer.real() << "s");
            }
            itsRunning = !endOfStream;
        } catch (InterruptedException&) {
            break;
        }
    }

    // 7) Clean up
    itsSource.reset();
    MonitoringSingleton::invalidatePoint("SourceTaskDuration");
    MonitoringSingleton::invalidatePoint("ProcessingDuration");
    // Destroying this is safe even if the object was not initialised
    MonitoringSingleton::destroy();
}

bool IngestPipeline::ingestOne(void)
{
    casa::Timer timer;
    //ASKAPLOG_DEBUG_STR(logger, "Waiting for data");
    timer.mark();
    // all ranks are active up front
    ASKAPDEBUGASSERT(itsSource);
    VisChunk::ShPtr chunk(itsSource->next());
    if (itsConfig.rank() == 0) {
        ASKAPLOG_INFO_STR(logger, "Source task execution time " << timer.real() << "s");
    }
    MonitoringSingleton::update<double>("SourceTaskDuration",timer.real(), MonitorPointStatus::OK, "s");
    if (chunk.get() == 0) {
        return true; // Finished
    }

    if (itsConfig.rank() == 0) {
        ASKAPLOG_INFO_STR(logger, "Received one VisChunk. Timestamp: " << chunk->time());
    } else {
        ASKAPLOG_DEBUG_STR(logger, "Received one VisChunk. Timestamp: " << chunk->time());
    }

    // For each task call process on the VisChunk as long as this rank stays active
    double processingTime = 0.;
    for (unsigned int i = 0; i < itsTasks.size(); ++i) {
         if (chunk || itsTasks[i]->isAlwaysActive()) {
             timer.mark();
             itsTasks[i]->process(chunk);
             if (itsConfig.rank() == 0) {
                 ASKAPLOG_INFO_STR(logger, itsTasks[i]->getName() << " execution time "
                       << timer.real() << "s");
             }
             processingTime += timer.real();
         }
    }
    
    MonitoringSingleton::update<double>("ProcessingDuration",processingTime, MonitorPointStatus::OK, "s");

    return false; // Not finished
}

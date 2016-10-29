/// @file TaskFactory.cc
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
#include "TaskFactory.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "Common/ParameterSet.h"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "ingestpipeline/calcuvwtask/CalcUVWTask.h"
#include "ingestpipeline/caltask/CalTask.h"
#include "ingestpipeline/chanavgtask/ChannelAvgTask.h"
#include "ingestpipeline/chanseltask/ChannelSelTask.h"
#include "ingestpipeline/chanmergetask/ChannelMergeTask.h"
#include "ingestpipeline/simplemonitortask/SimpleMonitorTask.h"
#include "ingestpipeline/flagtask/FlagTask.h"
#include "ingestpipeline/fileflagtask/FileFlagTask.h"
#include "ingestpipeline/mssink/MSSink.h"
#include "ingestpipeline/sourcetask/MetadataSource.h"
#include "ingestpipeline/sourcetask/VisSource.h"
#include "ingestpipeline/sourcetask/ISource.h"
#include "ingestpipeline/sourcetask/MergedSource.h"
#include "ingestpipeline/sourcetask/ParallelMetadataSource.h"
#include "ingestpipeline/sourcetask/NoMetadataSource.h"
#include "ingestpipeline/derippletask/DerippleTask.h"
#include "ingestpipeline/tcpsink/TCPSink.h"
#include "ingestpipeline/phasetracktask/FringeRotationTask.h"
#include "ingestpipeline/beamscattertask/BeamScatterTask.h"
#include "ingestpipeline/bufferedtask/BufferedTask.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

ASKAP_LOGGER(logger, ".TaskFactory");

using namespace askap;
using namespace askap::cp::ingest;

TaskFactory::TaskFactory(const Configuration& config)
    : itsConfig(config)
{
}

ITask::ShPtr TaskFactory::createTask(const TaskDesc& taskDescription)
{
    // Extract task type & parameters
    const TaskDesc::Type type = taskDescription.type();
    const LOFAR::ParameterSet params = taskDescription.params();

    // Create the task
    ITask::ShPtr task;
    switch (type) {
        case TaskDesc::CalcUVWTask :
            task.reset(new CalcUVWTask(params, itsConfig));
            break;
        case  TaskDesc::CalTask :
            task.reset(new CalTask(params, itsConfig));
            break;
        case TaskDesc::ChannelAvgTask :
            task.reset(new ChannelAvgTask(params, itsConfig));
            break;
        case TaskDesc::ChannelSelTask :
            task.reset(new ChannelSelTask(params, itsConfig));
            break;
        case TaskDesc::ChannelMergeTask :
            task.reset(new ChannelMergeTask(params, itsConfig));
            break;
        case TaskDesc::MSSink :
            task.reset(new MSSink(params, itsConfig));
            break;
        case TaskDesc::FringeRotationTask :
            task.reset(new FringeRotationTask(params, itsConfig));
            break;
        case TaskDesc::SimpleMonitorTask :
            task.reset(new SimpleMonitorTask(params, itsConfig));
            break;
        case TaskDesc::FlagTask :
            task.reset(new FlagTask(params, itsConfig));
            break;
        case TaskDesc::FileFlagTask :
            task.reset(new FileFlagTask(params, itsConfig));
            break;
        case TaskDesc::DerippleTask:
            task.reset(new DerippleTask(params, itsConfig));
            break;
        case TaskDesc::TCPSink:
            task.reset(new TCPSink(params, itsConfig));
            break;
        case TaskDesc::BeamScatterTask:
            task.reset(new BeamScatterTask(params, itsConfig));
            break;
        case TaskDesc::BufferedTask:
            task.reset(new BufferedTask(params, itsConfig));
            break;
        default:
            ASKAPTHROW(AskapError, "Unknown task type specified");
            break;
    }

    task->setName(taskDescription.name());

    return task;
}

boost::shared_ptr< ISource > TaskFactory::createMergedSource(void)
{
    // Pre-conditions
    ASKAPCHECK(itsConfig.tasks().at(0).name().compare("MergedSource") == 0,
            "First defined task is not the Merged Source");

    // 1) Configure and create the metadata source
    IMetadataSource::ShPtr metadataSrc; // void shared pointer by default
    const int rank = itsConfig.rank();
    const int numProcs = itsConfig.nprocs();

    if ((numProcs == 1) || (rank == 0)) {
        ASKAPLOG_DEBUG_STR(logger, "Rank zero or serial case - creating metadata source");
        const std::string mdLocatorHost = itsConfig.metadataTopic().registryHost();
        const std::string mdLocatorPort = itsConfig.metadataTopic().registryPort();
        const std::string mdTopicManager = itsConfig.metadataTopic().topicManager();
        const std::string mdTopic = itsConfig.metadataTopic().topic();
        const unsigned int mdBufSz = 12; // TODO: Make this a tunable
        const std::string mdAdapterName = "IngestPipeline";
        metadataSrc.reset(new MetadataSource(mdLocatorHost,
                 mdLocatorPort, mdTopicManager, mdTopic, mdAdapterName, mdBufSz));
    }
    if (numProcs > 1) {
        // parallel case - wrap metadata source in an adapter
        // non-zero ranks will get an empty pointer as input
        metadataSrc.reset(new ParallelMetadataSource(metadataSrc));
    }

    // 2) Configure and create the visibility source
    const LOFAR::ParameterSet params = itsConfig.tasks().at(0).params();
    VisSource::ShPtr visSrc = createVisSource(params);

    // 3) Create and configure the merged source
    boost::shared_ptr< MergedSource > source(new MergedSource(params, itsConfig, metadataSrc, visSrc));
    return source;
}

boost::shared_ptr<IVisSource> TaskFactory::createVisSource(const LOFAR::ParameterSet &params) 
{
    if (itsConfig.receivingRank()) {
        // this is a receiving rank
        ASKAPLOG_DEBUG_STR(logger, "Rank "<<itsConfig.rank()<<" is a receiving rank with id="<<itsConfig.receiverId()<<
                  " (total number: "<<itsConfig.nReceivingProcs()<<" receivers) - setting up VisSource");
        return VisSource::ShPtr(new VisSource(params, itsConfig.receiverId()));
    } else {
        // this is a service rank
        ASKAPLOG_DEBUG_STR(logger, "Rank "<<itsConfig.rank()<<" is a service rank (total number: "<<
                   itsConfig.nprocs() - itsConfig.nReceivingProcs()<<" service ranks)");
    }
    return VisSource::ShPtr();
}

boost::shared_ptr< ISource > TaskFactory::createNoMetadataSource(void)
{
    // Pre-conditions
    ASKAPCHECK(itsConfig.tasks().at(0).name().compare("NoMetadataSource") == 0,
            "First defined task is not the NoMetadataSource");

    //  Configure and create the visibility source
    const LOFAR::ParameterSet params = itsConfig.tasks().at(0).params();
    boost::shared_ptr< NoMetadataSource > source(new NoMetadataSource(params, itsConfig, createVisSource(params)));
    return source;
}

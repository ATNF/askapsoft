/// @file NoMetadataSource.cc
///
/// @copyright (c) 2013 CSIRO
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
#include "NoMetadataSource.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <stdint.h>
#include <set>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/shared_array.hpp"
#include "Common/ParameterSet.h"
#include "cpcommon/VisDatagram.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/measures/Measures.h"

// Local package includes
#include "ingestpipeline/sourcetask/IVisSource.h"
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/ChannelManager.h"
#include "ingestpipeline/sourcetask/InterruptedException.h"
#include "ingestpipeline/sourcetask/MergedSource.h"

// mpi for basic synchronisation 
#include <mpi.h>

ASKAP_LOGGER(logger, ".NoMetadataSource");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

NoMetadataSource::NoMetadataSource(const LOFAR::ParameterSet& params,
                                   const Configuration& config,
                                   IVisSource::ShPtr visSrc) :
        itsVisSrc(visSrc),
        itsInterrupted(false),
        itsSignals(itsIOService, SIGINT, SIGTERM, SIGUSR1),
        itsCentreFreq(asQuantity(params.getString("centre_freq"))),
        itsTargetName(params.getString("target_name")),
        itsTargetDirection(asMDirection(params.getStringVector("target_direction"))),
        itsLastTimestamp(0u), itsVisConverter(params, config)
{
    itsCorrelatorMode = config.lookupCorrelatorMode(params.getString("correlator_mode"));

    // log TAI_UTC casacore measures table version and date
    const std::pair<double, std::string> measVersion = measuresTableVersion();
    itsMonitoringPointManager.submitPoint<float>("MeasuresTableMJD", 
            static_cast<float>(measVersion.first));
    itsMonitoringPointManager.submitPoint<std::string>("MeasuresTableVersion", measVersion.second);

    // Setup a signal handler to catch SIGINT, SIGTERM and SIGUSR1
    itsSignals.async_wait(boost::bind(&NoMetadataSource::signalHandler, this, _1, _2));
}

NoMetadataSource::~NoMetadataSource()
{
    itsSignals.cancel();
}

VisChunk::ShPtr NoMetadataSource::next(void)
{
    const long ONESECOND = 1000000; // 1 second timeout
    // Get the next VisDatagram if there isn't already one in the buffer
    while (!itsVis) {
        itsVis = itsVisSrc->next(ONESECOND); 

        itsIOService.poll();
        if (itsInterrupted) throw InterruptedException();
    }

    syncrhoniseLastTimestamp();

    // catch up if necessary; all datagrams should be processed below this 
    // method is only called once per integration (hence <= in the while-statement)
    // Using timestamp of 0 as initial value as it is way in the past.

    uint32_t nIgnoredOldDatagrams = 0;
    while (itsVis->timestamp <= itsLastTimestamp) {
           ++nIgnoredOldDatagrams;
           itsVis.reset();
           while (!itsVis) {
                 itsVis = itsVisSrc->next(ONESECOND); 

                 itsIOService.poll();
                 if (itsInterrupted) throw InterruptedException();
           }
    }
    // This is the BAT timestamp for the current integration being processed
    const casa::uLong currentTimestamp = itsVis->timestamp;

    if (nIgnoredOldDatagrams > 0) {
        ASKAPLOG_DEBUG_STR(logger, "Catching up to time: "<<bat2epoch(currentTimestamp)<<
                  ", ignored "<<nIgnoredOldDatagrams<<" successfully received datagrams.");
    }

    ASKAPCHECK(currentTimestamp != itsLastTimestamp,
            "Consecutive VisChunks have the same timestamp");
    itsLastTimestamp = currentTimestamp;

    // Now the streams are synced, start building a VisChunk
    VisChunk::ShPtr chunk = createVisChunk(currentTimestamp);

    // Determine how many VisDatagrams are expected for a single integration
    const casa::uInt interval = itsCorrelatorMode.interval();
    const casa::uInt timeout = interval * 2;

    // Read VisDatagrams and add them to the VisChunk. If itsVisSrc->next()
    // returns a null pointer this indicates the timeout has been reached.
    // In this case assume no more VisDatagrams for this integration will
    // be recieved and move on
    while (itsVis && currentTimestamp >= itsVis->timestamp) {
        itsIOService.poll();
        if (itsInterrupted) throw InterruptedException();

        if (currentTimestamp > itsVis->timestamp) {
            // If the VisDatagram is from a prior integration then discard it
            ASKAPLOG_WARN_STR(logger, "Received VisDatagram from past integration");
            itsVis = itsVisSrc->next(timeout);
            continue;
        }

        itsVisConverter.add(*itsVis);
        itsVis.reset();

        if (itsVisConverter.gotAllExpectedDatagrams()) {
            // This integration is finished
            break;
        }

        itsVis = itsVisSrc->next(timeout);
        if (!itsVis) {
            ASKAPLOG_DEBUG_STR(logger, "finishing ingesting chunk at "<<bat2epoch(currentTimestamp)<<
                      " due to timeout");
        }
    }

    ASKAPLOG_DEBUG_STR(logger, "VisChunk built with " << itsVisConverter.datagramsCount() <<
            " of expected " << itsVisConverter.datagramsExpected() << " visibility datagrams");
    ASKAPLOG_DEBUG_STR(logger, "     - ignored " << itsVisConverter.datagramsIgnored()
            << " successfully received datagrams");

    const std::pair<uint32_t, uint32_t> bufferUsage = itsVisSrc->bufferUsage();
    const float bufferUsagePercent = bufferUsage.second != 0 ? static_cast<float>(bufferUsage.first) / bufferUsage.second * 100. : 100.;

    ASKAPLOG_DEBUG_STR(logger, "VisSource buffer has "<<bufferUsage.first<<" datagrams ("<<bufferUsagePercent<<"% full)"); 

    // Submit monitoring data
    itsMonitoringPointManager.submitPoint<uint32_t>("PacketsBuffered", bufferUsage.first);
    itsMonitoringPointManager.submitPoint<float>("BufferUsagePercent", bufferUsagePercent);

    itsMonitoringPointManager.submitPoint<int32_t>("PacketsLostCount",
            itsVisConverter.datagramsExpected() - itsVisConverter.datagramsCount());
    if (itsVisConverter.datagramsExpected() != 0) {
        itsMonitoringPointManager.submitPoint<float>("PacketsLostPercent",
            (itsVisConverter.datagramsExpected() - itsVisConverter.datagramsCount()) / static_cast<float>(itsVisConverter.datagramsExpected()) * 100.);
    }
    itsMonitoringPointManager.submitMonitoringPoints(*chunk);

    return chunk;
}

VisChunk::ShPtr NoMetadataSource::createVisChunk(const casa::uLong timestamp)
{
    itsVisConverter.initVisChunk(timestamp, itsCorrelatorMode);
    VisChunk::ShPtr chunk = itsVisConverter.visChunk();
    ASKAPASSERT(chunk);

    // filling fields which are specific to this NoMetadataSource

    // Add the scan index
    chunk->scan() = 0;

    chunk->targetName() = itsTargetName;

    chunk->channelWidth() = itsCorrelatorMode.chanWidth().getValue("Hz");

    // Frequency vector is not of length nRows, but instead nChannels
    chunk->frequency() = itsVisConverter.channelManager().localFrequencies(
            itsVisConverter.config().receiverId(),
            itsCentreFreq.getValue("Hz") - chunk->channelWidth() / 2.,
            chunk->channelWidth(),
            itsCorrelatorMode.nChan());

    chunk->directionFrame() = itsTargetDirection.getRef();

    
    // TODO!!
    // The handling of pointing directions below is not handled per beam.
    // It just takes the field centre direction from the parset and uses
    // that for all beam pointing directions.
    // MV: ultimately need to shift phase centres per beam
    chunk->phaseCentre().set(itsTargetDirection.getAngle());


    // Populate the per-antenna vectors
    const casa::uInt nAntenna = itsVisConverter.config().antennas().size();
    ASKAPDEBUGASSERT(nAntenna == chunk->targetPointingCentre().size());
    ASKAPDEBUGASSERT(nAntenna == chunk->actualPointingCentre().size());
    ASKAPDEBUGASSERT(nAntenna == chunk->actualPolAngle().size());
    for (casa::uInt i = 0; i < nAntenna; ++i) {
        chunk->targetPointingCentre()[i] = itsTargetDirection;
        chunk->actualPointingCentre()[i] = itsTargetDirection;
        chunk->actualPolAngle()[i] = 0.0;
    }

    return chunk;
}

void NoMetadataSource::signalHandler(const boost::system::error_code& error,
                                     int signalNumber)
{
    if (signalNumber == SIGTERM || signalNumber == SIGINT || signalNumber == SIGUSR1) {
        itsInterrupted = true;
    }
}

/// @brief synchronise itsLastTimestamp across all ranks
/// @details This method is probably only temporary. If
/// ingest pipeline is used in parallel mode, this method
/// ensures that all ranks have the same itsLastTimestamp
/// corresponding to the latest value received. This 
/// will help ingest pipeline to catch up if one of the
/// cards missed an integration.
/// @note Does nothing in the serial mode
void NoMetadataSource::syncrhoniseLastTimestamp()
{
   const size_t nRanks = itsVisConverter.config().nprocs();
   if (nRanks > 1) {
       ASKAPLOG_DEBUG_STR(logger, "synchronising last seen timestamp across all ranks");
       boost::shared_array<uint64_t> buf(new uint64_t[nRanks]);
       const int result = MPI_Allgather(&itsLastTimestamp, 1, MPI_LONG_LONG, buf.get(), 1, MPI_LONG_LONG, MPI_COMM_WORLD);
       ASKAPCHECK(result == MPI_SUCCESS, "Error gathering last time stamps, error="<<result);
       
       // iterate through all timestamps received from other ranks to find the maximum
       uint64_t curVal = itsLastTimestamp;
       for (size_t rank = 0; rank < nRanks; ++rank) {
            if (buf[rank] > curVal) {
                curVal = buf[rank];
            }
       }
       if (curVal > itsLastTimestamp) {
           ASKAPLOG_DEBUG_STR(logger, "Adjusting last seen timestamp forward for "<<
                           double(curVal - itsLastTimestamp)/1e6<<" seconds to match other ranks - need to catch up");
           itsLastTimestamp = curVal;
       } else {
           ASKAPLOG_DEBUG_STR(logger, "Last seen timestamp matches the latest across all ranks - already in sync");
       }
   }
}



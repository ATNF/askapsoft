/// @file MergedSource.cc
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
#include "MergedSource.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <set>
#include <stdint.h>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/bind/bind.hpp"
#include "Common/ParameterSet.h"
#include "cpcommon/TosMetadata.h"
#include "cpcommon/VisDatagram.h"
#include "utils/PolConverter.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/measures/Measures.h"
#include "casacore/measures/Measures/MeasConvert.h"
#include "casacore/measures/Measures/MeasFrame.h"
#include "casacore/measures/Measures/MCEpoch.h"
#include "casacore/measures/Measures/MCDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/casa/OS/Timer.h"

// Local package includes
#include "ingestpipeline/sourcetask/IVisSource.h"
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/ScanManager.h"
#include "ingestpipeline/sourcetask/ChannelManager.h"
#include "ingestpipeline/sourcetask/InterruptedException.h"
#include "configuration/Configuration.h"
#include "configuration/BaselineMap.h"

ASKAP_LOGGER(logger, ".MergedSource");

using namespace std;
using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

MergedSource::MergedSource(const LOFAR::ParameterSet& params,
        const Configuration& config,
        IMetadataSource::ShPtr metadataSrc,
        IVisSource::ShPtr visSrc, int /*numTasks*/, int id) :
     itsMetadataSrc(metadataSrc), itsVisSrc(visSrc),
     itsInterrupted(false),
     itsSignals(itsIOService, SIGINT, SIGTERM, SIGUSR1),
     itsLastTimestamp(-1), itsVisConverter(params, config, id)
{

    // log TAI_UTC casacore measures table version and date
    const std::pair<double, std::string> measVersion = measuresTableVersion();
    itsMonitoringPointManager.submitPoint<float>("MeasuresTableMJD", 
            static_cast<float>(measVersion.first));
    itsMonitoringPointManager.submitPoint<std::string>("MeasuresTableVersion", measVersion.second);

    // Setup a signal handler to catch SIGINT, SIGTERM and SIGUSR1
    itsSignals.async_wait(boost::bind(&MergedSource::signalHandler, this, _1, _2));
}

MergedSource::~MergedSource()
{
    itsSignals.cancel();
}

VisChunk::ShPtr MergedSource::next(void)
{
    casa::Timer timer;
    // Used for a timeout
    const long ONE_SECOND = 1000000;

    // Get metadata for a real (i.e. scan id >= 0) scan
    const uint32_t maxNoMetadataRetries = 10;
    for (int32_t count = 0; (!itsMetadata ||  itsMetadata->scanId() == ScanManager::SCANID_IDLE) && 
                             count < static_cast<int32_t>(maxNoMetadataRetries); ++count) {
        itsMetadata = itsMetadataSrc->next(ONE_SECOND * 10);
        checkInterruptSignal();
        if (itsMetadata && itsMetadata->scanId() < 0
                && itsMetadata->scanId() != ScanManager::SCANID_OBS_COMPLETE
                && itsMetadata->scanId() != ScanManager::SCANID_IDLE) {
            ASKAPTHROW(AskapError, "Invalid ScanID: " << itsMetadata->scanId());
        }

        if (itsMetadata && itsMetadata->scanId() == ScanManager::SCANID_IDLE) {
            ASKAPLOG_INFO_STR(logger,
                    "Skipping this cycle, metadata indicates SCANID_IDLE");
            count = -1;
        }
    } 
    ASKAPCHECK(itsMetadata, "Metadata streaming ceased, unable to recover after "<<
                            maxNoMetadataRetries<<" attempts");
    ASKAPASSERT(itsMetadata->scanId() != ScanManager::SCANID_IDLE);

    // Update the Scan Manager
    itsScanManager.update(itsMetadata->scanId());
    ASKAPLOG_DEBUG_STR(logger, "Processing scanId="<<itsMetadata->scanId()<<" mdata time="<<
             bat2epoch(itsMetadata->time()).getValue());

    // Check if the TOS/TOM has indicated the observation is complete
    if (itsScanManager.observationComplete()) {
        ASKAPLOG_INFO_STR(logger, "End-of-observation condition met");
        return VisChunk::ShPtr();
    }

    // Protect against producing VisChunks with the same timestamp
    ASKAPCHECK(itsMetadata->time() != itsLastTimestamp,
            "Consecutive VisChunks have the same timestamp");
    itsLastTimestamp = itsMetadata->time();

    // Get the next VisDatagram if there isn't already one in the buffer
    const uint32_t maxNoDataRetries = 10;
    for (uint32_t count = 0; !itsVis && count < maxNoDataRetries; ++count) {
        itsVis = itsVisSrc->next(ONE_SECOND);
        checkInterruptSignal();
        if (itsVis) { 
            ASKAPLOG_DEBUG_STR(logger, "Received non-zero itsVis; time="<<bat2epoch(itsVis->timestamp));
        } else {
            ASKAPLOG_DEBUG_STR(logger, "Received zero itsVis, retrying");
        }
    }
    ASKAPCHECK(itsVis, "Reached maximum number of retries for id="<<itsVisConverter.id()<<
            ", the correlator ioc does not seem to send data to this rank. Reached the limit of "<<
            maxNoDataRetries<<" retry attempts");

    ASKAPLOG_DEBUG_STR(logger, "Before aligning metadata and visibility");

    // a hack to account for malformed BAT which can glitch different way for
    // different correlator cards, eventually we should remove this code 
    // (and probably rework the logic of this method which has too much BETA legacy
    //  and also fudged 10s timouts which were probably put there by Ben during early BETA debugging)
    if (itsMetadata->time() != itsVis->timestamp) {
        const casa::uLong timeMismatch = itsVis->timestamp > itsMetadata->time() ? 
              itsVis->timestamp -  itsMetadata->time() : itsMetadata->time() - itsVis->timestamp;
        if (timeMismatch < 2500000ul) {
            ASKAPLOG_DEBUG_STR(logger, "Detected BAT glitch between metadata and visibility stream on card "<<
                 itsVisConverter.id() + 1<<" mismatch = "<<float(timeMismatch)/1e3<<" ms");
            ASKAPLOG_DEBUG_STR(logger, "    faking metadata timestamp to read "<<bat2epoch(itsVis->timestamp).getValue());
            itsMetadata->time(itsVis->timestamp);
        }
    }
    //

    // Find data with matching timestamps
    bool logCatchup = true;
    while (itsMetadata->time() != itsVis->timestamp) {

        // If the VisDatagram timestamps are in the past (with respect to the
        // TosMetadata) then read VisDatagrams until they catch up
        while (!itsVis || itsMetadata->time() > itsVis->timestamp) {
            if (logCatchup) {
                ASKAPLOG_DEBUG_STR(logger, "Reading extra VisDatagrams to catch up");
                logCatchup = false;
            }
            itsVis = itsVisSrc->next(ONE_SECOND);

            checkInterruptSignal();
        }
        checkInterruptSignal();

        if (itsMetadata->time() < itsVis->timestamp) {
            ASKAPLOG_WARN_STR(logger, "Visibility data stream is ahead ("<<bat2epoch(itsVis->timestamp).getValue()<<
                     ") of metadata stream ("<<bat2epoch(itsMetadata->time()).getValue()<<"), skipping the cycle for this card");
            VisChunk::ShPtr chunk = createVisChunk(*itsMetadata);
            ASKAPDEBUGASSERT(chunk);
            // invalidate metadata to force reading new record
            itsMetadata.reset();
            return chunk;
        }

        /*
            // this original Ben's code seem to conflict with 
            // parallel synchronisation in the ADE case
            // instead of trying to align metadata and visibilities
            // if metadata are lagging, just return flagged chunk.
            // This may cause problems if the metadata take longer to 
            // reach Pawsey centre. In that case, all logic has to be
            // re-worked as there is too much BETA legacy anyway.
 
        // But if the timestamp in the VisDatagram is in the future (with
        // respect to the TosMetadata) then it is time to fetch new TosMetadata
        if (!itsMetadata || itsMetadata->time() < itsVis->timestamp) {
            if (logCatchup) {
                ASKAPLOG_DEBUG_STR(logger, "Reading extra TosMetadata to catch up");
                logCatchup = false;
            }
            itsMetadata = itsMetadataSrc->next(ONE_SECOND);
            checkInterruptSignal();
            if (itsMetadata) {
                itsScanManager.update(itsMetadata->scanId());
                if (itsScanManager.observationComplete()) {
                    ASKAPLOG_INFO_STR(logger, "End-of-observation condition met");
                    return VisChunk::ShPtr();
                }
            } else {
                ASKAPLOG_DEBUG_STR(logger, "Received empty metadata - timeout waiting");
            }
        }
        */
    }
    ASKAPLOG_DEBUG_STR(logger, "Aligned datagram time and metadata time: "<<bat2epoch(itsVis->timestamp).getValue());

    // Now the streams are synced, start building a VisChunk
    VisChunk::ShPtr chunk = createVisChunk(*itsMetadata);
    ASKAPDEBUGASSERT(chunk);

    const CorrelatorMode& mode = itsVisConverter.config().lookupCorrelatorMode(itsMetadata->corrMode());
    const casa::uInt timeout = mode.interval() * 2;

    double decodingTime = 0.;

    // Read VisDatagrams and add them to the VisChunk. If itsVisSrc->next()
    // returns a null pointer this indicates the timeout has been reached.
    // In this case assume no more VisDatagrams for this integration will
    // be recieved and move on
    while (itsVis && itsMetadata->time() >= itsVis->timestamp) {
        checkInterruptSignal();

        if (itsMetadata->time() > itsVis->timestamp) {
            // If the VisDatagram is from a prior integration then discard it
            ASKAPLOG_WARN_STR(logger, "Received VisDatagram from past integration");
            itsVis = itsVisSrc->next(timeout);
            continue;
        }

        timer.mark();
        itsVisConverter.add(*itsVis);
        decodingTime += timer.real();
        itsVis.reset();

        if (itsVisConverter.gotAllExpectedDatagrams()) {
            // This integration is finished
            break;
        }
        itsVis = itsVisSrc->next(timeout);
    }

    ASKAPLOG_DEBUG_STR(logger, "VisChunk built with " << itsVisConverter.datagramsCount() <<
            " of expected " << itsVisConverter.datagramsExpected() << " visibility datagrams");
    ASKAPLOG_DEBUG_STR(logger, "     - ignored " << itsVisConverter.datagramsIgnored()
        << " successfully received datagrams");

    const std::pair<uint32_t, uint32_t> bufferUsage = itsVisSrc->bufferUsage();
    const float bufferUsagePercent = bufferUsage.second != 0 ? static_cast<float>(bufferUsage.first) / bufferUsage.second * 100. : 100.;

    ASKAPLOG_DEBUG_STR(logger, "VisSource buffer has "<<bufferUsage.first<<" datagrams ("<<bufferUsagePercent<<"% full)"); 
    ASKAPLOG_DEBUG_STR(logger, "Time it takes to unpack visibilities: "<<decodingTime<<" s");

    // Submit monitoring data
    itsMonitoringPointManager.submitPoint<uint32_t>("PacketsBuffered", bufferUsage.first);
    itsMonitoringPointManager.submitPoint<float>("BufferUsagePercent", bufferUsagePercent);

    itsMonitoringPointManager.submitPoint<float>("VisCornerTurnDuration", decodingTime);

    const int32_t datagramsLost =  itsVisConverter.datagramsExpected() - 
           itsVisConverter.datagramsCount() - itsVisConverter.datagramsIgnored();
    ASKAPDEBUGASSERT(datagramsLost >= 0);
    itsMonitoringPointManager.submitPoint<int32_t>("PacketsLostCount", datagramsLost);
    if (itsVisConverter.datagramsExpected() != 0) {
        itsMonitoringPointManager.submitPoint<float>("PacketsLostPercent",
            static_cast<float>(datagramsLost) / itsVisConverter.datagramsExpected() * 100.);
    }
    itsMonitoringPointManager.submitMonitoringPoints(*chunk);

    itsMetadata.reset();
    return chunk;
}

VisChunk::ShPtr MergedSource::createVisChunk(const TosMetadata& metadata)
{
    const CorrelatorMode& corrMode = itsVisConverter.config().lookupCorrelatorMode(metadata.corrMode());

    itsVisConverter.initVisChunk(metadata.time(), corrMode);
    VisChunk::ShPtr chunk = itsVisConverter.visChunk();
    ASKAPDEBUGASSERT(chunk);

    const casa::uInt nAntenna = itsVisConverter.config().antennas().size();
    ASKAPCHECK(nAntenna > 0, "Must have at least one antenna defined");

    // Add the scan index
    chunk->scan() = itsScanManager.scanIndex();

    chunk->targetName() = metadata.targetName();
    chunk->directionFrame() = metadata.phaseDirection().getRef();

    // Determine and add the spectral channel width
    chunk->channelWidth() = corrMode.chanWidth().getValue("Hz");

    // Build frequencies vector
    // Frequency vector is not of length nRows, but instead nChannels
    chunk->frequency() = itsVisConverter.channelManager().localFrequencies(
            itsVisConverter.id(),
            metadata.centreFreq().getValue("Hz") - chunk->channelWidth() / 2.,
            chunk->channelWidth(),
            corrMode.nChan());

    // at this stage do not support variable phase centre
    const casa::MDirection phaseDir = metadata.phaseDirection();
    chunk->phaseCentre().set(phaseDir.getAngle());

    // Populate the per-antenna vectors
    const casa::MDirection::Ref targetDirRef = metadata.targetDirection().getRef();
    for (casa::uInt i = 0; i < nAntenna; ++i) {
        const string antName = itsVisConverter.config().antennas()[i].name();
        const TosMetadataAntenna mdant = metadata.antenna(antName);
        chunk->targetPointingCentre()[i] = convertToJ2000(chunk->time(), i, metadata.targetDirection());
        chunk->actualPointingCentre()[i] = convertToJ2000(chunk->time(), i, mdant.actualRaDec());

        chunk->actualPolAngle()[i] = mdant.actualPolAngle();

        const casa::Vector<casa::Double> azEl = mdant.actualAzEl().getAngle().getValue("deg");
        ASKAPASSERT(azEl.nelements() == 2);
        chunk->actualAzimuth()[i] = casa::Quantity(azEl[0],"deg");
        chunk->actualElevation()[i] = casa::Quantity(azEl[1], "deg");

        chunk->onSourceFlag()[i] = mdant.onSource();
        // flagging (previously was done when datagram was processed)
        const bool flagged = metadata.flagged() || mdant.flagged() ||
                             !mdant.onSource();
        if (flagged) {
            itsVisConverter.flagAntenna(i);
        }
    }

    return chunk;
}

/// @brief convert direction to J2000
/// @details Helper method to convert given direction to 
/// J2000 (some columns of the MS require fixed frame for
/// all rows, it is handy to convert AzEl directions early
/// in the processing chain).
/// @param[in] epoch UTC time since MJD=0
/// @param[in] ant antenna index (to get position on the ground)
/// @param[in] dir direction measure to convert
/// @return direction measure in J2000
casa::MDirection MergedSource::convertToJ2000(const casa::MVEpoch &epoch, casa::uInt ant, 
                                              const casa::MDirection &dir) const
{
   if (dir.getRef().getType() == casa::MDirection::J2000) {
       // already in J2000
       return dir;
   }
   casa::MPosition pos(casa::MVPosition(itsVisConverter.config().antennas().at(ant).position()), casa::MPosition::ITRF);

   // if performance is found critical (unlikely as we only do it per antenna), we could return a class
   // caching frame as there are at least two calls to this method with the same frame information. 
   casa::MeasFrame frame(casa::MEpoch(epoch, casa::MEpoch::UTC), pos);

   return casa::MDirection::Convert(dir, casa::MDirection::Ref(casa::MDirection::J2000, frame))();
}

void MergedSource::signalHandler(const boost::system::error_code& error,
                                     int signalNumber)
{
    if (signalNumber == SIGTERM || signalNumber == SIGINT || signalNumber == SIGUSR1) {
        itsInterrupted = true;
    }
}

void MergedSource::checkInterruptSignal()
{
    itsIOService.poll();
    if (itsInterrupted) {
        throw InterruptedException();
    }
}


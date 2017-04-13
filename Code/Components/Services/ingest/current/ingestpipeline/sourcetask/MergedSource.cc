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
        IVisSource::ShPtr visSrc) :
     itsMetadataSrc(metadataSrc), itsVisSrc(visSrc),
     itsIdleStream(false), itsBadCycle(false), itsInterrupted(false),
     itsSignals(itsIOService, SIGINT, SIGTERM, SIGUSR1),
     itsLastTimestamp(-1), itsVisConverter(params, config)
{
    ASKAPCHECK(bool(visSrc) == config.receivingRank(), "Receiving ranks should get visibility source object, service ranks shouldn't");

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

/// @brief populate itsVis with next datagram
/// @details This helper method is more or less equivalent to calling
/// next method for the visibility source, but has some logic to 
/// try getting non-zero shared pointer (i.e. some handling of timeouts).
/// @note itsVis may still be void after the call to this method if 
/// timeout has occurred. It is a requirement that itsMetadata object 
//  is valid before this method is called. If itsVis is valid before this 
/// method is called, nothing is done
/// @param[in] maxNoDataRetries maximum number of retries (cycle-long
/// timeouts before giving up). The value of 1 is a special case where timeout
/// cause the cycle to be ignored instead of the exception being thrown.
/// @return true if itsVis is invalid at the completion of this method and cycle must be skipped
bool MergedSource::ensureValidVis(casa::uInt maxNoDataRetries) 
{
   ASKAPDEBUGASSERT(itsMetadata);
   ASKAPDEBUGASSERT(itsVisSrc);
   const CorrelatorMode& mode = itsVisConverter.config().lookupCorrelatorMode(itsMetadata->corrMode());
   const casa::uInt timeout = mode.interval();
   itsBadCycle = false;

   for (uint32_t count = 0; !itsVis && count < maxNoDataRetries; ++count) {
        itsVis = itsVisSrc->next(timeout);
        checkInterruptSignal();
        if (itsVis) { 
            //ASKAPLOG_DEBUG_STR(logger, "Received non-zero itsVis; time="<<bat2epoch(itsVis->timestamp));

            // a hack to account for malformed BAT which can glitch different way for
            // different correlator cards, eventually we should remove this code 
            // (and probably rework the logic of this method which has too much BETA legacy
            //  and also fudged 10s timouts which were probably put there by Ben during early BETA debugging)
            if (itsMetadata->time() != itsVis->timestamp) {
                const casa::uLong timeMismatch = itsVis->timestamp > itsMetadata->time() ? 
                      itsVis->timestamp -  itsMetadata->time() : itsMetadata->time() - itsVis->timestamp;
                if (timeMismatch < mode.interval() / 2u) {
                    ASKAPLOG_ERROR_STR(logger, "Detected BAT glitch between metadata and visibility stream on card "<<
                         itsVisConverter.config().receiverId() + 1<<" mismatch = "<<float(timeMismatch)/1e3<<" ms");
                    ASKAPLOG_DEBUG_STR(logger, "    visibility stream: 0x"<<std::hex<<itsVis->timestamp<<" mdata: 0x"<<std::hex<<
                                               itsMetadata->time()<<" diff (abs value): 0x"<<std::hex<<timeMismatch);
                    ASKAPLOG_DEBUG_STR(logger, "    faking metadata timestamp to read "<<bat2epoch(itsVis->timestamp).getValue());
                    itsMetadata->time(itsVis->timestamp);
                    itsBadCycle = true;
                }
            }
            //
        } else {
            // standard behaviour is to try a few times before aborting
            ASKAPLOG_DEBUG_STR(logger, "Received zero itsVis after "<<count + 1<<" attempt(s)");
       }
   }
   if (!itsVis) {
       ASKAPCHECK(maxNoDataRetries == 1, "Reached maximum number of retries for id="<<itsVisConverter.config().receiverId()<<
            ", the correlator ioc does not seem to send data to this rank. Reached the limit of "<<
            maxNoDataRetries<<" retry attempts");
       // special case - ignoring this stream
       // invalidate metadata to force reading new record
       itsMetadata.reset();
       ASKAPLOG_ERROR_STR(logger, "Stream "<<itsVisConverter.config().receiverId()<<
                      " has no data, most likely correlator IOC is not sending data to this rank. Ignoring this data stream.");
       itsIdleStream = true;
       return true;
   }
   return false;
}

VisChunk::ShPtr MergedSource::next(void)
{
    casa::Timer timer;
    // Used for a timeout
    const long ONE_SECOND = 1000000;
    const long HUNDRED_MILLISECONDS = 100000;

    // Get metadata for a real (i.e. scan id >= 0) scan
    const uint32_t maxNoMetadataRetries = 3;
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
            ASKAPLOG_DEBUG_STR(logger,
                    "Skipping this cycle, metadata indicates SCANID_IDLE");
            count = -1;
        }
    } 
    ASKAPCHECK(itsMetadata, "Metadata streaming ceased, unable to recover after "<<
                            maxNoMetadataRetries<<" attempts");
    ASKAPASSERT(itsMetadata->scanId() != ScanManager::SCANID_IDLE);

    // Update the Scan Manager
    itsScanManager.update(itsMetadata->scanId());
    //ASKAPLOG_DEBUG_STR(logger, "Processing scanId="<<itsMetadata->scanId()<<" mdata time="<<
    //         bat2epoch(itsMetadata->time()).getValue());

    // Check if the TOS/TOM has indicated the observation is complete
    if (itsScanManager.observationComplete()) {
        ASKAPLOG_INFO_STR(logger, "End-of-observation condition met");
        return VisChunk::ShPtr();
    }

    // Protect against producing VisChunks with the same timestamp
    ASKAPCHECK(itsMetadata->time() != itsLastTimestamp,
            "Consecutive VisChunks have the same timestamp");
    itsLastTimestamp = itsMetadata->time();

    if (!itsVisConverter.config().receivingRank()) {
        // service rank - return chunk with zero dimensions 
        VisChunk::ShPtr dummy(new VisChunk(0,0,0,0));
        // invalidate metadata to force reading new record
        itsMetadata.reset();
        return dummy;
    }
    ASKAPDEBUGASSERT(itsVisSrc);
    //const CorrelatorMode& mode = itsVisConverter.config().lookupCorrelatorMode(itsMetadata->corrMode());
    //const casa::uInt timeout = mode.interval();

    VisChunk::ShPtr chunk = createVisChunk(*itsMetadata);
    ASKAPDEBUGASSERT(chunk);

    if (itsIdleStream) {
        if (itsVisSrc->bufferUsage().first > 0) {
            // there is something in the buffer, reactivate receiving
            ASKAPLOG_WARN_STR(logger, "Stream "<<itsVisConverter.config().receiverId()<<
                    " has some data, attempting to reactivate receiving");
            itsIdleStream = false;
        } else {
            // invalidate metadata to force reading new record
            itsMetadata.reset();
            return chunk;
        }
    }

    // Get the next VisDatagram if there isn't already one in the buffer
    const uint32_t maxNoDataRetries = 1;
    if (ensureValidVis(maxNoDataRetries)) {
        return chunk;
    }

    ASKAPASSERT(itsVis);

    // Find data with matching timestamps
    bool logCatchup = true;
    while (itsMetadata->time() != itsVis->timestamp) {

        // If the VisDatagram timestamps are in the past (with respect to the
        // TosMetadata) then read VisDatagrams until they catch up
        if (itsMetadata->time() > itsVis->timestamp) {
            ASKAPDEBUGASSERT(itsVis);
            if (logCatchup) {
                ASKAPLOG_DEBUG_STR(logger, "Reading extra VisDatagrams to catch up for stream id="<<itsVisConverter.config().receiverId()<<
                                           ", metadata time: "<<bat2epoch(itsMetadata->time()).getValue()<<
                                           " visibility time: "<<bat2epoch(itsVis->timestamp).getValue());
                logCatchup = false;
            }
            itsVis.reset();

            if (ensureValidVis(maxNoDataRetries)) {
                return chunk;
            }
            ASKAPASSERT(itsVis);
        }
        checkInterruptSignal();

        if (itsMetadata->time() < itsVis->timestamp) {
            ASKAPLOG_WARN_STR(logger, "Visibility data stream "<<itsVisConverter.config().receiverId()<<" is ahead ("<<bat2epoch(itsVis->timestamp).getValue()<<
                     ") of metadata stream ("<<bat2epoch(itsMetadata->time()).getValue()<<"), skipping the cycle for this card");
            // invalidate metadata to force reading new record
            itsMetadata.reset();
            return chunk;
        }
    }

    //ASKAPLOG_DEBUG_STR(logger, "Aligned datagram time and metadata time: "<<bat2epoch(itsVis->timestamp).getValue());

    // Now the streams are synced, start building a VisChunk
    double decodingTime = 0.;

    // Read VisDatagrams and add them to the VisChunk. If itsVisSrc->next()
    // returns a null pointer this indicates the timeout has been reached.
    // In this case assume no more VisDatagrams for this integration will
    // be recieved and move on
    while (itsVis && itsMetadata->time() >= itsVis->timestamp) {
        checkInterruptSignal();

        if (itsMetadata->time() > itsVis->timestamp) {
            // If the VisDatagram is from a prior integration then discard it
            ASKAPLOG_WARN_STR(logger, "Received VisDatagram from past integration. This shouldn't happen. Stream id = "<<itsVisConverter.config().receiverId());
            itsVis = itsVisSrc->next(HUNDRED_MILLISECONDS);
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
        itsVis = itsVisSrc->next(HUNDRED_MILLISECONDS);
    }

    ASKAPLOG_DEBUG_STR(logger, "VisChunk built with " << itsVisConverter.datagramsCount() <<
            " of expected " << itsVisConverter.datagramsExpected() << " visibility datagrams ("<<
            itsVisConverter.datagramsIgnored()<<" intentionally ignored)");

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

    if (itsBadCycle) {
        chunk->flag().set(true);
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
            itsVisConverter.config().receiverId(),
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


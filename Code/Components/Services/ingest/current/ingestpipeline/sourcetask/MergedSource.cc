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
/// Original BETA code by Ben Humphreys <ben.humphreys@csiro.au>
/// @author Max Voronkov <max.voronkov@csiro.au>

// Include own header file first
#include "MergedSource.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <set>
#include <stdint.h>
#include <iomanip>

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "boost/tuple/tuple.hpp"
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/bind/bind.hpp"
#include "Common/ParameterSet.h"
#include "cpcommon/TosMetadata.h"
#include "cpcommon/VisDatagram.h"
#include "askap/scimath/utils/PolConverter.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/measures/Measures.h"
#include "casacore/measures/Measures/MeasConvert.h"
#include "casacore/measures/Measures/MeasFrame.h"
#include "casacore/measures/Measures/MCEpoch.h"
#include "casacore/measures/Measures/MCDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/casa/OS/Timer.h"
#include "casacore/casa/OS/Time.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/MatrixMath.h"

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
     itsLastTimestamp(-1), itsVisConverter(params, config), 
     itsBeamOffsetsFromMetadata(false), itsBeamOffsetsFromParset(false),
     itsBadUVWCycleCounter(0u), itsMaxBadUVWCycles(params.getInt32("baduvw_maxcycles",-1))
{
    ASKAPCHECK(bool(visSrc) == config.receivingRank(), "Receiving ranks should get visibility source object, service ranks shouldn't");

    if (itsMaxBadUVWCycles < 0) {
        ASKAPLOG_DEBUG_STR(logger, "Ingest pipeline will try to flag samples with UVWs failing the length cross-check");
    } else {
        if (itsMaxBadUVWCycles == 0) {
            ASKAPLOG_DEBUG_STR(logger, "Ingest pipeline will abort if UVWs in metadata fail the length cross-check");
        } else {
            ASKAPLOG_DEBUG_STR(logger, "Ingest pipeline will abort if UVWs in metadata fail the length cross-check for "<<itsMaxBadUVWCycles<<" in a row");
        }
    }

    // log TAI_UTC casacore measures table version and date
    const std::pair<double, std::string> measVersion = measuresTableVersion();
    itsMonitoringPointManager.submitPoint<float>("MeasuresTableMJD", 
            static_cast<float>(measVersion.first));
    itsMonitoringPointManager.submitPoint<std::string>("MeasuresTableVersion", measVersion.second);
    // additional check that the table has been updated less then one month ago
    if (config.receiverId() == 0) {
        casacore::Time now;
        if (now.modifiedJulianDay() - measVersion.first > 30.) {
            ASKAPLOG_ERROR_STR(logger, "Measures table is more than one month old. Consider updating!");
        }
    }
 
    // configure beam offsets behaviour
    const std::string beamOffsetsOrigin = params.getString("beamoffsets_origin", "metadata");
    if (beamOffsetsOrigin == "metadata") {
        ASKAPLOG_DEBUG_STR(logger, "Beam offsets will be taken from metadata stream");
        itsBeamOffsetsFromMetadata = true;
    } else if (beamOffsetsOrigin == "parset") {
        ASKAPLOG_DEBUG_STR(logger, "Static beam offsets will be taken from parset");
        itsBeamOffsetsFromParset = true;
        ASKAPCHECK(config.feedInfoDefined(), "Required information on beam offsets is missing in the parset!");
    } else {
        ASKAPCHECK(beamOffsetsOrigin == "none", "Unsupported beamoffsets_origin: "<<beamOffsetsOrigin);
        ASKAPLOG_DEBUG_STR(logger, "Source task will not load beam offsets");
    }
    ASKAPASSERT(!itsBeamOffsetsFromMetadata || !itsBeamOffsetsFromParset);
    //

    // fill the array layout info which is used to perform cross-checks on UVWs
    // We could've extracted this info on-the-fly, but it is marginally better to take it
    // out of the loop
    const std::vector<Antenna>& antennas = config.antennas();
    itsArrayLayout.resize(antennas.size(), 3u);
    for (casacore::uInt ant = 0; ant < antennas.size(); ++ant) {
         casacore::Vector<casacore::Double> antPos = antennas[ant].position();
         ASKAPCHECK(antPos.nelements() == 3, "Expect exactly 3 elements for antenna "<<ant<<" position");
         itsArrayLayout.row(ant) = antPos;
    }
    //

    // Setup a signal handler to catch SIGINT, SIGTERM and SIGUSR1
    itsSignals.async_wait(boost::bind(&MergedSource::signalHandler, this, boost::placeholders::_1, boost::placeholders::_2));
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
bool MergedSource::ensureValidVis(casacore::uInt maxNoDataRetries) 
{
   ASKAPDEBUGASSERT(itsMetadata);
   ASKAPDEBUGASSERT(itsVisSrc);
   const CorrelatorMode& mode = itsVisConverter.config().lookupCorrelatorMode(itsMetadata->corrMode());
   const casacore::uInt timeout = mode.interval();
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
                const casacore::uLong timeMismatch = itsVis->timestamp > itsMetadata->time() ? 
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
    casacore::Timer timer;
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
    //const casacore::uInt timeout = mode.interval();

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
    uint64_t lastCatchupVisBAT = 0ul;
    while (itsMetadata->time() != itsVis->timestamp) {

        // If the VisDatagram timestamps are in the past (with respect to the
        // TosMetadata) then read VisDatagrams until they catch up
        if (itsMetadata->time() > itsVis->timestamp) {
            ASKAPDEBUGASSERT(itsVis);
            if (!logCatchup && lastCatchupVisBAT != itsVis->timestamp) {
                logCatchup = true;
            }
            if (logCatchup) {
                ASKAPLOG_DEBUG_STR(logger, "Reading extra VisDatagrams to catch up for stream id="<<itsVisConverter.config().receiverId()<<
                                           ", metadata time: "<<bat2epoch(itsMetadata->time()).getValue()<<
                                           " visibility time: "<<bat2epoch(itsVis->timestamp).getValue());
                logCatchup = false;
                lastCatchupVisBAT = itsVis->timestamp;
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

/// @brief helper method to flag and report on bad UVWs
/// @details It decomposes the given rows back into antennas and report in the log with
/// different severity depending on the stream ID (to avoid spamming the log).
/// This method is a work around of UVW metadata problem (see ASKAPSDP-3431)
/// @param[in] rowsWithBadUVWs set of rows to flag
/// @param[in] timestamp BAT for reporting
void MergedSource::flagDueToBadUVWs(const std::set<casacore::uInt> &rowsWithBadUVWs, const casacore::uLong timestamp) {
    //ASKAPLOG_DEBUG_STR(logger, "Inside flagDueToBadUVWs, nRowsWithBadUVWs = "<<rowsWithBadUVWs.size());    

    ASKAPDEBUGASSERT(rowsWithBadUVWs.size() > 0);
    VisChunk::ShPtr chunk = itsVisConverter.visChunk();
    ASKAPDEBUGASSERT(chunk);
    const casacore::uInt nAntenna = itsVisConverter.config().antennas().size();
    ASKAPDEBUGASSERT(nAntenna > 0);
    const casacore::Vector<casacore::uInt>& antenna1 = chunk->antenna1();
    const casacore::Vector<casacore::uInt>& antenna2 = chunk->antenna2();
    // 1) get set of all antennas and good antennas separately
    std::set<casacore::uInt> goodAntennas;
    std::set<casacore::uInt> antennas;
    for (casacore::uInt row = 0; row<chunk->nRow(); ++row) {
         ASKAPDEBUGASSERT(row < antenna1.nelements());
         ASKAPDEBUGASSERT(row < antenna2.nelements());
         const casacore::uInt ant1 = antenna1[row];
         const casacore::uInt ant2 = antenna2[row];
         antennas.insert(ant1);
         antennas.insert(ant2);
         if ((ant1 != ant2) && (rowsWithBadUVWs.find(row) == rowsWithBadUVWs.end()) && 
                                itsVisConverter.isAntennaGood(ant1) && itsVisConverter.isAntennaGood(ant2)) {
             goodAntennas.insert(ant1);
             goodAntennas.insert(ant2);
         }
    }

    //ASKAPLOG_DEBUG_STR(logger, "Inside flagDueToBadUVWs, nAntennas = "<<antennas.size()<<" nGoodAntennas = "<<goodAntennas.size());    

    // 2) flag antennas which are not in the good list, build the list for reporting
    std::string listOfBadAntennas;
    for (std::set<casacore::uInt>::const_iterator ciAnt = antennas.begin(); ciAnt != antennas.end(); ++ciAnt) {
         if (goodAntennas.find(*ciAnt) == goodAntennas.end()) {
             ASKAPASSERT(*ciAnt < nAntenna);
             // only proceed if antenna is not flagged already
             if (itsVisConverter.isAntennaGood(*ciAnt)) {
                 itsVisConverter.flagAntenna(*ciAnt);
                 const string antName = itsVisConverter.config().antennas()[*ciAnt].name();
                 if (listOfBadAntennas.size() == 0) {
                     listOfBadAntennas = antName;
                 } else {
                     listOfBadAntennas += ", " + antName;
                 }
             }
         }
    }
    
    if (listOfBadAntennas.size() == 0) {
        listOfBadAntennas = "none";
    }

    //ASKAPLOG_DEBUG_STR(logger, "Inside flagDueToBadUVWs, listOfBadAntennas: "<<listOfBadAntennas);    

    // 3) check that anything is left (this shouldn't happen unless we have tricky per-beam issues)
    casacore::uInt nExplicitlyFlaggedRows = 0;
    casacore::Cube<casacore::Bool> &flags = chunk->flag();
    for (std::set<casacore::uInt>::const_iterator ci = rowsWithBadUVWs.begin(); ci != rowsWithBadUVWs.end(); ++ci) {
         ASKAPASSERT(*ci < chunk->nRow());
         const casacore::uInt ant1 = antenna1[*ci];
         const casacore::uInt ant2 = antenna2[*ci];
         if (itsVisConverter.isAntennaGood(ant1) && itsVisConverter.isAntennaGood(ant2)) {
             ++nExplicitlyFlaggedRows;
             ASKAPDEBUGASSERT(*ci < flags.nrow());
             flags.yzPlane(*ci).set(true);
         }
    }
    std::string msg = "Flagged the following antennas due to failed uvw vector length check: "+listOfBadAntennas+
            " (currently "+utility::toString<casacore::uInt>(itsBadUVWCycleCounter)+" cycle in a row).";
    if (nExplicitlyFlaggedRows != 0) {
        msg += " In addition, "+utility::toString<casacore::uInt>(nExplicitlyFlaggedRows)+
               " rows were flagged, which do not correpond to all baselines of some set of antennas.";
    }
    // we could've reversed chunk timestamp, but it is handy to pass what is in the metadata directly to avoid nasty surprises with precision
    if (itsVisConverter.config().receiverId() == 0) {
       ASKAPLOG_ERROR_STR(logger, "" << msg << " Timestamp: "<<bat2epoch(timestamp)<<" or 0x"<<std::hex<<timestamp);

       // some commissioning code below to store the details on affected baselines into a file 
       // we may need to comment it out in the future or to make it optional based on parset parameter
       const casacore::Vector<casacore::uInt>& beam1 = chunk->beam1();
       ASKAPDEBUGASSERT(beam1.nelements() == chunk->nRow());
       std::ofstream os("baduvw_baselines.dbg", std::ios::app);
       os << "# "<< msg << " Timestamp: "<<bat2epoch(timestamp)<<" or 0x"<<std::hex<<timestamp<<" = "<<std::dec<<timestamp<<" :"<<std::endl;
       for (std::set<casacore::uInt>::const_iterator ci = rowsWithBadUVWs.begin(); ci != rowsWithBadUVWs.end(); ++ci) {
            ASKAPDEBUGASSERT(*ci < chunk->nRow());
            const casacore::uInt ant1 = antenna1[*ci];
            const casacore::uInt ant2 = antenna2[*ci];
            const casacore::uInt beam = beam1[*ci];
            ASKAPDEBUGASSERT(ant1 < nAntenna);
            ASKAPDEBUGASSERT(ant2 < nAntenna);
            const string ant1Name = itsVisConverter.config().antennas()[ant1].name();
            const string ant2Name = itsVisConverter.config().antennas()[ant2].name();
            const std::string explicitlyFlaggedRowMark = (itsVisConverter.isAntennaGood(ant1) && itsVisConverter.isAntennaGood(ant2)) ? " *" : "";
            os<<*ci<<" "<<ant1<<" "<<ant2<<" "<<beam<<" "<<ant1Name<<" "<<ant2Name;
            if (itsVisConverter.isAntennaGood(ant1) && itsVisConverter.isAntennaGood(ant2)) {
                os<<" *";
            }
            os<<std::endl;
       }
       // end of the commissioning hack
    } else {
       ASKAPLOG_INFO_STR(logger, "" << msg << " Timestamp: "<<bat2epoch(timestamp)<<" or 0x"<<std::hex<<timestamp);
    }
}

VisChunk::ShPtr MergedSource::createVisChunk(const TosMetadata& metadata)
{
    const CorrelatorMode& corrMode = itsVisConverter.config().lookupCorrelatorMode(metadata.corrMode());

    itsVisConverter.initVisChunk(metadata.time(), corrMode);
    VisChunk::ShPtr chunk = itsVisConverter.visChunk();
    ASKAPDEBUGASSERT(chunk);

    const casacore::uInt nAntenna = itsVisConverter.config().antennas().size();
    ASKAPCHECK(nAntenna > 0, "Must have at least one antenna defined");
    ASKAPASSERT(nAntenna == itsArrayLayout.nrow());
    ASKAPDEBUGASSERT(3u == itsArrayLayout.ncolumn());

    // Add the scan index
    chunk->scan() = itsScanManager.scanIndex();

    chunk->targetName() = metadata.targetName();
    chunk->directionFrame() = metadata.phaseDirection().getRef();

    // Determine and add the spectral channel width
    chunk->channelWidth() = corrMode.chanWidth().getValue("Hz");

    //const bool unsupported = chunk->nRow() > (casacore::max(chunk->beam1()) + 1)*406u;

    // Build frequencies vector
    // Frequency vector is not of length nRows, but instead nChannels
    chunk->frequency() = itsVisConverter.channelManager().localFrequencies(
            itsVisConverter.config().receiverId(),
            metadata.centreFreq().getValue("Hz") - chunk->channelWidth() / 2. + corrMode.freqOffset().getValue("Hz"),
            chunk->channelWidth(),
            corrMode.nChan());

    // at this stage do not support variable phase centre
    const casacore::MDirection phaseDir = metadata.phaseDirection();
    chunk->phaseCentre().set(phaseDir.getAngle());

    // The following buffer is used only to get uvw's in the right form.
    // It is possible to avoid buffering and/or do better cross-checks make the
    // code more flexible later on, if necessary.
    // Dimensions are nAntenna x nBeam(in uvw metadata)
    casacore::Matrix<casacore::Double> uvwBuffer;
    
    // Populate the per-antenna vectors
    const casacore::MDirection::Ref targetDirRef = metadata.targetDirection().getRef();
    for (casacore::uInt i = 0; i < nAntenna; ++i) {
        const string antName = itsVisConverter.config().antennas()[i].name();
        const TosMetadataAntenna mdant = metadata.antenna(antName);
        chunk->targetPointingCentre()[i] = convertToJ2000(chunk->time(), i, metadata.targetDirection());
        chunk->actualPointingCentre()[i] = convertToJ2000(chunk->time(), i, mdant.actualRaDec());

        chunk->actualPolAngle()[i] = mdant.actualPolAngle();

        const casacore::Vector<casacore::Double> azEl = mdant.actualAzEl().getAngle().getValue("deg");
        ASKAPASSERT(azEl.nelements() == 2);
        chunk->actualAzimuth()[i] = casacore::Quantity(azEl[0],"deg");
        chunk->actualElevation()[i] = casacore::Quantity(azEl[1], "deg");

        chunk->onSourceFlag()[i] = mdant.onSource();
        // flagging (previously was done when datagram was processed)
        const bool flagged = metadata.flagged() || mdant.flagged() ||
                             !mdant.onSource();
        if (flagged) {
            itsVisConverter.flagAntenna(i);
        } else {
            // filling uvw buffer for each antenna
            if (uvwBuffer.nelements() == 0) {
                uvwBuffer.resize(nAntenna, mdant.uvw().nelements());
            }
            ASKAPDEBUGASSERT(uvwBuffer.nrow() == nAntenna);
            ASKAPCHECK(uvwBuffer.ncolumn() == mdant.uvw().nelements(), 
                 "The uvw vector in the metadata changes size from antenna to antenna, this is unexpected. Offending antenna "<<antName); 
            uvwBuffer.row(i) = mdant.uvw();
            /*
            // for debug
            if ((itsVisConverter.config().receiverId() == 0) && (i == 3)) {
                ASKAPASSERT(uvwBuffer.row(i).nelements()>=3);
                ASKAPLOG_INFO_STR(logger, "Antenna id="<<i<<" time: "<<std::fixed<<std::setprecision(15)<<chunk->time()<<" uvw: "<<std::fixed<<std::setprecision(15)<<uvwBuffer.row(i)[0]<<" "<<
                      uvwBuffer.row(i)[1]<<" "<<uvwBuffer.row(i)[2]);
            }
            //
            */
            ASKAPCHECK(uvwBuffer.ncolumn() % 3 == 0, "Expect UVW metadata to be a vector with the length which is an integral multiple of 3");
            for (casacore::uInt beam = 0; beam < uvwBuffer.ncolumn() / 3; ++beam) {
                 casacore::Double bslnNorm2 = 0.;
                 for (casacore::uInt offset = beam * 3; offset < (beam + 1) * 3; ++offset) {
                      ASKAPDEBUGASSERT(offset < uvwBuffer.ncolumn());
                      const casacore::Double curVal = uvwBuffer(i, offset);
                      ASKAPCHECK(!std::isnan(curVal), "NaN encountered in UVW received in metadata for antenna: "<<antName);
                      bslnNorm2 += casacore::square(curVal);
                 }
                 ASKAPCHECK(bslnNorm2 > 1e-12, "Expect non-zero per-antenna UVW in metadata - encountered a vector which is the Earth centre. Most likely junk metadata received for antenna: "<<antName<<" and (1-based) beam "<<beam + 1);
                 ASKAPCHECK(bslnNorm2 < 4.07044e13, "Encountered UVW vector which suggests an antenna lies way beyond Earth's surface. Most likely junk metadata received for antenna: "<<antName<<" and (1-based) beam "<<beam + 1);
            }     
        }
    }
    /*
    if (unsupported) {
        chunk.reset(new VisChunk(0,0,0,0));
    }
    */
    // now populate uvw vector in the chunk
    std::set<casacore::uInt> rowsWithBadUVWs;
    for (casacore::uInt row=0; row<chunk->nRow(); ++row) {
         // it is possible to move access methods outside the loop, but the overhead is small
         const casacore::uInt beam = chunk->beam1()[row];
         ASKAPCHECK(beam == chunk->beam2()[row], "Cross-beam correlations are not supported at the moment");
         const casacore::uInt antenna1 = chunk->antenna1()[row];
         const casacore::uInt antenna2 = chunk->antenna2()[row];
         ASKAPASSERT(antenna1 < nAntenna);
         ASKAPASSERT(antenna2 < nAntenna);
         if (itsVisConverter.isAntennaGood(antenna1) && itsVisConverter.isAntennaGood(antenna2)) {
             double uvwLength2 = 0., layoutLength2 = 0.;
             for (casacore::uInt coord = 0, offset = beam * 3; coord < 3; ++coord,++offset) {
                  ASKAPASSERT(offset < uvwBuffer.ncolumn());
                  chunk->uvw()[row](coord) = uvwBuffer(antenna1,offset) - uvwBuffer(antenna2,offset);
                  ASKAPCHECK(!std::isnan(chunk->uvw()[row](coord)), "Received NaN as one of the baseline spacings for row="<<row<<" (antennas: "<<
                             antenna1<<" "<<antenna2<<") coordinate="<<coord<<" beam="<<beam);
                  uvwLength2 += casacore::square(chunk->uvw()[row](coord));
                  layoutLength2 += casacore::square(itsArrayLayout(antenna1,coord) - itsArrayLayout(antenna2,coord));
             }
             if (casacore::abs(casacore::sqrt(uvwLength2) - casacore::sqrt(layoutLength2)) >= 1e-3) {
                 rowsWithBadUVWs.insert(row);
                 if ((static_cast<int>(itsBadUVWCycleCounter) >= itsMaxBadUVWCycles) && (itsMaxBadUVWCycles >= 0)) {
                     ASKAPTHROW(CheckError, "The length of uvw vector for row="<<row<<" (antennas: "<<
                             antenna1<<" ("<<itsVisConverter.config().antennas()[antenna1].name()<<") "<<antenna2<<" ("<<itsVisConverter.config().antennas()[antenna2].name()<<
                             "), beam: "<<beam<<") is more than 1mm different from the baseline length expected from array layout ("<<
                             casacore::sqrt(uvwLength2)<<" metres vs. "<<casacore::sqrt(layoutLength2)<<" metres). Junk metadata are suspected for either of the antennas for epoch "<<
                             bat2epoch(metadata.time())<<" (this is "<<(itsMaxBadUVWCycles + 1)<<" consecutive cycle which failed the check)");
                 } 
             }
         }
    }
    if (rowsWithBadUVWs.size() > 0) {
        ++itsBadUVWCycleCounter;
        // flag antennas or isolated rows which didn't pass the check
        // metadata BAT is passed just for reporting
        flagDueToBadUVWs(rowsWithBadUVWs, metadata.time());
    } else {
        itsBadUVWCycleCounter = 0;
    }
     
    if (itsBeamOffsetsFromParset) {
        // populate beam offsets from configuration here
        itsVisConverter.config().feed().fillMatrix(chunk->beamOffsets());
    }
    if (itsBeamOffsetsFromMetadata) {
        // populate beam offsets from metadata
        chunk->beamOffsets().reference(metadata.beamOffsets().copy());
    }
    

    /*
    if (unsupported) {
        ASKAPLOG_FATAL_STR(logger, "Attempted to observe in unsupported ingest configuration, results are undefined");
    }
    */

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
casacore::MDirection MergedSource::convertToJ2000(const casacore::MVEpoch &epoch, casacore::uInt ant, 
                                              const casacore::MDirection &dir) const
{
   if (dir.getRef().getType() == casacore::MDirection::J2000) {
       // already in J2000
       return dir;
   }
   casacore::MPosition pos(casacore::MVPosition(itsVisConverter.config().antennas().at(ant).position()), casacore::MPosition::ITRF);

   // if performance is found critical (unlikely as we only do it per antenna), we could return a class
   // caching frame as there are at least two calls to this method with the same frame information. 
   casacore::MeasFrame frame(casacore::MEpoch(epoch, casacore::MEpoch::UTC), pos);

   return casacore::MDirection::Convert(dir, casacore::MDirection::Ref(casacore::MDirection::J2000, frame))();
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


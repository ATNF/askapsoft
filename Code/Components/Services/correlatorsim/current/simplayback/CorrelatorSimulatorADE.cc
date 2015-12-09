/// @file CorrelatorSimulatorADE.cc
///
/// @copyright (c) 2015 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>

// Include own header file first
#include "simplayback/CorrelatorSimulatorADE.h"

// Include package level header file
#include "askap_correlatorsim.h"

// System includes
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip> 
#include <vector>
#include <cmath>
#include <unistd.h>
#include <inttypes.h>
#include <algorithm>
#include <mpi.h>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "askap/AskapLogging.h"
#include "ms/MeasurementSets/MeasurementSet.h"
#include "ms/MeasurementSets/MSColumns.h"
#include "casa/Arrays/Matrix.h"
#include "casa/Arrays/Vector.h"
#include "measures/Measures/MDirection.h"
#include "measures/Measures/Stokes.h"
#include "cpcommon/VisDatagramADE.h"

// casa
#include <measures/Measures/MEpoch.h>
#include <measures/Measures/MeasConvert.h>
#include <measures/Measures/MCEpoch.h>

// Local package includes
#include "simplayback/CorrProdMap.h"
#include "simplayback/CorrBuffer.h"
#include "simplayback/DatagramLimit.h"

#define VERBOSE
// Expand coarse channels into fine channels
#define CHANNEL_EXPAND
// Cases for shelf count: 1, 2, N (N>0)
#define SHELFCASE 2


#define SIZEOF_ARRAY(a) (sizeof( a ) / sizeof( a[ 0 ] ))

using namespace askap;
using namespace askap::cp;
using namespace casa;

// Forward declaration
void checkStokesType (const Stokes::StokesTypes stokestype);
uint32_t getCorrProdIndex (const uint32_t ant1, const uint32_t ant2, 
		const Stokes::StokesTypes stokestype);


ASKAP_LOGGER(logger, ".CorrelatorSimulatorADE");


CorrelatorSimulatorADE::CorrelatorSimulatorADE(const std::string& dataset,
        const std::string& hostname,
        const std::string& port,
        const int shelf,
        const unsigned int nAntennaIn,
        const unsigned int nCoarseChannel,
        const unsigned int nChannelSub,
        const double coarseBandwidth,
        const std::string& inputMode,
        const unsigned int delay)
        : itsShelf(shelf),
        itsNAntenna(nAntennaIn), itsNCorrProd(0), itsNSlice(0),
        itsNCoarseChannel(nCoarseChannel),
        itsNChannelSub(nChannelSub), itsCoarseBandwidth(coarseBandwidth),
        itsFineBandwidth(0.0), itsInputMode(inputMode), 
        itsCurrentTime(0), itsDelay(delay), itsCurrentRow(0),
        firstPayloadSent(false)
{
    itsMS.reset(new casa::MeasurementSet(dataset, casa::Table::Old));
	itsPort.reset(new askap::cp::VisPortADE(hostname, port));

    initBuffer();
}



CorrelatorSimulatorADE::~CorrelatorSimulatorADE()
{
    itsMS.reset();
    itsPort.reset();
}



bool CorrelatorSimulatorADE::sendNext(void)
{
    uint64_t previousTime = itsCurrentTime;

    // Get buffer data from measurement set
    if (getBufferData()) {  // if successful in getting data ...

        // The data from measurement set does not fill the whole buffer,
        // so fill in the missing data by copying from existing one.
        // First, fill in the data for the missing correlation products
        // due to antennas not present in measurement set.
        fillCorrProdInBuffer();

        // Then, fill in the data for coarse channels not present in
        // measurement set.
        fillChannelInBuffer();

        // Delay transmission for every new time stamp in measurement
        if (itsCurrentTime > previousTime) {
            cout << "Shelf " << itsShelf;
            cout << " detects new time stamp: " << itsCurrentTime << endl;
            double delay = static_cast<double>(itsDelay) / 1000000.0;
            cout << "Shelf " << itsShelf << " pauses transmission for" << 
                    delay << " seconds" << endl;
            usleep(itsDelay);
            cout << "Shelf " << itsShelf << " resumes transmission" << endl;
        }

        // Shelf 1 must send the first payload.
        // Other shelves must wait for this.
        //cout << "Initial: Shelf " << itsShelf << ": firstPayloadSent? ";
        //cout << firstPayloadSent << endl;
        while (!firstPayloadSent) {
            if (itsShelf == 1) {
                firstPayloadSent = sendFirstPayload();
                MPI_Bcast(&firstPayloadSent,1,MPI_LOGICAL,1,MPI_COMM_WORLD);
                if (!firstPayloadSent) {
                    cout << "Shelf " << itsShelf;
                    cout << " cannot send the first payload" << endl;
                    return false;
                }
            }
            else {  // other shelves
                //usleep(1);
                MPI_Bcast(&firstPayloadSent,1,MPI_LOGICAL,1,MPI_COMM_WORLD);
            }
        }
        //cout << "After: Shelf " << itsShelf << ": firstPayloadSent? ";
        //cout << firstPayloadSent << endl;

        return sendBufferData();
    }
    else {
        cout << "No more data in measurement set" << endl;
        return false;
    }
	return true;
}



// Initialize buffer.
// It's a 2D array with the size of the total number of correlation products
// and coarse channels.
//
void CorrelatorSimulatorADE::initBuffer()
{
    cout << "Initializing buffer ..." << endl;
    ROMSColumns msc(*itsMS);

    // Get reference to columns of interest
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casa::ROMSAntennaColumns& antc = msc.antenna();
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();
    const uint32_t nRow = msc.nrow();
    cout << "  Total rows in measurement set: " << nRow << endl;
    cout << "  Checking all rows ..." << endl;
    casa::Double currentTime = 0.0;

    uint32_t dataDescId;
    uint32_t descSpwId, descPolId;
    uint32_t nChan = 0;
    uint32_t nCorr = 0;
    int32_t currentBeam = 0;
    int32_t antMin = 9999;
    int32_t antMax = 0;
    int32_t beamMin = 9999;
    int32_t beamMax = 0;
    uint32_t nTime = 0;
    uint32_t nRowOfConstTime;
    uint32_t nRowOfConstBeam;
    for (uint32_t row = 0; row < nRow; ++row) {

        if (msc.time()(row) > currentTime) {
            currentTime = msc.time()(row);
            ++nTime;
            //cout << "    Time changed in row " << row << ": " 
            //        << currentTime << endl;
            //cout << "    Rows of constant time: " << 
            //        nRowOfConstTime << endl;
            nRowOfConstTime = 1;
        }
        else {
            ++nRowOfConstTime;
        }

        if (msc.feed1()(row) != currentBeam) {
            currentBeam = msc.feed1()(row);
            nRowOfConstBeam = 1;
        }
        else {
            ++nRowOfConstBeam;
        }
            
        dataDescId = msc.dataDescId()(row);
        descSpwId = ddc.spectralWindowId()(dataDescId);
        nChan = spwc.numChan()(descSpwId);
        descPolId = ddc.polarizationId()(dataDescId);
        nCorr = polc.numCorr()(descPolId);
        if (nCorr != 4) {
            cout << "    Row " << row << " has " << nCorr << 
                    " correlations" << endl;
        }
        
        antMin = min(antMin,msc.antenna1()(row));
        antMax = max(antMax,msc.antenna1()(row));

        beamMin = min(beamMin,msc.feed1()(row));
        beamMax = max(beamMax,msc.feed1()(row));
    }
    cout << "  Checking all rows: done" << endl;
    
    cout << "  Time interval count  : " << nTime << endl;
    //cout << "  Rows of constant time: " << nRowOfConstTime << endl;
    cout << "  Beam range           : " << beamMin << " ~ " << beamMax << endl;
    //cout << "  Rows of constant beam: " << nRowOfConstBeam << endl;

    // Antennas
    const uint32_t nAntMeas = antc.nrow();
    const uint32_t nAntMeasCheck = antMax - antMin + 1;
    ASKAPCHECK(nAntMeas == nAntMeasCheck, 
            "Disagreement in antenna count in measurement set");
    cout << "  Antenna count in measurement set: " << nAntMeas << endl;
    for (uint32_t ant = 0; ant < nAntMeas; ++ant) {
        string antName = antc.name()(ant);
        string antNameNumber = antName.substr(2,2);
        stringstream convert(antNameNumber);
        uint32_t antIndex;
        convert >> antIndex;
        antIndices.push_back(antIndex-1);   // antenna name is 1-based
        cout << "  antenna name: " << antName << " -> index: " << 
               antIndices[ant] << endl;
    }
    
    //int nAntMeas = antMax - antMin + 1; // from measurement data
    uint32_t nAntCorr = itsNAntenna;         // from parameter file
    //cout << "  Antenna count in measurement set: " << nAntMeas << endl;
    cout << "  Antenna count in correlator sim : " << nAntCorr << endl;
    
    // Correlation products, calculated from parameter file
    // Note that correlator may send more data than available antenna
    // (which means empty data will get sent too).
    itsNCorrProd = itsCorrProdMap.getTotal (nAntCorr);
    cout << "  Correlation product count       : " << itsNCorrProd << endl;

    // channel count
    buffer.nChanMeas = nChan;
    cout << "  Channel count: " << nChan << endl;

    // Data matrix
    cout << "  Measurement set data matrix (corr,channel): " << nCorr << 
            ", " << nChan << endl;

    // Creating buffer
    // Note that buffer contains all correlation products (as requested in
    // parset, which is usually more than available in measurement set) and
    // all channels (as requested in parset)
    cout << "  Creating buffer according to parset ..." << endl;
    buffer.init(itsNCorrProd, itsNCoarseChannel);
    cout << "    correlation products x coarse channels: " << itsNCorrProd << 
            " x " << itsNCoarseChannel << endl; 
    cout << "  Creating buffer: done" << endl;

    // card count
    buffer.nCard = itsNCoarseChannel / DATAGRAM_CHANNELMAX + 1;
    cout << "  Card count computed from parset: " << buffer.nCard << endl;

    cout << "Initializing buffer: done" << endl;
    cout << endl;
}   // initBuffer



bool CorrelatorSimulatorADE::getBufferData()
{
#ifdef VERBOSE
    //cout << "Getting buffer data ..." << endl;
#endif
    ROMSColumns msc(*itsMS);

    // Get reference to columns of interest
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();

    const uint32_t nRow = msc.nrow();
    if (itsCurrentRow >= nRow) {
#ifdef VERBOSE
        cout << "  No more data available" << endl;
        cout << "Getting buffer data: done" << endl;
#endif
        return false;
    }

    // Note, the measurement set stores integration midpoint (in seconds), 
    // while the TOS (and it is assumed the correlator) deal with 
    // integration start (in microseconds).
    // In addition, TOS time is BAT and the measurement set normally has 
    // UTC time (the latter is not checked here as we work with the column 
    // as a column of doubles rather than column of measures)
    // Precision of a single double may not be enough in general, 
    // but should be fine for this emulator (ideally need to represent 
    // time as two doubles)
    const casa::Double currentTime = msc.time()(itsCurrentRow);
    const casa::MEpoch epoch(
            casa::MVEpoch(casa::Quantity(currentTime,"s")),
            casa::MEpoch::Ref(casa::MEpoch::UTC));
    const casa::MVEpoch epochTAI = casa::MEpoch::Convert(epoch,
            casa::MEpoch::Ref(casa::MEpoch::TAI))().getValue();
    const uint64_t microsecondsPerDay = 86400000000ull;
    const uint64_t startOfDayBAT = 
            uint64_t(epochTAI.getDay()*microsecondsPerDay);
    const long Tint = static_cast<long>(msc.interval()(itsCurrentRow) 
            * 1000000);
    const uint64_t startBAT = startOfDayBAT +
            uint64_t(epochTAI.getDayFraction() * microsecondsPerDay) -
            uint64_t(Tint / 2);

    // ideally we need to carry 64-bit BAT in the payload explicitly
    buffer.timeStamp = static_cast<long>(startBAT);
    itsCurrentTime = buffer.timeStamp;
    buffer.beam = static_cast<uint32_t>(msc.feed1()(itsCurrentRow));
#ifdef VERBOSE
    //cout << "  Time " << buffer.timeStamp << ", beam " << buffer.beam << endl;
#endif

    while ((itsCurrentRow < nRow) &&
        (static_cast<uint32_t>(msc.feed1()(itsCurrentRow)) == buffer.beam)) {

        uint32_t dataDescId = msc.dataDescId()(itsCurrentRow);
        uint32_t descSpwId = ddc.spectralWindowId()(dataDescId);
        uint32_t nChan = spwc.numChan()(descSpwId);
        uint32_t descPolId = ddc.polarizationId()(dataDescId);
        uint32_t nCorr = polc.numCorr()(descPolId);
        casa::Matrix<casa::Complex> data = msc.data()(itsCurrentRow);
        const casa::Vector<casa::Double> frequencies = 
                spwc.chanFreq()(descSpwId);
        ASKAPCHECK(nChan == frequencies.size(), 
                "Disagreement in the number of channels in measurement set");
        //buffer.nChanMeas = nChan;
        for (uint32_t chan = 0; chan < nChan; ++chan) {
            buffer.freqId[chan].block = 1;          // dummy value
            buffer.freqId[chan].card = 1;           // dummy value
            buffer.freqId[chan].channel = chan + 1; // 1-based
            buffer.freqId[chan].freq = frequencies[chan];
        }

        uint32_t ant1 = antIndices[msc.antenna1()(itsCurrentRow)];
        uint32_t ant2 = antIndices[msc.antenna2()(itsCurrentRow)];
        if (ant1 > ant2) {
            std::swap(ant1,ant2);
        }

        casa::Vector<casa::Int> stokesTypesInt = polc.corrType()(descPolId);
        //cout << "    antenna " << ant1 << ", " << ant2 << endl;
        for (uint32_t corr = 0; corr < nCorr; ++corr) {
            Stokes::StokesTypes stokesType = Stokes::type(stokesTypesInt(corr));
            if (stokesType == Stokes::XX) {
                corr = 0;
            }
            else if (stokesType == Stokes::XY) {
                corr = 1;
            }
            else if (stokesType == Stokes::YX) {
                corr = 2;
            }
            else if (stokesType == Stokes::YY) {
                corr = 3;
            }
            else {
                checkStokesType (stokesType);
            }

            // put visibility data into buffer, 
            // except when the Stokes type is YX for the same antenna
            if ((ant1 != ant2) || (stokesType != Stokes::YX)) {
                //cout << ant1 << ", " << ant2 << ", " << corr << endl;
                uint32_t corrProd = itsCorrProdMap.getIndex(ant1, ant2, corr);
                //cout << "  corrProd: " << corrProd << endl;
                //cout << "  size: " << buffer.corrProdIsFilled.size() << endl;
                ASKAPCHECK(!buffer.corrProdIsFilled[corrProd],
                        "Correlator product " << corrProd << 
                        " is already filled");
                for (uint32_t chan = 0; chan < nChan; ++chan) {
                    buffer.data[corrProd][chan].vis.real = 
                            data(corr,chan).real();
                    buffer.data[corrProd][chan].vis.imag = 
                            data(corr,chan).imag();
                }   // channel
                buffer.corrProdIsFilled[corrProd] = true;
                buffer.corrProdIsOriginal[corrProd] = true;
            }
        }   // correlation
        ++itsCurrentRow;
    }   // row

    buffer.ready = true;

#ifdef VERBOSE
    //cout << "Getting buffer data: done" << endl;
#endif
    return (buffer.ready);
}   // getBuffer



// Fill empty correlation product data from original ones (from 
// measurement set)
void CorrelatorSimulatorADE::fillCorrProdInBuffer()
{
#ifdef VERBOSE
//    cout << "Filling empty correlation products in buffer ..." << endl;
#endif

    // set initial correlation product index to force search from beginning
    int32_t originalCP = -1; 
    for (uint32_t cp = 0; cp < buffer.data.size(); ++cp) {
        // if the correlation product has no data
        if (!buffer.corrProdIsFilled[cp]) {
            // find the next available original data
            originalCP = buffer.findNextOriginalCorrProd(originalCP);

            // if found
            if (originalCP >= 0) {
                // fill in the empty correlation product with original data
                buffer.copyCorrProd(originalCP, cp);
            }
            // if none can be found
            else {
                // search from the beginning
                originalCP = buffer.findNextOriginalCorrProd(-1);
                ASKAPCHECK(originalCP >= 0, 
                        "Still cannot find original data");
                buffer.copyCorrProd(originalCP, cp);
            }
        }
    }   // correlation product

    // check
    for (uint32_t cp = 0; cp < buffer.data.size(); ++cp) {
        ASKAPCHECK(buffer.corrProdIsFilled[cp], "Correlation product " <<
                cp << " is still empty");
    }

#ifdef VERBOSE
//    cout << "Filling empty correlation products in buffer: done" << endl;
#endif

}   // fillCorrProdInBuffer



// Fill empty channel data from original ones (from measurement set)
//
void CorrelatorSimulatorADE::fillChannelInBuffer()
{
#ifdef VERBOSE
//    cout << "Filling empty channels in buffer ..." << endl;
#endif
    uint32_t sourceChan = buffer.nChanMeas - 1;

    // Frequency increment
    const double freqInc = buffer.freqId[1].freq - buffer.freqId[0].freq;

    for (uint32_t chan = buffer.nChanMeas; 
            chan < itsNCoarseChannel; ++chan) {

        buffer.freqId[chan].block = buffer.freqId[sourceChan].block;
        buffer.freqId[chan].card = buffer.freqId[sourceChan].card;
        buffer.freqId[chan].channel = chan + 1; // 1-based
        buffer.freqId[chan].freq = buffer.freqId[0].freq + freqInc * chan;
    }
#ifdef VERBOSE
//    cout << "Filling empty channels in buffer: done" << endl;
#endif

}   // fillChannelInBuffer



// TO BE DEPRECATED
//
void CorrelatorSimulatorADE::renumberChannelAndCard() {
#ifdef VERBOSE
    cout << "Renumbering channels and cards ..." << endl;
#endif
    for (uint32_t chan = 0; chan < itsNCoarseChannel; ++chan) {
        if (buffer.freqId[chan].channel > DATAGRAM_CHANNELMAX) {
            buffer.freqId[chan].card = (buffer.freqId[chan].channel - 1) /
                    DATAGRAM_CHANNELMAX + 1;
            buffer.freqId[chan].channel = (buffer.freqId[chan].channel - 1) %
                    DATAGRAM_CHANNELMAX + 1;
        }
        //buffer.freqId[chan].print();
    }
#ifdef VERBOSE
//    cout << "Renumbering channels and cards: done" << endl;
#endif
}



#ifdef CHANNEL_EXPAND

bool CorrelatorSimulatorADE::sendFirstPayload()
{
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << " sends the first payload ..." << endl;
    cout << "Note that this payload is only a form of handshake. ";
    cout << "The content will be sent again. " << endl;
#endif
    askap::cp::VisDatagramADE payload;

    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;
    payload.timestamp = buffer.timeStamp;
    payload.beamid = buffer.beam;

    const uint32_t nCorrProd = buffer.data.size();
    const uint32_t nCorrProdPerSlice = 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    const uint32_t nSlice = nCorrProd / nCorrProdPerSlice;

    // The total number of simulated fine channels in correlator
    const uint32_t nFineCorrChan = itsNCoarseChannel * itsNChannelSub;

    const double freqMin = buffer.freqId[0].freq;
    const double freqInc = (buffer.freqId[1].freq - freqMin) / itsNChannelSub;

    // for all simulated fine channels in correlator 
    // note:
    // - in the ordering of correlator's transmission
    // - this is NOT buffer channels
    for (uint32_t fineCorrChan = 0; fineCorrChan < nFineCorrChan; 
            ++fineCorrChan) {

        // compute the card of this channel
        uint32_t card = (fineCorrChan / DATAGRAM_NCHANNEL) % DATAGRAM_NCARD;
        payload.card = card + DATAGRAM_CARDMIN;

        // compute the block of this channel
        uint32_t block = fineCorrChan / DATAGRAM_NCHANNEL / DATAGRAM_NCARD;
        payload.block = block + DATAGRAM_BLOCKMIN;

        // get the channel in measurement set (mapping)
        uint32_t cardCorrChan = fineCorrChan % DATAGRAM_NCHANNEL;
        uint32_t cardMeasChan = itsChannelMap.toCorrelator(cardCorrChan);
        payload.channel = cardMeasChan + DATAGRAM_CHANNELMIN;

        // calculate frequency
        uint32_t fineMeasChan = ((block * DATAGRAM_NCARD) + card) * 
                DATAGRAM_NCHANNEL + cardMeasChan;
        payload.freq = freqMin + freqInc * fineMeasChan;

        // compute coarse channel number in measurement set
        // (correspond to channel in buffer)
        uint32_t coarseMeasChan = fineMeasChan / itsNChannelSub;

        // for each slice of correlation products
        for (uint32_t slice = 0; slice < nSlice; ++slice) {
            payload.slice = slice;
            payload.baseline1 = slice * nCorrProdPerSlice;
            payload.baseline2 = payload.baseline1 + nCorrProdPerSlice - 1;

            for (uint32_t corrProdInSlice = 0; 
                    corrProdInSlice < nCorrProdPerSlice; 
                    ++corrProdInSlice) {
                uint32_t corrProd = corrProdInSlice + 
                        slice * nCorrProdPerSlice;
                payload.vis[corrProdInSlice].real = 
                        buffer.data[corrProd][coarseMeasChan].vis.real;
                payload.vis[corrProdInSlice].imag = 
                        buffer.data[corrProd][coarseMeasChan].vis.imag;
            }   // correlation product in slice

            if (card == 0) {
                itsPort->send(payload);
                cout << "The first payload is sent" << endl;
                return true;    // abort as soon as the payload is sent
            }
        }   // slice
    }   // simulated channel in correlator
    return false;
}   // sendFirstPayload



bool CorrelatorSimulatorADE::sendBufferData()
{
#ifdef VERBOSE
    cout << "  Shelf " << itsShelf << " sends beam " << buffer.beam << endl;
#endif
    askap::cp::VisDatagramADE payload;

    // Data that is constant for the whole buffer
    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;
    payload.timestamp = buffer.timeStamp;
    payload.beamid = buffer.beam;

    const uint32_t nCorrProd = buffer.data.size();
    const uint32_t nCorrProdPerSlice = 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    const uint32_t nSlice = nCorrProd / nCorrProdPerSlice;

    // The total number of simulated fine channels in correlator
    const uint32_t nFineCorrChan = itsNCoarseChannel * itsNChannelSub;

    const double freqMin = buffer.freqId[0].freq;
    const double freqInc = (buffer.freqId[1].freq - freqMin) / itsNChannelSub;
#ifdef VERBOSE
//    cout << "Frequency increment: " << freqInc << endl;
#endif

    // for all simulated fine channels in correlator 
    // note:
    // - in the ordering of correlator's transmission
    // - this is NOT buffer channels
    for (uint32_t fineCorrChan = 0; fineCorrChan < nFineCorrChan; 
            ++fineCorrChan) {

        // compute the card of this channel
        uint32_t card = (fineCorrChan / DATAGRAM_NCHANNEL) % DATAGRAM_NCARD;
        payload.card = card + DATAGRAM_CARDMIN;

        // compute the block of this channel
        uint32_t block = fineCorrChan / DATAGRAM_NCHANNEL / DATAGRAM_NCARD;
        payload.block = block + DATAGRAM_BLOCKMIN;

        // get the channel in measurement set (mapping)
        uint32_t cardCorrChan = fineCorrChan % DATAGRAM_NCHANNEL;
        uint32_t cardMeasChan = itsChannelMap.toCorrelator(cardCorrChan);
        payload.channel = cardMeasChan + DATAGRAM_CHANNELMIN;

        // calculate frequency
        uint32_t fineMeasChan = ((block * DATAGRAM_NCARD) + card) * 
                DATAGRAM_NCHANNEL + cardMeasChan;
        payload.freq = freqMin + freqInc * fineMeasChan;

        // compute coarse channel number in measurement set
        // (correspond to channel in buffer)
        uint32_t coarseMeasChan = fineMeasChan / itsNChannelSub;

#ifdef VERBOSE
//        cout << "corrChan " << corrChan << ", measChan " << measChan <<
//            ", coarseChan " << coarseChan << ", card " << card <<
//            ", block " << block << ", freq " << payload.freq << endl;
#endif

        // for each slice of correlation products
        for (uint32_t slice = 0; slice < nSlice; ++slice) {
            payload.slice = slice;
            payload.baseline1 = slice * nCorrProdPerSlice;
            payload.baseline2 = payload.baseline1 + nCorrProdPerSlice - 1;

            for (uint32_t corrProdInSlice = 0; 
                    corrProdInSlice < nCorrProdPerSlice; 
                    ++corrProdInSlice) {
                uint32_t corrProd = corrProdInSlice + 
                        slice * nCorrProdPerSlice;
                payload.vis[corrProdInSlice].real = 
                        buffer.data[corrProd][coarseMeasChan].vis.real;
                payload.vis[corrProdInSlice].imag = 
                        buffer.data[corrProd][coarseMeasChan].vis.imag;
            }   // correlation product in slice

            // send the slice
#if SHELFCASE == 1
            itsPort->send(payload);
#elif SHELFCASE == 2
            // test for 2 cards
            if (card % 2 == itsShelf - 1) {
                itsPort->send(payload);
            }
#else
            cout << "Illegal preprocessor case" << endl;
#endif
        }   // slice
    }   // simulated channel in correlator

    buffer.reset();

#ifdef VERBOSE
    //cout << "Shelf " << itsShelf << " finished Sending buffer data" << endl;
#endif
    return true;
}


#else   // NOT CHANNEL_EXPAND

bool CorrelatorSimulatorADE::sendBufferData()
{
#ifdef VERBOSE
    cout << "Sending buffer data ..." << endl;
#endif
    askap::cp::VisDatagramADE payload;
    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;
    payload.timestamp = buffer.timeStamp;
    payload.beamid = buffer.beam;

    const uint32_t nChan = buffer.freqId.size();
    const uint32_t nCorrProd = buffer.data.size();
    const uint32_t nCorrProdPerSlice = 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    const uint32_t nSlice = nCorrProd / nCorrProdPerSlice;
    const uint32_t channelRange = DATAGRAM_CHANNELMAX - 
            DATAGRAM_CHANNELMIN + 1;
    //cout << "  Size: " << nCorrProd << " x " << nChan << endl;
    //cout << "  Number of slices: " << nSlice << endl;

    for (uint32_t card = 0; card < buffer.nCard; ++card) {

        chanFirst = card * channelRange;
        chanLast = chanFirst + channelRange - 1;

        for (uint32_t chanInCard = chanFirst; 
                chanInCard <= chanLast; ++chanInCard) {

            // convert channel ordering
            chan = c.toCorrelator(chanInCard);

            payload.block = buffer.freqId[chan].block;
            payload.card = buffer.freqId[chan].card;
            payload.channel = buffer.freqId[chan].channel;
            payload.freq = buffer.freqId[chan].freq;

            for (uint32_t slice = 0; slice < nSlice; ++slice) {
                payload.slice = slice;
                payload.baseline1 = slice * nCorrProdPerSlice;
                payload.baseline2 = payload.baseline1 +
                        nCorrProdPerSlice - 1;

                for (uint32_t corrProdInSlice = 0; 
                        corrProdInSlice < nCorrProdPerSlice; 
                        ++corrProdInSlice) {
                    uint32_t corrProd = corrProdInSlice + 
                            slice * nCorrProdPerSlice;
                    payload.vis[corrProdInSlice].real = 
                            buffer.data[corrProd][chan].vis.real;
                    payload.vis[corrProdInSlice].imag = 
                            buffer.data[corrProd][chan].vis.imag;
                    buffer.data[corrProd][chan].ready = false;
                }   // correlation product in slice

                // send the slice
                itsPort->send(payload);

            }   // slice
        }   // channel
    }   // card

    buffer.reset();
    //buffer.ready = false;

#ifdef VERBOSE
    cout << "Sending buffer data: done" << endl;
#endif
    return true;
}

#endif  // CHANNEL_EXPAND



// TO BE DEPRECATED
// Send data of zero visibility for the whole baselines and channels 
// for only 1 time period.
bool CorrelatorSimulatorADE::sendNextZero(void)
{
#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::sendNextZero..." << endl;
#endif
    // construct payload
    askap::cp::VisDatagramADE payload;
    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;
    payload.timestamp = 0;
    payload.block = 1;      // part of freq index
    payload.card = 1;       // part of freq index
    payload.beamid = 1;

    // coarse channel
    for (uint32_t cChannel = 0; cChannel < itsNCoarseChannel; ++cChannel) { 

        // subdividing coarse channel into fine channel
        for (uint32_t subDiv = 0; subDiv < itsNChannelSub; ++subDiv) {

            uint32_t fChannel = cChannel * itsNChannelSub + subDiv;
            payload.channel = fChannel;
            payload.freq = itsFineBandwidth * fChannel; // part of freq index

            // payload slice
            for (uint32_t slice = 0; slice < itsNSlice; ++slice) {

                payload.slice = slice;
                payload.baseline1 = slice * 
                VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
                payload.baseline2 = payload.baseline1 +
                VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE - 1;

                // gather all visibility of baselines in this slice 
                // (= correlation product: antenna & polarisation product)
                for (uint32_t baseInSlice = 0; 
                        baseInSlice < VisDatagramTraits<VisDatagramADE>::
						MAX_BASELINES_PER_SLICE; ++baseInSlice) { 
					//cout << "baseline: " << baseline << endl;
                    payload.vis[baseInSlice].real = 0.0;
                    payload.vis[baseInSlice].imag = 0.0;
                }

                // send data in this slice 
                cout << "shelf " << itsShelf << 
                        " send payload channel " << cChannel << 
                        ", sub " << subDiv << ", slice " << slice << endl; 

				if (slice % 2 == 0) {   // shelf 1 sends even number slice
					if (itsShelf == 1) {
						itsPort->send(payload);
					}
				}
				else {                  // slice 2 sends odd number slice
					if (itsShelf == 2) {
						itsPort->send(payload);
					}
				}
				//itsPort->send(payload);
				
                usleep (itsDelay);
				
            }   // slice
        }   // channel subdivision
    }   // coarse channel

#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::sendNextZero: shelf " <<
            itsShelf << ": done" << endl;
#endif

    return false;   // to stop calling this function
}



// TO BE DEPRECATED
// Extract one data point from measurement set and send the data for all 
// baselines and channels for the time period given in the measurement set.
bool CorrelatorSimulatorADE::sendNextExpand(void)
{
#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::sendNextExpand..." << endl;
#endif
    ROMSColumns msc(*itsMS);

    // Get a reference to the columns of interest
    const casa::ROMSFieldColumns& fieldc = msc.field();
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    //const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    //const casa::ROMSPolarizationColumns& polc = msc.polarization();
    const unsigned int nRow = msc.nrow(); 

    const uint32_t indexBase = itsCorrProdMap.getIndexBase();
    //cout << "Index base for correlation product: " << indexBase << endl;

    // Record the current timestamp 
    const casa::Double currentIntegration = msc.time()(itsCurrentRow);
    ASKAPLOG_DEBUG_STR(logger, "Processing integration with timestamp "
            << msc.timeMeas()(itsCurrentRow));

    // Some general constraints
    ASKAPCHECK(fieldc.nrow() == 1, "Currently only support a single field");

    // construct payload
    askap::cp::VisDatagramADE payload;
    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;

	casa::Matrix<casa::Complex> data;
	
	// Get visibility data for this time period, but only use one data point.
	// WARNING: Floating point comparison for time identification!
    while (itsCurrentRow < nRow && 
			(currentIntegration == msc.time()(itsCurrentRow))) {

        const int dataDescId = msc.dataDescId()(itsCurrentRow);
        const double refFreq = spwc.refFrequency()(dataDescId);
        payload.freq = refFreq;

        // Some per row constraints
        // This code needs the dataDescId to remain constant for all rows
        // in the integration being processed
        ASKAPCHECK(msc.dataDescId()(itsCurrentRow) == dataDescId,
                "Data description ID must remain constant for a given integration");
		
        // Note, the measurement set stores integration midpoint (in seconds), 
		// while the TOS ()and it is assumed the correlator) deal with integration 
		// start (in microseconds).
        // In addition, TOS time is BAT and the measurement set normally has 
		// UTC time (the latter is not checked here as we work with the column as 
		// a column of doubles rather than column of measures).
        // precision of a single double may not be enough in general, but should 
		// be fine for this emulator (ideally need to represent time as two doubles)
        const casa::MEpoch epoch(casa::MVEpoch(casa::Quantity
				(currentIntegration,"s")), casa::MEpoch::Ref(casa::MEpoch::UTC));
        const casa::MVEpoch epochTAI = casa::MEpoch::Convert(epoch,
                casa::MEpoch::Ref(casa::MEpoch::TAI))().getValue();
        const uint64_t microsecondsPerDay = 86400000000ull;
        const uint64_t startOfDayBAT = uint64_t(epochTAI.getDay()*
				microsecondsPerDay);
        const long Tint = static_cast<long>(msc.interval()(itsCurrentRow) * 
				1000000);
        const uint64_t startBAT = startOfDayBAT + 
				uint64_t(epochTAI.getDayFraction() * microsecondsPerDay) -
				uint64_t(Tint / 2);

        // ideally we need to carry 64-bit BAT in the payload explicitly
        payload.timestamp = static_cast<long>(startBAT);
        ASKAPCHECK(msc.feed1()(itsCurrentRow) == msc.feed2()(itsCurrentRow),
                "feed1 and feed2 must be equal");

        // NOTE: The Correlator IOC uses one-based beam indexing, so need to add
        // one to the zero-based indexes from the measurement set.
        payload.beamid = msc.feed1()(itsCurrentRow) + 1;
#ifdef VERBOSE
        std::cout << itsCurrentRow << ", " << payload.beamid << std::endl;
#endif

		// get visibility data in this row
        data = msc.data()(itsCurrentRow);
		
		++itsCurrentRow;
	}

	cout << "shelf " << itsShelf << 
		" send payload for time period ending in row " << 
        itsCurrentRow-1 << endl;
	
	// Send a data point acquired above for the whole slice ...
	// for all coarse channel
	for (uint32_t cChannel = 0; cChannel < itsNCoarseChannel; ++cChannel) { 

		// subdividing coarse channel into fine channel
		for (uint32_t subDiv = 0; subDiv < itsNChannelSub; ++subDiv) {

			uint32_t fChannel = cChannel * itsNChannelSub + subDiv;
			payload.channel = fChannel;
			//payload.freq = itsFineBandwidth * fChannel; // part of freq index
			payload.block = 1;      // part of freq index
			payload.card = 1;       // part of freq index

			// for all payload slice in this fine channel
			for (uint32_t slice = 0; slice < itsNSlice; ++slice) {

				payload.slice = slice;
				payload.baseline1 = indexBase + slice * 
				VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
				payload.baseline2 = payload.baseline1 +
				VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE - 1;

				// for all baselines in this slice 
				// (= correlation product: antenna & polarisation product)
				for (uint32_t baseInSlice = 0; 
						baseInSlice < VisDatagramTraits<VisDatagramADE>::
						MAX_BASELINES_PER_SLICE; ++baseInSlice) { 

					// Use one visibility data for all baselines
					payload.vis[baseInSlice].real = data(0,0).real();
					payload.vis[baseInSlice].imag = data(0,0).imag();
				}

				// send data in this slice using 2 MPI processes in an
				// interleaved fashion
				//if (slice % 2 == 0) {   // shelf 1 sends even number slice
				//	if (itsShelf == 1) {
				//		itsPort->send(payload);
				//	}
				//}
				//else {                  // slice 2 sends odd number slice
				//	if (itsShelf == 2) {
				//		itsPort->send(payload);
				//	}
				//}
                itsPort->send(payload);

				// User-defined delay
				usleep (itsDelay);
				
			}   // slice
		}   // channel subdivision
	}   // coarse channel
		
#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::sendNextExpand: shelf " <<
            itsShelf << ": done" << endl;
#endif

    if (itsCurrentRow >= nRow) {
		cout << "Shelf " << itsShelf << " has no more data" << endl;
        return false;
    } else {
        return true;	// Have more data (next time period)
    }
}



uint32_t CorrelatorSimulatorADE::getCorrProdIndex 
        (const uint32_t ant1, const uint32_t ant2, 
		const Stokes::StokesTypes stokesType)
{
	uint32_t stokesValue;
	if (stokesType == Stokes::XX) {
		stokesValue = 0;
	}
	else if (stokesType == Stokes::XY) {
		stokesValue = 1;
	}
	else if (stokesType == Stokes::YX) {
		stokesValue = 2;
	}
	else if (stokesType == Stokes::YY) {
		stokesValue = 3;
	}
	else {
		checkStokesType (stokesType);
        return 0;
	}
    cout << ant1 << ", " << ant2 << ", " << stokesValue << endl;
	return itsCorrProdMap.getIndex (ant1, ant2, stokesValue);
}


void checkStokesType (const Stokes::StokesTypes stokesType)
{
	ASKAPCHECK (stokesType == Stokes::XX || stokesType == Stokes::XY ||
			stokesType == Stokes::YX || stokesType == Stokes::YY,
			"Unsupported stokes type");
}

#ifdef VERBOSE
#undef VERBOSE
#endif


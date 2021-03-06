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
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "cpcommon/VisDatagramADE.h"

// casa
#include "casacore/measures/Measures/MEpoch.h"
#include "casacore/measures/Measures/MeasConvert.h"
#include "casacore/measures/Measures/MCEpoch.h"

// Local package includes
#include "simplayback/CorrProdMap.h"
#include "simplayback/CorrBuffer.h"
#include "simplayback/DatagramLimit.h"

// Make the simulator runs in indefinite loop
//#define LOOP

// Output lots of feedback to user
//#define VERBOSE

// Reorder antennas so that the numbering is contiguous (no jump)
#define REORDER_ANTENNA

// Visibility buffer with constant size
//#define NEW_BUFFER

#define SIZEOF_ARRAY(a) (sizeof( a ) / sizeof( a[ 0 ] ))

using namespace askap;
using namespace askap::cp;
using namespace casa;

// Forward declaration
void checkStokesType (const Stokes::StokesTypes& stokestype);
uint32_t getCorrProdIndex (const uint32_t ant1, const uint32_t ant2, 
		const Stokes::StokesTypes& stokestype);

ASKAP_LOGGER(logger, ".CorrelatorSimulatorADE");


CorrelatorSimulatorADE::CorrelatorSimulatorADE(
        const std::string& mode,
        const std::string& dataset,
        const std::string& hostname,
        const std::string& port,
        const uint32_t shelf,
        const uint32_t nShelves,
        const uint32_t nAntennaIn,
        const uint32_t nCoarseChannel,
        const uint32_t nFineChannel,
        const uint32_t nChannelSub,
        const double coarseBandwidth,
        const uint32_t delay,
		const CardFailMode& failMode)
        : itsMode(mode), itsShelf(shelf), itsNShelves(nShelves),
        itsNAntenna(nAntennaIn), itsNCorrProd(0), itsNSlice(0),
        itsNCoarseChannel(nCoarseChannel), itsNFineChannel(nFineChannel),
        itsNChannelSub(nChannelSub), itsCoarseBandwidth(coarseBandwidth),
        itsFineBandwidth(0.0), itsCurrentTime(0),
        itsDelay(delay), itsFailMode(failMode), 
		itsCurrentRow(0), itsDataReadCounter(0), itsDataSentCounter(0)
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
	bool continueStatus;

    // Get buffer data from measurement set
    bool bufferDataExist = false;

#ifdef NEW_BUFFER

    if (getNewBufferData()) {
        bufferDataExist = true;
    }

#else

    if (getBufferData()) {
        bufferDataExist = true;
    }

#endif

    if (bufferDataExist) {

#ifdef NEW_BUFFER

        // Fill each channel with data from existing correlation products
        for (uint32_t chan = 0; chan < data[0].size(); ++chan) {
            fillOneChannel(uint32_t chan);
        }

        // Fill each correlation product with data from existing channels
        for (uint32_t cp = 0; cp < data.size(); ++cp) {
            fillOneCorrProduct(uint32_t cp);
        }

#else	// old buffer

        // The data from measurement set does not fill the whole buffer,
        // so fill in the missing data by copying from existing one.
        // First, fill in the data for the missing correlation products
        // due to antennas not present in measurement set.
        fillCorrProdInBuffer();

        // Then, fill in the data for coarse channels not present in
        // measurement set.
        fillChannelInBuffer();

#endif
		//itsBuffer.print("all");

        // Delay transmission for every new time stamp in measurement
        if (itsCurrentTime > previousTime) {
            double delay = static_cast<double>(itsDelay) / 1000000.0;
            cout << "Shelf " << itsShelf << 
                    ": new time stamp " << itsCurrentTime << 
                    ", pausing " << delay << " seconds" << endl;
            usleep(itsDelay);
            cout << "Shelf " << itsShelf << ": transmitting ..." << endl;
        }

		// Send the data
#ifdef NEW_BUFFER
        continueStatus = sendBigBufferData();     // @@@ make this @@@
#else
       	continueStatus = sendBufferData();
#endif

/* 		switched off for the time being
		if ((itsDataReadCounter > 0) && 
				(itsFailMode.miss == itsDataReadCounter)) {
			cout << "Shelf " << itsShelf << 
					": is simulating missing transmission at cycle " << 
					itsDataReadCounter << endl;
			itsBuffer.reset();
			continueStatus = true;
		}
		else {	// ... or send the data as usual
        	continueStatus = sendBufferData();
		}
*/
    }
    else {	// buffer data does not exist
        cout << "Shelf " << itsShelf << 
                ": no more data in measurement set" << endl;
		cout << "Shelf " << itsShelf << ": read " << itsDataReadCounter <<
				"x & sent " << itsDataSentCounter << "x" << endl;
		continueStatus = false;
    }
#ifdef VERBOSE
    cout << "Shelf " << itsShelf <<
    		": data read: " << itsDataReadCounter <<
    		", data sent: " << itsDataSentCounter << endl;
#endif
	return continueStatus;
}   // sendNext



void CorrelatorSimulatorADE::resetCurrentRow(void)
{
	itsCurrentRow = 0;
}



//------------------------------------------------------------------------
// Internal functions


// Initialize buffer.
// It's a 2D array with the size of the total number of correlation products
// and coarse channels.
//
void CorrelatorSimulatorADE::initBuffer()
{
    const uint32_t DISPLAY_SHELF = 1;

    cout << "Shelf " << itsShelf << ": initializing buffer ..." << endl;

    ROMSColumns msc(*itsMS);

    // Get reference to columns of interest
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casa::ROMSAntennaColumns& antc = msc.antenna();
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();
    const uint32_t nRow = msc.nrow();
    if (itsShelf == DISPLAY_SHELF) {
        cout << "  Reading measurement set ..." << endl;
        cout << "    Total rows in measurement set: " << nRow << endl;
    }
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
        ASKAPCHECK(nCorr == 4,
            "Row " << row << " has illegal number of correlations " << nCorr);
        
        antMin = min(antMin,msc.antenna1()(row));
        antMax = max(antMax,msc.antenna1()(row));

        beamMin = min(beamMin,msc.feed1()(row));
        beamMax = max(beamMax,msc.feed1()(row));
    }
    
    if (itsShelf == DISPLAY_SHELF) {
        cout << "    Time interval count  : " << nTime << endl;
        cout << "    Beam range           : " << beamMin << " ~ " << 
                beamMax << endl;
    }

    // Antennas
    const uint32_t nAntMeas = antc.nrow();
    const uint32_t nAntMeasCheck = antMax - antMin + 1;
    ASKAPCHECK(nAntMeas == nAntMeasCheck, 
            "Disagreement in antenna count in measurement set");
    
    if (itsShelf == DISPLAY_SHELF) {
        cout << "    Antenna count: " << nAntMeas << endl;
    }
#ifdef REORDER_ANTENNA
    cout << "    Antenna is reordered into a compact list" << endl;
    for (uint32_t ant = 0; ant < nAntMeas; ++ant) {
        itsAntIndices.push_back(ant);
        cout << "      antenna name: " << antc.name()(ant) << " -> index: " <<
                itsAntIndices[ant] << endl;
    }
#else
    for (uint32_t ant = 0; ant < nAntMeas; ++ant) {
        string antName = antc.name()(ant);
        string antNameNumber = antName.substr(2,2);
        stringstream convert(antNameNumber);
        uint32_t antIndex;
        convert >> antIndex;
        itsAntIndices.push_back(antIndex-1);   // antenna name is 1-based
        if (itsShelf == DISPLAY_SHELF) {
            cout << "      antenna name: " << antName << " -> index: " << 
                    itsAntIndices[ant] << endl;
        }
    }
#endif

    // channel count
    itsBuffer.nChanMeas = nChan;
    uint32_t nCorrProdMeas = itsCorrProdMap.getTotal(nAntMeas);
    if (itsShelf == DISPLAY_SHELF) {
        cout << "    Correlation product count: " << nCorrProdMeas << endl;
        cout << "    Channel count: " << nChan << endl;
        // Data matrix
        cout << "    Measurement set data: " << 
                "correlation products x channels: " << 
                nCorrProdMeas << " x " << nChan << endl;
        cout << "  Reading measurement set: done" << endl;
    }
   
    uint32_t nAntCorr = itsNAntenna;         // from parameter file
    if (itsShelf == DISPLAY_SHELF) {
        cout << "  Creating buffer for simulation data ..." << endl;
        cout << "    Antennas to be simulated: " << nAntCorr << endl;
    }
    
    // Correlation products, calculated from parameter file
    // Note that correlator may send more data than available antenna
    // (which means empty data will get sent too).
    itsNCorrProd = itsCorrProdMap.getTotal(nAntCorr);
    if (itsShelf == DISPLAY_SHELF) {
        cout << "    Correlation products to be simulated: " << 
                itsNCorrProd << endl;
    }

    // Creating buffer
    // Note that buffer contains all correlation products (as requested in
    // parset, which is usually more than available in measurement set) and
    // all channels (as requested in parset)

    const uint32_t maxAntenna = 36;
    const uint32_t maxCorrProd = itsCorrProdMap.getTotal(maxAntenna);

#ifdef NEW_BUFFER

	// Buffer contains max correlation products (as derived from max number
	// of antennas) and max number of fine channels in a card 
    const uint32_t maxFineChannelInCard = 216;  // = 4 coarse channels x 54
    itsBuffer.init(maxCorrProd, maxFineChannelInCard);
    if (itsShelf == DISPLAY_SHELF) {
        cout << "    Buffer data: max correlation products x " << 
				"max fine channels: " << maxCorrProd <<
                " x " << maxFineChannelInCard << endl;
    }

#else

	// Buffer contains max numbers of antennas (correlation products)
	// and the number of coarse channels as required by parset
    itsBuffer.init(maxCorrProd, itsNCoarseChannel);
    //itsBuffer.init(itsNCorrProd, itsNCoarseChannel);
    if (itsShelf == DISPLAY_SHELF) {
        cout << "    Buffer data: max correlation products x " <<
				"coarse channels: " << maxCorrProd << 
                " x " << itsNCoarseChannel << endl; 
	}

#endif
	
    if (itsShelf == DISPLAY_SHELF) {
        cout << "  Creating buffer for simulation data: done" << endl;
    }
    // card count
    itsBuffer.nCard = itsNCoarseChannel / DATAGRAM_CHANNELMAX + 1;
    if (itsShelf == DISPLAY_SHELF) {
        cout << "  Total cards: " << itsBuffer.nCard << endl;
    }
    cout << "Shelf " << itsShelf << ": initializing buffer: done" << endl;
    cout << endl;
}   // initBuffer



#ifdef NEW_BUFFER

bool CorrelatorSimulatorADE::getNewBufferData()
{
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << ": getting buffer data ..." << endl;
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
    itsBuffer.timeStamp = static_cast<long>(startBAT);
    itsCurrentTime = itsBuffer.timeStamp;
    itsBuffer.beam = static_cast<uint32_t>(msc.feed1()(itsCurrentRow));
#ifdef VERBOSE
    cout << "  Time " << itsBuffer.timeStamp << ", beam " <<
        itsBuffer.beam << endl;
#endif

    while ((itsCurrentRow < nRow) &&
            (static_cast<uint32_t>(msc.feed1()(itsCurrentRow)) ==
            itsBuffer.beam)) {

        uint32_t dataDescId = msc.dataDescId()(itsCurrentRow);
        uint32_t descSpwId = ddc.spectralWindowId()(dataDescId);

        uint32_t nChan = spwc.numChan()(descSpwId);
        if (itsMode == coarseChannels) {
            nChan = 54 * nChan;
        }
        //uint32_t descPolId = ddc.polarizationId()(dataDescId);
        //uint32_t nCorr = polc.numCorr()(descPolId);
        casa::Matrix<casa::Complex> data = msc.data()(itsCurrentRow);
        const casa::Vector<casa::Double> frequencies =
                spwc.chanFreq()(descSpwId);
        ASKAPCHECK(nChan == frequencies.size(),
                "Disagreement in the number of channels in measurement set");
        for (uint32_t chan = 0; chan < nChan; ++chan) {
            itsBuffer.freqId[chan].block = 1;          // dummy value
            itsBuffer.freqId[chan].card = 1;           // dummy value
            itsBuffer.freqId[chan].channel = chan + 1; // 1-based
            itsBuffer.freqId[chan].freq = frequencies[chan];
        }

        uint32_t ant1 = itsAntIndices[msc.antenna1()(itsCurrentRow)];
        uint32_t ant2 = itsAntIndices[msc.antenna2()(itsCurrentRow)];
        if (ant1 > ant2) {
            std::swap(ant1,ant2);
        }

        uint32_t descPolId = ddc.polarizationId()(dataDescId);
        uint32_t nCorr = polc.numCorr()(descPolId);
        casa::Vector<casa::Int> stokesTypesInt = polc.corrType()(descPolId);
        //cout << "    antenna " << ant1 << ", " << ant2 << endl;
        uint32_t corr;
        for (uint32_t c = 0; c < nCorr; ++c) {
            //Stokes::StokesTypes stokesType = Stokes::type(stokesTypesInt(c));
            //corr = Stokes::type(stokesTypesInt(c)) - Stokes::XX;
            //ASKAPCHECK((corr >= 0) && (corr <= 3), 
            //        "Illegal value of correlation");
            if (Stokes::type(stokesTypesInt(c)) == Stokes::XX) {
                corr = 0;
            }
            else if (Stokes::type(stokesTypesInt(c)) == Stokes::XY) {
                corr = 1;
            }
            else if (Stokes::type(stokesTypesInt(c)) == Stokes::YX) {
                corr = 2;
            }
            else if (Stokes::type(stokesTypesInt(c)) == Stokes::YY) {
                corr = 3;
            }
            else {
                checkStokesType (stokesType);
            }

            // put visibility data into buffer,
            // except when the Stokes type is YX for the same antenna
            if ((ant1 != ant2) || 
                    (Stokes::type(stokesTypesInt(c)) != Stokes::YX)) {
                uint32_t corrProd =
                        itsCorrProdMap.getIndex(ant1, ant2, corr) - 1;
                // Note that this is the channel ordering as in meas. set
                // If the 
                // and the channels are fine channels.
                for (uint32_t chan = 0; chan < nChan; ++chan) {
                    itsBuffer.insert(corrProd, chan, data(corr,chan));
                }   // channel
            }
        }   // correlation
        ++itsCurrentRow;
    }   // row

    itsBuffer.ready = true;

#ifdef VERBOSE
    cout << "Shelf " << itsShelf << ": getting buffer data: done" << endl;
#endif
    if (itsBuffer.ready) {
        ++itsDataReadCounter;
    }
    return (itsBuffer.ready);
}   // getBuffer


#else   // old buffer


bool CorrelatorSimulatorADE::getBufferData()
{
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << ": getting buffer data ..." << endl;
#endif
    ROMSColumns msc(*itsMS);

    // Get reference to columns of interest
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();

    const uint32_t nRow = msc.nrow();

//#ifdef LOOP

	// If the current row is the last row, quit
    if (itsCurrentRow >= nRow) {
		//itsCurrentRow = 0;
        cout << "  The last row" << endl;
        cout << "Getting buffer data: done" << endl;
		return false;
	}

//#else	// play once

	// If the current row is the last row, quit
//    if (itsCurrentRow >= nRow) {
//#ifdef VERBOSE
//        cout << "  No more data available" << endl;
//        cout << "Getting buffer data: done" << endl;
//#endif
//        return false;
//    }
//#endif

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
    itsBuffer.timeStamp = static_cast<long>(startBAT);
    itsCurrentTime = itsBuffer.timeStamp;
    itsBuffer.beam = static_cast<uint32_t>(msc.feed1()(itsCurrentRow));
#ifdef VERBOSE
    cout << "  Time " << itsBuffer.timeStamp << ", beam " << 
		itsBuffer.beam << endl;
#endif

    while ((itsCurrentRow < nRow) &&
            (static_cast<uint32_t>(msc.feed1()(itsCurrentRow)) == 
			itsBuffer.beam)) {

        uint32_t dataDescId = msc.dataDescId()(itsCurrentRow);
        uint32_t descSpwId = ddc.spectralWindowId()(dataDescId);
        uint32_t nChan = spwc.numChan()(descSpwId);
        uint32_t descPolId = ddc.polarizationId()(dataDescId);
        uint32_t nCorr = polc.numCorr()(descPolId);
        casa::Matrix<casa::Complex> data = msc.data()(itsCurrentRow);
        const casa::Vector<casa::Double> frequencies = 
                spwc.chanFreq()(descSpwId);
		ASKAPCHECK(nChan > 0, "nChan: " << nChan);
        ASKAPCHECK(nChan == frequencies.size(), 
                "Disagreement in the number of channels in measurement set");
        for (uint32_t chan = 0; chan < nChan; ++chan) {
            itsBuffer.freqId[chan].block = 1;          // dummy value
            itsBuffer.freqId[chan].card = 1;           // dummy value
            itsBuffer.freqId[chan].channel = chan + 1; // 1-based
            itsBuffer.freqId[chan].freq = frequencies[chan];
			//cout << "row " << itsCurrentRow << ", chan " << chan << 
			//		", freq: " << frequencies[chan] << endl;
			ASKAPCHECK(frequencies[chan] > 0.0, "frequencies[" << chan << "]: " <<
					frequencies[chan]);
        }

        uint32_t ant1 = itsAntIndices[msc.antenna1()(itsCurrentRow)];
        uint32_t ant2 = itsAntIndices[msc.antenna2()(itsCurrentRow)];
        if (ant1 > ant2) {
            std::swap(ant1,ant2);
        }

        casa::Vector<casa::Int> stokesTypesInt = polc.corrType()(descPolId);
        //cout << "    antenna " << ant1 << ", " << ant2 << endl;
        uint32_t corr = 0;
        for (uint32_t c = 0; c < nCorr; ++c) {
            Stokes::StokesTypes stokesType = Stokes::type(stokesTypesInt(c));
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
                uint32_t corrProd = 
						itsCorrProdMap.getIndex(ant1, ant2, corr) - 1;
                ASKAPCHECK((corrProd >= 0) && 
						(corrProd < itsBuffer.data.size()),
                        "Illegal corrProd: " << corrProd <<
                        ". Range: 0~" << itsBuffer.data.size()-1);
                ASKAPCHECK(!itsBuffer.corrProdIsFilled[corrProd],
                        "Correlator product " << corrProd << 
                        " is already filled. ant1, ant2, corr: " << ant1 << 
						", " << ant2 << ", " << corr);
                // Note that this is the channel ordering as in meas. set
                // If it contains only the coarse channels, it needs to be
                // expanded into fine channels @@@@@@@@@@
                for (uint32_t chan = 0; chan < nChan; ++chan) {
//#ifdef NEW_BUFFER
//					itsBuffer.insert(corrProd, chan, data(corr,chan));
//#else
                    itsBuffer.data[corrProd][chan].vis.real = 
                            data(corr,chan).real();
                    itsBuffer.data[corrProd][chan].vis.imag = 
                            data(corr,chan).imag();
//#endif
                }   // channel
#ifndef NEW_BUFFER
                itsBuffer.corrProdIsFilled[corrProd] = true;
                itsBuffer.corrProdIsOriginal[corrProd] = true;
#endif
            }
        }   // correlation
        ++itsCurrentRow;
    }   // row

    itsBuffer.ready = true;

#ifdef VERBOSE
    cout << "Shelf " << itsShelf << ": getting buffer data: done" << endl;
#endif
	if (itsBuffer.ready) {
		++itsDataReadCounter;
	}
    return (itsBuffer.ready);
}   // getBuffer (old buffer)

#endif  // NEW_BUFFER



// Fill empty correlation product data from original ones (from 
// measurement set)
void CorrelatorSimulatorADE::fillCorrProdInBuffer()
{
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": filling empty correlation products in buffer ..." << endl;
#endif
    // Find the first original data
    // set initial correlation product index to force search from beginning
    //int32_t originalCP = -1;
    int32_t firstOriginalCP = itsBuffer.findNextOriginalCorrProd(-1);
    ASKAPCHECK(firstOriginalCP >= 0, "Cannot find the first original data");

    // If the original data is not the first data in the buffer,
    // fill in the empty section BEFORE the first original data.
    if (firstOriginalCP > 0) {
        for (int32_t cp = 0; cp < firstOriginalCP; ++cp) {
            itsBuffer.copyCorrProd(firstOriginalCP, cp);
        }
    }

    // Fill in the empty sections AFTER the first original data.
    int32_t originalCP = firstOriginalCP;
    for (int32_t cp = firstOriginalCP+1; cp < (int)itsBuffer.data.size(); 
			++cp) {
        // If this slot is filled, take this as the original data
        if (itsBuffer.corrProdIsFilled[cp]) {
            originalCP = cp;
        }
        else {  // This slot contains no data,
            // so fill it with the the latest original data
            itsBuffer.copyCorrProd(originalCP, cp);
        }
    }
/*
    // set initial correlation product index to force search from beginning
    int32_t originalCP = -1; 
    for (uint32_t cp = 0; cp < itsBuffer.data.size(); ++cp) {
        // if the correlation product has no data
        if (!itsBuffer.corrProdIsFilled[cp]) {
            // find the next available original data
            originalCP = itsBuffer.findNextOriginalCorrProd(originalCP);

            // if found
            if (originalCP >= 0) {
                // fill in the empty correlation product with original data
                itsBuffer.copyCorrProd(originalCP, cp);
            }
            // if none can be found
            else {
                // search from the beginning
                originalCP = itsBuffer.findNextOriginalCorrProd(-1);
                ASKAPCHECK(originalCP >= 0, 
                        "Still cannot find original data");
                itsBuffer.copyCorrProd(originalCP, cp);
            }
        }
    }   // correlation product

    // check
    for (uint32_t cp = 0; cp < itsBuffer.data.size(); ++cp) {
        ASKAPCHECK( itsBuffer.corrProdIsFilled[cp], "Correlation product " <<
                cp << " is still empty");
    }
*/
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": filling empty correlation products in buffer: done" << endl;
#endif

}   // fillCorrProdInBuffer



// Fill empty channel data from original ones (from measurement set)
//
void CorrelatorSimulatorADE::fillChannelInBuffer()
{
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": filling empty channels in buffer ..." << endl;
#endif
    uint32_t sourceChan = itsBuffer.nChanMeas - 1;
    //uint32_t sourceChan = 0;

    // Frequency increment
    const double freqInc = itsBuffer.freqId[1].freq - itsBuffer.freqId[0].freq;
	ASKAPCHECK(freqInc > 0.0, "Illegal frequency increment: " << freqInc);

    //for (uint32_t chan = itsBuffer.nChanMeas; 
    //        chan < itsNCoarseChannel; ++chan) {
    //for (uint32_t chan = sourceChan+1; 
    //        chan < itsNCoarseChannel*itsNChannelSub; ++chan) {
	for (uint32_t chan = sourceChan+1;
			chan < itsNCoarseChannel; ++chan) {
        itsBuffer.freqId[chan].block = itsBuffer.freqId[sourceChan].block;
        itsBuffer.freqId[chan].card = itsBuffer.freqId[sourceChan].card;
        itsBuffer.freqId[chan].channel = chan + 1; // 1-based
        itsBuffer.freqId[chan].freq = itsBuffer.freqId[0].freq + freqInc * chan;
		itsBuffer.copyChannel(sourceChan, chan);
    }
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": filling empty channels in buffer: done" << endl;
#endif

}   // fillChannelInBuffer



void CorrelatorSimulatorADE::fillTestBuffer
        (askap::cp::VisDatagramADE &payload) {

#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": filling test buffer ..." << endl;
#endif
    itsTestBuffer.timeStamp = payload.timestamp;
    itsTestBuffer.beam = payload.beamid;
    uint32_t corrChan = payload.channel - DATAGRAM_CHANNELMIN;
    uint32_t measChan = itsChannelMap.fromCorrelator(corrChan);
    uint32_t card = payload.card - DATAGRAM_CARDMIN;
    uint32_t block = payload.block - DATAGRAM_BLOCKMIN;
    // total contiguous channel
    uint32_t chan = ((block * DATAGRAM_NCARD) + card) * DATAGRAM_NCHANNEL
            + measChan;
    itsTestBuffer.freqId[chan].block = payload.block;
    itsTestBuffer.freqId[chan].card = payload.card;
    itsTestBuffer.freqId[chan].channel = measChan + DATAGRAM_CHANNELMIN;
    itsTestBuffer.freqId[chan].freq = payload.freq;
    for (uint32_t corrProd = 0;
            corrProd < payload.baseline2 - payload.baseline1 + 1; 
            ++corrProd) {
        itsTestBuffer.data[payload.baseline1 + corrProd - 1][chan].vis.real
                = payload.vis[corrProd].real;
        itsTestBuffer.data[payload.baseline1 + corrProd - 1][chan].vis.imag
                = payload.vis[corrProd].imag;
    }
#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": filling test buffer: done" << endl;
#endif
}



void CorrelatorSimulatorADE::checkTestBuffer() {

#ifdef VERBOSE
    cout << "Shelf " << itsShelf << 
            ": checking test buffer ..." << endl;
#endif
    const double coarseFreqInc = itsBuffer.freqId[1].freq - 
		itsBuffer.freqId[0].freq;
    const double freqMin = itsBuffer.freqId[0].freq;
    const double freqInc = coarseFreqInc / itsNChannelSub;
    const double small = 0.00001;
#ifdef VERBOSE
    cout << "  Channel count: " << itsTestBuffer.freqId.size() << endl;
    const double freqMax = itsBuffer.freqId[itsBuffer.freqId.size()-1].freq;
    cout << "  Freq min ~ max: " << freqMin << " ~ " << freqMax << endl;
#endif
    for (uint32_t chan = 0; chan < itsTestBuffer.freqId.size(); ++chan) {

        // Check for channel, card, block that should be in the buffer
        // and those that are not supposed to be in the buffer
        uint32_t totalCardExpected = chan / DATAGRAM_NCHANNEL;
        uint32_t channelExpected = chan % DATAGRAM_NCHANNEL + 
                DATAGRAM_CHANNELMIN;
        uint32_t blockExpected = totalCardExpected / DATAGRAM_NCARD +
                DATAGRAM_BLOCKMIN;
        uint32_t cardExpected = totalCardExpected % DATAGRAM_NCARD +
                DATAGRAM_CARDMIN;

        // If the data belongs to this shelf
        if (cardExpected == itsShelf) {
            // Check for data validity
            ASKAPCHECK(channelExpected == itsTestBuffer.freqId[chan].channel,
                    "Expected channel " << channelExpected << 
                    " <> received " << itsTestBuffer.freqId[chan].channel);
            ASKAPCHECK(cardExpected == itsTestBuffer.freqId[chan].card,
                    "Expected card " << cardExpected << 
                    " <> received " << itsTestBuffer.freqId[chan].card);
            ASKAPCHECK(blockExpected == itsTestBuffer.freqId[chan].block,
                    "Expected block " << blockExpected << 
                    " <> received " << itsTestBuffer.freqId[chan].block);
            double freqExpected = (freqMin + freqInc * chan) / 1000000.0;
            ASKAPCHECK((freqExpected + small >= itsTestBuffer.freqId[chan].freq)
                    && (freqExpected - small <= itsTestBuffer.freqId[chan].freq),
                    "Expected frequency " << freqExpected <<
                    " <> received " << itsTestBuffer.freqId[chan].freq);
        }
        else {  // If the data does not belong to this shelf
            // All data must be zero
            ASKAPCHECK(itsTestBuffer.freqId[chan].channel == 0,
                    "Non zero channel " << itsTestBuffer.freqId[chan].channel);
            ASKAPCHECK(itsTestBuffer.freqId[chan].card == 0,
                    "Non zero card " << itsTestBuffer.freqId[chan].card);
            ASKAPCHECK(itsTestBuffer.freqId[chan].block == 0,
                    "Non zero block " << itsTestBuffer.freqId[chan].block);
            ASKAPCHECK(itsTestBuffer.freqId[chan].freq == 0.0,
                    "Non zero frequency " << itsTestBuffer.freqId[chan].freq);
        }
#ifdef VERBOSE
        cout << chan + 1 << ": " << itsTestBuffer.freqId[chan].card << ", " << 
                itsTestBuffer.freqId[chan].channel << ", " <<
                ", " << itsTestBuffer.freqId[chan].freq << endl;
#endif
    }
    cout << "Shelf " << itsShelf << 
            ": checking test buffer: PASS" << endl;
}



bool CorrelatorSimulatorADE::sendBufferData()
{
#ifdef VERBOSE
    cout << "  Shelf " << itsShelf << " sends beam " << itsBuffer.beam << endl;
#endif
    askap::cp::VisDatagramADE payload;

    // Data that is constant for the whole buffer
    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;
    payload.timestamp = itsBuffer.timeStamp;
    payload.beamid = itsBuffer.beam + 1;

    const uint32_t nCorrProd = itsBuffer.data.size();
    const uint32_t nCorrProdPerSlice = 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    const uint32_t nSlice = nCorrProd / nCorrProdPerSlice;

    // The total number of simulated fine channels in correlator
    const uint32_t nFineCorrChan = itsNCoarseChannel * itsNChannelSub;

    const double coarseFreqInc = (itsBuffer.freqId[1].freq - 
		itsBuffer.freqId[0].freq) / 1000000.0;
    const double freqMin = itsBuffer.freqId[0].freq / 1000000.0;
    const double freqInc = coarseFreqInc / itsNChannelSub;
    
#ifdef VERBOSE
	cout << "nCorrProd: " << nCorrProd << endl;
	cout << "nCorrProdPerSlice: " << nCorrProdPerSlice << endl;
	cout << "nSlice: " << nSlice << endl;
	cout << "nFineCorrChan: " << nFineCorrChan << endl;
	cout << "coarseFreqInc: " << coarseFreqInc << endl;
	cout << "freqMin: " << freqMin << endl;
    cout << "freqInc: " << freqInc << endl;
#endif
    ASKAPCHECK(freqInc > 0.0, "Illegal frequency increment: " << freqInc);

    if (itsMode == "test") {
        // Test buffer used to check the transmitted data
#ifdef VERBOSE
        cout << "  Creating test buffer ..." << endl;
#endif
        itsTestBuffer.init(itsNCorrProd, itsNCoarseChannel*itsNChannelSub);
#ifdef VERBOSE
        cout << "    Correlation products x fine channels: " <<
                itsNCorrProd << " x " << itsNCoarseChannel*itsNChannelSub << 
                endl;
        cout << "  Creating test buffer: done" << endl;
#endif
        vector<uint32_t> testChannel;
        testChannel.resize(DATAGRAM_NCHANNEL);
        vector<double> testFreq;
        testFreq.resize(DATAGRAM_NCHANNEL);
    }

    // for all simulated fine channels in correlator 
    // note:
    // - in the ordering of correlator's transmission
    // - this is NOT buffer channels
    for (uint32_t fineCorrChan = 0; fineCorrChan < nFineCorrChan; 
            ++fineCorrChan) {

        const uint32_t totalCard = fineCorrChan / DATAGRAM_NCHANNEL;
        const uint32_t cardCorrChan = fineCorrChan % DATAGRAM_NCHANNEL;

        const uint32_t block = totalCard / DATAGRAM_NCARD;
        const uint32_t card = totalCard % DATAGRAM_NCARD;

        payload.channel = cardCorrChan + DATAGRAM_CHANNELMIN;
        ASKAPCHECK((payload.channel >= DATAGRAM_CHANNELMIN) &&
                (payload.channel <= DATAGRAM_CHANNELMAX),
                "Payload channel is out of range");

        payload.card = card + DATAGRAM_CARDMIN;
        ASKAPCHECK((payload.card >= DATAGRAM_CARDMIN) &&
                (payload.card <= DATAGRAM_CARDMAX),
                "Payload card is out of range");

        payload.block = block + DATAGRAM_BLOCKMIN;
        ASKAPCHECK((payload.block >= DATAGRAM_BLOCKMIN) &&
                (payload.block <= DATAGRAM_BLOCKMAX),
                "Payload block is out of range");

        // calculate frequency by first converting channel number 
        // according to the numbering in measurement set 
        const uint32_t cardMeasChan = 
                itsChannelMap.fromCorrelator(cardCorrChan);
        const uint32_t fineMeasChan = ((block * DATAGRAM_NCARD) + card) * 
                DATAGRAM_NCHANNEL + cardMeasChan;
        //payload.freq = (freqMin + freqInc * fineMeasChan) / 1000000.0;
        payload.freq = freqMin + freqInc * fineMeasChan;

        // compute coarse channel number in measurement set
        // (correspond to channel in buffer)
        uint32_t coarseMeasChan = fineMeasChan / itsNChannelSub;
		//cout << "============================================" << endl;
		//cout << "Channel fine & coarse: " << fineMeasChan << ", " <<
		//		coarseMeasChan << endl;

        // for each slice of correlation products
        for (uint32_t slice = 0; slice < nSlice; ++slice) {
            payload.slice = slice;
            payload.baseline1 = slice * nCorrProdPerSlice + 1;
            payload.baseline2 = payload.baseline1 + nCorrProdPerSlice - 1;

			//cout << "-------------------------------------------" << endl;
			//cout << "slice: " << slice << endl;
            for (uint32_t corrProdInSlice = 0; 
                    corrProdInSlice < nCorrProdPerSlice; 
                    ++corrProdInSlice) {
                uint32_t corrProd = corrProdInSlice + 
                        slice * nCorrProdPerSlice;
                payload.vis[corrProdInSlice].real = 
                        itsBuffer.data[corrProd][coarseMeasChan].vis.real;
                payload.vis[corrProdInSlice].imag = 
                        itsBuffer.data[corrProd][coarseMeasChan].vis.imag;
				//cout << "vis: [" << payload.vis[corrProdInSlice].real <<
				//		", " << payload.vis[corrProdInSlice].imag << 
				//		"]" << endl;
				/*
				if ((payload.vis[corrProdInSlice].real < 1.0e-10) &&
					(payload.vis[corrProdInSlice].imag < 1.0e-10)) {
					cout << "very small: " << slice << ", " << 
						corrProdInSlice << endl;
				}
				*/
            }   // correlation product in slice
			
            // Card is sending its payload
            //if (card % itsNShelves == itsShelf - 1) {
            if (totalCard % itsNShelves == itsShelf - 1) {
                itsPort->send(payload);

                if (itsMode == "test") {
                    fillTestBuffer(payload);
                }
            }
        }   // slice
    }   // simulated channel in correlator

    itsBuffer.reset();

#ifdef VERBOSE
    cout << "Shelf " << itsShelf << " finished Sending buffer data" << endl;
#endif

    if (itsMode == "test") {
        checkTestBuffer();
    }
	++itsDataSentCounter;
    return true;

}   // sendBufferData



// TODO: use boost::optional for illegal return value
//
uint32_t CorrelatorSimulatorADE::getCorrProdIndex 
        (uint32_t ant1, uint32_t ant2, const Stokes::StokesTypes& stokesType)
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
        return 0;   // WARNING: this is actually a valid answer
	}
    cout << ant1 << ", " << ant2 << ", " << stokesValue << endl;
	return itsCorrProdMap.getIndex (ant1, ant2, stokesValue);
}



//void CorrelatorSimulatorADE::getFailMode()
//{
//}



void checkStokesType (const Stokes::StokesTypes& stokesType)
{
	ASKAPCHECK (stokesType == Stokes::XX || stokesType == Stokes::XY ||
			stokesType == Stokes::YX || stokesType == Stokes::YY,
			"Unsupported stokes type");
}

#ifdef VERBOSE
#undef VERBOSE
#endif


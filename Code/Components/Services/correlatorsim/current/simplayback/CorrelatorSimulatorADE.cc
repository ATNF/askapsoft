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

//#define VERBOSE
//#define HARDWARELIKE
#define CORRBUFFER

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
        itsDelay(delay), itsCurrentRow(0)  
{
	if (itsInputMode == "zero") {
        itsMS.reset();
        ROMSColumns msc(*itsMS);
        cout << "Antenna count: " << itsNAntenna << endl;
    }
    else if (itsInputMode == "expand") {
		itsMS.reset(new casa::MeasurementSet(dataset, casa::Table::Old));
        ROMSColumns msc(*itsMS);

        cout << "Getting antennas from measurement set" << endl;
        const casa::ROMSAntennaColumns& antc = msc.antenna();
        itsNAntenna = antc.nrow();
        cout << "Antenna count: " << itsNAntenna << endl;
        vector<string> antennaNames;
        for (unsigned int ant = 0; ant < itsNAntenna; ++ant) {
            string antName = antc.name()(ant);
            antennaNames.push_back (antName);
            cout << "  antenna " << ant << ": " << antennaNames[ant] << endl;
        }

        //cout << "Querying the size of data in measurement set" << endl;
	    //casa::Matrix<casa::Complex> data = msc.data()(0);
        //cout << "Data size: " << SIZEOF_ARRAY( data ) << ", " << 
        //        SIZEOF_ARRAY( data[0] ) << endl;
        //cout << "Data size: " << sizeof(data) << ", " << 
        //        sizeof(data[0]) << endl;

        initBuffer();

 	}
	else {
		cout << "ERROR in CorrelatorSimulatorADE: illegal input mode: " <<
                itsInputMode << endl;
	}

    // Port
	itsPort.reset(new askap::cp::VisPortADE(hostname, port));
	
    // Compute the total number of correlation products
    //CorrProdMap itsCorrProd(0,1);
    itsNCorrProd = itsCorrProdMap.getTotal (itsNAntenna);
    cout << "Total correlation products (baselines): " << itsNCorrProd << endl;
    cout << "Max baselines per slice               : " <<
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE << endl;
    itsNSlice = itsNCorrProd / 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    int remainder = itsNCorrProd %
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    if (remainder == 0) {
        cout << "Baselines fit exactly into " << itsNSlice << " slices" <<
            endl;
    }
    else {
        cout << "Baselines fit partially into " << itsNSlice + 1 <<
           " slices" << endl; 
    }
    //if (itsNCorrProd > 
    //        VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE) {
    //    ASKAPCHECK (itsNCorrProd % 
    //        VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE == 0,
    //        "The number of baselines is not divisible by slice");
    //}

    itsFineBandwidth = itsCoarseBandwidth / itsNChannelSub;
}


CorrelatorSimulatorADE::~CorrelatorSimulatorADE()
{
    itsMS.reset();
    itsPort.reset();
}


bool CorrelatorSimulatorADE::sendNext(void)
{
	if (itsInputMode == "zero") {
		return sendNextZero();
	}
	else if (itsInputMode == "expand") {
#ifdef CORRBUFFER
        if (getBufferData()) {
            return sendBufferData();
        }
#else
		return sendNextExpand();
#endif
	}
	return 1;
}


#ifdef HARDWARELIKE

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

    // block
    for (uint32_t block = 0; block < 8; ++block) { 

        // card
        for (uint32_t card = 0; card < MAXCARD; ++card) {

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
    
#else   // NOT hardware-like

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

#endif  // hardware-like


#ifdef CORRBUFFER

// Steps: 
// 1) Initialize the buffer
// 2) Gather data in 1 time interval into buffer
// 3) Send the data in the buffer, slice by slice, until all data has been sent
// Repeat 2 & 3 until all data has been sent
//
// Initialize the buffer
void CorrelatorSimulatorADE::initBuffer ()
{
    
    cout << "Initializing buffer ..." << endl;
    ROMSColumns msc(*itsMS);

    // Get reference to columns of interest
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casa::ROMSAntennaColumns& antc = msc.antenna();
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();

    const uint32_t nAntenna = antc.nrow();
    cout << "  Antenna count: " << nAntenna << endl;

    const int nRow = msc.nrow();
    cout << "  Total rows in measurement set: " << nRow << endl;
    cout << "  Checking all rows ..." << endl;
    casa::Double currentTime = 0.0;

    int dataDescId;
    unsigned int descSpwId, descPolId;
    unsigned int nChan, nCorr;
    //uint32_t row = 0;
    //uint32_t channelMin = 999999999999;
    //uint32_t channelMax = 0;
    int antMin = 9999;
    int antMax = 0;
    int nTime = 0;
    int nRowOfConstTime;
    for (int row = 0; row < nRow; ++row) {

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
        //baselineMin = min(baselineMin,
        //channelMin = min(channelMin,
    }
    cout << "  Checking all rows: done" << endl;
    
    cout << "  Time interval count (= buffer count): " << nTime << endl;
    cout << "  Rows of constant time: " << nRowOfConstTime << endl;
    cout << "  Antenna range: " << antMin << " ~ " << antMax << endl;
    // verify number of antenna here
    int nAnt = antMax - antMin + 1;
    cout << "  Data matrix (corr,channel): " << nCorr << ", " <<
            nChan << endl;
    //cout << "  Baseline range: " << baselineMin << " ~ " <<
    //        baselineMax << endl;
    //cout << "  Channel range: " << channelMin << " ~ " << channelMax << endl;

    cout << "  Creating buffer ..." << endl;
    int nBufferUnit = nRowOfConstTime * nCorr * nChan;
    cout << "    Size = row of constant time x corr x channel = " << 
            nBufferUnit << endl;

    cout << "  Creating buffer: done" << endl;

    cout << "Initializing buffer: done" << endl;
    cout << endl;
}



// Gather data in 1 time interval into buffer
bool CorrelatorSimulatorADE::getBufferData (void)
{
/*
#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::getBufferData ..." << endl;
#endif
    ROMSColumns msc(*itsMS);

    // Get a reference to the columns of interest
    const casa::ROMSFieldColumns& fieldc = msc.field();
    const casa::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();
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
        std::cout<<payload.beamid<<std::endl;
	
		// get visibility data in this row
        data = msc.data()(itsCurrentRow);
		
		++itsCurrentRow;
	}
*/
}


bool CorrelatorSimulatorADE::sendBufferData (void)
{
/*
	cout << "shelf " << itsShelf << 
			" send payload for time period ending in row " << itsCurrentRow-1 << 
			endl; 
	
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

                // TODO
                // Make the interleave changes automatically with
                // the number of MPI processes.
                //
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
*/
}

#else   // NOT CORRBUFFER

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
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::ROMSPolarizationColumns& polc = msc.polarization();
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
			" send payload for time period ending in row " << itsCurrentRow-1 << 
			endl; 
	
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

                // TODO
                // Make the interleave changes automatically with
                // the number of MPI processes.
                //
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

#endif  // CORRBUFFER


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

#ifdef HARDWARELIKE
#undef HARDWARELIKE
#endif

#ifdef CORRBUFFER
#undef CORRBUFFER
#endif


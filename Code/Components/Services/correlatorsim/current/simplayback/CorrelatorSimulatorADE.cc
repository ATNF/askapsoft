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

//#define VERBOSE

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
        const unsigned int nAntenna,
        const unsigned int nCoarseChannel,
        const unsigned int nChannelSub,
        const double coarseBandwidth,
        const std::string& inputMode,
        const unsigned int delay)
        : itsShelf(shelf),
        itsNAntenna(nAntenna), itsNCorrProd(0), itsNSlice(0),
        itsNCoarseChannel(nCoarseChannel),
        itsNChannelSub(nChannelSub), itsCoarseBandwidth(coarseBandwidth),
        itsFineBandwidth(0.0), itsInputMode(inputMode), 
        itsDelay(delay), itsCurrentRow(0)  
{
/*
    if (expansionFactor > 1) {
        ASKAPLOG_DEBUG_STR(logger, "Using expansion factor of " 
                << expansionFactor);
    } else {
        ASKAPLOG_DEBUG_STR(logger, "No expansion factor");
    }
*/

	if (itsInputMode == "expand") {
		itsMS.reset(new casa::MeasurementSet(dataset, casa::Table::Old));
	}
	else {
		itsMS.reset ();
	}
	itsPort.reset(new askap::cp::VisPortADE(hostname, port));
	
    // Compute the total number of correlation products
    CorrProdMap itsCorrProd;
    itsNCorrProd = itsCorrProd.getTotal (nAntenna);
    itsNSlice = itsNCorrProd / 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    ASKAPCHECK (itsNCorrProd % 
            VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE == 0,
            "The number of baselines is not divisible by slice");

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
		return sendNextExpand();
	}
	return 1;
}


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
	// In the whole table, not just for this integration
 
    // Record the timestamp for the current integration that is
    // being processed
    const casa::Double currentIntegration = msc.time()(itsCurrentRow);
    ASKAPLOG_DEBUG_STR(logger, "Processing integration with timestamp "
            << msc.timeMeas()(itsCurrentRow));

    // Some general constraints
    ASKAPCHECK(fieldc.nrow() == 1, "Currently only support a single field");

    // construct payload
    askap::cp::VisDatagramADE payload;
    payload.version = VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;

    // Process rows until none are left or the timestamp
    // changes, indicating the end of this integration
    while (itsCurrentRow < nRow && 
			(currentIntegration == msc.time()(itsCurrentRow))) {

		cout << "shelf " << itsShelf << " sends data in row " << itsCurrentRow 
				<< " / " << nRow << endl;
		
        // Define some useful variables
        const int dataDescId = msc.dataDescId()(itsCurrentRow);
        //const unsigned int descPolId = ddc.polarizationId()(dataDescId);
        //const unsigned int descSpwId = ddc.spectralWindowId()(dataDescId);
        //const unsigned int nCorr = polc.numCorr()(descPolId);
        //const unsigned int nChan = spwc.numChan()(descSpwId);

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
	
        const casa::Matrix<casa::Complex> data = msc.data()(itsCurrentRow);

		// coarse channel
		for (uint32_t cChannel = 0; cChannel < itsNCoarseChannel; ++cChannel) { 

			// subdividing coarse channel into fine channel
			for (uint32_t subDiv = 0; subDiv < itsNChannelSub; ++subDiv) {

				uint32_t fChannel = cChannel * itsNChannelSub + subDiv;
				payload.channel = fChannel;
				payload.freq = itsFineBandwidth * fChannel; // part of freq index
				payload.block = 1;      // part of freq index
				payload.card = 1;       // part of freq index

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
						payload.vis[baseInSlice].real = data(0,0).real();
						payload.vis[baseInSlice].imag = data(0,0).imag();
					}

					// send data in this slice 
#ifdef VERBOSE
					cout << "shelf " << itsShelf << 
							" send payload channel " << cChannel << 
							", sub " << subDiv << ", slice " << slice << endl; 
#endif
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
		
        itsCurrentRow = itsCurrentRow + rowIncrement;
		//cout << "itsCurrentRow: " << itsCurrentRow << endl;
	}	
#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::sendNextExpand: shelf " <<
            itsShelf << ": done" << endl;
#endif

    if (itsCurrentRow >= nRow) {
		cout << "No more data" << endl;
        return false; // Indicate there is no more data after this payload
    } else {
		cout << "time: " << currentIntegration << " / " << 
				msc.time()(itsCurrentRow) << endl;
        return true;
    }
    //return false;   // to stop calling this function
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

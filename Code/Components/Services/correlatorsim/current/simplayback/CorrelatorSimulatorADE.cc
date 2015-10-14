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
#ifdef VERBOSE
    cout << "CorrelatorSimulatorADE::sendNext..." << endl;
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
                        baseInSlice < VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE; ++baseInSlice) { 
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
    cout << "CorrelatorSimulatorADE::sendNext: shelf " <<
            itsShelf << ": done" << endl;
#endif

    return false;   // to stop calling this function
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

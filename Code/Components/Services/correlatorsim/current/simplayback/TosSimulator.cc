/// @file TosSimulator.cc
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
#include "TosSimulator.h"

// Include package level header file
#include "askap_correlatorsim.h"

// System includes
#include <string>
#include <iomanip>
#include <cmath>
#include <inttypes.h>

// ASKAPsoft and casa includes
#include "askap/AskapError.h"
#include "askap/AskapLogging.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Quanta.h"
#include "casacore/measures/Measures/MDirection.h"
#include "tosmetadata/MetadataOutputPort.h"
#include <casacore/measures/Measures/MEpoch.h>
#include <casacore/measures/Measures/MeasConvert.h>
#include <casacore/measures/Measures/MCEpoch.h>

// ICE interface includes
#include "CommonTypes.h"
#include "TypedValues.h"

// Make indefinite loop
//#define LOOP

//#define TEST
//#define VERBOSE
#define CARDFREQ
#define RENAME_ANTENNA

// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::interfaces;
using namespace askap::interfaces::datapublisher;
using namespace casacore;

ASKAP_LOGGER(logger, ".TosSimulator");

TosSimulator::TosSimulator(const std::string& dataset,
        const std::string& locatorHost,
        const std::string& locatorPort,
        const std::string& topicManager,                
        const std::string& topic,
		const unsigned int nAntenna,
        const double metadataSendFail,
        const unsigned int delay)
    : itsMetadataSendFailChance(metadataSendFail), itsCurrentRow(0),
        itsRandom(0.0, 1.0), itsNAntenna(nAntenna), itsDelay(delay)
{
	if (dataset == "") {
		itsMS.reset ();
	}
    else {
		itsMS.reset(new casacore::MeasurementSet(dataset, casacore::Table::Old));
	}
    itsPort.reset(new askap::cp::icewrapper::MetadataOutputPort(locatorHost,
                locatorPort, topicManager, topic));
}



TosSimulator::~TosSimulator()
{
    itsMS.reset();
    itsPort.reset();
}



bool TosSimulator::sendNext(void)
{
	//cout << "TosSimulator::sendNext ..." << endl;
	
    ROMSColumns msc(*itsMS);

    // Get a reference to the columns of interest
    const casacore::ROMSAntennaColumns& antc = msc.antenna();
    //const casacore::ROMSFeedColumns& feedc = msc.feed();
    const casacore::ROMSFieldColumns& fieldc = msc.field();
    const casacore::ROMSSpWindowColumns& spwc = msc.spectralWindow();
    const casacore::ROMSDataDescColumns& ddc = msc.dataDescription();
    //const casacore::ROMSPolarizationColumns& polc = msc.polarization();
    //const casacore::ROMSPointingColumns& pointingc = msc.pointing();

    // Define some useful variables
    const casacore::Int dataDescId = msc.dataDescId()(itsCurrentRow);
    //const casacore::uInt descPolId = ddc.polarizationId()(dataDescId);
    const casacore::uInt descSpwId = ddc.spectralWindowId()(dataDescId);
    const casacore::uInt nRow = msc.nrow(); // In the whole table, not just for this integration
    //const casacore::uInt nCorr = polc.numCorr()(descPolId);
    const casacore::uInt nAntennaMS = antc.nrow();
    const casacore::ROArrayColumn<casacore::Double>& antPosColumn = antc.position();

	cout << "The antenna count in measurement set is " << nAntennaMS << 
			", requested in parset is " << itsNAntenna << endl;

    // Record the timestamp for the current integration that is
    // being processed
    casacore::Double currentIntegration = msc.time()(itsCurrentRow);
    ASKAPLOG_DEBUG_STR(logger, "Processing integration with timestamp "
            << msc.timeMeas()(itsCurrentRow));

    //////////////////////////////////////////////////////////////
    // Metadata
    //////////////////////////////////////////////////////////////

    // Some constraints
    ASKAPCHECK(fieldc.nrow() == 1, "Currently only support a single field");

    // Initialize the metadata message
    askap::cp::TosMetadata metadata;

    // Note, the measurement set stores integration midpoint (in seconds), while the TOS
    // (and it is assumed the correlator) deal with integration start (in microseconds)
    // In addition, TOS time is BAT and the measurement set normally has UTC time
    // (the latter is not checked here as we work with the column as a column of doubles
    // rather than column of measures)
        
    // precision of a single double may not be enough in general, but should be fine for 
    // this emulator (ideally need to represent time as two doubles)
    const casacore::MEpoch epoch(casacore::MVEpoch(casacore::Quantity(currentIntegration,"s")), 
    		casacore::MEpoch::Ref(casacore::MEpoch::UTC));
    const casacore::MVEpoch epochTAI = casacore::MEpoch::Convert(epoch,
    		casacore::MEpoch::Ref(casacore::MEpoch::TAI))().getValue();
    const uint64_t microsecondsPerDay = 86400000000ull;
    const uint64_t startOfDayBAT = uint64_t(epochTAI.getDay()*microsecondsPerDay);
    const long Tint = static_cast<long>(msc.interval()(itsCurrentRow) * 1000 * 1000);
    const uint64_t startBAT = startOfDayBAT + 
			uint64_t(epochTAI.getDayFraction()*microsecondsPerDay) - uint64_t(Tint / 2);

    // ideally we want to carry BAT explicitly as 64-bit unsigned integer, 
	// leave it as it is for now
    metadata.time(static_cast<long>(startBAT));
    metadata.scanId(msc.scanNumber()(itsCurrentRow));
    metadata.flagged(false);
	
    // Calculate and set the centre frequency
    const casacore::Vector<casacore::Double> frequencies = spwc.chanFreq()(descSpwId);
    casacore::Double centreFreq = 0.0;

#ifdef CARDFREQ
    // Each card contains 4 (coarse) channels: (0, 1, 2, 3).
    // Frequency of channel 2 is used as the centre frequency.
    centreFreq = frequencies[2];
    //centreFreq = (frequencies[0] + frequencies[3]) * 0.5;
#else
    const casacore::uInt nChan = frequencies.size();
    if (nChan % 2 == 0) {
        centreFreq = (frequencies(nChan / 2) + frequencies((nChan / 2) + 1) ) / 2.0;
    } else {
        centreFreq = frequencies(nChan / 2);
    }
#endif

    metadata.centreFreq(casacore::Quantity(centreFreq, "Hz"));

#ifdef VERBOSE
    cout << "TOSSim: centre frequency of 4 coarse channels: " << 
            centreFreq << endl;
#endif

#ifdef TEST
    cout << "channel count: " << nChan << endl;
    for (casacore::uInt i = 0; i < nChan; ++i) {
        cout << i << ": " << frequencies[i] << endl;
    }
    cout << "centre frequency: " << centreFreq << endl;
#endif

    // Target Name
    const casacore::Int fieldId = msc.fieldId()(itsCurrentRow);
    metadata.targetName(fieldc.name()(fieldId));

    // Target Direction
    const casacore::Vector<casacore::MDirection> dirVec = fieldc.phaseDirMeasCol()(fieldId);
    const casacore::MDirection direction = dirVec(0);
    metadata.targetDirection(direction);

    // Phase Centre
    metadata.phaseDirection(direction);

    // Correlator Mode
    metadata.corrMode("standard");

	//cout << "before antenna" << endl;
	
    ////////////////////////////////////////
    // Metadata - per antenna
    ////////////////////////////////////////

    // MV: I found the code below with define statements very very ugly. Ideally, it should be re-designed/re-written but for now
    // I'll make only quick and dirty fix

	// Note the number of antennas is as requested in parset instead of 
	// that is actually available in measurement set.
	std::string name;

#ifdef RENAME_ANTENNA
    for (casacore::uInt i = 0; i < itsNAntenna; ++i) {
		stringstream ss;
		ss << i+1;
		if (i < 9) {
			name = "ak0" + ss.str();
		}
		else {
			name = "ak" + ss.str();
		}
#else
    //for (casacore::uInt i = 0; i < nAntennaMS; ++i) {
        //name = antc.name().getColumn()(i);
#endif

        int indexIntoMS = -1;
        for (casacore::uInt testAnt = 0; testAnt < nAntennaMS; ++testAnt) {
             if (name == std::string(antc.name().getColumn()(testAnt))) {
                 indexIntoMS = static_cast<int>(testAnt);
                 break;
             }
        }

        TosMetadataAntenna antMetadata(name);

        // <antenna name>.actual_radec
        antMetadata.actualRaDec(direction);

        // <antenna name>.actual_azel
        MDirection::Ref targetFrame = MDirection::Ref(MDirection::AZEL);
#ifdef RENAME_ANTENNA
        targetFrame.set(MeasFrame(antc.positionMeas()(0), epoch));
#else
        //targetFrame.set(MeasFrame(antc.positionMeas()(i), epoch));
#endif
        const MDirection azel = MDirection::Convert(direction.getRef(),
                targetFrame)(direction);
        antMetadata.actualAzEl(azel);

        // <antenna name>.actual_pol
        antMetadata.actualPolAngle(0.0);

        // <antenna name>.on_source
        // TODO: Current no flagging, but it would be good to read this from the
        // actual measurement set
        antMetadata.onSource(true);

        // <antenna name>.flagged
        // TODO: Current no flagging, but it would be good to read this from the
        // actual measurement set
        antMetadata.flagged(false);

        // <antenna name>.uvw
        // Ideally, we need to send realistic uvws here. However, there is not
        // enough information to deduce the original values from what is stored in the MS.
        // (although, in principle, one could compose a set of numbers which would work -
        // but there are infinite number of ways to do it). Moreover, there seems to be some
        // logic to send metadata on more antennas than available in the MS. I (MV) don't quite
        // understand the current state of the code - such features like sending antennas which are
        // not in the MS wouldn't work with the ingest unless one simulates metadata correctly. 
        // I looked at this code to fix functional tests. For that purpose it would be ok to 
        // fudge uvw's based on antenna position. However, care must be taken on the user side as
        // this hack wouldn't work in all circumstances. Some serious re-design effort would be required
        // to fix this properly.
        if (indexIntoMS >= 0) {
           const casacore::Vector<casacore::Double> antPos = antPosColumn(indexIntoMS);
           ASKAPASSERT(antPos.nelements() == 3u);
           casacore::Vector<casacore::Double> dummyUVW(36*3,0.);
           for (casacore::uInt elem = 0; elem < dummyUVW.nelements(); ++elem) {
                dummyUVW[elem] = antPos[elem % 3];
           }
           antMetadata.uvw(dummyUVW);
        } else {
           // can't do much except sending some non-zero number - if this case is triggered ingest is
           // unlikely to work. Proper simulation of uvw is needed for undefined antennas.
           antMetadata.uvw(casacore::Vector<casacore::Double>(36*3,1e6));
        }

        metadata.addAntenna(antMetadata);
    }

	//cout << "after antenna" << endl;

    // Find the end of the current integration (i.e. find the next timestamp)
    // or the end of the table
    while (itsCurrentRow != nRow && (currentIntegration == msc.time()(itsCurrentRow))) {
        itsCurrentRow++;
    }

    // Send the payload, however we use a RNG to simulate random send failure
    if (itsRandom.gen() > itsMetadataSendFailChance) {
        cout << "TOSSim: pausing " << itsDelay / 1000000 << " seconds" << endl;
        usleep(itsDelay);
        cout << "TOSSim: transmitting ..." << endl;
        itsPort->send(metadata);
    } else {
        ASKAPLOG_DEBUG_STR(logger, "Simulating metadata send failure this cycle");
    }

//#ifdef LOOP

    if (itsCurrentRow >= nRow) {
		ASKAPLOG_INFO_STR(logger,"End of a loop");
		
        ASKAPLOG_INFO_STR(logger,
                "Sending additional metadata message indicating end-of-observation");
        metadata.scanId(-2);    // -2
        cout << "TOSSim: pausing " << itsDelay / 1000000 << " seconds" << endl;
        usleep(itsDelay);
        cout << "TOSSim: transmitting the final data" << endl;
        itsPort->send(metadata);
		
		return false;
	} else {
		return true;
	}

//#else
/*
    // If this is the final payload send another with scan == -1, 
	// indicating the observation has ended
    if (itsCurrentRow == nRow) {

        ASKAPLOG_INFO_STR(logger,
                "Sending additional metadata message indicating end-of-observation"
				);
        metadata.scanId(-2);	// -2

        cout << "TOSSim: pausing " << itsDelay / 1000000 << " seconds" << endl;
        usleep(itsDelay);
        cout << "TOSSim: transmitting the final data" << endl;
        itsPort->send(metadata);

		//cout << "TosSimulator::sendNext: no more data" << endl;
        return false; // Indicate there is no more data after this payload

    } else {
		//cout << "TosSimulator::sendNext: more data" << endl;
        return true;
    }
*/
//#endif

}	// sendNext



void TosSimulator::resetCurrentRow(void)
{
    itsCurrentRow = 0;
}


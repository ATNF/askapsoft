/// @file SpectralLineMaster.cc
///
/// @copyright (c) 2009 CSIRO
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
#include "SpectralLineMaster.h"

// System includes
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <Common/ParameterSet.h>
#include <fitting/Params.h>
#include <fitting/Axes.h>
#include <dataaccess/IConstDataSource.h>
#include <dataaccess/TableConstDataSource.h>
#include <dataaccess/IConstDataIterator.h>
#include <dataaccess/IDataConverter.h>
#include <dataaccess/IDataSelector.h>
#include <dataaccess/IDataIterator.h>
#include <dataaccess/SharedIter.h>
#include <casacore/casa/Quanta.h>
#include <imageaccess/BeamLogger.h>

// Local includes
#include "distributedimager/IBasicComms.h"
#include "messages/SpectralLineWorkUnit.h"
#include "messages/SpectralLineWorkRequest.h"
#include "Tracing.h"

using namespace std;
using namespace askap::cp;
using namespace askap;

ASKAP_LOGGER(logger, ".SpectralLineMaster");

SpectralLineMaster::SpectralLineMaster(LOFAR::ParameterSet& parset,
                                       askap::cp::IBasicComms& comms)
    : itsParset(parset), itsComms(comms), itsBeamList()
{
    itsDoingPreconditioning = false;
    const vector<string> preconditioners = itsParset.getStringVector("preconditioner.Names", std::vector<std::string>());
    for (vector<string>::const_iterator pc = preconditioners.begin(); pc != preconditioners.end(); ++pc) {
        if ((*pc) == "Wiener" || (*pc) == "NormWiener" || (*pc) == "Robust" || (*pc) == "GaussianTaper") {
            itsDoingPreconditioning = true;
        }
    }

}

SpectralLineMaster::~SpectralLineMaster()
{
}

void SpectralLineMaster::run(void)
{
    // Read from the configruation the list of datasets to process
    const vector<string> ms = getDatasets(itsParset);
    if (ms.size() == 0) {
        ASKAPTHROW(std::runtime_error, "No datasets specified in the parameter set file");
    }

    // Get info from each measurement set so we know how many channels, what channels, etc.
    isMSGroupInfo = MSGroupInfo(ms);
    const casa::uInt nChan = isMSGroupInfo.getTotalNumChannels();
    ASKAPCHECK(nChan > 0, "# of channels is zero");
    const casa::Quantity f0 = isMSGroupInfo.getFirstFreq();
    const casa::Quantity freqinc = isMSGroupInfo.getFreqInc();

    // Define reference channel for giving restoring beam
    std::string reference = itsParset.getString("restore.beamReference", "mid");
    if (reference == "mid") {
        itsBeamReferenceChannel = nChan / 2;
    } else if (reference == "first") {
        itsBeamReferenceChannel = 0;
    } else if (reference == "last") {
        itsBeamReferenceChannel = nChan - 1;
    } else { // interpret reference as a 0-based channel nuumber
        unsigned int num = atoi(reference.c_str());
        if (num < nChan) {
            itsBeamReferenceChannel = num;
        } else {
            ASKAPLOG_WARN_STR(logger, "beamReference value (" << reference
                              << ") not valid. Using middle value of " << nChan / 2);
            itsBeamReferenceChannel = nChan / 2;
        }
    }


    // Create an image cube builder
    Tracing::entry(Tracing::WriteImage);
    itsImageCube.reset(new CubeBuilder(itsParset, nChan, f0, freqinc));
    itsPSFCube.reset(new CubeBuilder(itsParset, nChan, f0, freqinc, "psf"));
    itsResidualCube.reset(new CubeBuilder(itsParset, nChan, f0, freqinc, "residual"));
    itsWeightsCube.reset(new CubeBuilder(itsParset, nChan, f0, freqinc, "weights"));
    if (itsParset.getBool("restore", false)) {
        // Only create these if we are restoring, as that is when they get made
        if (itsDoingPreconditioning) {
            itsPSFimageCube.reset(new CubeBuilder(itsParset, nChan, f0, freqinc, "psf.image"));
        }
        itsRestoredCube.reset(new CubeBuilder(itsParset, nChan, f0, freqinc, "restored"));
    }
    Tracing::exit(Tracing::WriteImage);

    // Send work orders to the worker processes, handling out
    // more work to the workers as needed.

    // Global channel is the channel offset across all measurement sets
    // For example, the first MS has 16 channels, then the global channel
    // number for the first (local) channel in the second MS is 16 (zero
    // based indexing).
    unsigned int globalChannel = 0;

    // Tracks all outstanding workunits, that is, those that have not
    // been completed
    unsigned int outstanding = 0;

    // Iterate over all measurement sets
    for (unsigned int n = 0; n < ms.size(); ++n) {
        const unsigned int msChannels = isMSGroupInfo.getNumChannels(n);
        ASKAPLOG_DEBUG_STR(logger, "Creating work orders for measurement set "
                           << ms[n] << " with " << msChannels << " channels");

        // Iterate over all channels in the measurement set
        for (unsigned int localChan = 0; localChan < msChannels; ++localChan) {

            int id; // Id of the process the WorkRequest message is received from

            // Wait for a worker to request some work
            SpectralLineWorkRequest wrequest;
            itsComms.receiveMessageAnySrc(wrequest, id);
            // If the channel number is CHANNEL_UNINITIALISED then this indicates
            // there is no image associated with this message. If the channel
            // number is initialised yet the params pointer is null this indicates
            // that an an attempt was made to process this channel but an exception
            // was thrown.
            if (wrequest.get_globalChannel() != SpectralLineWorkRequest::CHANNEL_UNINITIALISED) {
                if (wrequest.get_params().get() != 0) {
                    handleImageParams(wrequest.get_params(), wrequest.get_globalChannel());
                } else {
                    ASKAPLOG_WARN_STR(logger, "Global channel " << wrequest.get_globalChannel()
                                      << " has failed - will be set to zero in the cube.");
                    recordBeamFailure(wrequest.get_globalChannel());
                }
                --outstanding;
            }

            // Send the workunit to the worker
            ASKAPLOG_INFO_STR(logger, "Master is allocating workunit " << ms[n]
                              << ", local channel " <<  localChan << ", global channel "
                              << globalChannel << " to worker " << id);
            SpectralLineWorkUnit wu;
            wu.set_payloadType(SpectralLineWorkUnit::WORK);
            wu.set_dataset(ms[n]);
            wu.set_globalChannel(globalChannel);
            wu.set_localChannel(localChan);
            wu.set_channelFrequency(f0.getValue("Hz") + globalChannel * freqinc.getValue("Hz"));
            itsComms.sendMessage(wu, id);
            ++outstanding;

            ++globalChannel;
        }
    }

    // Wait for all outstanding workunits to complete
    while (outstanding > 0) {
        int id;
        SpectralLineWorkRequest wrequest;
        itsComms.receiveMessageAnySrc(wrequest, id);
        if (wrequest.get_globalChannel() != SpectralLineWorkRequest::CHANNEL_UNINITIALISED) {
            if (wrequest.get_params().get() != 0) {
                handleImageParams(wrequest.get_params(), wrequest.get_globalChannel());
            } else {
                ASKAPLOG_WARN_STR(logger, "Global channel " << wrequest.get_globalChannel()
                                  << " has failed - will be set to zero in the cube.");
                recordBeamFailure(wrequest.get_globalChannel());
            }
            --outstanding;
        }
    }

    // Send each worker a response to indicate there are
    // no more work units. This is done separate to the above loop
    // since we need to make sure even workers that never received
    // a workunit are send the "DONE" message.
    for (int id = 1; id < itsComms.getNumNodes(); ++id) {
        SpectralLineWorkUnit wu;
        wu.set_payloadType(SpectralLineWorkUnit::DONE);
        itsComms.sendMessage(wu, id);
    }

    logBeamInfo();

    itsImageCube.reset();
}

// Utility function to get dataset names from parset.
std::vector<std::string> SpectralLineMaster::getDatasets(const LOFAR::ParameterSet& parset)
{
    if (parset.isDefined("dataset") && parset.isDefined("dataset0")) {
        ASKAPTHROW(std::runtime_error,
                   "Both dataset and dataset0 are specified in the parset");
    }

    // First look for "dataset" and if that does not exist try "dataset0"
    vector<string> ms;
    if (parset.isDefined("dataset")) {
        ms = itsParset.getStringVector("dataset", true);
    } else {
        string key = "dataset0";   // First key to look for
        long idx = 0;
        while (parset.isDefined(key)) {
            const string value = parset.getString(key);
            ms.push_back(value);

            ostringstream ss;
            ss << "dataset" << idx + 1;
            key = ss.str();
            ++idx;
        }
    }

    return ms;
}

void SpectralLineMaster::handleImageParams(askap::scimath::Params::ShPtr params,
        unsigned int chan)
{
    Tracing::entry(Tracing::WriteImage);

    // Pre-conditions
    ASKAPCHECK(params->has("model.slice"), "Params are missing model parameter");
    ASKAPCHECK(params->has("psf.slice"), "Params are missing psf parameter");
    ASKAPCHECK(params->has("residual.slice"), "Params are missing residual parameter");
    ASKAPCHECK(params->has("weights.slice"), "Params are missing weights parameter");
    if (itsParset.getBool("restore", false)) {
        ASKAPCHECK(params->has("image.slice"), "Params are missing image parameter");
        if (itsDoingPreconditioning) {
            ASKAPCHECK(params->has("psf.image.slice"), "Params are missing psf.image parameter");
        }
    }

    if (itsParset.getBool("restore", false)) {
        // Record the restoring beam
        const askap::scimath::Axes &axes = params->axes("image.slice");
        recordBeam(axes, chan);
        storeBeam(chan);
    }

    // Write image
    {
        const casa::Array<double> imagePixels(params->value("model.slice"));
        casa::Array<float> floatImagePixels(imagePixels.shape());
        casa::convertArray<float, double>(floatImagePixels, imagePixels);
        itsImageCube->writeSlice(floatImagePixels, chan);
    }

    // Write PSF
    {
        const casa::Array<double> imagePixels(params->value("psf.slice"));
        casa::Array<float> floatImagePixels(imagePixels.shape());
        casa::convertArray<float, double>(floatImagePixels, imagePixels);
        itsPSFCube->writeSlice(floatImagePixels, chan);
    }

    // Write residual
    {
        const casa::Array<double> imagePixels(params->value("residual.slice"));
        casa::Array<float> floatImagePixels(imagePixels.shape());
        casa::convertArray<float, double>(floatImagePixels, imagePixels);
        itsResidualCube->writeSlice(floatImagePixels, chan);
    }

    // Write weights
    {
        const casa::Array<double> imagePixels(params->value("weights.slice"));
        casa::Array<float> floatImagePixels(imagePixels.shape());
        casa::convertArray<float, double>(floatImagePixels, imagePixels);
        itsWeightsCube->writeSlice(floatImagePixels, chan);
    }


    if (itsParset.getBool("restore", false)) {

        if (itsDoingPreconditioning) {
            // Write preconditioned PSF image
            {
                const casa::Array<double> imagePixels(params->value("psf.image.slice"));
                casa::Array<float> floatImagePixels(imagePixels.shape());
                casa::convertArray<float, double>(floatImagePixels, imagePixels);
                itsPSFimageCube->writeSlice(floatImagePixels, chan);
            }
        }

        // Write Restored image
        {
            const casa::Array<double> imagePixels(params->value("image.slice"));
            casa::Array<float> floatImagePixels(imagePixels.shape());
            casa::convertArray<float, double>(floatImagePixels, imagePixels);
            itsRestoredCube->writeSlice(floatImagePixels, chan);
        }
    }

    Tracing::exit(Tracing::WriteImage);
}


void SpectralLineMaster::recordBeam(const askap::scimath::Axes &axes,
                                    const unsigned int globalChannel)
{

    if (axes.has("MAJMIN")) {
        // this is a restored image with beam parameters set
        ASKAPCHECK(axes.has("PA"), "PA axis should always accompany MAJMIN");
        ASKAPLOG_INFO_STR(logger, "Found beam for image.slice, channel " <<
                          globalChannel << ", with shape " <<
                          axes.start("MAJMIN") * 180. / M_PI * 3600. << "x" <<
                          axes.end("MAJMIN") * 180. / M_PI * 3600. << ", " <<
                          axes.start("PA") * 180. / M_PI);

        casa::Vector<casa::Quantum<double> > beamVec(3, 0.);
        beamVec[0] = casa::Quantum<double>(axes.start("MAJMIN"), "rad");
        beamVec[1] = casa::Quantum<double>(axes.end("MAJMIN"), "rad");
        beamVec[2] = casa::Quantum<double>(axes.start("PA"), "rad");

        itsBeamList[globalChannel] = beamVec;

    }

}

void SpectralLineMaster::recordBeamFailure(const unsigned int globalChannel)
{

    casa::Vector<casa::Quantum<double> > beamVec(3, 0.);
    itsBeamList[globalChannel] = beamVec;
    if (globalChannel == itsBeamReferenceChannel) {
        ASKAPLOG_WARN_STR(logger, "Beam reference channel " << itsBeamReferenceChannel
                          << " has failed - output cubes have no restoring beam.");
    }

}


void SpectralLineMaster::storeBeam(const unsigned int globalChannel)
{
    if (globalChannel == itsBeamReferenceChannel) {
        itsRestoredCube->addBeam(itsBeamList[globalChannel]);
    }
}

void SpectralLineMaster::logBeamInfo()
{

    if (itsParset.getBool("restore", false)) {
        askap::accessors::BeamLogger beamlog(itsParset.makeSubset("restore."));
        if (beamlog.filename() != "") {
            ASKAPCHECK(itsBeamList.begin()->first == 0, "Beam list doesn't start at channel 0");
            ASKAPCHECK((itsBeamList.size() == (itsBeamList.rbegin()->first + 1)),
                       "Beam list doesn't finish at channel " << itsBeamList.size() - 1);
            std::vector<casa::Vector<casa::Quantum<double> > > beams;
            std::map<unsigned int, casa::Vector<casa::Quantum<double> > >::iterator beam;
            for (beam = itsBeamList.begin(); beam != itsBeamList.end(); beam++) {
                beams.push_back(beam->second);
            }
            beamlog.beamlist() = beams;
            ASKAPLOG_INFO_STR(logger, "Writing list of individual channel beams to beam log "
                              << beamlog.filename());
            beamlog.write();
        }
    }

}

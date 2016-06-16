/// @file ContinuumMaster.cc
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
#include "ContinuumMaster.h"

// System includes
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askapparallel/AskapParallel.h>

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
#include <dataaccess/TableInfoAccessor.h>
#include <casacore/casa/Quanta.h>
#include <imageaccess/BeamLogger.h>
#include <parallel/ImagerParallel.h>
#include <measurementequation/SynthesisParamsHelper.h>

// Local includes
#include "distributedimager/AdviseDI.h"
#include "distributedimager/IBasicComms.h"
#include "distributedimager/CalcCore.h"
#include "messages/ContinuumWorkUnit.h"
#include "messages/ContinuumWorkRequest.h"
#include "Tracing.h"

//casacore includes
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"

using namespace std;
using namespace askap::cp;
using namespace askap;

ASKAP_LOGGER(logger, ".ContinuumMaster");

ContinuumMaster::ContinuumMaster(LOFAR::ParameterSet& parset,
                                       askapparallel::AskapParallel& comms)
    : itsParset(parset), itsComms(comms), itsBeamList()
{
}

ContinuumMaster::~ContinuumMaster()
{
}

void ContinuumMaster::run(void)
{
    // Read from the configruation the list of datasets to process
    const vector<string> ms = getDatasets(itsParset);
    if (ms.size() == 0) {
        ASKAPTHROW(std::runtime_error, "No datasets specified in the parameter set file");
    }
  
    vector<int> theBeams = getBeams();
    
  
    
    const double targetPeakResidual = synthesis::SynthesisParamsHelper::convertQuantity(
                itsParset.getString("threshold.majorcycle", "-1Jy"), "Jy");
    int nChanperCore = itsParset.getInt32("nchanpercore", 0);
   
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
   
    unsigned int nWorkers = itsComms.nProcs() - 1;
    unsigned int nWorkersPerGroup = nWorkers/itsComms.nGroups();
   
    vector<unsigned int> allocation;
    allocation.resize(itsComms.nProcs());
    for (unsigned int n = 0; n < allocation.size(); ++n) {
       allocation[n] = 0;
    }
    // get the number of beams
    size_t beam = theBeams[0];
    // Iterate over all measurement sets
    for (unsigned int n = 0; n < ms.size(); ++n) {
        
        askap::accessors::TableConstDataSource ds(ms[n]);
        const casa::MeasurementSet in(ms[n]);
        const casa::ROMSColumns srcCols(in);
        const casa::ROMSFeedColumns& fc = srcCols.feed();
        
        const int fieldEntries = fc.nrow();
        
        const casa::uInt firstAnt = casa::ROScalarColumn<casa::Int>(in.feed(),"ANTENNA_ID")(0);
        unsigned int nBeams = 0;
        
        for (int row = 0; row<fieldEntries; ++row) {
                if (casa::ROScalarColumn<casa::Int>(in.feed(),"ANTENNA_ID")(row) == firstAnt)
                    ++nBeams;
                else
                    break;
        }
        
        askap::accessors::IDataSelectorPtr sel = ds.createSelector();
        askap::accessors::IDataConverterPtr conv = ds.createConverter();
        conv->setFrequencyFrame(casa::MFrequency::Ref(casa::MFrequency::TOPO), "Hz");
        conv->setDirectionFrame(casa::MDirection::Ref(casa::MDirection::J2000));
        const askap::accessors::IConstDataSharedIter it = ds.createConstIterator(sel, conv);

        const unsigned int msChannels = it->nChannel();
        unsigned int workChannels = nWorkersPerGroup * nChanperCore;
        
        if (msChannels < workChannels) {
            
            ASKAPTHROW(std::runtime_error,
                       "Insufficient work for resources available reduce nchanpercore");
            
        }
        else {
            ASKAPLOG_WARN_STR(logger, "Too much work for resources avaiable - can deal with this though - will only process " << workChannels << " channels");
        }
        ASKAPLOG_INFO_STR(logger, "Creating work orders for measurement set "
                           << ms[n] << " processing " << workChannels << " channels and " << nChanperCore
                           << " channels per core");

        
        if (msChannels > nWorkersPerGroup * nChanperCore) {
           ASKAPLOG_WARN_STR(logger,"Only " << nWorkersPerGroup * nChanperCore << " of " << msChannels << " will be processed");
        }
        
        // ASKAPLOG_INFO_STR(logger,"There are " << nWorkersPerGroup << " workers per group and " << nBeams << " beams to process");
        int minWorkers = msChannels * itsComms.nGroups() * nBeams/nChanperCore;
        
        ASKAPLOG_WARN_STR(logger,"Will currently only process beam " << beam);
        
        
        ASKAPLOG_INFO_STR(logger, "There are  "
                          << itsComms.nGroups() << " groups of " << nWorkers
                          << " with " << nWorkersPerGroup);
        
        
        
        
        
        for (size_t group = 0; group<itsComms.nGroups(); ++group) {
            
            // Iterate over all channels in the measurement set
            // but each core just needs to ask once before it is allocated all its channels
            for (unsigned int localChan = 0; localChan < workChannels; localChan = localChan+nChanperCore) {
                
                casa::Quantity freq;
                
                freq = casa::Quantity(it->frequency()[localChan],"Hz");
                
                int id; // Id of the process the WorkRequest message is received from
                
                // Wait for a worker to request some work
                ASKAPLOG_INFO_STR(logger, "Master is waiting for a worker to request some work for channel " << localChan <<  " frequency " << freq);
                
                // wait for valid work request
                ContinuumWorkRequest wrequest;
                int inGroup = 0;
                int inRange = 0;
                while (inGroup == 0 || inRange == 0)
                {
                    
                    inRange = 0;
                    wrequest.receiveRequest(id, itsComms);
                    if (id > group*nWorkersPerGroup && id <= (group+1)*nWorkersPerGroup) {
                        inGroup = 1;
                        ASKAPLOG_INFO_STR(logger, "Master received request from " << id << " Worker in group "
                                          << group);
                        /// we need the channel to fall into the allocation for this id : the position this id holds in the group
                        /// multiplied by the number of channels per core
                        /// what is the position this rank holds in the group
                        
                        
                        unsigned int posInGroup = (id % nWorkersPerGroup);
                        if (posInGroup == 0) {
                            posInGroup = nWorkersPerGroup;
                        }
                        posInGroup = posInGroup-1;
                        
                        unsigned int baseChannel = posInGroup * nChanperCore;
                        unsigned int topChannel = baseChannel + nChanperCore - 1;
                        
                        if (localChan >= baseChannel && localChan <= topChannel) {
                            inRange = 1;
                        }
                        
                        
                    }
                    if (!inGroup || !inRange) {
                        ASKAPLOG_INFO_STR(logger, "Master received request from " << id
                                          << " Worker NOT in group or already has full allocation or Channel not in allocation");
                        ContinuumWorkUnit wu;
                        wu.set_payloadType(ContinuumWorkUnit::NA);
                        wu.sendUnit(id,itsComms);
                    }
                }
                // If the worker is not in this group send NA (not applicable) and it will drop it
                // If the channel number is CHANNEL_UNINITIALISED then this indicates
                // there is no image associated with this message. If the channel
                // number is initialised yet the params pointer is null this indicates
                // that an an attempt was made to process this channel but an exception
                // was thrown.
                
                
                // Send the workunit to the worker
                ASKAPLOG_INFO_STR(logger, "Master is allocating workunit " << ms[n]
                                  << ", local channel " <<  localChan << ", global channel "
                                  << globalChannel << " to worker " << id);
                ContinuumWorkUnit wu;
                wu.set_payloadType(ContinuumWorkUnit::WORK);
                wu.set_dataset(ms[n]);
                wu.set_globalChannel(globalChannel);
                wu.set_localChannel(localChan);
                wu.set_beam(beam);
                wu.set_channelFrequency(freq.getValue("Hz"));
                wu.sendUnit(id,itsComms);
                ++outstanding;
                ++globalChannel;
                
            }
            
        }
        
    }
    
    
    ASKAPLOG_INFO_STR(logger, "Master all outstanding work-units are allocated");
    
    // Send each worker a response to indicate there are
    // no more work units. This is done separate to the above loop
    // since we need to make sure even workers that never received
    // a workunit are send the "DONE" message.
    for (int id = 1; id < itsComms.nProcs(); ++id) {
        ContinuumWorkUnit wu;
        wu.set_payloadType(ContinuumWorkUnit::DONE);
        wu.sendUnit(id,itsComms);
    }

    
    ASKAPLOG_INFO_STR(logger, "Master is about to broadcast first <empty> model");
    
    // this parset need to know direction and frequency for the final maps/models
    // But I dont want to run Cadvise as it is too specific to the old imaging requirements
   
    char ChannelPar[64];
   
    sprintf(ChannelPar,"[1,0]"); // we are the master
   
    LOFAR::ParameterSet unitParset = itsParset;
   
    unitParset.replace("Channels",ChannelPar);
    
    synthesis::AdviseDI diadvise(itsComms,unitParset);
    diadvise.addMissingParameters();
    
    const bool writeAtMajorCycle = unitParset.getBool("Images.writeAtMajorCycle", false);
    const int nCycles = unitParset.getInt32("ncycles", 0);
    
    ASKAPLOG_INFO_STR(logger,"*****");
    ASKAPLOG_INFO_STR(logger,"Parset" << diadvise.getParset());
    ASKAPLOG_INFO_STR(logger,"*****");
    
    
    if (nCycles == 0) {
        synthesis::ImagerParallel imager(itsComms, diadvise.getParset());
        ASKAPLOG_INFO_STR(logger, "Master beginning single cycle");
        imager.broadcastModel(); // initially empty model
     
        imager.receiveNE();
        
        /// No Minor Cycle to mimic cimager
        
        
        /// Implicit receive in here
        
        /// imager.solveNE();
        /// Write out the images
        imager.writeModel();

    }
    else {
        synthesis::ImagerParallel imager(itsComms, diadvise.getParset());
        for (int cycle = 0; cycle < nCycles; ++cycle) {
            ASKAPLOG_INFO_STR(logger, "Master beginning major cycle ** " << cycle);
            
            if (cycle==0) {
                imager.broadcastModel(); // initially empty model
            }
            /// Minor Cycle
            /// Implicit receive in here
            imager.calcNE(); // resets the itsNE
            imager.solveNE();
            
            imager.broadcastModel();
            
            if (imager.params()->has("peak_residual")) {
                const double peak_residual = imager.params()->scalarValue("peak_residual");
                ASKAPLOG_INFO_STR(logger, "Major Cycle " << cycle << " Reached peak residual of " << peak_residual);
                if (peak_residual < targetPeakResidual) {
                    ASKAPLOG_INFO_STR(logger, "It is below the major cycle threshold of "
                                      << targetPeakResidual << " Jy. Stopping.");
                    imager.broadcastModel();
                    break;
                } else {
                    if (targetPeakResidual < 0) {
                        ASKAPLOG_INFO_STR(logger, "Major cycle flux threshold is not used.");
                    } else {
                        ASKAPLOG_INFO_STR(logger, "It is above the major cycle threshold of "
                                          << targetPeakResidual << " Jy. Continuing.");
                    }
                }
            }
            if (writeAtMajorCycle) {
                ASKAPLOG_INFO_STR(logger, "Writing out model");
                imager.writeModel(std::string(".beam") + utility::toString(beam) + std::string(".majorcycle.") + utility::toString(cycle + 1));
            }
            else {
                ASKAPLOG_INFO_STR(logger, "Not writing out model");
            }
            if (cycle == nCycles-1) {
                imager.calcNE(); // resets the itsNE
                imager.receiveNE();
                imager.writeModel();

            }
            
        }
        
    }
        
    
    
    
    
    logBeamInfo();

}

// Utility function to get dataset names from parset.
std::vector<std::string> ContinuumMaster::getDatasets(const LOFAR::ParameterSet& parset)
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
// Utility function to get dataset names from parset.
std::vector<int> ContinuumMaster::getBeams()
{
    std::vector<int> bs;
    
    if (itsParset.isDefined("beams")) {
        bs = itsParset.getInt32Vector("beams",bs);
    
    }
    else {
        bs.push_back(0);
    }
    return bs;
}

void ContinuumMaster::handleImageParams(askap::scimath::Params::ShPtr params,
                                           unsigned int chan)
{
    Tracing::entry(Tracing::WriteImage);
    bool doingPreconditioning=false;
    const vector<string> preconditioners=itsParset.getStringVector("preconditioner.Names",std::vector<std::string>());
    for (vector<string>::const_iterator pc = preconditioners.begin(); pc != preconditioners.end(); ++pc) {
        if( (*pc)=="Wiener" || (*pc)=="NormWiener" || (*pc)=="Robust" || (*pc) == "GaussianTaper") {
            doingPreconditioning=true;
        }
    }

    // Pre-conditions
    ASKAPCHECK(params->has("model.slice"), "Params are missing model parameter");
    ASKAPCHECK(params->has("psf.slice"), "Params are missing psf parameter");
    ASKAPCHECK(params->has("residual.slice"), "Params are missing residual parameter");
    ASKAPCHECK(params->has("weights.slice"), "Params are missing weights parameter");
    if (itsParset.getBool("restore", false) ) {
        ASKAPCHECK(params->has("image.slice"), "Params are missing image parameter");
        if (doingPreconditioning) {
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


    if (itsParset.getBool("restore", false)){

        if(doingPreconditioning) {
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


void ContinuumMaster::recordBeam(const askap::scimath::Axes &axes,
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

void ContinuumMaster::recordBeamFailure(const unsigned int globalChannel)
{

    casa::Vector<casa::Quantum<double> > beamVec(3, 0.);
    itsBeamList[globalChannel] = beamVec;
    if (globalChannel == itsBeamReferenceChannel) {
        ASKAPLOG_WARN_STR(logger, "Beam reference channel " << itsBeamReferenceChannel
                          << " has failed - output cubes have no restoring beam.");
    }
    
}


void ContinuumMaster::storeBeam(const unsigned int globalChannel)
{
    if (globalChannel == itsBeamReferenceChannel) {
        itsRestoredCube->addBeam(itsBeamList[globalChannel]);
    }
}

void ContinuumMaster::logBeamInfo()
{

    if (itsParset.getBool("restore", false)){
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

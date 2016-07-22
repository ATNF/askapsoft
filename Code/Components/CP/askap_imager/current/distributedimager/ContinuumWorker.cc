/// @file ContinuumWorker.cc
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
#include "ContinuumWorker.h"

// System includes
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <fitting/Equation.h>
#include <fitting/INormalEquations.h>
#include <fitting/ImagingNormalEquations.h>
#include <fitting/Params.h>
#include <gridding/IVisGridder.h>
#include <gridding/VisGridderFactory.h>
#include <measurementequation/SynthesisParamsHelper.h>
#include <measurementequation/ImageFFTEquation.h>
#include <measurementequation/SynthesisParamsHelper.h>
#include <dataaccess/IConstDataSource.h>
#include <dataaccess/TableConstDataSource.h>
#include <dataaccess/IConstDataIterator.h>
#include <dataaccess/IDataConverter.h>
#include <dataaccess/IDataSelector.h>
#include <dataaccess/IDataIterator.h>
#include <dataaccess/SharedIter.h>
#include <utils/PolConverter.h>
#include <Common/ParameterSet.h>
#include <Common/Exceptions.h>
#include <casacore/casa/OS/Timer.h>
#include <parallel/ImagerParallel.h>
#include <imageaccess/BeamLogger.h>
// CASA Includes

// Local includes
#include "distributedimager/AdviseDI.h"
#include "distributedimager/CalcCore.h"
#include "distributedimager/MSSplitter.h"
#include "messages/ContinuumWorkUnit.h"
#include "messages/ContinuumWorkRequest.h"
#include "distributedimager/CubeBuilder.h"

using namespace std;
using namespace askap::cp;
using namespace askap;
using namespace askap::scimath;
using namespace askap::synthesis;
using namespace askap::accessors;

ASKAP_LOGGER(logger, ".ContinuumWorker");

ContinuumWorker::ContinuumWorker(LOFAR::ParameterSet& parset,
                                       askapparallel::AskapParallel& comms)
    : itsParset(parset), itsComms(comms)
{
    itsGridder_p = VisGridderFactory::make(itsParset);
    // lets properly size the storage
    const int nchanpercore = itsParset.getInt32("nchanpercore", 1);
    workUnits.resize(0);
    itsParsets.resize(0);

    // lets calculate a base
    unsigned int nWorkers = itsComms.nProcs() - 1;
    unsigned int nWorkersPerGroup = nWorkers/itsComms.nGroups();

    unsigned int id = itsComms.rank();
    // e. g. rank 8, 3 per group should be pos. 1 (zero index)
    unsigned int posInGroup = (id% nWorkersPerGroup);

    if (posInGroup == 0) {
        posInGroup = nWorkersPerGroup;
    }
    posInGroup = posInGroup - 1;
    this->baseChannel = posInGroup * nchanpercore;
    ASKAPLOG_INFO_STR(logger,"Distribution: Id " << id << " nWorkers " << nWorkers << " nGroups " << itsComms.nGroups());

    ASKAPLOG_INFO_STR(logger,"Distribution: Base channel " << this->baseChannel << " PosInGrp " << posInGroup);

    itsDoingPreconditioning = false;
    const vector<string> preconditioners = itsParset.getStringVector("preconditioner.Names", std::vector<std::string>());
    for (vector<string>::const_iterator pc = preconditioners.begin(); pc != preconditioners.end(); ++pc) {
        if ((*pc) == "Wiener" || (*pc) == "NormWiener" || (*pc) == "Robust" || (*pc) == "GaussianTaper") {
            itsDoingPreconditioning = true;
        }
    }

}

ContinuumWorker::~ContinuumWorker()
{
    itsGridder_p.reset();


}

void ContinuumWorker::run(void)
{
    // Send the initial request for work
    ContinuumWorkRequest wrequest;
    ASKAPLOG_INFO_STR(logger,"Worker is sending request for work");






    wrequest.sendRequest(itsMaster,itsComms);
    while (1) {

        ContinuumWorkUnit wu;
        ASKAPLOG_INFO_STR(logger,"Worker is waiting for work allocation");
        wu.receiveUnitFrom(itsMaster,itsComms);
        if (wu.get_payloadType() == ContinuumWorkUnit::DONE) {
            ASKAPLOG_INFO_STR(logger,"Worker has received complete allocation");
            break;
        }
        else if (wu.get_payloadType() == ContinuumWorkUnit::NA) {
            sleep(1);
            wrequest.sendRequest(itsMaster,itsComms);
            continue;
        }
        else {


            const string ms = wu.get_dataset();
            ASKAPLOG_INFO_STR(logger, "Received Work Unit for dataset " << ms
                              << ", local (topo) channel " << wu.get_localChannel()
                              << ", global (topo) channel " << wu.get_globalChannel()
                              << ", frequency " << wu.get_channelFrequency()/1.e6 << " MHz"
                              << ", width " << wu.get_channelWidth()/1e3 << " kHz");
            processWorkUnit(wu);
            ASKAPLOG_INFO_STR(logger,"Worker is sending request for work");
            if (wu.get_payloadType() == ContinuumWorkUnit::LAST) {
                break;
            }
            else {
                wrequest.sendRequest(itsMaster,itsComms);
            }
        }

    }

    if (workUnits.size()>=1)
        processChannels();

}

void ContinuumWorker::processWorkUnit(ContinuumWorkUnit& wu)
{



    // This also needs to set the frequencies and directions for all the images
    LOFAR::ParameterSet unitParset = itsParset;

    char ChannelPar[64];

    sprintf(ChannelPar,"[1,%d]",wu.get_localChannel()+1);



    bool usetmpfs = unitParset.getBool("usetmpfs",true);

    if (usetmpfs)
    {
        const string ms = wu.get_dataset();

        const string shm_root = unitParset.getString("tmpfs","/dev/shm");

        std::ostringstream pstr;

        pstr << shm_root << "/"<< ms << "_chan_" << wu.get_localChannel()+1 << "_beam_" << wu.get_beam() << ".ms";

        const string outms = pstr.str();
        pstr << ".working";

        const string outms_flag = pstr.str();

        string param = "beams";
        std::ostringstream bstr;

        bstr<<"[" << wu.get_beam() << "]";

        unitParset.replace(param, bstr.str().c_str());

        struct stat buffer;

        while (stat (outms_flag.c_str(), &buffer) == 0) {
            // flag file exists - someone is writing

            sleep(1);
        }
        if (stat (outms.c_str(), &buffer) == 0) {
            ASKAPLOG_WARN_STR(logger, "Split file already exists");
        }
        else if (stat (outms.c_str(), &buffer) != 0 && stat (outms_flag.c_str(), &buffer) != 0) {
            // file cannot be read

            // drop trigger
            ofstream trigger;
            trigger.open(outms_flag.c_str());
            trigger.close();
            MSSplitter mySplitter(unitParset);

            mySplitter.split(ms,outms,wu.get_localChannel()+1,wu.get_localChannel()+1,1,unitParset);
            unlink(outms_flag.c_str());

        }


        wu.set_dataset(outms);


        sprintf(ChannelPar,"[1,1]");
    }
    unitParset.replace("dataset",wu.get_dataset());
    unitParset.replace("Channels",ChannelPar);


    ASKAPLOG_INFO_STR(logger,"getting advice on missing parametrs");
    synthesis::AdviseDI diadvise(itsComms,unitParset);
    diadvise.addMissingParameters();
    ASKAPLOG_INFO_STR(logger,"advice received");
    ASKAPLOG_INFO_STR(logger,"storing workUnit");
    workUnits.push_back(wu);
    ASKAPLOG_INFO_STR(logger,"storing parset");
    itsParsets.push_back(diadvise.getParset());

}


void
ContinuumWorker::processSnapshot(LOFAR::ParameterSet& unitParset)
{
}
void ContinuumWorker::buildSpectralCube() {

    ASKAPLOG_INFO_STR(logger,"Processing multiple channels local solver mode");
    /// This is the spectral cube builder
    /// it marshalls the following tasks:
    /// 1. building a spectral cube image
    /// 2. local minor cycle solving of each channel
    /// 3. Conversion to FITS and output
    /// Note: at this stage it will generate a cube per
    /// node.
    /// Therefore the cube will be distributed across the
    /// supercomputer as a function of frequency and beams
    ///
    /// First lets set up the cube

    /// The number of channels allocated to this rank
    const int nchanpercore = itsParsets[0].getInt32("nchanpercore", 1);
    /// the base channel of this allocation. we know this as the channel allocations
    /// are sorted
    Quantity f0(workUnits[0].get_channelFrequency(),"Hz");
    /// The width of a channel. THis does <NOT> take account of the variable width
    /// of BArycentric channels
    Quantity freqinc(workUnits[0].get_channelWidth(),"Hz");


    ASKAPLOG_INFO_STR(logger,"Configuring Spectral Cube");
    ASKAPLOG_INFO_STR(logger,"nchan: " << nchanpercore << " base f0: " << f0.getValue("MHz")
    << " width: " << freqinc.getValue("MHz") <<" (" << workUnits[0].get_channelWidth() << ")");

    std::string root = "image";

    std::string img_name = root + std::string(".ch.") \
    + utility::toString(workUnits[0].get_globalChannel());

    root = "psf";
    std::string psf_name = root + std::string(".ch.") \
    + utility::toString(workUnits[0].get_globalChannel());

    root = "residual";

    std::string residual_name = root + std::string(".ch.") \
    + utility::toString(workUnits[0].get_globalChannel());

    root = "weights";

    std::string weights_name = root + std::string(".ch.") \
    + utility::toString(workUnits[0].get_globalChannel());


    itsImageCube.reset(new CubeBuilder(itsParsets[0], nchanpercore, f0, freqinc,img_name));
    itsPSFCube.reset(new CubeBuilder(itsParsets[0], nchanpercore, f0, freqinc, psf_name));
    itsResidualCube.reset(new CubeBuilder(itsParsets[0], nchanpercore, f0, freqinc, residual_name));
    itsWeightsCube.reset(new CubeBuilder(itsParsets[0], nchanpercore, f0, freqinc, weights_name));

    if (itsParset.getBool("restore", false)) {
        root = "psf.image";
        std::string psf_image_name = root + std::string(".ch.") \
        + utility::toString(workUnits[0].get_globalChannel());
        root = "image.restored";
        std::string restored_image_name = root + std::string(".ch.") \
        + utility::toString(workUnits[0].get_globalChannel());
        // Only create these if we are restoring, as that is when they get made
        if (itsDoingPreconditioning) {
            itsPSFimageCube.reset(new CubeBuilder(itsParsets[0], nchanpercore, f0, freqinc, psf_image_name));
        }
        itsRestoredCube.reset(new CubeBuilder(itsParsets[0], nchanpercore, f0, freqinc, restored_image_name));
    }

    /// What are the plans for the deconvolution?
    ASKAPLOG_INFO_STR(logger,"Ascertaining Cleaning Plan");
    const bool writeAtMajorCycle = itsParsets[0].getBool("Images.writeAtMajorCycle",false);
    const int nCycles = itsParsets[0].getInt32("ncycles", 0);
    std::string majorcycle = itsParsets[0].getString("threshold.majorcycle", "-1Jy");
    const double targetPeakResidual = SynthesisParamsHelper::convertQuantity(majorcycle, "Jy");

    const int uvwMachineCacheSize = itsParsets[0].getInt32("nUVWMachines", 1);
    ASKAPCHECK(uvwMachineCacheSize > 0 ,
               "Cache size is supposed to be a positive number, you have "
               << uvwMachineCacheSize);

    const double uvwMachineCacheTolerance = SynthesisParamsHelper::convertQuantity(itsParsets[0].getString("uvwMachineDirTolerance", "1e-6rad"), "rad");

    ASKAPLOG_INFO_STR(logger,
                      "UVWMachine cache will store " << uvwMachineCacheSize << " machines");
    ASKAPLOG_INFO_STR(logger, "Tolerance on the directions is "
                      << uvwMachineCacheTolerance / casa::C::pi * 180. * 3600. << " arcsec");
    // we need to loop over the channels
    int workUnitCount = 0; // the workUnits include different epochs (for the same channel)
    // the order is strictly by channel - with multiple work units per channel.
    // so you can increment the workUnit until the frequency changes - then you know you
    // have all the workunits



    for (int chan=0; chan < nchanpercore; ++chan) { // not all of these will have work

        if (workUnitCount >= workUnits.size()) {
            ASKAPLOG_INFO_STR(logger, "Out of work with workUnit " << workUnitCount);
            break;
        }
        ASKAPLOG_INFO_STR(logger, "Starting to process channel " << chan \
        << " with workUnit " << workUnitCount << " used for the root ");

        int initialChannelWorkUnit = workUnitCount+1;

        double frequency=workUnits[workUnitCount].get_channelFrequency();
        const string colName = itsParsets[workUnitCount].getString("datacolumn", "DATA");
        const string ms = workUnits[workUnitCount].get_dataset();
        TableDataSource ds(ms, TableDataSource::DEFAULT, colName);

        /// Need to set up the rootImager here
        CalcCore rootImager(itsParsets[workUnitCount],itsComms,ds,workUnits[workUnitCount].get_localChannel());

        /// set up the image for this channel
        setupImage(rootImager.params(), frequency);



        /// need to put in the major and minor cycle loops
        /// If we are doing more than one major cycle I need to reset
        /// the workUnit count to permit a re-read of the input data.
        /// LOOP:

        for (int majorCycleNumber=0; majorCycleNumber < nCycles; ++majorCycleNumber) {

            workUnitCount = initialChannelWorkUnit;

            /// But first lets test to see how we are doing.
            /// calcNE for the rootImager

            rootImager.calcNE();

            while (workUnitCount < workUnits.size() && frequency == workUnits[workUnitCount].get_channelFrequency()) {
                /// need a working imager to allow a merge over epochs for this channel


                const string myMs = workUnits[workUnitCount].get_dataset();

                TableDataSource ds(myMs, TableDataSource::DEFAULT, colName);

                ds.configureUVWMachineCache(uvwMachineCacheSize, uvwMachineCacheTolerance);

                CalcCore workingImager(itsParsets[workUnitCount],itsComms,ds,workUnits[workUnitCount].get_localChannel());

            /// this loop does the calcNE and the merge
                workingImager.replaceModel(rootImager.params());
                workingImager.calcNE();
                rootImager.getNE()->merge(*workingImager.getNE());

                workUnitCount++;
            }
            /// now we have a "full" set of NE we can SolveNE to update the model
            rootImager.solveNE();

            if (rootImager.params()->has("peak_residual")) {
                const double peak_residual = rootImager.params()->scalarValue("peak_residual");
                ASKAPLOG_INFO_STR(logger, "Reached peak residual of " << peak_residual);
                if (peak_residual < targetPeakResidual) {
                    ASKAPLOG_INFO_STR(logger, "It is below the major cycle threshold of "
                                      << targetPeakResidual << " Jy. Stopping.");
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
            if (majorCycleNumber+1 == nCycles) {
                ASKAPLOG_INFO_STR(logger,"Reached maximum majorcycle count");

            }
            else {

                /// But we dont want to keep merging into the same NE
                /// so lets reset
                ASKAPLOG_INFO_STR(logger,"Reset normal equations");
                rootImager.getNE()->reset();
                // the model is now updated but the NE are empty ... - lets go again
            }
            if (writeAtMajorCycle) {
                ASKAPLOG_WARN_STR(logger,"Write at major cycle not currently supported in this mode");
            }

        }
        ASKAPLOG_INFO_STR(logger,"Adding model.slice");
        ASKAPCHECK(rootImager.params()->has("image.slice"), "Params are missing image.slice parameter");
        rootImager.params()->add("model.slice", rootImager.params()->value("image.slice"));
        ASKAPCHECK(rootImager.params()->has("model.slice"), "Params are missing model.slice parameter");

        rootImager.check();


        if (itsParsets[0].getBool("restore", false)) {
            ASKAPLOG_INFO_STR(logger,"Running restore");
            rootImager.restoreImage();
        }
        ASKAPLOG_INFO_STR(logger,"writing channel into cube");

        handleImageParams(rootImager.params(), workUnits[workUnitCount-1].get_localChannel());
        /// outside the clean-loop write out the slice


    }
    /// outside the channel loop    donvert the image to FIT

    return;
}
void ContinuumWorker::handleImageParams(askap::scimath::Params::ShPtr params,
        unsigned int chan)
{


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

}


void ContinuumWorker::recordBeam(const askap::scimath::Axes &axes,
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


void ContinuumWorker::storeBeam(const unsigned int globalChannel)
{
    if (globalChannel == itsBeamReferenceChannel) {
        itsRestoredCube->addBeam(itsBeamList[globalChannel]);
    }
}

void ContinuumWorker::logBeamInfo()
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

void ContinuumWorker::processChannels()
{



    LOFAR::ParameterSet& unitParset = itsParsets[0];
    int localChannel;
    int globalChannel;

    const string colName = unitParset.getString("datacolumn", "DATA");
    const string ms = workUnits[0].get_dataset();
    const bool writeAtMajorCycle = unitParset.getBool("Images.writeAtMajorCycle",false);

    std::string majorcycle = unitParset.getString("threshold.majorcycle", "-1Jy");
    const double targetPeakResidual = SynthesisParamsHelper::convertQuantity(majorcycle, "Jy");

    const int nCycles = unitParset.getInt32("ncycles", 0);

    const int uvwMachineCacheSize = unitParset.getInt32("nUVWMachines", 1);
    ASKAPCHECK(uvwMachineCacheSize > 0 ,
               "Cache size is supposed to be a positive number, you have "
               << uvwMachineCacheSize);

    const double uvwMachineCacheTolerance = SynthesisParamsHelper::convertQuantity(unitParset.getString("uvwMachineDirTolerance", "1e-6rad"), "rad");

    ASKAPLOG_INFO_STR(logger,
                      "UVWMachine cache will store " << uvwMachineCacheSize << " machines");
    ASKAPLOG_INFO_STR(logger, "Tolerance on the directions is "
                      << uvwMachineCacheTolerance / casa::C::pi * 180. * 3600. << " arcsec");

    const bool localSolver = unitParset.getBool("solverpercore",false);

    if (localSolver) {
        this->buildSpectralCube();
        return;
    }
    ASKAPLOG_INFO_STR(logger,"Processing multiple channels central solver mode");
    TableDataSource ds0(ms, TableDataSource::DEFAULT, colName);

    ds0.configureUVWMachineCache(uvwMachineCacheSize, uvwMachineCacheTolerance);



    bool usetmpfs = unitParset.getBool("usetmpfs",false);
    if (usetmpfs) {
        // probably in spectral line mode
        localChannel = 0;

    }
    else {
        localChannel = workUnits[0].get_localChannel();
    }
    globalChannel = workUnits[0].get_globalChannel();

    CalcCore rootImager(itsParsets[0],itsComms,ds0,localChannel);


    if (nCycles == 0) {


        rootImager.receiveModel();
        rootImager.calcNE();

        for (size_t i = 1; i < workUnits.size(); ++i) {

            const string myMs = workUnits[i].get_dataset();

            TableDataSource ds(myMs, TableDataSource::DEFAULT, colName);

            ds.configureUVWMachineCache(uvwMachineCacheSize, uvwMachineCacheTolerance);

            if (usetmpfs) {
                localChannel = 0;
            }
            else {
                localChannel = workUnits[i].get_localChannel();
            }


            CalcCore workingImager(itsParsets[i],itsComms,ds,localChannel);

            workingImager.replaceModel(rootImager.params());


            workingImager.calcNE();

            if (i>0) {
                ASKAPLOG_INFO_STR(logger,"Merging " << i << " of " << workUnits.size()-1 << " into NE");
                rootImager.getNE()->merge(*workingImager.getNE());

                ASKAPLOG_INFO_STR(logger,"Merged");
            }


        }

        ASKAPLOG_INFO_STR(logger,"Sending NE to master for single cycle ");

        rootImager.sendNE();
        rootImager.getNE()->reset();

        ASKAPLOG_INFO_STR(logger,"Sent");




    }
    else {


        for (size_t n = 0; n <= nCycles; n++) {
            if (writeAtMajorCycle) {
                ASKAPLOG_WARN_STR(logger,"Write at major cycle not currently supported in this mode");
            }

            ASKAPLOG_INFO_STR(logger,"Worker waiting to receive new model");
            rootImager.receiveModel();
            ASKAPLOG_INFO_STR(logger, "Worker received model for cycle  " << n);

            if (rootImager.params()->has("peak_residual")) {
                const double peak_residual = rootImager.params()->scalarValue("peak_residual");
                ASKAPLOG_INFO_STR(logger, "Reached peak residual of " << peak_residual);
                if (peak_residual < targetPeakResidual) {
                    ASKAPLOG_INFO_STR(logger, "It is below the major cycle threshold of "
                                      << targetPeakResidual << " Jy. Stopping.");
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

            ASKAPLOG_INFO_STR(logger, "Worker calculating NE");
            rootImager.calcNE();

            std::vector<std::string> names = rootImager.getNE()->unknowns();

            ASKAPLOG_INFO_STR(logger,"Unknowns are " << names);

            rootImager.check();

            for (size_t i = 1; i < workUnits.size(); ++i) {

                const string myMs = workUnits[i].get_dataset();

                TableDataSource ds(myMs, TableDataSource::DEFAULT, colName);

                ds.configureUVWMachineCache(uvwMachineCacheSize, uvwMachineCacheTolerance);

                if (usetmpfs) {
                    localChannel = 0;
                }
                else {
                    localChannel = workUnits[i].get_localChannel();
                }


                CalcCore workingImager(itsParsets[i],itsComms,ds,localChannel);

                workingImager.replaceModel(rootImager.params());
                ASKAPLOG_INFO_STR(logger, "workingImager Model" << workingImager.params());
                ASKAPLOG_INFO_STR(logger, "rootImager Model" << rootImager.params());
                workingImager.calcNE();

                if (i>0) {
                    ASKAPLOG_INFO_STR(logger,"Merging " << i << " of " << workUnits.size()-1 << " into NE");
                    rootImager.getNE()->merge(*workingImager.getNE());

                    ASKAPLOG_INFO_STR(logger,"Merged");
                }


            }

            ASKAPLOG_INFO_STR(logger,"Worker sending NE to master for cycle " << n);
            rootImager.sendNE();

        }


    }

}


void ContinuumWorker::setupImage(const askap::scimath::Params::ShPtr& params,
                                    double channelFrequency)
{
    try {
        ASKAPLOG_INFO_STR(logger,"Setting up image");
        const LOFAR::ParameterSet parset = itsParset.makeSubset("Images.");

        const int nfacets = parset.getInt32("nfacets", 1);
        const string name("image.slice");
        const vector<string> direction = parset.getStringVector("direction");
        const vector<string> cellsize = parset.getStringVector("cellsize");
        const vector<int> shape = parset.getInt32Vector("shape");
        //const vector<double> freq = parset.getDoubleVector("frequency");
        const int nchan = 1;

        if (!parset.isDefined("polarisation")) {
            ASKAPLOG_INFO_STR(logger, "Polarisation frame is not defined, "
                              << "only stokes I will be generated");
        }
        const vector<string> stokesVec = parset.getStringVector("polarisation",
                                         vector<string>(1, "I"));

        // there could be many ways to define stokes, e.g. ["XX YY"] or ["XX","YY"] or "XX,YY"
        // to allow some flexibility we have to concatenate all elements first and then
        // allow the parser from PolConverter to take care of extracting the products.
        string stokesStr;
        for (size_t i = 0; i < stokesVec.size(); ++i) {
            stokesStr += stokesVec[i];
        }
        const casa::Vector<casa::Stokes::StokesTypes>
        stokes = scimath::PolConverter::fromString(stokesStr);

        const bool ewProj = parset.getBool("ewprojection", false);
        if (ewProj) {
            ASKAPLOG_INFO_STR(logger, "Image will have SCP/NCP projection");
        } else {
            ASKAPLOG_INFO_STR(logger, "Image will have plain SIN projection");
        }

        ASKAPCHECK(nfacets > 0,
                   "Number of facets is supposed to be a positive number, you gave " << nfacets);
        ASKAPCHECK(shape.size() >= 2,
                   "Image is supposed to be at least two dimensional. " <<
                   "check shape parameter, you gave " << shape);

        if (nfacets == 1) {
            SynthesisParamsHelper::add(*params, name, direction, cellsize, shape, ewProj,
                                       channelFrequency, channelFrequency, nchan, stokes);
            // SynthesisParamsHelper::add(*params, name, direction, cellsize, shape, ewProj,
            //                            freq[0], freq[1], nchan, stokes);
        } else {
            // this is a multi-facet case
            const int facetstep = parset.getInt32("facetstep", casa::min(shape[0], shape[1]));
            ASKAPCHECK(facetstep > 0,
                       "facetstep parameter is supposed to be positive, you have " << facetstep);
            ASKAPLOG_INFO_STR(logger, "Facet centers will be " << facetstep <<
                              " pixels apart, each facet size will be "
                              << shape[0] << " x " << shape[1]);
            // SynthesisParamsHelper::add(*params, name, direction, cellsize, shape, ewProj,
            //                            freq[0], freq[1], nchan, stokes, nfacets, facetstep);
            SynthesisParamsHelper::add(*params, name, direction, cellsize, shape, ewProj,
                                       channelFrequency, channelFrequency,
                                       nchan, stokes, nfacets, facetstep);
        }


    } catch (const LOFAR::APSException &ex) {
        throw AskapError(ex.what());
    }
}
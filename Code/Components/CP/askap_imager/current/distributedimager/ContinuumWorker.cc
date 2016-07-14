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

// CASA Includes

// Local includes
#include "distributedimager/AdviseDI.h"
#include "distributedimager/IBasicComms.h"
#include "distributedimager/SolverCore.h"
#include "distributedimager/CalcCore.h"
#include "distributedimager/Tracing.h"
#include "distributedimager/MSSplitter.h"
#include "messages/ContinuumWorkUnit.h"
#include "messages/ContinuumWorkRequest.h"

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
    const int nchanpercore = itsParset.getInt32("nchanpercore", 1);





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
                              << ", frequency " << wu.get_channelFrequency()/1.e6 << "MHz");

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
        unitParset.replace("dataset",outms);

        sprintf(ChannelPar,"[1,1]");
    }

    unitParset.replace("Channels",ChannelPar);


    ASKAPLOG_INFO_STR(logger,"getting advice on missing parametrs");
    synthesis::AdviseDI diadvise(itsComms,unitParset);
    diadvise.addMissingParameters();
    ASKAPLOG_INFO_STR(logger,"advice received");


    // store the datatable accessor - this is not storing the data
    // i am still working on that
    // Now trying to store the parset. the ds. and an imager.
    ASKAPLOG_INFO_STR(logger,"storing workUnit <probably> no longer required");
    workUnits.push_back(wu); // this isn't needed anymore... i think all the info
    // is now in the parset ....
    ASKAPLOG_INFO_STR(logger,"storing parset"); 
    itsParsets.push_back(diadvise.getParset());

}


void
ContinuumWorker::processSnapshot(LOFAR::ParameterSet& unitParset)
{
}

void ContinuumWorker::processChannels()
{



    LOFAR::ParameterSet& unitParset = itsParsets[0];
    int localChannel;

    const string colName = unitParset.getString("datacolumn", "DATA");
    const string ms = workUnits[0].get_dataset();

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

    TableDataSource ds0(ms, TableDataSource::DEFAULT, colName);

    ds0.configureUVWMachineCache(uvwMachineCacheSize, uvwMachineCacheTolerance);


    bool usetmpfs = unitParset.getBool("usetmpfs",false);
    if (usetmpfs) {
        localChannel = 0;

    }
    else {
        localChannel = workUnits[0].get_localChannel();
    }



    if (nCycles == 0) {

        CalcCore rootImager(itsParsets[0],itsComms,ds0,localChannel);
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
        CalcCore rootImager(itsParsets[0],itsComms,ds0,localChannel);

        for (size_t n = 0; n <= nCycles; n++) {


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



    for (size_t i = 0; i < workUnits.size(); ++i) { // wrap up
        // this needs full path
        const string myMs = workUnits[i].get_dataset();
        struct stat buffer;
        if (stat (myMs.c_str(), &buffer) == 0) {
            unlink(myMs.c_str());
        }
    }

}


void ContinuumWorker::setupImage(const askap::scimath::Params::ShPtr& params,
                                    double channelFrequency)
{
    try {
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

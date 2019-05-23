/// @file SpectralLineWorker.cc
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
#include "SpectralLineWorker.h"

// System includes
#include <string>
#include <sstream>
#include <stdexcept>
#include <vector>

// ASKAPsoft includes
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>
#include <askap/scimath/fitting/Equation.h>
#include <askap/scimath/fitting/INormalEquations.h>
#include <askap/scimath/fitting/ImagingNormalEquations.h>
#include <askap/scimath/fitting/Params.h>
#include <askap/gridding/IVisGridder.h>
#include <askap/gridding/VisGridderFactory.h>
#include <askap/measurementequation/SynthesisParamsHelper.h>
#include <askap/measurementequation/ImageFFTEquation.h>
#include <askap/measurementequation/SynthesisParamsHelper.h>
#include <askap/dataaccess/IConstDataSource.h>
#include <askap/dataaccess/TableConstDataSource.h>
#include <askap/dataaccess/IConstDataIterator.h>
#include <askap/dataaccess/IDataConverter.h>
#include <askap/dataaccess/IDataSelector.h>
#include <askap/dataaccess/IDataIterator.h>
#include <askap/dataaccess/SharedIter.h>
#include <askap/dataaccess/ParsetInterface.h>
#include <askap/scimath/utils/PolConverter.h>
#include <Common/ParameterSet.h>
#include <Common/Exceptions.h>
#include <casacore/casa/OS/Timer.h>
#include "askap/messages/SpectralLineWorkUnit.h"
#include "askap/messages/SpectralLineWorkRequest.h"

// Local includes
//
#include "distributedimager/IBasicComms.h"
#include "distributedimager/SolverCore.h"
#include "distributedimager/Tracing.h"

using namespace std;
using namespace askap::cp;
using namespace askap;
using namespace askap::scimath;
using namespace askap::synthesis;
using namespace askap::accessors;

ASKAP_LOGGER(logger, ".SpectralLineWorker");

SpectralLineWorker::SpectralLineWorker(LOFAR::ParameterSet& parset,
                                       askap::cp::IBasicComms& comms)
    : itsParset(parset), itsComms(comms)
{
    itsGridder_p = VisGridderFactory::make(itsParset);
}

SpectralLineWorker::~SpectralLineWorker()
{
    itsGridder_p.reset();
}

void SpectralLineWorker::run(void)
{
    // Send the initial request for work
    SpectralLineWorkRequest wrequest;
    itsComms.sendMessage(wrequest, itsMaster);

    while (1) {

        // Get a workunit
        SpectralLineWorkUnit wu;
        itsComms.receiveMessage(wu, itsMaster);

        if (wu.get_payloadType() == SpectralLineWorkUnit::DONE) {
            // Indicates all workunits have been assigned already
            ASKAPLOG_DEBUG_STR(logger, "Received DONE signal");
            break;
        }

        const string ms = wu.get_dataset();
        ASKAPLOG_DEBUG_STR(logger, "Received Work Unit for dataset " << ms
                           << ", local channel " << wu.get_localChannel()
                           << ", global channel " << wu.get_globalChannel()
                           << ", frequency " << wu.get_channelFrequency()/1.e6 << "MHz");
        askap::scimath::Params::ShPtr params;
        try {
            params = processWorkUnit(wu);
        } catch (AskapError& e) {
            ASKAPLOG_WARN_STR(logger, "Failure processing channel " << wu.get_globalChannel());
            ASKAPLOG_WARN_STR(logger, "Exception detail: " << e.what());
        } catch (const std::exception& e) {
            ASKAPLOG_WARN_STR(logger, "Failure processing channel " << wu.get_globalChannel());
            ASKAPLOG_WARN_STR(logger, "Exception detail: " << e.what());
        }

        // Send the params to the master, which also implicitly requests
        // more work
        ASKAPLOG_DEBUG_STR(logger, "Sending params back to master for local channel " << wu.get_localChannel()
                           << ", global channel " << wu.get_globalChannel()
                           << ", frequency " << wu.get_channelFrequency()/1.e6 << "MHz");
        wrequest.set_globalChannel(wu.get_globalChannel());
        wrequest.set_params(params);
        itsComms.sendMessage(wrequest, itsMaster);
        wrequest.set_params(askap::scimath::Params::ShPtr()); // Free memory
    }
}

askap::scimath::Params::ShPtr SpectralLineWorker::processWorkUnit(const SpectralLineWorkUnit& wu)
{
    const string colName = itsParset.getString("datacolumn", "DATA");
    const string ms = wu.get_dataset();

    const int uvwMachineCacheSize = itsParset.getInt32("nUVWMachines", 1);
    ASKAPCHECK(uvwMachineCacheSize > 0 ,
               "Cache size is supposed to be a positive number, you have "
               << uvwMachineCacheSize);

    const double uvwMachineCacheTolerance = SynthesisParamsHelper::convertQuantity(
            itsParset.getString("uvwMachineDirTolerance", "1e-6rad"), "rad");

    ASKAPLOG_DEBUG_STR(logger,
                       "UVWMachine cache will store " << uvwMachineCacheSize << " machines");
    ASKAPLOG_DEBUG_STR(logger, "Tolerance on the directions is "
                       << uvwMachineCacheTolerance / casacore::C::pi * 180. * 3600. << " arcsec");

    TableDataSource ds(ms, TableDataSource::DEFAULT, colName);
    ds.configureUVWMachineCache(uvwMachineCacheSize, uvwMachineCacheTolerance);
    IDataSelectorPtr sel = ds.createSelector();
    sel->chooseCrossCorrelations();
    sel << itsParset;
    IDataConverterPtr conv = ds.createConverter();

    conv->setFrequencyFrame(casacore::MFrequency::Ref(casacore::MFrequency::TOPO), "Hz");
    conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
    IDataSharedIter it = ds.createIterator(sel, conv);

    if (!itsParset.isDefined("Images.name")) {
        ASKAPTHROW(std::runtime_error, "Image name is not defined in parameter set");
    }
    const string imagename = itsParset.getString("Images.name");
    if (imagename.at(0) == '[') {
        ASKAPTHROW(std::runtime_error, "Image name specified as a std::vector.");
    }

    const unsigned int localChannel = wu.get_localChannel();
    const unsigned int globalChannel = wu.get_globalChannel();
    double channelFrequency = wu.get_channelFrequency();
    ASKAPCHECK(localChannel < it->nChannel(), "Invalid local channel number");
    ASKAPCHECK(localChannel <= globalChannel, "Local channel > global channel");
    return processChannel(ds, imagename, localChannel, globalChannel,channelFrequency);
}

askap::scimath::Params::ShPtr
SpectralLineWorker::processChannel(askap::accessors::TableDataSource& ds,
                                   const std::string& imagename,
                                   unsigned int localChannel,
                                   unsigned int globalChannel,
                                   double channelFrequency)
{
    askap::scimath::Params::ShPtr model_p(new Params());
    setupImage(model_p,channelFrequency);

    casacore::Timer timer;

    // Setup data iterator
    IDataSelectorPtr sel = ds.createSelector();
    sel->chooseCrossCorrelations();
    sel->chooseChannels(1, localChannel);
    sel << itsParset;
    IDataConverterPtr conv = ds.createConverter();
    conv->setFrequencyFrame(casacore::MFrequency::Ref(casacore::MFrequency::TOPO), "Hz");
    conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));
    IDataSharedIter it = ds.createIterator(sel, conv);

    ASKAPLOG_DEBUG_STR(logger, "Calculating normal equations for channel " << globalChannel);
    ASKAPCHECK(model_p, "model_p is not correctly initialized");
    askap::scimath::INormalEquations::ShPtr ne_p;
    askap::scimath::Equation::ShPtr equation_p;

    // Setup measurement equations
    equation_p = askap::scimath::Equation::ShPtr(new ImageFFTEquation(*model_p, it, itsGridder_p, itsParset));

    std::string majorcycle = itsParset.getString("threshold.majorcycle", "-1Jy");
    const double targetPeakResidual = SynthesisParamsHelper::convertQuantity(majorcycle, "Jy");
    const bool writeAtMajorCycle = itsParset.getBool("Images.writeAtMajorCycle", false);
    const int nCycles = itsParset.getInt32("ncycles", 0);
    SolverCore solverCore(itsParset, itsComms, model_p);
    if (nCycles == 0) {
        // Calc NE
        timer.mark();

        // Setup normal equations
        ne_p = ImagingNormalEquations::ShPtr(new ImagingNormalEquations(*model_p));
        ASKAPCHECK(ne_p, "ne_p is not correctly initialized");

        ASKAPCHECK(equation_p, "equation_p is not correctly initialized");
        Tracing::entry(Tracing::CalcNE);
        equation_p->calcEquations(*ne_p);
        Tracing::exit(Tracing::CalcNE);

        ASKAPLOG_DEBUG_STR(logger, "Calculated normal equations for channel "
                           << globalChannel << " in "
                           << timer.real() << " seconds ");

        // Solve NE
        solverCore.solveNE(ne_p);
    } else {
        for (int cycle = 0; cycle < nCycles; ++cycle) {
            ASKAPLOG_INFO_STR(logger, "*** Starting major cycle " << cycle << " ***");

            // Calc NE
            timer.mark();

            if (cycle > 0) {
                equation_p->setParameters(*model_p);
            }

            // Setup normal equations
            ne_p = ImagingNormalEquations::ShPtr(new ImagingNormalEquations(*model_p));
            ASKAPCHECK(ne_p, "ne_p is not correctly initialized");


            ASKAPCHECK(equation_p, "equation_p is not correctly initialized");
            Tracing::entry(Tracing::CalcNE);
            equation_p->calcEquations(*ne_p);
            Tracing::exit(Tracing::CalcNE);

            ASKAPLOG_DEBUG_STR(logger, "Calculated normal equations for channel "
                               << globalChannel << " in "
                               << timer.real() << " seconds ");

            // Solve NE
            solverCore.solveNE(ne_p);

            if (model_p->has("peak_residual")) {
                const double peak_residual = model_p->scalarValue("peak_residual");
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
            if (cycle + 1 >= nCycles) {
                ASKAPLOG_INFO_STR(logger, "Reached " << nCycles
                                  << " cycle(s), the maximum number of major cycles. Stopping.");
            }

            if (writeAtMajorCycle) {
                stringstream ss;
                ss << ".ch." << globalChannel << ".majorcycle." << cycle + 1;
                solverCore.writeModel(ss.str());
            }

        }
        ASKAPLOG_INFO_STR(logger, "*** Finished major cycles ***");
        ne_p = ImagingNormalEquations::ShPtr(new ImagingNormalEquations(*model_p));
        equation_p->setParameters(*model_p);
        Tracing::entry(Tracing::CalcNE);
        equation_p->calcEquations(*ne_p);
        Tracing::exit(Tracing::CalcNE);
        solverCore.addNE(ne_p);
    } // end cycling block


    model_p->add("model.slice", model_p->value("image.slice"));
    if (itsParset.getBool("restore", false)) {
        solverCore.restoreImage();
    }

    return model_p;
}

void SpectralLineWorker::setupImage(const askap::scimath::Params::ShPtr& params,
                                    double channelFrequency)
{
    try {
        const LOFAR::ParameterSet parset = itsParset.makeSubset("Images.");

        const int nfacets = parset.getInt32("nfacets", 1);
        const string name("image.slice");
        const std::vector<std::string> direction = parset.getStringVector("direction");
        const std::vector<std::string> cellsize = parset.getStringVector("cellsize");
        const std::vector<int> shape = parset.getInt32Vector("shape");
        //const std::vector<double> freq = parset.getDoubleVector("frequency");
        const int nchan = 1;

        const std::vector<std::string>
            stokesVec = parset.getStringVector("polarisation", std::vector<std::string>(1,"I"));

        // there could be many ways to define stokes, e.g. ["XX YY"] or ["XX","YY"] or "XX,YY"
        // to allow some flexibility we have to concatenate all elements first and then
        // allow the parser from PolConverter to take care of extracting the products.
        string stokesStr;
        for (size_t i = 0; i < stokesVec.size(); ++i) {
            stokesStr += stokesVec[i];
        }
        const casacore::Vector<casacore::Stokes::StokesTypes>
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
            const int facetstep = parset.getInt32("facetstep", casacore::min(shape[0], shape[1]));
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

/// @file SolverCore.cc
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
#include "CalcCore.h"

// System includes
#include <string>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <Common/ParameterSet.h>
#include <fitting/INormalEquations.h>
#include <fitting/ImagingNormalEquations.h>
#include <fitting/Params.h>
#include <fitting/Solver.h>
#include <fitting/Quality.h>
#include <measurementequation/ImageSolverFactory.h>
#include <measurementequation/SynthesisParamsHelper.h>
#include <measurementequation/ImageRestoreSolver.h>
#include <measurementequation/IImagePreconditioner.h>
#include <measurementequation/WienerPreconditioner.h>
#include <measurementequation/GaussianTaperPreconditioner.h>
#include <measurementequation/ImageMultiScaleSolver.h>
#include <measurementequation/ImageParamsHelper.h>
#include <measurementequation/CalibrationApplicatorME.h>
#include <measurementequation/CalibrationIterator.h>
#include <calibaccess/CalibAccessFactory.h>
#include <casacore/casa/OS/Timer.h>
#include <dataaccess/TableDataSource.h>
#include <dataaccess/ParsetInterface.h>
#include <measurementequation/ImageFFTEquation.h>
#include <parallel/GroupVisAggregator.h>
#include <utils/MultiDimArrayPlaneIter.h>

// Local includes
#include "distributedimager/IBasicComms.h"
#include "distributedimager/Tracing.h"

// Using
using namespace askap;
using namespace askap::accessors;
using namespace askap::cp;
using namespace askap::scimath;
using namespace askap::synthesis;

ASKAP_LOGGER(logger, ".CalcCore");

CalcCore::CalcCore(LOFAR::ParameterSet& parset,
                       askap::askapparallel::AskapParallel& comms,
                       accessors::TableDataSource ds, int localChannel)
    : ImagerParallel(comms,parset), itsParset(parset), itsComms(comms),itsData(ds),itsChannel(localChannel)
{
    /// We need to set the calibration info here
    /// the ImagerParallel constructor will do the work to
    /// obtain the itsSolutionSource - but that is a provate member of
    /// the parent class.
    /// Not sure whether to use it directly or copy it.

}

CalcCore::~CalcCore()
{

}
void CalcCore::doCalc()
{

    casa::Timer timer;
    timer.mark();

    ASKAPLOG_INFO_STR(logger, "Calculating NE .... for channel " << itsChannel);
    if (!itsEquation) {

        accessors::TableDataSource ds = itsData;

        // Setup data iterator

        IDataSelectorPtr sel = ds.createSelector();

        sel->chooseCrossCorrelations();
        sel << parset();
        sel->chooseChannels(1, itsChannel);

        IDataConverterPtr conv = ds.createConverter();
        conv->setFrequencyFrame(casa::MFrequency::Ref(casa::MFrequency::TOPO), "Hz");
        conv->setDirectionFrame(casa::MDirection::Ref(casa::MDirection::J2000));
        conv->setEpochFrame();

        IDataSharedIter it = ds.createIterator(sel, conv);


        ASKAPCHECK(itsModel, "Model not defined");
        ASKAPCHECK(gridder(), "Gridder not defined");
        // calibration can go below if required

        if (!getSolutionSource()) {
            ASKAPLOG_INFO_STR(logger,"Not applying calibration");
            ASKAPLOG_INFO_STR(logger, "building FFT/measurement equation" );
            boost::shared_ptr<ImageFFTEquation> fftEquation(new ImageFFTEquation (*itsModel, it, gridder()));
            ASKAPDEBUGASSERT(fftEquation);
            fftEquation->useAlternativePSF(parset());
            fftEquation->setVisUpdateObject(GroupVisAggregator::create(itsComms));
            itsEquation = fftEquation;
        } else {
            ASKAPLOG_INFO_STR(logger, "Calibration will be performed using solution source");
            boost::shared_ptr<ICalibrationApplicator> calME(new CalibrationApplicatorME(getSolutionSource()));
            // fine tune parameters
            ASKAPDEBUGASSERT(calME);
            calME->scaleNoise(parset().getBool("calibrate.scalenoise",false));
            calME->allowFlag(parset().getBool("calibrate.allowflag",false));
            calME->beamIndependent(parset().getBool("calibrate.ignorebeam", false));
            //
            IDataSharedIter calIter(new CalibrationIterator(it,calME));
            boost::shared_ptr<ImageFFTEquation> fftEquation(
                                                            new ImageFFTEquation (*itsModel, calIter, gridder()));
            ASKAPDEBUGASSERT(fftEquation);
            fftEquation->useAlternativePSF(parset());
            fftEquation->setVisUpdateObject(GroupVisAggregator::create(itsComms));
            itsEquation = fftEquation;
        }


    }
    else {
        ASKAPLOG_INFO_STR(logger, "Reusing measurement equation and updating with latest model images" );
        itsEquation->setParameters(*itsModel);
    }
    ASKAPCHECK(itsEquation, "Equation not defined");
    ASKAPCHECK(itsNe, "NormalEquations not defined");
    itsEquation->calcEquations(*itsNe);

    ASKAPLOG_INFO_STR(logger,"Calculated normal equations in "<< timer.real()
                      << " seconds ");

}

void CalcCore::calcNE()
{


    reset();
    /// Now we need to recreate the normal equations
    if (!itsNe)
        itsNe=ImagingNormalEquations::ShPtr(new ImagingNormalEquations(*itsModel));

    ASKAPCHECK(gridder(), "Gridder not defined");
    ASKAPCHECK(itsModel, "Model not defined");
    ASKAPCHECK(itsNe, "NormalEquations not defined");


    doCalc();




}

void CalcCore::reset()
{
    itsNe->reset();
}

void CalcCore::check()
{
    std::vector<std::string> names = itsNe->unknowns();
    const ImagingNormalEquations &checkRef =
    dynamic_cast<const ImagingNormalEquations&>(*itsNe);

    casa::Vector<double> diag(checkRef.normalMatrixDiagonal(names[0]));
    casa::Vector<double> dv = checkRef.dataVector(names[0]);
    casa::Vector<double> slice(checkRef.normalMatrixSlice(names[0]));
    casa::Vector<double> pcf(checkRef.preconditionerSlice(names[0]));

    ASKAPLOG_INFO_STR(logger, "Max data: " << max(dv) << " Max PSF: " << max(slice) << " Normalised: " << max(dv)/max(slice));

}

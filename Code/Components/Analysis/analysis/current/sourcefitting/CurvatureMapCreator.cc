/// @file
///
/// Defining the curvature map for use with Selavy
///
/// @copyright (c) 2018 CSIRO
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#include <sourcefitting/CurvatureMapCreator.h>
#include <askap_analysis.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

#include <askap/askapparallel/AskapParallel.h>
#include <Common/ParameterSet.h>
#include <string>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <parallelanalysis/Weighter.h>
#include <outputs/DistributedImageWriter.h>
#include <casainterface/CasaInterface.h>
#include <analysisparallel/SubimageDef.h>
#include <casacore/scimath/Mathematics/Convolver.h>
#include <duchamp/Cubes/cubes.hh>

#include <casacore/images/Images/PagedImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/images/Images/ImageOpener.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/MIRIADImage.h>
#include <casainterface/CasaInterface.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".curvaturemap");

namespace askap {

namespace analysis {


CurvatureMapCreator::CurvatureMapCreator(askap::askapparallel::AskapParallel &comms,
        const LOFAR::ParameterSet &parset):
    itsComms(&comms), itsParset(parset)
{
    itsFilename = parset.getString("curvatureImage", "");
    ASKAPLOG_DEBUG_STR(logger, "Define a CurvatureMapCreator to write to image " <<
                       itsFilename);
}

void CurvatureMapCreator::initialise(duchamp::Cube &cube,
                                     analysisutilities::SubimageDef &subdef,
                                     boost::shared_ptr<Weighter> weighter)
{

    itsCube = &cube;
    itsSubimageDef = &subdef;
    itsWeighter = weighter;

    casacore::Slicer slicer = analysisutilities::subsectionToSlicer(cube.pars().section());
    analysisutilities::fixSlicer(slicer, cube.header().getWCS());
    const boost::shared_ptr<SubImage<Float> > sub =
        analysisutilities::getSubImage(cube.pars().getImageFile(), slicer);
    itsShape = sub->shape();

    duchamp::Section sec = itsSubimageDef->section(itsComms->rank() - 1);
    sec.parse(itsShape.asStdVector());
    duchamp::Section secMaster = itsSubimageDef->section(-1);
    secMaster.parse(itsShape.asStdVector());
    itsLocation = casacore::IPosition(sec.getStartList());

    ASKAPLOG_DEBUG_STR(logger, "Initialised CurvatureMapCreator with shape=" <<
                       itsShape << " and location=" << itsLocation);

}

void CurvatureMapCreator::calculate()
{

    casacore::Array<float> inputArray(itsShape, itsCube->getArray(), casacore::SHARE);

    casacore::IPosition kernelShape(2, 3, 3);
    casacore::Array<float> kernel(kernelShape, 1.);
    kernel(casacore::IPosition(2, 1, 1)) = -8.;

    ASKAPLOG_DEBUG_STR(logger, "Defined a kernel for the curvature map calculations: " << kernel);

    casacore::Convolver<float> convolver(kernel, itsShape);
    ASKAPLOG_DEBUG_STR(logger, "Defined a convolver");

    itsArray = casacore::MaskedArray<float>(inputArray, itsWeighter->cutoffMask());
    ASKAPLOG_DEBUG_STR(logger, "About to convolve");
    convolver.linearConv(itsArray.getRWArray(), inputArray);
    ASKAPLOG_DEBUG_STR(logger, "Convolving done.");

    this->findSigma();

    this->maskBorders();

}


void CurvatureMapCreator::findSigma()
{

    itsSigmaCurv = madfm(itsArray, False) / Statistics::correctionFactor;
    ASKAPLOG_DEBUG_STR(logger, "Found sigma_curv = " << itsSigmaCurv);

}

void CurvatureMapCreator::maskBorders()
{
    int nsubx = itsSubimageDef->nsubx();
    int nsuby = itsSubimageDef->nsuby();
    int overlapx = itsSubimageDef->overlapx() / 2;
    int overlapy = itsSubimageDef->overlapy() / 2;
    int rank = itsComms->rank() - 1;
    int xminOffset = (rank % nsubx == 0) ? 0 : overlapx;
    int xmaxOffset = (rank % nsubx == (nsubx - 1)) ? 0 : overlapx;
    int yminOffset = (rank / nsubx == 0) ? 0 : overlapy;
    int ymaxOffset = (rank / nsubx == (nsuby - 1)) ? 0 : overlapy;
    ASKAPLOG_DEBUG_STR(logger, "xminOffset=" << xminOffset <<
                       ", xmaxOffset=" << xmaxOffset <<
                       ", yminOffset=" << yminOffset <<
                       ", ymaxOffset=" << ymaxOffset);
    ASKAPLOG_DEBUG_STR(logger, "Starting with location=" << itsLocation <<
                       " and shape=" << itsShape);
    casacore::IPosition blc(itsLocation), trc(itsShape - 1);
    blc[0] = xminOffset;
    blc[1] = yminOffset;
    trc[0] -= xmaxOffset;
    trc[1] -= ymaxOffset;
    casacore::Slicer arrSlicer(blc, trc, Slicer::endIsLast);
    ASKAPLOG_DEBUG_STR(logger, "Defined a masking Slicer " << arrSlicer);
    casacore::Array<float> newArr = itsArray.getRWArray()(arrSlicer);
    ASKAPLOG_DEBUG_STR(logger, "Have extracted a subarray of shape " << newArr.shape());
    itsArray.getRWArray().assign(newArr);
    itsLocation += blc;
    itsShape = trc - blc + 1;
    ASKAPLOG_DEBUG_STR(logger, "Now have location=" << itsLocation <<
                       " and shape=" << itsShape);
}


void CurvatureMapCreator::write()
{
    if (itsFilename != "") {
        ASKAPLOG_DEBUG_STR(logger, "In CurvatureMapCreator::write()");

        DistributedImageWriter writer(*itsComms, itsParset, itsCube, itsFilename);
        ASKAPLOG_DEBUG_STR(logger, "Creating the output image " << itsFilename);
        writer.create();
        ASKAPLOG_DEBUG_STR(logger, "Writing curvature map of shape " <<
                           itsArray.shape() << " to " << itsFilename);
        writer.write(itsArray, itsLocation, true);
        ASKAPLOG_DEBUG_STR(logger, "Curvature image written");
    }

}


}

}

/// @file
///
/// Class to handle extraction of a summed spectrum corresponding to a source.
///
/// @copyright (c) 2011 CSIRO
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
#include <extraction/NoiseSpectrumExtractor.h>
#include <askap_analysis.h>
#include <extraction/SourceDataExtractor.h>
#include <extraction/SpectralBoxExtractor.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>

#include <imageaccess/CasaImageAccess.h>

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/casa/Arrays/ArrayPartMath.h>
#include <casacore/images/Images/ImageInterface.h>
#include <casacore/images/Images/ImageOpener.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/MIRIADImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/measures/Measures/Stokes.h>

#include <Common/ParameterSet.h>

#include <duchamp/Utils/Statistics.hh>

using namespace askap::analysis::sourcefitting;

ASKAP_LOGGER(logger, ".noiseSpectrumExtractor");

namespace askap {

namespace analysis {

NoiseSpectrumExtractor::NoiseSpectrumExtractor(const LOFAR::ParameterSet& parset):
    SpectralBoxExtractor(parset)
{

    itsAreaInBeams = parset.getFloat("noiseArea", 50);
    itsRobustFlag = parset.getBool("robust", true);

    casa::Stokes stk;
    itsCurrentStokes = itsStokesList[0];
    itsInputCube = itsCubeStokesMap[itsCurrentStokes];
    if (itsStokesList.size() > 1) {
        ASKAPLOG_WARN_STR(logger, "Noise Extractor: " <<
                          "Will only use the first provided Stokes parameter: " <<
                          stk.name(itsCurrentStokes));
        itsStokesList = casa::Vector<casa::Stokes::StokesTypes>(1, itsCurrentStokes);
        itsCubeStokesMap.clear();
        itsCubeStokesMap[itsCurrentStokes] = itsInputCube;
    }

    this->initialiseArray();
    this->setBoxWidth();

}

void NoiseSpectrumExtractor::setBoxWidth()
{

    if (this->openInput()) {
        Vector<Quantum<Double> >
        inputBeam = itsInputCubePtr->imageInfo().restoringBeam().toVector();
        ASKAPLOG_DEBUG_STR(logger, "Beam for input cube = " << inputBeam);
        if (inputBeam.size() == 0) {
            ASKAPLOG_WARN_STR(logger, "Input image \"" << itsInputCube <<
                              "\" has no beam information. " <<
                              "Using box width value from parset of " <<
                              itsBoxWidth << "pix");
        } else {

            int dirCoNum = itsInputCoords.findCoordinate(casa::Coordinate::DIRECTION);
            casa::DirectionCoordinate dirCoo = itsInputCoords.directionCoordinate(dirCoNum);
            double fwhmMajPix = inputBeam[0].getValue(dirCoo.worldAxisUnits()[0]) /
                                fabs(dirCoo.increment()[0]);
            double fwhmMinPix = inputBeam[1].getValue(dirCoo.worldAxisUnits()[1]) /
                                fabs(dirCoo.increment()[1]);
            double beamAreaInPix = M_PI * fwhmMajPix * fwhmMinPix;

            itsBoxWidth = int(ceil(sqrt(itsAreaInBeams * beamAreaInPix)));

            ASKAPLOG_INFO_STR(logger, "Noise Extractor: Using box of area " <<
                              itsAreaInBeams << " beams (each of area " <<
                              beamAreaInPix << " pix), or a square of " <<
                              itsBoxWidth << " pix on the side");

        }

        this->closeInput();
    } else ASKAPLOG_ERROR_STR(logger, "Could not open image");
}


void NoiseSpectrumExtractor::extract()
{

    this->defineSlicer();
    if (this->openInput()) {

        ASKAPLOG_INFO_STR(logger, "Extracting noise spectrum from " << itsInputCube <<
                          " surrounding source ID " << itsSourceID <<
                          " with slicer " << itsSlicer);

        boost::shared_ptr<SubImage<Float> >
        sub(new SubImage<Float>(*itsInputCubePtr, itsSlicer));

        ASKAPASSERT(sub->size() > 0);
        const casa::MaskedArray<Float> msub(sub->get(), sub->getMask());
        casa::Array<Float> subarray(sub->shape());
        subarray = msub;

        ASKAPLOG_DEBUG_STR(logger, "subarray.shape = " << subarray.shape());

        casa::IPosition outBLC(itsArray.ndim(), 0);
        casa::IPosition outTRC(itsArray.shape() - 1);

        casa::Array<Float> noisearray;
        if (itsRobustFlag) {
            noisearray = partialMadfms(subarray, IPosition(2, 0, 1)).
                         reform(itsArray(outBLC, outTRC).shape()) /
                         Statistics::correctionFactor;
        } else {
            noisearray = partialRmss(subarray, IPosition(2, 0, 1)).
                         reform(itsArray(outBLC, outTRC).shape());
        }

        itsArray(outBLC, outTRC) = noisearray;

        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}


}

}

/// @file
///
/// XXX Notes on program XXX
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
#include <extraction/CubeletExtractor.h>
#include <askap_analysis.h>
#include <extraction/SourceDataExtractor.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>

#include <imageaccess/ImageAccessFactory.h>

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/images/Images/ImageInterface.h>
#include <casacore/images/Images/ImageOpener.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/MIRIADImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/coordinates/Coordinates/CoordinateUtil.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/SpectralCoordinate.h>
#include <casacore/coordinates/Coordinates/StokesCoordinate.h>
#include <casacore/measures/Measures/Stokes.h>

#include <Common/ParameterSet.h>

using namespace askap::analysis::sourcefitting;

ASKAP_LOGGER(logger, ".cubeletextractor");

namespace askap {

namespace analysis {

CubeletExtractor::CubeletExtractor(const LOFAR::ParameterSet& parset):
    SourceDataExtractor(parset)
{
    std::vector<unsigned int>
    padsizes = parset.getUintVector("padSize", std::vector<unsigned int>(2, 5));

    if (padsizes.size() > 2) {
        ASKAPLOG_WARN_STR(logger,
                          "Only using the first two elements of the padSize vector");
    }

    itsSpatialPad = padsizes[0];
    if (padsizes.size() > 1) {
        itsSpectralPad = padsizes[1];
    } else {
        itsSpectralPad = padsizes[0];
    }

    itsOutputFilenameBase = parset.getString("cubeletOutputBase", "");
}

void CubeletExtractor::defineSlicer()
{

    if (this->openInput()) {
        IPosition shape = itsInputCubePtr->shape();
        casa::IPosition blc(shape.size(), 0);
        casa::IPosition trc = shape - 1;

        long zero = 0;
        blc(itsLngAxis) = std::max(zero, itsSource->getXmin() - itsSpatialPad + itsSource->getXOffset());
        blc(itsLatAxis) = std::max(zero, itsSource->getYmin() - itsSpatialPad + itsSource->getYOffset());
        if (itsSpcAxis>=0){
            blc(itsSpcAxis) = std::max(zero, itsSource->getZmin() - itsSpectralPad + itsSource->getZOffset());
        }

        trc(itsLngAxis) = std::min(shape(itsLngAxis) - 1,
                                   itsSource->getXmax() + itsSpatialPad + itsSource->getXOffset());
        trc(itsLatAxis) = std::min(shape(itsLatAxis) - 1,
                                   itsSource->getYmax() + itsSpatialPad + itsSource->getYOffset());
        if (itsSpcAxis >= 0){
            trc(itsSpcAxis) = std::min(shape(itsSpcAxis) - 1,
                                       itsSource->getZmax() + itsSpectralPad + itsSource->getZOffset());
        }
        /// @todo Not yet dealing with Stokes axis properly.

        itsSlicer = casa::Slicer(blc, trc, casa::Slicer::endIsLast);
        this->closeInput();
        this->initialiseArray();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

void CubeletExtractor::initialiseArray()
{
    if (this->openInput()) {
        int lngsize = itsSlicer.length()(itsLngAxis);
        int latsize = itsSlicer.length()(itsLatAxis);
        int spcsize = 0;
        if (itsSpcAxis >= 0){
            spcsize = itsSlicer.length()(itsSpcAxis);
        }
        casa::IPosition shape(itsInputCubePtr->shape());
        shape(itsLngAxis) = lngsize;
        shape(itsLatAxis) = latsize;
        if(itsSpcAxis >= 0){
            shape(itsSpcAxis) = spcsize;
        }
        ASKAPLOG_DEBUG_STR(logger,
                           "Cubelet extraction: Initialising array to zero with shape " <<
                           shape);
        itsArray = casa::Array<Float>(shape, 0.0);
        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

void CubeletExtractor::extract()
{
    this->defineSlicer();
    if (this->openInput()) {

        ASKAPLOG_INFO_STR(logger,
                          "Extracting cubelet from " << itsInputCube <<
                          " surrounding source ID " << itsSourceID <<
                          " with slicer " << itsSlicer);

        boost::shared_ptr<SubImage<Float> >
        sub(new SubImage<Float>(*itsInputCubePtr, itsSlicer));

        ASKAPASSERT(sub->size() > 0);
        const casa::MaskedArray<Float> msub(sub->get(), sub->getMask());
        ASKAPASSERT(itsArray.size() == msub.size());
        itsArray = msub;

        sub.reset();

        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

void CubeletExtractor::writeImage()
{
    ASKAPLOG_INFO_STR(logger, "Writing cube cutout to " << itsOutputFilename);

    itsInputCube = itsInputCubeList[0];
    if (this->openInput()) {
        IPosition inshape = itsInputCubePtr->shape();
        casa::CoordinateSystem newcoo;
        if (itsStkAxis>=0){
            newcoo = casa::CoordinateUtil::defaultCoords4D();
        } else {
            newcoo = casa::CoordinateUtil::defaultCoords3D();
        }

        int dirCoNum = itsInputCoords.findCoordinate(casa::Coordinate::DIRECTION);
        casa::DirectionCoordinate dircoo(itsInputCoords.directionCoordinate(dirCoNum));
        newcoo.replaceCoordinate(dircoo, newcoo.findCoordinate(casa::Coordinate::DIRECTION));

        if (itsSpcAxis >= 0){
            int spcCoNum = itsInputCoords.findCoordinate(casa::Coordinate::SPECTRAL);
            casa::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
            newcoo.replaceCoordinate(spcoo, newcoo.findCoordinate(casa::Coordinate::SPECTRAL));
        }

        casa::Vector<Int> stkvec(itsStokesList.size());
        if (itsStkAxis >= 0) {
            for (size_t i = 0; i < stkvec.size(); i++) {
                stkvec[i] = itsStokesList[i];
            }
            casa::StokesCoordinate stkcoo(stkvec);
            newcoo.replaceCoordinate(stkcoo, newcoo.findCoordinate(casa::Coordinate::STOKES));
        }

        // shift the reference pixel for the spatial coords, so that
        // the RA/DEC (or whatever) are correct. Leave the
        // spectral/stokes axes untouched.
        casa::IPosition outshape(itsSlicer.ndim(), 1);
        int lngAxis = newcoo.directionAxesNumbers()[0];
        int latAxis = newcoo.directionAxesNumbers()[1];
        outshape(lngAxis) = itsSlicer.length()(itsLngAxis);
        outshape(latAxis) = itsSlicer.length()(itsLatAxis);
        if (itsSpcAxis >= 0){
            int spcAxis = newcoo.spectralAxisNumber();
            outshape(spcAxis) = itsSlicer.length()(itsSpcAxis);
        }
        if (itsStkAxis >= 0){
            int stkAxis = newcoo.polarizationAxisNumber();
            outshape(stkAxis) = stkvec.size();
        }
        casa::Vector<Float> shift(outshape.size(), 0);
        casa::Vector<Float> incrFac(outshape.size(), 1);
        shift(lngAxis) = itsSource->getXmin() - itsSpatialPad + itsSource->getXOffset();
        shift(latAxis) = itsSource->getYmin() - itsSpatialPad + itsSource->getYOffset();
        if (itsSpcAxis >= 0){
            int spcAxis = newcoo.spectralAxisNumber();
            shift(spcAxis) = itsSource->getZmin() - itsSpectralPad + itsSource->getZOffset();
        }
        casa::Vector<Int> newshape = outshape.asVector();

        newcoo.subImageInSitu(shift, incrFac, newshape);

        Array<Float> newarray(itsArray.reform(outshape));

        boost::shared_ptr<accessors::IImageAccess> ia = accessors::imageAccessFactory(itsParset);
        ia->create(itsOutputFilename, newarray.shape(), newcoo);

        // write the array
        ia->write(itsOutputFilename, newarray);

        // write the flux units
        std::string units = itsInputCubePtr->units().getName();
        ia->setUnits(itsOutputFilename, units);

        this->writeBeam(itsOutputFilename);
        updateHeaders(itsOutputFilename);

        if (itsInputCubePtr->isMasked()) {
            // copy the image mask to the cubelet, if there is one.
            casa::LogicalArray
            mask(itsInputCubePtr->pixelMask().getSlice(itsSlicer).reform(outshape));
            ia->makeDefaultMask(itsOutputFilename);
            ia->writeMask(itsOutputFilename, mask, casa::IPosition(outshape.nelements(), 0));
        }

        this->closeInput();

    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

}

}

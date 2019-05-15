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
/// @author XXX XXX <XXX.XXX@csiro.au>
///
#include <extraction/MomentMapExtractor.h>
#include <askap_analysis.h>
#include <extraction/SourceDataExtractor.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

#include <string>
#include <iostream>
#include <sstream>

#include <sourcefitting/RadioSource.h>

#include <askap/imageaccess/ImageAccessFactory.h>

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/ArrayLogical.h>
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

#include <duchamp/PixelMap/Voxel.hh>

#include <Common/ParameterSet.h>

using namespace askap::analysis::sourcefitting;

ASKAP_LOGGER(logger, ".momentmapextractor");



namespace askap {

namespace analysis {

MomentMapExtractor::MomentMapExtractor(const LOFAR::ParameterSet& parset):
    SourceDataExtractor(parset)
{
    itsSpatialMethod = parset.getString("spatialMethod", "box");
    if (itsSpatialMethod != "fullfield" && itsSpatialMethod != "box") {
        ASKAPLOG_WARN_STR(logger,
                          "The value of spatialMethod='" << itsSpatialMethod <<
                          "' is not recognised - setting spatialMethod='box'");
        itsSpatialMethod = "box";
    }
    itsFlagUseDetection = parset.getBool("useDetectedPixels", true);

    itsPadSize = parset.getUint("padSize", 5);

    itsOutputFilenameBase = parset.getString("momentOutputBase", "");

    for (int i = 0; i < 3; i++) {
        itsMomentRequest[i] = false;
    }
    std::vector<int> request = parset.getIntVector("moments", std::vector<int>(1, 0));
    bool haveDud = false;
    for (size_t i = 0; i < request.size(); i++) {
        if (request[i] < 0 || request[i] > 2) {
            haveDud = true;
        } else {
            itsMomentRequest[i] = true;
        }
    }
    std::vector<int> momentsUsed;
    for (int i = 0; i < 3; i++) {
        if (itsMomentRequest[i]) {
            momentsUsed.push_back(i);
        }
    }
    if (haveDud) {
        ASKAPLOG_WARN_STR(logger,
                          "You requested invalid moments. Only doing " <<
                          casa::Vector<Int>(momentsUsed));
    } else {
        ASKAPLOG_INFO_STR(logger,
                          "Will compute the following moments " <<
                          casa::Vector<Int>(momentsUsed));
    }

    itsMom0map = casa::Array<Float>();
    itsMom1map = casa::Array<Float>();
    itsMom2map = casa::Array<Float>();

}

void MomentMapExtractor::defineSlicer()
{

    if (this->openInput()) {
        IPosition shape = itsInputCubePtr->shape();
        casa::IPosition blc(shape.size(), 0);
        casa::IPosition trc = shape - 1;

        long zero = 0;
        blc(itsSpcAxis) = std::max(zero, itsSource->getZmin() - 3 + itsSource->getZOffset());
        trc(itsSpcAxis) = std::min(shape(itsSpcAxis) - 1, itsSource->getZmax() + 3 + itsSource->getZOffset());

        if (itsSpatialMethod == "box") {
            blc(itsLngAxis) = std::max(zero,
                                       itsSource->getXmin() - itsPadSize + itsSource->getXOffset());
            blc(itsLatAxis) = std::max(zero,
                                       itsSource->getYmin() - itsPadSize + itsSource->getYOffset());
            trc(itsLngAxis) = std::min(shape(itsLngAxis) - 1,
                                       itsSource->getXmax() + itsPadSize + itsSource->getXOffset());
            trc(itsLatAxis) = std::min(shape(itsLatAxis) - 1,
                                       itsSource->getYmax() + itsPadSize + itsSource->getYOffset());
            /// @todo Not yet dealing with Stokes axis properly.
        } else if (itsSpatialMethod == "fullfield") {
            // Don't need to do anything here, as we use the Slicer
            // based on the full image shape.
        } else {
            ASKAPTHROW(AskapError,
                       "Incorrect value for method ('" <<
                       itsSpatialMethod << "') in cube cutout");
        }

        itsSlicer = casa::Slicer(blc, trc, casa::Slicer::endIsLast);
        ASKAPLOG_DEBUG_STR(logger, itsSlicer);
        this->closeInput();
        this->initialiseArray();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

casa::IPosition MomentMapExtractor::arrayShape()
{
    int lngsize = itsSlicer.length()(itsLngAxis);
    int latsize = itsSlicer.length()(itsLatAxis);
    casa::IPosition shape(4, lngsize, latsize, 1, 1);
    return shape;
}

void MomentMapExtractor::initialiseArray()
{
    if (this->openInput()) {
        casa::IPosition shape = this->arrayShape();
        itsArray = casa::Array<Float>(shape, 0.0);
        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

void MomentMapExtractor::extract()
{
    this->defineSlicer();
    if (this->openInput()) {

        ASKAPLOG_INFO_STR(logger,
                          "Extracting moment map from " << itsInputCube <<
                          " surrounding source ID " << itsSourceID <<
                          " with slicer " << itsSlicer);

        const boost::shared_ptr<SubImage<Float> >
        sub(new SubImage<Float>(*itsInputCubePtr, itsSlicer));

        ASKAPASSERT(sub->size() > 0);
        const casa::MaskedArray<Float> msub(sub->get(), sub->getMask());
        casa::Array<Float> subarray(sub->shape());
        subarray = msub;

        if (itsMomentRequest[0]) {
            this->getMom0(subarray);
        }
        if (itsMomentRequest[1]) {
            this->getMom1(subarray);
        }
        if (itsMomentRequest[2]) {
            this->getMom2(subarray);
        }

        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

void MomentMapExtractor::writeImage()
{

    itsInputCube = itsInputCubeList[0];
    if (this->openInput()) {
        IPosition inshape = itsInputCubePtr->shape();
        casa::CoordinateSystem newcoo = casa::CoordinateUtil::defaultCoords4D();

        int dirCoNum = itsInputCoords.findCoordinate(casa::Coordinate::DIRECTION);
        int spcCoNum = itsInputCoords.findCoordinate(casa::Coordinate::SPECTRAL);
        int stkCoNum = itsInputCoords.findCoordinate(casa::Coordinate::STOKES);

        casa::DirectionCoordinate dircoo(itsInputCoords.directionCoordinate(dirCoNum));
        casa::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
        casa::Vector<Int> stkvec(itsStokesList.size());
        for (size_t i = 0; i < stkvec.size(); i++) {
            stkvec[i] = itsStokesList[i];
        }
        casa::StokesCoordinate stkcoo(stkvec);

        newcoo.replaceCoordinate(dircoo, newcoo.findCoordinate(casa::Coordinate::DIRECTION));
        newcoo.replaceCoordinate(spcoo, newcoo.findCoordinate(casa::Coordinate::SPECTRAL));
        if (stkCoNum >= 0) {
            newcoo.replaceCoordinate(stkcoo, newcoo.findCoordinate(casa::Coordinate::STOKES));
        }

        int lngAxis = newcoo.directionAxesNumbers()[0];
        int latAxis = newcoo.directionAxesNumbers()[1];
        int stkAxis = newcoo.polarizationAxisNumber();
        casa::IPosition outshape(4, 1);
        outshape(lngAxis) = itsSlicer.length()(itsLngAxis);
        outshape(latAxis) = itsSlicer.length()(itsLatAxis);
        outshape(stkAxis) = stkvec.size();
        if (itsSpatialMethod == "box") {
            // shift the reference pixel for the spatial coords, so
            // that the RA/DEC (or whatever) are correct. Leave the
            // spectral/stokes axes untouched.  only want to do this
            // if we are trimming.
            casa::Vector<Float> shift(outshape.size(), 0);
            casa::Vector<Float> incrFac(outshape.size(), 1);
            shift(lngAxis) = itsSource->getXmin() - itsPadSize + itsSource->getXOffset();
            shift(latAxis) = itsSource->getYmin() - itsPadSize + itsSource->getYOffset();
            casa::Vector<Int> newshape = outshape.asVector();
            newcoo.subImageInSitu(shift, incrFac, newshape);
        }

        for (int i = 0; i < 3; i++) {
            if (itsMomentRequest[i]) {

                casa::LogicalArray theMask;
                std::string newunits;
                switch (i) {
                    case 0:
                        itsArray = itsMom0map;
                        theMask = itsMom0mask.reform(outshape);
                        if (spcoo.restFrequency() > 0.) {
                            newunits = itsInputCubePtr->units().getName() + " " +
                                       spcoo.velocityUnit();
                        } else {
                            newunits = itsInputCubePtr->units().getName() + " " +
                                       spcoo.worldAxisUnits()[0];
                        }
                        break;
                    case 1:
                        itsArray = itsMom1map;
                        theMask = itsMom1mask.reform(outshape);
                        if (spcoo.restFrequency() > 0.) {
                            newunits = spcoo.velocityUnit();
                        } else {
                            newunits = spcoo.worldAxisUnits()[0];
                        }
                        break;
                    case 2:
                        itsArray = itsMom2map;
                        theMask = itsMom2mask.reform(outshape);
                        if (spcoo.restFrequency() > 0.) {
                            newunits = spcoo.velocityUnit();
                        } else {
                            newunits = spcoo.worldAxisUnits()[0];
                        }
                        break;
                }

                Array<Float> newarray(itsArray.reform(outshape));

                std::string filename = this->outfile(i);
                ASKAPLOG_INFO_STR(logger, "Writing moment-" << i << " map to '" <<
                                  filename << "'");
                boost::shared_ptr<accessors::IImageAccess> ia = accessors::imageAccessFactory(itsParset);
                ia->create(filename, newarray.shape(), newcoo);

                // write the array
                ia->write(filename, newarray);

                ia->setUnits(filename, newunits);

                this->writeBeam(filename);
                updateHeaders(filename);

                ia->makeDefaultMask(filename);
                ia->writeMask(filename, theMask, casa::IPosition(outshape.nelements(),0));

            }
        }

        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

std::string MomentMapExtractor::outfile(int moment)
{
    std::stringstream ss;
    ss << moment;
    std::string filename = itsOutputFilename;
    size_t loc;
    while (loc = filename.find("%m"),
            loc != std::string::npos) {
        filename.replace(loc, 2, ss.str());
    }
    return filename;
}

double MomentMapExtractor::getSpectralIncrement()
{
    double specIncr;
    int spcCoNum = itsInputCoords.findCoordinate(casa::Coordinate::SPECTRAL);
    casa::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
    if (spcoo.restFrequency() > 0.) {
        // can convert to velocity
        double vel1, vel2;
        spcoo.pixelToVelocity(vel1, 0);
        spcoo.pixelToVelocity(vel2, 1);
        specIncr = fabs(vel1 - vel2);
    } else {
        // can't do velocity conversion, so just use the WCS spectral units
        specIncr = fabs(spcoo.increment()[0]);
    }
    return specIncr;
}

double MomentMapExtractor::getSpectralIncrement(int z)
{
    int spcCoNum = itsInputCoords.findCoordinate(casa::Coordinate::SPECTRAL);
    casa::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
    double specIncr;
    if (spcoo.restFrequency() > 0.) {
        casa::Quantum<Double> vel, velMinus, velPlus;
        ASKAPASSERT(spcoo.pixelToVelocity(vel, double(z)));
        ASKAPASSERT(spcoo.pixelToVelocity(velMinus, double(z - 1)));
        ASKAPASSERT(spcoo.pixelToVelocity(velPlus, double(z + 1)));
//        ASKAPLOG_DEBUG_STR(logger, velMinus << " " << vel << " " << velPlus);
        specIncr = fabs(velPlus.getValue() - velMinus.getValue()) / 2.;
    } else {
        // can't do velocity conversion, so just use the WCS spectral units
        specIncr = fabs(spcoo.increment()[0]);
    }
//    ASKAPLOG_DEBUG_STR(logger, "Channel " << z << " had spectral increment " << specIncr);
    return specIncr;
}

double MomentMapExtractor::getSpecVal(int z)
{
    int spcCoNum = itsInputCoords.findCoordinate(casa::Coordinate::SPECTRAL);
    casa::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
    double specval;
    if (spcoo.restFrequency() > 0.) {
        casa::Quantum<Double> vel;
        ASKAPASSERT(spcoo.pixelToVelocity(vel, double(z)));
        specval = vel.getValue();
    } else {
        ASKAPASSERT(spcoo.toWorld(specval, double(z)));
    }
    return specval;
}

void MomentMapExtractor::getMom0(const casa::Array<Float> &subarray)
{

    ASKAPLOG_INFO_STR(logger, "Extracting moment-0 map");
    itsMom0map = casa::Array<Float>(this->arrayShape(), 0.0);
    uint zeroInt = 0;
    casa::LogicalArray basemask(this->arrayShape(), true);
    if (itsInputCubePtr->hasPixelMask()) {
        casa::LogicalArray mskslice = itsInputCubePtr->pixelMask().getSlice(itsSlicer);
        casa::LogicalArray
        mskTmp = (partialNTrue(mskslice, casa::IPosition(1, itsSpcAxis)) > zeroInt);
        basemask = mskTmp.reform(this->arrayShape());
    }
    itsMom0mask = casa::LogicalArray(this->arrayShape(), false);

    // To get the mask to be applied in FITS images, we divide through
    // at the end by this array. Valid pixels have value of 1. Masked
    // pixels have value of 0. That way the pixels that should be
    // masked are converted to nans.
    casa::Array<float> maskScaler(this->arrayShape(), 0.);

    casa::IPosition outloc(4, 0), inloc(4, 0);
    casa::IPosition start = itsSlicer.start();

    if (itsFlagUseDetection) {
        std::vector<PixelInfo::Voxel> voxlist = itsSource->getPixelSet();
        std::vector<PixelInfo::Voxel>::iterator vox;
        for (vox = voxlist.begin(); vox != voxlist.end(); vox++) {
            int x = vox->getX() - start(itsLngAxis) + itsSource->getXOffset();
            int y = vox->getY() - start(itsLatAxis) + itsSource->getYOffset();
            int zin = vox->getZ() - start(itsSpcAxis) + itsSource->getZOffset();
            int zfull = vox->getZ() + itsSource->getZOffset();
            outloc(itsLngAxis) = inloc(itsLngAxis) = x;
            outloc(itsLatAxis) = inloc(itsLatAxis) = y;
            inloc(itsSpcAxis) = zin;
            itsMom0map(outloc) = itsMom0map(outloc) + subarray(inloc) * getSpectralIncrement(zfull);
            itsMom0mask(outloc) = true;
            maskScaler(outloc) = 1.;
        }
    } else {
        // just sum each spectrum over the slicer's range.
        casa::IPosition outBLC(4, 0), outTRC(itsMom0map.shape() - 1);
        casa::Array<Float> sumarray = partialSums(subarray, casa::IPosition(1, itsSpcAxis));
        itsMom0map(outBLC, outTRC) = sumarray.reform(itsMom0map(outBLC, outTRC).shape()) * getSpectralIncrement();
        itsMom0mask(outBLC, outTRC) = true;
    }
    itsMom0mask = itsMom0mask && basemask;

    itsMom0map /= maskScaler;

}

void MomentMapExtractor::getMom1(const casa::Array<Float> &subarray)
{
    ASKAPLOG_INFO_STR(logger, "Extracting moment-1 map");
    itsMom1map = casa::Array<Float>(this->arrayShape(), 0.0);
    uint zeroInt = 0;
    casa::LogicalArray basemask(this->arrayShape(), true);
    if (itsInputCubePtr->hasPixelMask()) {
        casa::LogicalArray mskslice = itsInputCubePtr->pixelMask().getSlice(itsSlicer);
        casa::LogicalArray
        mskTmp = (partialNTrue(mskslice, casa::IPosition(1, itsSpcAxis)) > zeroInt);
        basemask = mskTmp.reform(this->arrayShape());
    }
    itsMom1mask = casa::LogicalArray(this->arrayShape(), false);

    casa::IPosition start = itsSlicer.start();

    if (itsMom0map.size() == 0) this->getMom0(subarray);
    casa::Array<Float> sumNuS(itsMom1map.shape(), 0.0);
    casa::Array<Float> sumS = itsMom0map / this->getSpectralIncrement();
    if (itsFlagUseDetection) {
        casa::IPosition outloc(4, 0), inloc(4, 0);
        std::vector<PixelInfo::Voxel> voxlist = itsSource->getPixelSet();
        std::vector<PixelInfo::Voxel>::iterator vox;
        for (vox = voxlist.begin(); vox != voxlist.end(); vox++) {
            int x = vox->getX() - start(itsLngAxis) + itsSource->getXOffset();
            int y = vox->getY() - start(itsLatAxis) + itsSource->getYOffset();
            int zin = vox->getZ() - start(itsSpcAxis) + itsSource->getZOffset();
            int zfull = vox->getZ() + itsSource->getZOffset();
            outloc(itsLngAxis) = inloc(itsLngAxis) = x;
            outloc(itsLatAxis) = inloc(itsLatAxis) = y;
            inloc(itsSpcAxis) = zin;
            sumNuS(outloc) = sumNuS(outloc) +
                             subarray(inloc) * this->getSpecVal(zfull) * getSpectralIncrement(zfull);
            itsMom1mask(outloc) = true;
        }
    } else {
        // just sum each spectrum over the slicer's range.
        casa::IPosition outBLC(itsMom1map.ndim(), 0), outTRC(itsMom1map.shape() - 1);
        casa::Array<Float> nuArray(subarray.shape(), 0.);
        for (int z = 0; z < subarray.shape()(itsSpcAxis); z++) {
            casa::IPosition blc(subarray.ndim(), 0), trc = subarray.shape() - 1;
            blc(itsSpcAxis) = trc(itsSpcAxis) = z;
            nuArray(blc, trc) = this->getSpecVal(z + start(itsSpcAxis));
        }
        casa::Array<Float> nuSubarray = nuArray * subarray;
        casa::Array<Float> sumarray = partialSums(nuSubarray, casa::IPosition(1, itsSpcAxis));
        sumNuS(outBLC, outTRC) = sumarray.reform(sumNuS(outBLC, outTRC).shape()) * this->getSpectralIncrement();
        itsMom1mask(outBLC, outTRC) = true;
    }

    float zero = 0.;
    itsMom1mask = itsMom1mask && basemask;
    itsMom1mask = itsMom1mask && (itsMom0map > zero);

    itsMom1map = (sumNuS / itsMom0map);

}

void MomentMapExtractor::getMom2(const casa::Array<Float> &subarray)
{
    ASKAPLOG_INFO_STR(logger, "Extracting moment-2 map");
    itsMom2map = casa::Array<Float>(this->arrayShape(), 0.0);
    uint zeroInt = 0;
    casa::LogicalArray basemask(this->arrayShape(), true);
    if (itsInputCubePtr->hasPixelMask()) {
        casa::LogicalArray mskslice = itsInputCubePtr->pixelMask().getSlice(itsSlicer);
        casa::LogicalArray
        mskTmp = (partialNTrue(mskslice, casa::IPosition(1, itsSpcAxis)) > zeroInt);
        basemask = mskTmp.reform(this->arrayShape());
    }
    itsMom2mask = casa::LogicalArray(this->arrayShape(), false);
    casa::IPosition start = itsSlicer.start();

    if (itsMom1map.size() == 0) this->getMom1(subarray);
    casa::Array<Float> sumNu2S(itsMom2map.shape(), 0.0);
    casa::Array<Float> sumS = itsMom0map / this->getSpectralIncrement();
    if (itsFlagUseDetection) {
        casa::IPosition outloc(4, 0), inloc(4, 0);
        std::vector<PixelInfo::Voxel> voxlist = itsSource->getPixelSet();
        std::vector<PixelInfo::Voxel>::iterator vox;
        for (vox = voxlist.begin(); vox != voxlist.end(); vox++) {
            int x = vox->getX() - start(itsLngAxis) + itsSource->getXOffset();
            int y = vox->getY() - start(itsLatAxis) + itsSource->getYOffset();
            int zin = vox->getZ() - start(itsSpcAxis) + itsSource->getZOffset();
            int zfull = vox->getZ() + itsSource->getZOffset();
            outloc(itsLngAxis) = inloc(itsLngAxis) = x;
            outloc(itsLatAxis) = inloc(itsLatAxis) = y;
            inloc(itsSpcAxis) = zin;
            sumNu2S(outloc) = sumNu2S(outloc) +
                              subarray(inloc) *
                              (this->getSpecVal(zfull) - itsMom1map(outloc)) *
                              (this->getSpecVal(zfull) - itsMom1map(outloc)) *
                              getSpectralIncrement(zfull);
            itsMom2mask(outloc) = true;
        }
    } else {
        // just sum each spectrum over the slicer's range.
        casa::IPosition outBLC(itsMom2map.ndim(), 0), outTRC(itsMom2map.shape() - 1);
        casa::IPosition shapeIn(subarray.shape());
        casa::IPosition shapeMap(shapeIn); shapeMap(itsSpcAxis) = 1;
        casa::Array<Float> nu2Array(shapeIn, 0.);
        casa::Array<Float> meanNu(itsMom1map.reform(shapeMap));
        for (int z = 0; z < subarray.shape()(itsSpcAxis); z++) {
            casa::IPosition blc(subarray.ndim(), 0), trc = subarray.shape() - 1;
            blc(itsSpcAxis) = trc(itsSpcAxis) = z;
            nu2Array(blc, trc) = this->getSpecVal(z + start(itsSpcAxis));
            nu2Array(blc, trc) = (nu2Array(blc, trc) - meanNu);
        }
        casa::Array<Float> nu2Subarray = nu2Array * nu2Array * subarray;
        casa::Array<Float> sumarray = partialSums(nu2Subarray, casa::IPosition(1, itsSpcAxis));
        sumNu2S(outBLC, outTRC) = sumarray.reform(sumNu2S(outBLC, outTRC).shape()) * this->getSpectralIncrement();
        itsMom2mask(outBLC, outTRC) = true;
    }

    itsMom2map = (sumNu2S / itsMom0map);

    float zero = 0.;
    itsMom2mask = itsMom2mask && basemask;
    itsMom2mask = itsMom2mask && (itsMom0map > zero);
    itsMom2mask = itsMom2mask && (itsMom2map > zero);

    itsMom2map = sqrt(itsMom2map);


}


}

}

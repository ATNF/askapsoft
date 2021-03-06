/// @file
///
/// @copyright (c) 2008 CSIRO
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///

#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <sourcefitting/Fitter.h>
#include <sourcefitting/FittingParameters.h>
#include <sourcefitting/FitResults.h>
#include <sourcefitting/SubComponent.h>
#include <sourcefitting/SubThresholder.h>
#include <analysisparallel/SubimageDef.h>
#include <casainterface/CasaInterface.h>
#include <mathsutils/MathsUtils.h>
#include <outputs/CataloguePreparation.h>

#include <polarisation/StokesSpectrum.h>
#include <polarisation/StokesImodel.h>
#include <catalogues/CasdaComponent.h>

#include <duchamp/fitsHeader.hh>
#include <duchamp/PixelMap/Voxel.hh>
#include <duchamp/PixelMap/Object2D.hh>
#include <duchamp/PixelMap/Object3D.hh>
#include <duchamp/Cubes/cubes.hh>
#include <duchamp/Detection/detection.hh>
#include <duchamp/Outputs/columns.hh>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/AnnotationWriter.hh>
#include <duchamp/Outputs/KarmaAnnotationWriter.hh>
#include <duchamp/Utils/Section.hh>
#include <duchamp/Utils/utils.hh>
#include <duchamp/Detection/finders.hh>

#include <casacore/scimath/Fitting/FitGaussian.h>
#include <casacore/scimath/Functionals/Gaussian1D.h>
#include <casacore/scimath/Functionals/Gaussian2D.h>
#include <casacore/scimath/Functionals/Gaussian3D.h>
#include <casacore/casa/namespace.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/MaskedArray.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <Common/LofarTypedefs.h>
using namespace LOFAR::TYPES;
#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
#include <Common/Exceptions.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <string>
#include <algorithm>
#include <utility>
#include <math.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".radioSource");

using namespace duchamp;
using namespace askap::analysisutilities;

namespace askap {

namespace analysis {

namespace sourcefitting {

RadioSource::RadioSource():
    duchamp::Detection()
{
    itsFlagHasFit = false;
    itsFlagAtEdge = false;
    itsHeader = duchamp::FitsHeader();
    itsFitParams = FittingParameters();
    itsNoiseLevel = itsFitParams.noiseLevel();

    initialiseAlphaBetaMaps();
}

//**************************************************************//

RadioSource::RadioSource(duchamp::Detection obj):
    duchamp::Detection(obj)
{
    itsFlagHasFit = false;
    itsFlagAtEdge = false;
    itsHeader = duchamp::FitsHeader();
    itsFitParams = FittingParameters();
    itsNoiseLevel = itsFitParams.noiseLevel();

    initialiseAlphaBetaMaps();

}

//**************************************************************//

RadioSource::RadioSource(const RadioSource& src):
    duchamp::Detection(src)
{
    operator=(src);
}

//**************************************************************//

RadioSource& RadioSource::operator= (const duchamp::Detection& det)
{
    ((duchamp::Detection &) *this) = det;
    itsFlagHasFit = false;
    itsFlagAtEdge = false;
    itsFitParams = FittingParameters();
    itsHeader = duchamp::FitsHeader();
    itsNoiseLevel = itsFitParams.noiseLevel();

    initialiseAlphaBetaMaps();

    return *this;
}

//**************************************************************//

RadioSource& RadioSource::operator= (const RadioSource& src)
{
    ((duchamp::Detection &) *this) = src;
    itsFlagAtEdge = src.itsFlagAtEdge;
    itsFlagHasFit = src.itsFlagHasFit;
    itsNoiseLevel = src.itsNoiseLevel;
    itsDetectionThreshold = src.itsDetectionThreshold;
    itsHeader = src.itsHeader;
    itsBox = src.itsBox;
    itsFitParams = src.itsFitParams;
    itsBestFitMap = src.itsBestFitMap;
    itsBestFitType = src.itsBestFitType;
    itsAlphaMap = src.itsAlphaMap;
    itsBetaMap = src.itsBetaMap;
    itsAlphaError = src.itsAlphaError;
    itsBetaError = src.itsBetaError;
    return *this;
}

//**************************************************************//

void RadioSource::initialiseAlphaBetaMaps()
{

    std::vector<std::string>::iterator type;
    std::vector<std::string> typelist = availableFitTypes;

    for (type = typelist.begin(); type < typelist.end(); type++) {
        itsAlphaMap[*type] = std::vector<double>(1, defaultAlpha);
        itsAlphaError[*type] = std::vector<double>(1, 0.);
        itsBetaMap[*type] = std::vector<double>(1, defaultBeta);
        itsBetaError[*type] = std::vector<double>(1, 0.);
    }

    itsAlphaMap["best"] = std::vector<double>(1, defaultAlpha);
    itsAlphaError["best"] = std::vector<double>(1, 0.);
    itsBetaMap["best"] = std::vector<double>(1, defaultBeta);
    itsBetaError["best"] = std::vector<double>(1, 0.);

}

//**************************************************************//

void RadioSource::addOffsets(long xoff, long yoff, long zoff)
{
    this->Detection::addOffsets(xoff, yoff, zoff);

    std::map<std::string, FitResults>::iterator fit;
    for (fit = itsBestFitMap.begin(); fit != itsBestFitMap.end(); fit++) {
        std::vector<casa::Gaussian2D<Double> >::iterator gauss;
        for (gauss = fit->second.fits().begin();
                gauss != fit->second.fits().end();
                gauss++) {
            gauss->setXcenter(gauss->xCenter() + xoff);
            gauss->setYcenter(gauss->yCenter() + yoff);
        }
    }

}

//**************************************************************//

void RadioSource::defineBox(duchamp::Section &sec,
                            int spectralAxis)
{

    int ndim = (spectralAxis >= 0) ? 3 : 2;
    casa::IPosition start(ndim, 0), end(ndim, 0), stride(ndim, 1);
    start(0) = std::max(long(sec.getStart(0) - this->xSubOffset),
                        this->getXmin() - itsFitParams.boxPadSize());
    end(0)   = std::min(long(sec.getEnd(0) - this->xSubOffset),
                        this->getXmax() + itsFitParams.boxPadSize());
    start(1) = std::max(long(sec.getStart(1) - this->ySubOffset),
                        this->getYmin() - itsFitParams.boxPadSize());
    end(1)   = std::min(long(sec.getEnd(1) - this->ySubOffset),
                        this->getYmax() + itsFitParams.boxPadSize());
    if (spectralAxis >= 0) {
        start(2) = std::max(long(sec.getStart(spectralAxis) - this->zSubOffset),
                            this->getZmin() - itsFitParams.boxPadSize());
        end(2)   = std::min(long(sec.getEnd(spectralAxis) - this->zSubOffset),
                            this->getZmax() + itsFitParams.boxPadSize());
    }

    if (start >= end) {
        ASKAPLOG_DEBUG_STR(logger,
                           "RadioSource::defineBox failing : sec=" << sec.getSection() <<
                           ", offsets: " << this->xSubOffset << " " << this->ySubOffset <<
                           " " << this->zSubOffset <<
                           ", mins: " << this->getXmin() << " " << this->getYmin() <<
                           " " << this->getZmin() <<
                           ", maxs: " << this->getXmax() << " " << this->getYmax() <<
                           " " << this->getZmax() <<
                           ", boxpadsize: " << itsFitParams.boxPadSize());
        ASKAPTHROW(AskapError,
                   "RadioSource::defineBox bad slicer: end(" <<
                   end << ") < start (" << start << ")");
    }
    itsBox = casa::Slicer(start, end, stride, Slicer::endIsLast);
}

//**************************************************************//

std::string RadioSource::boundingSubsection(std::vector<size_t> dim,
        bool fullSpectralRange)
{

    const int lng = itsHeader.getWCS()->lng;
    const int lat = itsHeader.getWCS()->lat;
    const int spec = itsHeader.getWCS()->spec;
    std::vector<std::string> sectionlist(dim.size(), "1:1");
    long first, last;
    for (int ax = 0; ax < int(dim.size()); ax++) {
        std::stringstream ss;
        if (ax == spec) {
            if (fullSpectralRange) {
                first = 1;
                last = dim[ax];
            } else {
                first = std::max(1L, this->zmin - itsFitParams.boxPadSize() + 1);
                last = std::min(long(dim[ax]), this->zmax + itsFitParams.boxPadSize() + 1);
            }
        } else if (ax == lng) {
            first = this->xmin - itsFitParams.boxPadSize() + 1;
            last = this->xmax + itsFitParams.boxPadSize() + 1;
            if (itsFitParams.useNoise()) {
                first = std::min(first, this->xpeak - itsFitParams.noiseBoxSize() / 2 + 1);
                last = std::max(last, this->xpeak + itsFitParams.noiseBoxSize() / 2 + 1);
            }
            first = std::max(first, 1L);
            last = std::min(last, long(dim[ax]));

        } else if (ax == lat) {
            first = this->ymin - itsFitParams.boxPadSize() + 1;
            last = this->ymax + itsFitParams.boxPadSize() + 1;
            if (itsFitParams.useNoise()) {
                first = std::min(first, this->ypeak - itsFitParams.noiseBoxSize() / 2 + 1);
                last = std::max(last, this->ypeak + itsFitParams.noiseBoxSize() / 2 + 1);
            }
            first = std::max(first, 1L);
            last = std::min(last, long(dim[ax]));

        } else {
            first = last = 1;
        }
        ss << first << ":" << last;
        sectionlist[ax] = ss.str();
    }
    std::stringstream secstr;
    secstr << "[ " << sectionlist[0];
    for (size_t i = 1; i < dim.size(); i++) {
        secstr << "," << sectionlist[i];
    }
    secstr << "]";

    return secstr.str();
}


//**************************************************************//

void RadioSource::setAtEdge(duchamp::Cube &cube,
                            analysisutilities::SubimageDef &subimage,
                            int workerNum)
{
    bool flagBoundary = false;
    bool flagAdj = cube.pars().getFlagAdjacent();
    float threshS = cube.pars().getThreshS();
    float threshV = cube.pars().getThreshV();

    long xminEdge, xmaxEdge, yminEdge, ymaxEdge, zminEdge, zmaxEdge;

    if (workerNum < 0) {  // if it is the Master node
        xminEdge = yminEdge = zminEdge = 0;
        xmaxEdge = cube.getDimX() - 1;
        ymaxEdge = cube.getDimY() - 1;
        zmaxEdge = cube.getDimZ() - 1;
    } else {
        std::vector<unsigned int> nsub = subimage.nsub();
        std::vector<unsigned int> overlap = subimage.overlap();
        unsigned int colnum = workerNum % nsub[0];
        unsigned int rownum = workerNum / nsub[0];
        unsigned int znum = workerNum / (nsub[0] * nsub[1]);
        xminEdge = (colnum == 0) ? 0 : overlap[0];
        xmaxEdge = (colnum == nsub[0] - 1) ?
                   cube.getDimX() - 1 : cube.getDimX() - 1 - overlap[0];
        yminEdge = (rownum == 0) ? 0 : overlap[1];
        ymaxEdge = (rownum == nsub[1] - 1) ?
                   cube.getDimY() - 1 : cube.getDimY() - 1 - overlap[1];
        zminEdge = (znum == 0) ? 0 : overlap[2];
        zmaxEdge = (znum == nsub[2] - 1) ?
                   cube.getDimZ() - 1 : cube.getDimZ() - 1 - overlap[2];
    }


    if (flagAdj) {
        flagBoundary = flagBoundary || (this->getXmin() <= xminEdge);
        flagBoundary = flagBoundary || (this->getXmax() >= xmaxEdge);
        flagBoundary = flagBoundary || (this->getYmin() <= yminEdge);
        flagBoundary = flagBoundary || (this->getYmax() >= ymaxEdge);

        if (cube.getDimZ() > 1) {
            flagBoundary = flagBoundary || (this->getZmin() <= zminEdge);
            flagBoundary = flagBoundary || (this->getZmax() >= zmaxEdge);
        }
    } else {
        flagBoundary = flagBoundary || ((this->getXmin() - xminEdge) < threshS);
        flagBoundary = flagBoundary || ((xmaxEdge - this->getXmax()) < threshS);
        flagBoundary = flagBoundary || ((this->getYmin() - yminEdge) < threshS);
        flagBoundary = flagBoundary || ((ymaxEdge - this->getYmax()) < threshS);

        if (cube.getDimZ() > 1) {
            flagBoundary = flagBoundary || ((this->getZmin() - zminEdge) < threshV);
            flagBoundary = flagBoundary || ((zmaxEdge - this->getZmax()) < threshV);
        }
    }

    itsFlagAtEdge = flagBoundary;
}
//**************************************************************//

void RadioSource::setNoiseLevel(duchamp::Cube &cube)
{
    if (itsFitParams.useNoise() || !itsFitParams.doFit()) {
        std::vector<float> array(cube.getArray(),
                                 cube.getArray() + cube.getSize());
        std::vector<size_t> dim(cube.getDimArray(),
                                cube.getDimArray() + cube.getNumDim());
        this->setNoiseLevel(array, dim, itsFitParams.noiseBoxSize());
    } else {
        itsNoiseLevel = itsFitParams.noiseLevel();
    }
}

//**************************************************************//

void RadioSource::setNoiseLevel(std::vector<float> &array,
                                std::vector<size_t> &dim,
                                unsigned int boxSize)
{
    if (boxSize % 2 == 0) boxSize += 1;
    int hw = boxSize / 2;
    std::vector<float> localArray;
    long xmin = max(0, this->xpeak - hw);
    long ymin = max(0, this->ypeak - hw);
    int xsize = dim[0];
    int ysize = dim[1];
    long xmax = min(xsize - 1, this->xpeak + hw);
    long ymax = min(ysize - 1, this->ypeak + hw);

    unsigned int npix = (xmax - xmin + 1) * (ymax - ymin + 1);
    ASKAPASSERT(npix <= boxSize * boxSize);

    for (int x = xmin; x <= xmax; x++) {
        for (int y = ymin; y <= ymax; y++) {
            int pos = x + y * xsize;
            localArray.push_back(array[pos]);
        }
    }

    itsNoiseLevel = analysisutilities::findSpread(true, localArray);

}


//**************************************************************//

void RadioSource::setDetectionThreshold(duchamp::Cube &cube,
                                        bool flagVariableThreshold)
{

    if (flagVariableThreshold) {

        // Use the fact that the SNR array has been stored in the
        // Cube's recon array. So just need the max value from that
        // array to get peakSNR, and the minimum flux value of all
        // detected pixels to get the detection threshold.

        std::vector<PixelInfo::Voxel> voxSet = this->getPixelSet();

        std::vector<PixelInfo::Voxel>::iterator vox = voxSet.begin();
        itsDetectionThreshold = cube.getPixValue(vox->getX(), vox->getY(), vox->getZ());

        for (; vox < voxSet.end(); vox++) {
            float pixval = cube.getPixValue(vox->getX(), vox->getY(), vox->getZ());
            itsDetectionThreshold = std::min(itsDetectionThreshold, pixval);
        }

    } else {

        itsDetectionThreshold = cube.stats().getThreshold();

        if (cube.pars().getFlagGrowth()) {
            float growth;
            if (cube.pars().getFlagUserGrowthThreshold()) {
                growth = cube.pars().getGrowthThreshold();
                itsDetectionThreshold = std::min(itsDetectionThreshold, growth);
            } else {
                growth = cube.stats().snrToValue(cube.pars().getGrowthCut());
                itsDetectionThreshold = std::min(itsDetectionThreshold, growth);
            }
        }

    }

}

//**************************************************************//

void RadioSource::setDetectionThreshold(std::vector<PixelInfo::Voxel> &inVoxlist,
                                        std::vector<PixelInfo::Voxel> &inSNRvoxlist,
                                        bool flagMedianSearch)
{

    if (flagMedianSearch) {
        std::vector<PixelInfo::Voxel> voxSet = this->getPixelSet();
        std::vector<PixelInfo::Voxel>::iterator vox = voxSet.begin();
        this->peakSNR = 0.;

        for (; vox < voxSet.end(); vox++) {
            std::vector<PixelInfo::Voxel>::iterator pixvox = inVoxlist.begin();

            while (pixvox < inVoxlist.end() && !vox->match(*pixvox)) {
                pixvox++;
            }

            if (pixvox == inVoxlist.end()) {
                ASKAPLOG_ERROR_STR(logger,
                                   "Missing a voxel in the pixel list comparison: (" <<
                                   vox->getX() << "," << vox->getY() << ")");
            }

            float flux = pixvox->getF();
            if (vox == voxSet.begin()) {
                itsDetectionThreshold = flux;
            } else {
                itsDetectionThreshold = std::min(itsDetectionThreshold, flux);
            }

            std::vector<PixelInfo::Voxel>::iterator snrvox = inSNRvoxlist.begin();

            while (snrvox < inSNRvoxlist.end() && !vox->match(*snrvox)) {
                snrvox++;
            }

            if (snrvox == inSNRvoxlist.end()) {
                ASKAPLOG_ERROR_STR(logger,
                                   "Missing a voxel in the SNR list comparison: (" <<
                                   vox->getX() << "," << vox->getY() << ")");
            }

            flux = snrvox->getF();
            if (vox == voxSet.begin()) {
                this->peakSNR = flux;
            } else {
                this->peakSNR = std::max(this->peakSNR, flux);
            }
        }

    }

}
//**************************************************************//

void RadioSource::getFWHMestimate(std::vector<float> fluxarray,
                                  double & angle,
                                  double & maj,
                                  double & min)
{

    size_t dim[2];
    dim[0] = this->boxXsize();
    dim[1] = this->boxYsize();
    boost::scoped_ptr<duchamp::Image> smlIm(new duchamp::Image(dim));
    smlIm->saveArray(fluxarray.data(), this->boxSize());
    smlIm->setMinSize(1);
    float thresh = (itsDetectionThreshold + this->peakFlux) / 2.;
    smlIm->stats().setThreshold(thresh);
    std::vector<PixelInfo::Object2D> objlist = smlIm->findSources2D();
    std::vector<PixelInfo::Object2D>::iterator o;

    for (o = objlist.begin(); o < objlist.end(); o++) {
        duchamp::Detection tempobj;
        tempobj.addChannel(0, *o);
        tempobj.calcFluxes(fluxarray.data(), dim); // we need to know where the peak is.

        if ((tempobj.getXPeak() + this->boxXmin()) == this->getXPeak()  &&
                (tempobj.getYPeak() + this->boxYmin()) == this->getYPeak()) {
            // measure parameters only for source at peak
            angle = o->getPositionAngle();
            std::pair<double, double> axes = o->getPrincipalAxes();
            maj = std::max(axes.first, axes.second);
            min = std::min(axes.first, axes.second);
        }

    }

}

//**************************************************************//

std::vector<SubComponent>
RadioSource::getSubComponentList(casa::Matrix<casa::Double> pos,
                                 casa::Vector<casa::Double> &f)
{
    std::vector<SubComponent> cmpntlist;
    if (itsFitParams.useCurvature()) {

        // 1. get array of curvature from curvature map
        // 2. define bool array of correct size
        // 3. value of this is = (isInObject) && (curvature < -sigmaCurv)
        // 4. run lutz_detect to get list of objects
        // 5. for each object, define a subcomponent of zero size with correct peak & position

        casa::IPosition globalOffset(itsBox.start().size(), 0);
        globalOffset[0] = this->xSubOffset;
        globalOffset[1] = this->ySubOffset;

        casa::Slicer fullImageBox(itsBox.start() + globalOffset,
                                  itsBox.length(), Slicer::endIsLength);

        ASKAPLOG_DEBUG_STR(logger, "For curvature extraction, formed slicer " << fullImageBox << " with globalOffsets=" << globalOffset);

        // casa::Array<float> curvArray =
        //     analysisutilities::getPixelsInBox(itsFitParams.curvatureImage(),
        //                                       fullImageBox, false);
        casa::MaskedArray<float> curvArray =
            analysisutilities::getPixelsInBox(itsFitParams.curvatureImage(),
                                              fullImageBox, false);

        PixelInfo::Object2D spatMap = this->getSpatialMap();
        size_t dim[2];
        dim[0] = fullImageBox.length()[0];
        dim[1] = fullImageBox.length()[1];

        std::vector<float> fluxArray(fullImageBox.length().product(), 0.);
        std::vector<bool> summitMap(fullImageBox.length().product(), false);

        ASKAPLOG_DEBUG_STR(logger, "Thresholding curvature array for less than " << -1.*itsFitParams.sigmaCurv());
        for (size_t i = 0; i < f.size(); i++) {
            int x = int(pos(i, 0));
            int y = int(pos(i, 1));
            if (spatMap.isInObject(x, y)) {
                int loc = (x - this->boxXmin()) + this->boxXsize() * (y - this->boxYmin());
                fluxArray[loc] = float(f(i));
                summitMap[loc] = (curvArray.getArray().data()[loc] < -1.*itsFitParams.sigmaCurv());
            }
        }

        std::vector<Object2D> summitList = duchamp::lutz_detect(summitMap,
                                           this->boxXsize(),
                                           this->boxYsize(),
                                           1);
        ASKAPLOG_DEBUG_STR(logger, "Found " << summitList.size() << " summits");

        duchamp::Param par;
        par.setXOffset(fullImageBox.start()[0]);
        par.setYOffset(fullImageBox.start()[1]);
        for (std::vector<Object2D>::iterator obj = summitList.begin();
                obj < summitList.end();
                obj++) {
            duchamp::Detection det;
            det.addChannel(0, *obj);
            det.calcFluxes(fluxArray.data(), dim);
            ASKAPLOG_DEBUG_STR(logger, "Detection- xpeak=" << det.getXPeak() << ", ypeak=" << det.getYPeak());
            det.setOffsets(par);
            det.addOffsets();
            ASKAPLOG_DEBUG_STR(logger, "Detection- xpeak=" << det.getXPeak() << ", ypeak=" << det.getYPeak());
            SubComponent cmpnt;
            cmpnt.setPeak(det.getPeakFlux());
            // Need to correct the positions to put them in the current worker frame
            cmpnt.setX(det.getXPeak() - globalOffset[0]);
            cmpnt.setY(det.getYPeak() - globalOffset[1]);
            cmpnt.setPA(0.);
            cmpnt.setMajor(0.);
            cmpnt.setMinor(0.);
            cmpntlist.push_back(cmpnt);
            ASKAPLOG_DEBUG_STR(logger, "Found subcomponent " << cmpnt);
        }

    } else {
        SubThresholder subThresh;
        subThresh.define(*this, pos, f);
        cmpntlist = subThresh.find();
    }

    return cmpntlist;
}

//**************************************************************//

std::vector<SubComponent>
RadioSource::getThresholdedSubComponentList(std::vector<float> fluxarray)
{

    std::vector<SubComponent> fullList;
    size_t dim[2];
    dim[0] = this->boxXsize();
    dim[1] = this->boxYsize();
    boost::scoped_ptr<duchamp::Image> smlIm(new duchamp::Image(dim));
    smlIm->saveArray(fluxarray.data(), this->boxSize());
    smlIm->setMinSize(1);
    SubComponent base;
    base.setPeak(this->peakFlux);
    base.setX(this->xpeak);
    base.setY(this->ypeak);
    double a = 0., b = 0., c = 0.;

    if (this->getSize() < 3) {
        base.setPA(0);
        base.setMajor(1.);
        base.setMinor(1.);
        fullList.push_back(base);
        return fullList;
    }

    this->getFWHMestimate(fluxarray, a, b, c);
    base.setPA(a);
    base.setMajor(b);
    base.setMinor(c);
    const int numThresh = itsFitParams.numSubThresholds();
    float baseThresh = itsDetectionThreshold > 0 ?
                       log10(itsDetectionThreshold) : -6.;
    float threshIncrement = (log10(this->peakFlux) - baseThresh) / float(numThresh + 1);
    float thresh;
    int threshCtr = 0;
    std::vector<PixelInfo::Object2D> objlist;
    std::vector<PixelInfo::Object2D>::iterator obj;
    bool keepGoing;

    do {
        threshCtr++;
        thresh = pow(10., baseThresh + threshCtr * threshIncrement);
        smlIm->stats().setThreshold(thresh);
        objlist = smlIm->findSources2D();
        keepGoing = (objlist.size() == 1);
    } while (keepGoing && (threshCtr < numThresh));

    if (!keepGoing) {
        for (obj = objlist.begin(); obj < objlist.end(); obj++) {
            RadioSource newsrc;
            newsrc.setFitParams(itsFitParams);
            newsrc.setDetectionThreshold(thresh);
            newsrc.addChannel(0, *obj);
            newsrc.calcFluxes(fluxarray.data(), dim);
            newsrc.setBox(this->box());
            newsrc.addOffsets(this->boxXmin(), this->boxYmin(), 0);
            newsrc.xpeak += this->boxXmin();
            newsrc.ypeak += this->boxYmin();
            // now change the flux array so that we only see the current object
            std::vector<float> newfluxarray(this->boxSize(), 0.);

            for (size_t i = 0; i < this->boxSize(); i++) {
                size_t xbox = i % this->boxXsize();
                size_t ybox = i / this->boxXsize();
                PixelInfo::Object2D spatMap = newsrc.getSpatialMap();

                if (spatMap.isInObject(xbox + this->boxXmin(), ybox + this->boxYmin())) {
                    newfluxarray[i] = fluxarray[i];
                }
            }

            std::vector<SubComponent>
            newlist = newsrc.getThresholdedSubComponentList(newfluxarray);

            for (uInt i = 0; i < newlist.size(); i++) {
                fullList.push_back(newlist[i]);
            }
        }
    } else {
        fullList.push_back(base);
    }

    if (fullList.size() > 1) {
        std::sort(fullList.begin(), fullList.end());
        std::reverse(fullList.begin(), fullList.end());
    }

    return fullList;
}


//**************************************************************//

std::multimap<int, PixelInfo::Voxel>
RadioSource::findDistinctPeaks(casa::Vector<casa::Double> f)
{

    const int numThresh = itsFitParams.numSubThresholds();
    std::multimap<int, PixelInfo::Voxel> peakMap;
    std::multimap<int, PixelInfo::Voxel>::iterator pk;
    size_t dim[2];
    dim[0] = this->boxXsize();
    dim[1] = this->boxYsize();
    duchamp::Image smlIm(dim);
    std::vector<float> fluxarray(this->boxSize());

    for (size_t i = 0; i < this->boxSize(); i++) {
        fluxarray[i] = f(i);
    }

    smlIm.saveArray(fluxarray.data(), this->boxSize());
    smlIm.setMinSize(1);
    float baseThresh = log10(itsDetectionThreshold);
    float threshIncrement = (log10(this->peakFlux) - baseThresh) / float(numThresh);
    PixelInfo::Object2D spatMap = this->getSpatialMap();

    for (int i = 1; i <= numThresh; i++) {
        float thresh = pow(10., baseThresh + i * threshIncrement);
        smlIm.stats().setThreshold(thresh);
        std::vector<PixelInfo::Object2D> objlist = smlIm.findSources2D();
        std::vector<PixelInfo::Object2D>::iterator o;

        for (o = objlist.begin(); o < objlist.end(); o++) {
            duchamp::Detection tempobj;
            tempobj.addChannel(0, *o);
            tempobj.calcFluxes(fluxarray.data(), dim);
            bool pkInObj = spatMap.isInObject(tempobj.getXPeak() + this->boxXmin(),
                                              tempobj.getYPeak() + this->boxYmin());

            if (pkInObj) {
                PixelInfo::Voxel peakLoc(tempobj.getXPeak() + this->boxXmin(),
                                         tempobj.getYPeak() + this->boxYmin(),
                                         tempobj.getZPeak(),
                                         tempobj.getPeakFlux());
                int freq = 1;
                bool finished = false;

                if (peakMap.size() > 0) {
                    pk = peakMap.begin();

                    while (!finished && pk != peakMap.end()) {
                        if (!(pk->second == peakLoc)) {
                            pk++;
                        } else {
                            freq = pk->first + 1;
                            peakMap.erase(pk);
                            finished = true;
                        }
                    }
                }

                peakMap.insert(std::pair<int, PixelInfo::Voxel>(freq, peakLoc));
            }
        }
    }

    return peakMap;
}


//**************************************************************//

void RadioSource::prepareForFit(duchamp::Cube & cube, bool useArray)
{

    if (useArray) {
        this->setNoiseLevel(cube);
    } else {
        // if need to use the surrounding noise, we have to go extract
        // it from the image
        if (itsFitParams.useNoise()) {
            float noise = findSurroundingNoise(cube.pars().getImageFile(),
                                               this->xpeak + this->xSubOffset,
                                               this->ypeak + this->ySubOffset,
                                               itsFitParams.noiseBoxSize());
            this->setNoiseLevel(noise);
        } else {
            this->setNoiseLevel(1);
        }
    }

    this->setHeader(cube.header());
    this->setOffsets(cube.pars());
    if (!itsFitParams.doFit()) {
        itsFitParams.setBoxPadSize(1);
    }
    this->defineBox(cube.pars().section(), cube.header().getWCS()->spec);

}

//**************************************************************//

bool RadioSource::fitGauss(duchamp::Cube &cube)
{
    std::vector<float> array(cube.getArray(),
                             cube.getArray() + cube.getSize());
    std::vector<size_t> dim(cube.getDimArray(),
                            cube.getDimArray() + cube.getNumDim());

    if (itsFitParams.fitJustDetection()) {
        ASKAPLOG_DEBUG_STR(logger, "Fitting to detected pixels");
        std::vector<PixelInfo::Voxel> voxlist = this->getPixelSet(array.data(), dim.data());
        return fitGauss(voxlist);
    } else {
        return fitGauss(array, dim);
    }

}

//**************************************************************//

bool RadioSource::fitGauss(std::vector<PixelInfo::Voxel> &voxelList)
{
    int size = this->getSize();
    casa::Matrix<casa::Double> pos;
    casa::Vector<casa::Double> f;
    casa::Vector<casa::Double> sigma;
    pos.resize(size, 2);
    f.resize(size);
    sigma.resize(size);
    casa::Vector<casa::Double> curpos(2);
    curpos = 0;

    if (this->getZmin() != this->getZmax()) {
        ASKAPLOG_ERROR_STR(logger,
                           "Can only do fitting for two-dimensional objects!: " <<
                           "z-locations show a spread: " <<
                           " zmin=" << this->getZmin() <<
                           ", zmax=" << this->getZmax());
        return false;
    }

    int i = 0;
    std::vector<PixelInfo::Voxel>::iterator vox = voxelList.begin();

    for (; vox < voxelList.end(); vox++) {
        if (this->isInObject(*vox)) { // just to make sure it is a source pixel
            sigma(i) = itsNoiseLevel;
            curpos(0) = vox->getX();
            curpos(1) = vox->getY();
            pos.row(i) = curpos;
            f(i) = vox->getF();
            i++;
        }
    }

    return fitGauss(pos, f, sigma);
}

//**************************************************************//

bool RadioSource::fitGauss(std::vector<float> &fluxArray,
                           std::vector<size_t> &dimArray)
{

    if (this->getZcentre() != this->getZmin() || this->getZcentre() != this->getZmax()) {
        ASKAPLOG_ERROR(logger, "Can only do fitting for two-dimensional objects!");
        return false;
    }

    casa::Matrix<casa::Double> pos;
    casa::Vector<casa::Double> f;
    casa::Vector<casa::Double> sigma;
    pos.resize(this->boxSize(), 2);
    f.resize(this->boxSize());
    sigma.resize(this->boxSize());
    casa::Vector<casa::Double> curpos(2);
    curpos = 0;

    for (long x = this->boxXmin(); x <= this->boxXmax(); x++) {
        for (long y = this->boxYmin(); y <= this->boxYmax(); y++) {
            size_t i = (x - this->boxXmin()) +
                       (y - this->boxYmin()) * this->boxXsize();
            size_t j = x + y * dimArray[0];

            if (j < dimArray[0]*dimArray[1]) {
                f(i) = fluxArray[j];
            } else {
                f(i) = 0.;
            }

            sigma(i) = itsNoiseLevel;
            curpos(0) = x;
            curpos(1) = y;
            pos.row(i) = curpos;
        }
    }

    return fitGauss(pos, f, sigma);
}

//**************************************************************//

Fitter RadioSource::fitGauss(int nGauss,
                             std::vector<SubComponent> &estimateList,
                             casa::Matrix<casa::Double> &pos,
                             casa::Vector<casa::Double> &f,
                             casa::Vector<casa::Double> &sigma)
{
    Fitter newfit(itsFitParams);
    newfit.setNumGauss(nGauss);
    newfit.setEstimates(estimateList);
    newfit.setRetries();
    newfit.setMasks();
    newfit.fit(pos, f, sigma);
    return newfit;
}


bool RadioSource::fitGauss(casa::Matrix<casa::Double> &pos,
                           casa::Vector<casa::Double> &f,
                           casa::Vector<casa::Double> &sigma)
{

    ASKAPLOG_INFO_STR(logger, "Fitting source " << this->name <<
                      " at RA=" << this->raS << ", Dec=" << this->decS <<
                      ", or global position (x,y)=(" <<
                      this->getXcentre() + this->getXOffset() << "," <<
                      this->getYcentre() + this->getYOffset() << ")");

    if (this->getSpatialSize() < itsFitParams.minFitSize()) {
        ASKAPLOG_INFO_STR(logger, "Not fitting- source is too small - " <<
                          "spatial size = " << this->getSpatialSize() <<
                          " cf. minFitSize = " << itsFitParams.minFitSize());
        return false;
    }

    itsFitParams.saveBox(itsBox);
    itsFitParams.setPeakFlux(this->peakFlux);
    itsFitParams.setDetectThresh(itsDetectionThreshold);
    if (itsHeader.beam().min() > 0) {
        itsFitParams.setBeamSize(itsHeader.beam().min());
    } else {
        itsFitParams.setBeamSize(1.);
    }

    ASKAPLOG_DEBUG_STR(logger, "numSubThresh=" << itsFitParams.numSubThresholds());

    ASKAPLOG_INFO_STR(logger, "detect threshold = " << itsDetectionThreshold <<
                      ",  peak flux = " << this->peakFlux <<
                      ",  noise level = " << itsNoiseLevel);

    // Get the initial list of subcomponents
    std::vector<SubComponent> cmpntListReference = this->getSubComponentList(pos, f);
    ASKAPLOG_DEBUG_STR(logger, "Found " << cmpntListReference.size() << " subcomponents");

    for (uInt i = 0; i < cmpntListReference.size(); i++) {
        ASKAPLOG_DEBUG_STR(logger, "SubComponent: " << cmpntListReference[i]);
    }

    std::map<float, std::string> bestChisqMap; // map reduced-chisq to fitType

    std::vector<std::string>::iterator type;
    std::vector<std::string> typelist = availableFitTypes;

    for (type = typelist.begin(); type < typelist.end(); type++) {
        if (itsFitParams.hasType(*type)) {
            ASKAPLOG_INFO_STR(logger, "Commencing fits of type \"" << *type << "\"");
            itsFitParams.setFlagFitThisParam(*type);

            std::vector<SubComponent> cmpntList(cmpntListReference);

            // For any subcomponent that is smaller than the beam
            // (when comparing major axes), set its size to the beam
            // size. Always do this when fitting "psf" type.
            for (size_t i = 0; i < cmpntList.size(); i++) {
                cmpntList[i].fixSize(*type, itsHeader);
            }

            int ctr = 0;
            std::vector<Fitter> fit;
            int bestFit = -1;
            float bestRChisq = -1.;

            unsigned int minGauss, maxGauss;
            if (itsFitParams.numGaussFromGuess()) {
                minGauss = cmpntList.size();
                maxGauss = cmpntList.size();
                // maxGauss = 10;
                // if (minGauss > maxGauss){
                //     ASKAPLOG_WARN_STR(logger, "Island has " << minGauss << " subcomponents, which is too many. Not running fitting!");
                // }
            } else {
                minGauss = 1;
                maxGauss = std::min(size_t(itsFitParams.maxNumGauss()), f.size());
            }

            bool fitPossible = true;
            bool stopNow = false;
            std::vector<unsigned int> numGaussList;
            for (unsigned int g = minGauss; g <= maxGauss; g++) {
                numGaussList.push_back(g);
            }
//            for (unsigned int g = minGauss; g <= maxGauss && fitPossible && !stopNow; g++) {
            for (size_t ig = 0; ig < numGaussList.size() && !stopNow; ig++) {
                unsigned int g = numGaussList[ig];
                ASKAPLOG_DEBUG_STR(logger, "Number of Gaussian components = " << g);

                fit.push_back(fitGauss(g, cmpntList, pos, f, sigma));
                fitPossible = fit[ctr].fitExists();
                bool acceptable = fit[ctr].acceptable();
                bool okExceptChisq = fit[ctr].acceptableExceptChisq();

                if (!fit[ctr].passConverged() || !okExceptChisq) {
                    if (g > 1) {
                        numGaussList.push_back(g - 1);
                    }
                }

                if (fitPossible && okExceptChisq) {
                    if ((bestRChisq<0.) || (fit[ctr].redChisq() < bestRChisq)) {
                        bestFit = ctr;
                        bestRChisq = fit[ctr].redChisq();
                    }

                    if (!acceptable) {
                        // if we didn't pass the chi-squared test, but
                        // the fit is otherwise good

                        if (itsFitParams.numGaussFromGuess() &&
                                (fit[ctr].ndof() > 0) && (fit[ctr].passConverged())) {
                            // If we are just going on the number of
                            // Gaussians from the initial estimate, and
                            // the fit failed, we subtract the fit result
                            // and search again for an estimate, adding
                            // the brightest component to the list and
                            // re-doing. But only if that brightest
                            // component is brighter than the noise.

                            bool alreadyDone = false;
                            for (size_t i = 0; i < numGaussList.size() && !alreadyDone; i++) {
                                alreadyDone = numGaussList[i] == (g + 1);
                            }
                            if (!alreadyDone) {

                                ASKAPLOG_DEBUG_STR(logger, "Removing fitted Gaussian from array");
                                casa::Vector<casa::Double> newf = fit[ctr].subtractFit(pos, f);
                                ASKAPLOG_DEBUG_STR(logger, "Finding new subcomponents");
                                std::vector<SubComponent> newList=cmpntList;
                                std::vector<SubComponent> newGuessList =
                                    this->getSubComponentList(pos, newf);

                                if (newGuessList[0].peak() > itsDetectionThreshold) {
                                    newGuessList[0].fixSize(*type, itsHeader);
                                    ASKAPLOG_DEBUG_STR(logger, "Adding new subcomponent " <<
                                                       newGuessList[0]);
                                    newList.push_back(newGuessList[0]);
                                    cmpntList = newList;
                                    numGaussList.push_back(g + 1);
                                }

                            }
                        }
                    }

                }

                stopNow = itsFitParams.stopAfterFirstGoodFit() && acceptable;
                ctr++;

            } // end of 'g' for-loop
            ASKAPLOG_DEBUG_STR(logger, "Finished loop over Gaussians");
            
            if (bestFit >= 0) {
                itsFlagHasFit = true;

                itsBestFitMap[*type].saveResults(fit[bestFit]);

                bestChisqMap.insert(std::pair<float,
                                    std::string>(fit[bestFit].redChisq(), *type));
            }
        }
    } // end of type for-loop

    if (itsFlagHasFit) {

        itsBestFitType = bestChisqMap.begin()->second;
        itsBestFitMap["best"] = itsBestFitMap[itsBestFitType];

        ASKAPLOG_INFO_STR(logger, "BEST FIT: " <<
                          itsBestFitMap["best"].numGauss() << " Gaussians" <<
                          " with fit type \"" << bestChisqMap.begin()->second <<
                          "\", chisq = " << itsBestFitMap["best"].chisq() <<
                          ", chisq/nu =  "  << itsBestFitMap["best"].redchisq() <<
                          ", RMS = " << itsBestFitMap["best"].RMS());
        itsBestFitMap["best"].logIt("INFO");

    } else {
        itsFlagHasFit = false;
        if (itsFitParams.useGuessIfBad()) {
            ASKAPLOG_INFO_STR(logger, "Fits failed, so saving initial estimate (" <<
                              cmpntListReference.size() << " components) as solution");
            itsBestFitType = "guess";
            // set the components to be at least as big as the beam
            for (size_t i = 0; i < cmpntListReference.size(); i++) {
                casa::Gaussian2D<casa::Double> gauss = cmpntListReference[i].asGauss();
                if (cmpntListReference[i].maj() < itsHeader.beam().maj()) {
                    cmpntListReference[i].setMajor(itsHeader.beam().maj());
                    cmpntListReference[i].setMinor(itsHeader.beam().min());
                    cmpntListReference[i].setPA(itsHeader.beam().pa()*M_PI / 180.);
                } else {
                    cmpntListReference[i].setMinor(std::max(cmpntListReference[i].min(),
                                                            double(itsHeader.beam().min())));
                }
            }
            FitResults guess;
            guess.saveGuess(cmpntListReference);
            itsBestFitMap["guess"] = guess;
            itsBestFitMap["best"] = guess;
            for (type = typelist.begin(); type < typelist.end(); type++) {
                if (itsFitParams.hasType(*type)) {
                    itsBestFitMap[*type] = guess;
                }
            }
            ASKAPLOG_INFO_STR(logger,
                              "No good fit found, so saving initial guess as the fit result");
            itsBestFitMap["best"].logIt("INFO");
        } else {
            ASKAPLOG_INFO_STR(logger, "No good fit found.");
        }
    }

    ASKAPLOG_INFO_STR(logger, "-----------------------");
    return itsFlagHasFit;
}

//**************************************************************//

void RadioSource::findSpectralTerm(std::string imageName, int term, bool doCalc)
{

    std::string termtype[3] = {"", "spectral index", "spectral curvature"};

    ASKAPCHECK(term == 1 || term == 2,
               "Term number (" << term <<
               ") must be either 1 (for spectral index) or 2 (for spectral curvature)");


    if (!doCalc) {

        std::vector<std::string>::iterator type;
        std::vector<std::string> typelist = availableFitTypes;
        typelist.push_back("best");

        for (type = typelist.begin(); type < typelist.end(); type++) {
            int nfits = itsBestFitMap[*type].numFits();
            if (term == 1) {
                itsAlphaMap[*type] = std::vector<double>(nfits, defaultAlpha);
                itsAlphaError[*type] = std::vector<double>(nfits, 0.);
            } else if (term == 2) {
                itsBetaMap[*type] = std::vector<double>(nfits, defaultBeta);
                itsBetaError[*type] = std::vector<double>(nfits, 0.);
            }
        }



    } else {
        ASKAPLOG_DEBUG_STR(logger,
                           "About to find the " << termtype[term] <<
                           ", for image " << imageName);

        // Get taylor1 values for box, and define positions
        Slice xrange = casa::Slice(this->boxXmin() + this->getXOffset(),
                                   this->boxXmax() - this->boxXmin() + 1, 1);
        Slice yrange = casa::Slice(this->boxYmin() + this->getYOffset(),
                                   this->boxYmax() - this->boxYmin() + 1, 1);
        Slicer theBox = casa::Slicer(xrange, yrange);

        // casa::Array<casa::Float> flux_all = getPixelsInBox(imageName, theBox);
        casa::MaskedArray<casa::Float> flux_all = getPixelsInBox(imageName, theBox);

        std::vector<double> fluxvec;
        for (size_t i = 0; i < flux_all.size(); i++) {
            if (!isnan(flux_all.getArray().data()[i])) {
                fluxvec.push_back(flux_all.getArray().data()[i]);
            }
        }
        casa::Matrix<casa::Double> pos;
        casa::Vector<casa::Double> sigma;
        pos.resize(fluxvec.size(), 2);
        sigma.resize(fluxvec.size());
        casa::Vector<casa::Double> curpos(2);
        curpos = 0;

        // The following checks for pixels that have been blanked, and
        // ignores them
        int counter = 0;
        for (size_t i = 0; i < flux_all.size(); i++) {
            if (flux_all.getMask().data()[i]) {
                sigma(counter) = 1;
                curpos(0) = i % this->boxXsize() + this->boxXmin();
                curpos(1) = i / this->boxXsize() + this->boxYmin();
                pos.row(counter) = curpos;
                counter++;
            }
        }
        casa::Vector<casa::Double> f(fluxvec);

        // Set up fit with same parameters and do the fit
        std::vector<std::string>::iterator type;
        std::vector<std::string> typelist = availableFitTypes;

        for (type = typelist.begin(); type < typelist.end(); type++) {

            std::vector<double> termValues(itsBestFitMap[*type].numGauss(), 0.);
            std::vector<double> termErrors(itsBestFitMap[*type].numGauss(), 0.);

            if (itsBestFitMap[*type].fitExists() || itsBestFitMap[*type].fitIsGuess()) {

                ASKAPLOG_DEBUG_STR(logger, "Finding " << termtype[term] <<
                                   " values for fit type \"" << *type <<
                                   "\", with " << itsBestFitMap[*type].numGauss() <<
                                   " components ");

                std::vector<SubComponent> cmpnts = itsBestFitMap[*type].getCmpntList();
                itsFitParams.setFlagFitThisParam("height");
                itsFitParams.setNegativeFluxPossible(true);
                Fitter fit = fitGauss(itsBestFitMap[*type].numGauss(),
                                      cmpnts, pos, f, sigma);

                // Calculate taylor term value

                if (fit.fitExists() && fit.passConverged() && fit.passChisq()) {
                    // the fit is OK
                    ASKAPLOG_DEBUG_STR(logger,
                                       "Values for " << termtype[term] << " follow " <<
                                       "(" << itsBestFitMap[*type].numGauss() << " of them):");

                    for (unsigned int i = 0; i < itsBestFitMap[*type].numGauss(); i++) {
                        double Iref = itsBestFitMap[*type].gaussian(i).flux();
                        double Iref_err = itsBestFitMap[*type].errors(i)[0];
                        if (term == 1) {
                            termValues[i] = fit.gaussian(i).flux() / Iref;
                            termErrors[i] = abs(termValues[i]) *
                                            sqrt(Iref_err * Iref_err / (Iref * Iref) +
                                                 fit.error(i)[0] * fit.error(i)[0] / (fit.gaussian(i).flux() * fit.gaussian(i).flux()));
                        } else if (term == 2) {
                            double alpha = itsAlphaMap[*type][i];
                            double alpha_err = itsAlphaError[*type][i];
                            termValues[i] = fit.gaussian(i).flux() / Iref -
                                            0.5 * alpha * (alpha - 1.);
                            termErrors[i] = sqrt(fit.error(i)[0] * fit.error(i)[0] / (Iref * Iref) +
                                                 fit.error(i)[0] * fit.error(i)[0] * fit.gaussian(i).flux() * fit.gaussian(i).flux() / (Iref * Iref * Iref * Iref) +
                                                 (0.5 - alpha) * (0.5 - alpha) * alpha_err * alpha_err);
                        }
                        ASKAPLOG_INFO_STR(logger,
                                           "   Component " << i << ": " << termValues[i] <<
                                           " +- " << termErrors[i] <<
                                           ", calculated with fitted flux of " <<
                                           fit.gaussian(i).flux() <<
                                           ", peaking at " << fit.gaussian(i).height() <<
                                           ", best fit taylor0 flux of " << Iref);
                    }
                }

            }

            if (term == 1) {
                itsAlphaMap[*type] = termValues;
                itsAlphaError[*type] = termErrors;
            } else if (term == 2) {
                itsBetaMap[*type] = termValues;
                itsBetaError[*type] = termErrors;
            }
        }

        ASKAPLOG_DEBUG_STR(logger, "Finished finding the " << termtype[term] << " values");

    }

    if (term == 1) {
        itsAlphaMap["best"] = itsAlphaMap[itsBestFitType];
        itsAlphaError["best"] = itsAlphaError[itsBestFitType];
    } else if (term == 2) {
        itsBetaMap["best"] = itsBetaMap[itsBestFitType];
        itsBetaError["best"] = itsBetaError[itsBestFitType];
    }

}


//**************************************************************//

void RadioSource::extractSpectralTerms(LOFAR::ParameterSet &parset)
{

    // Define the parameter set, and
    LOFAR::ParameterSet spectralTermSubset = parset.makeSubset("spectralTerms.");
    // get the number of terms to fit
    int nterms = spectralTermSubset.getUint("nterms", 3);
    // get the peak SNR threshold above which we do the fitting.
    float thresholdForFit = spectralTermSubset.getFloat("snrThreshold", 0.);

    std::vector<std::string>::iterator type;
    std::vector<std::string> typelist = availableFitTypes;
    typelist.push_back("best");

    for (type = typelist.begin(); type < typelist.end(); type++) {
        int nfits = itsBestFitMap[*type].numFits();
        if (nterms > 1) {
            itsAlphaMap[*type] = std::vector<double>(nfits, defaultAlpha);
            itsAlphaError[*type] = std::vector<double>(nfits, 0.);
        }
        if (nterms > 2) {
            itsBetaMap[*type] = std::vector<double>(nfits, defaultBeta);
            itsBetaError[*type] = std::vector<double>(nfits, 0.);
        }
    }

    // Loop over fit types  - ie. the set of different component catalogues
    for (type = typelist.begin(); type < typelist.end(); type++) {

        if (itsBestFitMap[*type].isGood() || itsBestFitMap[*type].fitIsGuess()) {

            ASKAPLOG_DEBUG_STR(logger, "Extracting spectral index & curvature values for fit type \"" << *type <<
                               "\", with " << itsBestFitMap[*type].numGauss() << " components ");

            for (unsigned int i = 0; i < itsBestFitMap[*type].numGauss(); i++) {

                // make Component
                ASKAPLOG_DEBUG_STR(logger, "Making component for ID " << this->name << ", fit #" << i);
                CasdaComponent component(*this, parset, i, *type);

                // Only run the fit for things above the SNR threshold
                if ((itsBestFitMap[*type].gaussian(i).height() / itsNoiseLevel) > thresholdForFit) {

                    // make StokesSpectrum
                    ASKAPLOG_DEBUG_STR(logger, "Making Stokes Spectrum");
                    StokesSpectrum spectrum(spectralTermSubset, "I");
                    ASKAPLOG_DEBUG_STR(logger, "Setting component");
                    spectrum.setComponent(&component);
                    ASKAPLOG_DEBUG_STR(logger, "Extracting");
                    spectrum.extract();

                    // initialise StokesImodel
                    LOFAR::ParameterSet modelParset;
                    modelParset.add(LOFAR::KVpair("modelType", "taylor"));
                    modelParset.add(LOFAR::KVpair("recomputeAlphaBeta", true));
                    modelParset.add(LOFAR::KVpair("taylor.nterms", nterms));
                    ASKAPLOG_DEBUG_STR(logger, "Making StokesImodel");
                    StokesImodel model(modelParset);
                    ASKAPLOG_DEBUG_STR(logger, "Initialising");
                    model.initialise(spectrum, &component);

                    // get coefficients as alpha & beta, and their errors
                    if (nterms > 1) {
                        itsAlphaMap[*type][i] = model.coeff(1);
                        itsAlphaError[*type][i] = model.coeffErr(1);
                    }
                    if (nterms > 2) {
                        itsBetaMap[*type][i] = model.coeff(2);
                        itsBetaError[*type][i] = model.coeffErr(2);
                    }

                }
            }

        }

    }

}


//**************************************************************//

void RadioSource::printTableRow(std::ostream &stream,
                                duchamp::Catalogues::CatalogueSpecification columns,
                                size_t fitNum,
                                std::string fitType)
{

    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i), fitNum, fitType);
    }
    stream << "\n";

}

//**************************************************************//

casa::Unit getUnit(duchamp::Catalogues::Column &column)
{
    std::string desiredUnitsStr = column.getUnits();
    if (desiredUnitsStr[0] == '[') {
        // may have units in square brackets, eg. Jy/beam
        desiredUnitsStr = desiredUnitsStr.substr(1, desiredUnitsStr.size() - 2);
    }
    casa::Unit desiredUnits(desiredUnitsStr);
    return desiredUnits;

}

void RadioSource::printTableEntry(std::ostream &stream,
                                  duchamp::Catalogues::Column column,
                                  size_t fitNum,
                                  std::string fitType)
{

    // check that we are requesting a valid fit number
    ASKAPCHECK(fitNum < itsBestFitMap[fitType].numFits(),
               "fitNum=" << fitNum << ", but source " << this->getID() <<
               " only has " << itsBestFitMap[fitType].numFits() <<
               " fits for type " << fitType);

    // Define local variables that will get printed
    FitResults results = itsBestFitMap[fitType];
    casa::Gaussian2D<Double> gauss = itsBestFitMap[fitType].gaussian(fitNum);
    std::stringstream id;
    id << this->getID() << getSuffix(fitNum);
    std::vector<Double> deconv = deconvolveGaussian(gauss, itsHeader.getBeam());

    double thisRA, thisDec, zworld;
    itsHeader.pixToWCS(gauss.xCenter(), gauss.yCenter(), this->getZcentre(),
                       thisRA, thisDec, zworld);

    int lng = itsHeader.WCS().lng;
    int precision = -int(log10(fabs(itsHeader.WCS().cdelt[lng] * 3600. / 10.)));
    float pixscale = itsHeader.getAvPixScale() * 3600.; // convert from pixels to arcsec
    std::string raS  = decToDMS(thisRA, itsHeader.lngtype(), precision);
    std::string decS = decToDMS(thisDec, itsHeader.lattype(), precision);
    std::string name = itsHeader.getIAUName(thisRA, thisDec);
    float intfluxfit = gauss.flux();
    if (itsHeader.needBeamSize()) {
        intfluxfit /= itsHeader.beam().area(); // Convert from Jy/beam to Jy
    }
    double alpha = itsAlphaMap[fitType][fitNum];
    double beta = itsBetaMap[fitType][fitNum];
    std::string blankComment = "--";
    int flagGuess = results.fitIsGuess() ? 1 : 0;
    int flagSiblings = itsBestFitMap[fitType].numFits() > 1 ? 1 : 0;

    casa::Unit fluxUnits(itsHeader.getFluxUnits());
    casa::Unit intFluxUnits(itsHeader.getIntFluxUnits());

    std::string type = column.type();
    if (type == "ISLAND") {
        column.printEntry(stream, this->getID());
    } else if (type == "NUM") {
        column.printEntry(stream, id.str());
    } else if (type == "NAME") {
        column.printEntry(stream, name);
    } else if (type == "RA") {
        column.printEntry(stream, raS);
    } else if (type == "DEC") {
        column.printEntry(stream, decS);
    } else if (type == "RAJD") {
        column.printEntry(stream, thisRA);
    } else if (type == "DECJD") {
        column.printEntry(stream, thisDec);
    } else if (type == "RAERR") {
        column.printEntry(stream, 0.);
    } else if (type == "DECERR") {
        column.printEntry(stream, 0.);
    } else if (type == "X") {
        column.printEntry(stream, gauss.xCenter());
    } else if (type == "Y") {
        column.printEntry(stream, gauss.yCenter());
    } else if (type == "FINT") {
        double fluxscale = casa::Quantity(1., intFluxUnits).getValue(getUnit(column));
        column.printEntry(stream, this->getIntegFlux()*fluxscale);
    } else if (type == "FPEAK") {
        double fluxscale = casa::Quantity(1., fluxUnits).getValue(getUnit(column));
        column.printEntry(stream, this->getPeakFlux()*fluxscale);
    } else if (type == "FINTFIT") {
        double fluxscale = casa::Quantity(1., intFluxUnits).getValue(getUnit(column));
        column.printEntry(stream, intfluxfit * fluxscale);
    } else if (type == "FINTFITERR") {
        double fluxscale = casa::Quantity(1., intFluxUnits).getValue(getUnit(column));
        column.printEntry(stream, 0.*fluxscale);
    } else if (type == "FPEAKFIT") {
        double fluxscale = casa::Quantity(1., fluxUnits).getValue(getUnit(column));
        column.printEntry(stream, gauss.height()*fluxscale);
    } else if (type == "FPEAKFITERR") {
        double fluxscale = casa::Quantity(1., fluxUnits).getValue(getUnit(column));
        column.printEntry(stream, 0.*fluxscale);
    } else if (type == "MAJFIT") {
        column.printEntry(stream, gauss.majorAxis()*pixscale);
    } else if (type == "MINFIT") {
        column.printEntry(stream, gauss.minorAxis()*pixscale);
    } else if (type == "PAFIT") {
        column.printEntry(stream, gauss.PA() * 180. / M_PI);
    } else if (type == "MAJERR") {
        column.printEntry(stream, 0.);
    } else if (type == "MINERR") {
        column.printEntry(stream, 0.);
    } else if (type == "PAERR") {
        column.printEntry(stream, 0.);
    } else if (type == "MAJDECONV") {
        column.printEntry(stream, deconv[0]*pixscale);
    } else if (type == "MINDECONV") {
        column.printEntry(stream, deconv[1]*pixscale);
    } else if (type == "PADECONV") {
        column.printEntry(stream, deconv[2] * 180. / M_PI);
    } else if (type == "ALPHA") {
        column.printEntry(stream, alpha);
    } else if (type == "BETA") {
        column.printEntry(stream, beta);
    } else if (type == "CHISQFIT") {
        column.printEntry(stream, results.chisq());
    } else if (type == "RMSIMAGE") {
        double fluxscale = casa::Quantity(1., fluxUnits).getValue(getUnit(column));
        column.printEntry(stream, itsNoiseLevel * fluxscale);
    } else if (type == "RMSFIT") {
        double fluxscale = casa::Quantity(1., fluxUnits).getValue(getUnit(column));
        column.printEntry(stream, results.RMS()*fluxscale);
    } else if (type == "NFREEFIT") {
        column.printEntry(stream, results.numFreeParam());
    } else if (type == "NDOFFIT") {
        column.printEntry(stream, results.ndof());
    } else if (type == "NPIXFIT") {
        column.printEntry(stream, results.numPix());
    } else if (type == "NPIXOBJ") {
        column.printEntry(stream, this->getSize());
    } else if (type == "GUESS") {
        column.printEntry(stream, flagGuess);
    } else if (type == "FLAG1") {
        column.printEntry(stream, flagSiblings);
    } else if (type == "FLAG2") {
        column.printEntry(stream, flagGuess);
    } else if (type == "FLAG3") {
        column.printEntry(stream, 0);
    } else if (type == "FLAG4") {
        column.printEntry(stream, 0);
    } else if (type == "COMMENT") {
        column.printEntry(stream, blankComment);
    } else {
        // handles anything covered by duchamp code. If different column,
        // use the following.
        this->duchamp::Detection::printTableEntry(stream, column);
    }
}

//**************************************************************//

void
RadioSource::writeFitToAnnotationFile(boost::shared_ptr<duchamp::AnnotationWriter> &writer,
                                      int sourceNum,
                                      bool doEllipse,
                                      bool doBox)
{

    std::stringstream ss;
    ss << "# Source " << sourceNum << ":";
    writer->writeCommentString(ss.str());

    std::vector<double> pix(12);
    std::vector<double> world(12);

    for (int i = 0; i < 4; i++) {
        // set z-pixel values to zero
        pix[i * 3 + 2] = 0.;
    }

    std::vector<casa::Gaussian2D<Double> > fitSet = itsBestFitMap["best"].fitSet();
    std::vector<casa::Gaussian2D<Double> >::iterator fit;

    float pixscale = itsHeader.getAvPixScale();
    if (doEllipse) {
        for (fit = fitSet.begin(); fit < fitSet.end(); fit++) {
            pix[0] = fit->xCenter();
            pix[1] = fit->yCenter();
            itsHeader.pixToWCS(pix.data(), world.data());

            writer->ellipse(world[0],
                            world[1],
                            fit->majorAxis() * pixscale / 2.,
                            fit->minorAxis() * pixscale / 2.,
                            fit->PA() * 180. / M_PI);
        }
    }

    if (doBox) {
        pix[0] = pix[9] = this->getXmin() - itsFitParams.boxPadSize() - 0.5;
        pix[1] = pix[4] = this->getYmin() - itsFitParams.boxPadSize() - 0.5;
        pix[3] = pix[6] = this->getXmax() + itsFitParams.boxPadSize() + 0.5;
        pix[7] = pix[10] = this->getYmax() + itsFitParams.boxPadSize() + 0.5;
        itsHeader.pixToWCS(pix.data(), world.data(), 4);

        std::vector<double> x, y;
        for (int i = 0; i <= 4; i++) {
            x.push_back(world[(i % 4) * 3]);
            y.push_back(world[(i % 4) * 3 + 1]);
        }
        writer->joinTheDots(x, y);
    }

}

//**************************************************************//

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& blob, RadioSource& src)
{
    int32 l;
    int i;
    float f;
    double d;
    std::string s;
    bool b;
    int size = src.getSize();
    blob << size;
    std::vector<PixelInfo::Voxel> pixelSet = src.getPixelSet();

    for (i = 0; i < size; i++) {
        l = pixelSet[i].getX(); blob << l;
        l = pixelSet[i].getY(); blob << l;
        l = pixelSet[i].getZ(); blob << l;
    }

    l = src.xSubOffset; blob << l;
    l = src.ySubOffset; blob << l;
    l = src.zSubOffset; blob << l;
    b = src.haveParams; blob << b;
    f = src.totalFlux;  blob << f;
    f = src.intFlux;    blob << f;
    f = src.peakFlux;   blob << f;
    l = src.xpeak;      blob << l;
    l = src.ypeak;      blob << l;
    l = src.zpeak;      blob << l;
    f = src.peakSNR;    blob << f;
    f = src.xCentroid;  blob << f;
    f = src.yCentroid;  blob << f;
    f = src.zCentroid;  blob << f;
    s = src.centreType; blob << s;
    b = src.negSource;  blob << b;
    s = src.flagText;   blob << s;
    i = src.id;         blob << i;
    s = src.name;       blob << s;
    b = src.flagWCS;    blob << b;
    s = src.raS;        blob << s;
    s = src.decS;       blob << s;
    d = src.ra;         blob << d;
    d = src.dec;        blob << d;
    d = src.raWidth;    blob << d;
    d = src.decWidth;   blob << d;
    d = src.majorAxis;  blob << d;
    d = src.minorAxis;  blob << d;
    d = src.posang;     blob << d;
    b = src.specOK;     blob << b;
    s = src.specUnits;  blob << s;
    s = src.specType;   blob << s;
    s = src.fluxUnits;  blob << s;
    s = src.intFluxUnits; blob << s;
    s = src.lngtype;    blob << s;
    s = src.lattype;    blob << s;
    d = src.vel;        blob << d;
    d = src.velWidth;   blob << d;
    d = src.velMin;     blob << d;
    d = src.velMax;     blob << d;
    d = src.v20min;     blob << d;
    d = src.v20max;     blob << d;
    d = src.w20;        blob << d;
    d = src.v50min;     blob << d;
    d = src.v50max;     blob << d;
    d = src.w50;        blob << d;
    i = src.posPrec;    blob << i;
    i = src.xyzPrec;    blob << i;
    i = src.fintPrec;   blob << i;
    i = src.fpeakPrec;  blob << i;
    i = src.velPrec;    blob << i;
    i = src.snrPrec;    blob << i;
    b = src.itsFlagHasFit;     blob << b;
    b = src.itsFlagAtEdge;     blob << b;
    f = src.itsDetectionThreshold; blob << f;
    f = src.itsNoiseLevel; blob << f;
    blob << src.itsFitParams;
    size = src.itsBestFitMap.size();
    blob << size;
    std::map<std::string, FitResults>::iterator fit;

    for (fit = src.itsBestFitMap.begin(); fit != src.itsBestFitMap.end(); fit++) {
        blob << fit->first;
        blob << fit->second;
    }

    std::map<std::string, std::vector<double> >::iterator val;
    size = src.itsAlphaMap.size();
    blob << size;
    for (val = src.itsAlphaMap.begin(); val != src.itsAlphaMap.end(); val++) {
        blob << val->first;
        size = val->second.size();
        blob << size;

        for (int i = 0; i < size; i++) blob << val->second[i];
    }

    size = src.itsAlphaError.size();
    blob << size;
    for (val = src.itsAlphaError.begin(); val != src.itsAlphaError.end(); val++) {
        blob << val->first;
        size = val->second.size();
        blob << size;

        for (int i = 0; i < size; i++) blob << val->second[i];
    }

    size = src.itsBetaMap.size();
    blob << size;
    for (val = src.itsBetaMap.begin(); val != src.itsBetaMap.end(); val++) {
        blob << val->first;
        size = val->second.size();
        blob << size;

        for (int i = 0; i < size; i++) blob << val->second[i];
    }

    size = src.itsBetaError.size();
    blob << size;
    for (val = src.itsBetaError.begin(); val != src.itsBetaError.end(); val++) {
        blob << val->first;
        size = val->second.size();
        blob << size;

        for (int i = 0; i < size; i++) blob << val->second[i];
    }

    i = src.box().ndim(); blob << i;
    i = src.box().start()[0]; blob << i;
    i = src.box().start()[1]; blob << i;
    if (src.box().ndim() > 2) {
        i = src.box().start()[2]; blob << i;
    }
    i = src.box().end()[0]; blob << i;
    i = src.box().end()[1]; blob << i;
    if (src.box().ndim() > 2) {
        i = src.box().end()[2]; blob << i;
    }

    return blob;
}

//**************************************************************//

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &blob, RadioSource& src)
{
    int i;
    int32 l;
    bool b;
    float f;
    double d;
    std::string s;
    int32 size;
    blob >> size;

    for (i = 0; i < size; i++) {
        int32 x, y, z;
        blob >> x;
        blob >> y;
        blob >> z;
        src.addPixel(x, y, z);
    }

    blob >> l; src.xSubOffset = l;
    blob >> l; src.ySubOffset = l;
    blob >> l; src.zSubOffset = l;
    blob >> b; src.haveParams = b;
    blob >> f; src.totalFlux = f;
    blob >> f; src.intFlux = f;
    blob >> f; src.peakFlux = f;
    blob >> l; src.xpeak = l;
    blob >> l; src.ypeak = l;
    blob >> l; src.zpeak = l;
    blob >> f; src.peakSNR = f;
    blob >> f; src.xCentroid = f;
    blob >> f; src.yCentroid = f;
    blob >> f; src.zCentroid = f;
    blob >> s; src.centreType = s;
    blob >> b; src.negSource = b;
    blob >> s; src.flagText = s;
    blob >> i; src.id = i;
    blob >> s; src.name = s;
    blob >> b; src.flagWCS = b;
    blob >> s; src.raS = s;
    blob >> s; src.decS = s;
    blob >> d; src.ra = d;
    blob >> d; src.dec = d;
    blob >> d; src.raWidth = d;
    blob >> d; src.decWidth = d;
    blob >> d; src.majorAxis = d;
    blob >> d; src.minorAxis = d;
    blob >> d; src.posang = d;
    blob >> b; src.specOK = b;
    blob >> s; src.specUnits = s;
    blob >> s; src.specType = s;
    blob >> s; src.fluxUnits = s;
    blob >> s; src.intFluxUnits = s;
    blob >> s; src.lngtype = s;
    blob >> s; src.lattype = s;
    blob >> d; src.vel = d;
    blob >> d; src.velWidth = d;
    blob >> d; src.velMin = d;
    blob >> d; src.velMax = d;
    blob >> d; src.v20min = d;
    blob >> d; src.v20max = d;
    blob >> d; src.w20 = d;
    blob >> d; src.v50min = d;
    blob >> d; src.v50max = d;
    blob >> d; src.w50 = d;
    blob >> i; src.posPrec = i;
    blob >> i; src.xyzPrec = i;
    blob >> i; src.fintPrec = i;
    blob >> i; src.fpeakPrec = i;
    blob >> i; src.velPrec = i;
    blob >> i; src.snrPrec = i;
    blob >> b; src.itsFlagHasFit = b;
    blob >> b; src.itsFlagAtEdge = b;
    blob >> f; src.itsDetectionThreshold = f;
    blob >> f; src.itsNoiseLevel = f;
    blob >> src.itsFitParams;
    blob >> size;

    for (int i = 0; i < size; i++) {
        FitResults res;
        blob >> s >> res;
        src.itsBestFitMap[s] = res;
    }

    blob >> size;

    for (int i = 0; i < size; i++) {
        int32 vecsize;
        blob >> s >> vecsize;
        std::vector<double> vec(vecsize);

        for (int i = 0; i < vecsize; i++) blob >> vec[i];

        src.itsAlphaMap[s] = vec;
    }

    blob >> size;

    for (int i = 0; i < size; i++) {
        int32 vecsize;
        blob >> s >> vecsize;
        std::vector<double> vec(vecsize);

        for (int i = 0; i < vecsize; i++) {
            blob >> vec[i];
        }

        src.itsAlphaError[s] = vec;
    }

    blob >> size;

    for (int i = 0; i < size; i++) {
        int32 vecsize;
        blob >> s >> vecsize;
        std::vector<double> vec(vecsize);

        for (int i = 0; i < vecsize; i++) blob >> vec[i];

        src.itsBetaMap[s] = vec;
    }

    blob >> size;

    for (int i = 0; i < size; i++) {
        int32 vecsize;
        blob >> s >> vecsize;
        std::vector<double> vec(vecsize);

        for (int i = 0; i < vecsize; i++) blob >> vec[i];

        src.itsBetaError[s] = vec;
    }

    int ndim, x1, y1, z1, x2, y2, z2;
    blob >> ndim >> x1 >> y1;
    if (ndim > 2) {
        blob >> z1;
    }
    blob >> x2 >> y2;
    if (ndim > 2) {
        blob >> z2;
    }
    casa::IPosition start(ndim), end(ndim), stride(ndim, 1);
    start(0) = x1; start(1) = y1;
    end(0) = x2; end(1) = y2;
    if (ndim > 2) {
        start(2) = z1;
        end(2) = z2;
    }
    ASKAPCHECK(end >= start,
               "Slicer in blob transfer of RadioSource - start " << start << " > end " << end);
    Slicer box(start, end, stride, Slicer::endIsLast);;
    src.setBox(box);

    return blob;
}



}

}

}

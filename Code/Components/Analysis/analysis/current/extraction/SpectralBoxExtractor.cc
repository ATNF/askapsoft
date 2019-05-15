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
#include <extraction/SpectralBoxExtractor.h>
#include <askap_analysis.h>
#include <extraction/SourceDataExtractor.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

#include <sourcefitting/RadioSource.h>

#include <askap/imageaccess/ImageAccessFactory.h>

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

ASKAP_LOGGER(logger, ".spectralboxextractor");

namespace askap {

namespace analysis {

SpectralBoxExtractor::SpectralBoxExtractor(const LOFAR::ParameterSet& parset):
    SourceDataExtractor(parset)
{

    itsBoxWidth = parset.getInt16("spectralBoxWidth", defaultSpectralExtractionBoxWidth);

    itsOutputFilenameBase = parset.getString("spectralOutputBase", "");
    ASKAPCHECK(itsOutputFilenameBase != "", "Extraction: " <<
               "No output base name has been provided for the spectral output. " <<
               "Use spectralOutputBase.");

}

void SpectralBoxExtractor::initialiseArray()
{
    // Form itsArray and initialise to zero
    if (this->openInput()) {
        int specsize = itsInputCubePtr->shape()(itsSpcAxis);
        casacore::IPosition shape(itsInputCubePtr->shape().size(), 1);
        shape(itsSpcAxis) = specsize;
        if(itsStkAxis>-1){
            shape(itsStkAxis) = itsStokesList.size();
        }
        itsArray = casacore::Array<Float>(shape, 0.0);
        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}

void SpectralBoxExtractor::defineSlicer()
{

    if (this->openInput()) {
        IPosition shape = itsInputCubePtr->shape();
        ASKAPLOG_DEBUG_STR(logger, "Shape from input cube = " << shape);
        ASKAPCHECK(itsInputCoords.hasSpectralAxis(),
                   "Input cube \"" << itsInputCube << "\" has no spectral axis");
        ASKAPCHECK(itsInputCoords.hasDirectionCoordinate(),
                   "Input cube \"" << itsInputCube << "\" has no spatial axes");

        // define the slicer based on the source's peak pixel location and the box width.
        // Make sure we don't go over the edges of the image.
        int xmin, ymin, xmax, ymax;
        if (itsBoxWidth > 0) {
            int hw = (itsBoxWidth - 1) / 2;
            int xloc = int(itsXloc);
            int yloc = int(itsYloc);
            int zero = 0;
            ASKAPLOG_DEBUG_STR(logger, "Problematic bit: xloc="<<xloc<<" yloc="<<yloc<<" hw="<<hw <<" shape(itsLngAxis)="<<shape(itsLngAxis) << " shape(itsLatAxis)="<<shape(itsLatAxis));
            xmin = std::max(zero, xloc - hw);
            xmax = std::min(int(shape(itsLngAxis) - 1), xloc + hw);
            ymin = std::max(zero, yloc - hw);
            ymax = std::min(int(shape(itsLatAxis) - 1), yloc + hw);
            ASKAPLOG_DEBUG_STR(logger, "Problematic bit 2: xmin="<<xmin<<" xmax="<< xmax<< " ymin="<<ymin <<" ymax="<<ymax);
        } else {
            ASKAPASSERT(itsSource);
            // use the detected pixels of the source for the spectral
            // extraction, and the x/y ranges for slicer
            xmin = itsSource->getXmin() + itsSource->getXOffset();
            xmax = itsSource->getXmax() + itsSource->getXOffset();
            ymin = itsSource->getYmin() + itsSource->getYOffset();
            ymax = itsSource->getYmax() + itsSource->getYOffset();
        }
        casacore::IPosition blc(shape.size(), 0), trc(shape.size(), 0);
        blc(itsLngAxis) = xmin;
        blc(itsLatAxis) = ymin;
        blc(itsSpcAxis) = 0;
        trc(itsLngAxis) = xmax;
        trc(itsLatAxis) = ymax;
        trc(itsSpcAxis) = shape(itsSpcAxis) - 1;
        if (itsStkAxis > -1) {
            casacore::Stokes stk;
            blc(itsStkAxis) = trc(itsStkAxis) =
                                  itsInputCoords.stokesPixelNumber(stk.name(itsCurrentStokes));
        }
        ASKAPLOG_DEBUG_STR(logger, "Defining slicer for " << itsInputCubePtr->name() <<
                           " based on blc=" << blc << ", trc=" << trc);
        itsSlicer = casacore::Slicer(blc, trc, casacore::Slicer::endIsLast);

        this->closeInput();
    } else {
        ASKAPLOG_ERROR_STR(logger, "Could not open image");
    }
}


void SpectralBoxExtractor::writeImage()
{
    ASKAPLOG_INFO_STR(logger, "Writing spectrum to " << itsOutputFilename);

    casacore::CoordinateSystem newcoo = casacore::CoordinateUtil::defaultCoords4D();

    int dirCoNum = itsInputCoords.findCoordinate(casacore::Coordinate::DIRECTION);
    int spcCoNum = itsInputCoords.findCoordinate(casacore::Coordinate::SPECTRAL);
    int stkCoNum = itsInputCoords.findCoordinate(casacore::Coordinate::STOKES);

    casacore::DirectionCoordinate dircoo(itsInputCoords.directionCoordinate(dirCoNum));
    casacore::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
    casacore::Vector<Int> stkvec(itsStokesList.size());
    for (size_t i = 0; i < stkvec.size(); i++) {
        stkvec[i] = itsStokesList[i];
    }
    casacore::StokesCoordinate stkcoo(stkvec);

    newcoo.replaceCoordinate(dircoo, newcoo.findCoordinate(casacore::Coordinate::DIRECTION));
    newcoo.replaceCoordinate(spcoo, newcoo.findCoordinate(casacore::Coordinate::SPECTRAL));
    if (stkCoNum >= 0) {
        newcoo.replaceCoordinate(stkcoo, newcoo.findCoordinate(casacore::Coordinate::STOKES));
    }

    // shift the reference pixel for the spatial coords, so that
    // the RA/DEC (or whatever) are correct. Leave the
    // spectral/stokes axes untouched.
    int lngAxis = newcoo.directionAxesNumbers()[0];
    int latAxis = newcoo.directionAxesNumbers()[1];
    int spcAxis = newcoo.spectralAxisNumber();
    int stkAxis = newcoo.polarizationAxisNumber();
    casacore::IPosition outshape(4, 1);
    outshape(spcAxis) = itsSlicer.length()(itsSpcAxis);
    outshape(stkAxis) = stkvec.size();
    casacore::Vector<Float> shift(outshape.size(), 0);
    casacore::Vector<Float> incrFrac(outshape.size(), 1);
    shift(lngAxis) = itsXloc;
    shift(latAxis) = itsYloc;
    casacore::Vector<Int> newshape = outshape.asVector();
    newcoo.subImageInSitu(shift, incrFrac, newshape);

    Array<Float> newarray(itsArray.reform(outshape));

    boost::shared_ptr<accessors::IImageAccess> ia = accessors::imageAccessFactory(itsParset);
    ia->create(itsOutputFilename, newarray.shape(), newcoo);

    /// @todo save the new units - if units were per beam, remove this factor
    
    // write the array
    ia->write(itsOutputFilename, newarray);
    ia->setUnits(itsOutputFilename, itsOutputUnits.getName());

    // update the metadata
    updateHeaders(itsOutputFilename);

}


casacore::Array<Float> SpectralBoxExtractor::frequencies()
{
    casacore::Vector<Float> freqs;
    if (this->openInput() && itsSpcAxis) {
        casacore::IPosition shape(itsInputCubePtr->shape());
        freqs=casacore::Vector<Float>(shape(itsSpcAxis),0.);
        int spcCoNum = itsInputCoords.findCoordinate(casacore::Coordinate::SPECTRAL);
        casacore::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
        for(unsigned int i=0;i<shape(itsSpcAxis);i++){
            Double pix=i;
            Double freq;
            ASKAPCHECK(spcoo.toWorld(freq,pix),
                       "WCS conversion failed in calculating frequencies");
            freqs[i]=freq;
        }
        this->closeInput();
    } else ASKAPLOG_ERROR_STR(logger, "Could not open image");

    return freqs;
}

std::string SpectralBoxExtractor::freqUnit()
{
    std::string unit;
    if (this->openInput() && itsSpcAxis) {
        int spcCoNum = itsInputCoords.findCoordinate(casacore::Coordinate::SPECTRAL);
        casacore::SpectralCoordinate spcoo(itsInputCoords.spectralCoordinate(spcCoNum));
        casacore::Vector<casacore::String> units=spcoo.worldAxisUnits();
        if(units.size()>1){
            ASKAPLOG_WARN_STR(logger, "Multiple units in spectral axis: " << units);
        }
        unit=units[0];
        closeInput();
    }
    return unit;
}

}

}

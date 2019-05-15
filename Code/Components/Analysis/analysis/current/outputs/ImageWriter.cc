/// @file ImageWriter.cc
///
/// @copyright (c) 2014 CSIRO
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

#include <askap_analysis.h>
#include <outputs/ImageWriter.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

#include <string>
#include <duchamp/Cubes/cubes.hh>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
// #include <casacore/images/Images/PagedImage.h>
#include <casacore/images/Images/ImageInfo.h>
// #include <casacore/images/Images/ImageOpener.h>
// #include <casacore/images/Images/FITSImage.h>
// #include <casacore/images/Images/MIRIADImage.h>
#include <Common/ParameterSet.h>

#include <casainterface/CasaInterface.h>
#include <casacore/casa/aipstype.h>

#include <askap/imageaccess/ImageAccessFactory.h>
///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".imagewriter");

namespace askap {

namespace analysis {

ImageWriter::ImageWriter(const LOFAR::ParameterSet &parset,duchamp::Cube *cube, std::string imageName):
    itsParset(parset)
{
    this->copyMetadata(cube);
    itsImageName = imageName;
}

void ImageWriter::copyMetadata(duchamp::Cube *cube)
{

    const boost::shared_ptr<ImageInterface<Float> > imagePtr =
        analysisutilities::openImage(cube->pars().getImageFile());

    itsCoordSys = imagePtr->coordinates();
    itsShape = imagePtr->shape();
    itsBunit = imagePtr->units();
    itsImageInfo = imagePtr->imageInfo();

    // set the default tileshape based on the overall image shape.
    // this can be changed later if preferred (for smaller subsection writing).
    this->setTileshapeFromShape(itsShape);

}

void ImageWriter::setTileshapeFromShape(casacore::IPosition &shape)
{

    int specAxis = itsCoordSys.spectralAxisNumber();
    int lngAxis = itsCoordSys.directionAxesNumbers()[0];
    int latAxis = itsCoordSys.directionAxesNumbers()[1];
    itsTileshape = casacore::IPosition(shape.size(), 1);
    itsTileshape(lngAxis) = std::min(128L, shape(lngAxis));
    itsTileshape(latAxis) = std::min(128L, shape(latAxis));
    if (itsCoordSys.hasSpectralAxis()) {
        itsTileshape(specAxis) = std::min(16L, shape(specAxis));
    }
}


void ImageWriter::create()
{
    if (itsImageName != "") {
        boost::shared_ptr<accessors::IImageAccess> imageAcc = accessors::imageAccessFactory(itsParset);
        imageAcc->create(itsImageName, itsShape, itsCoordSys);
        imageAcc->makeDefaultMask(itsImageName);
        imageAcc->setUnits(itsImageName, itsBunit.getName());
        // imageAcc->setImageInfo...
        casacore::Vector<casacore::Quantity> beam=itsImageInfo.restoringBeam().toVector();
        imageAcc->setBeamInfo(itsImageName, beam[0].getValue("rad"), beam[1].getValue("rad"), beam[2].getValue("rad"));
        if (itsParset.isDefined("imageHistory")) {
            std::vector<std::string> historyMessages = itsParset.getStringVector("imageHistory", "");
            if (historyMessages.size() > 0) {
                for (std::vector<std::string>::iterator history = historyMessages.begin();
                     history < historyMessages.end(); history++) {
                    imageAcc->addHistory(itsImageName, *history);
                }
            }
        }
    }
}


void ImageWriter::write(float *data, const casacore::IPosition &shape, bool accumulate)
{
    ASKAPASSERT(shape.size() == itsShape.size());
    casacore::Array<Float> arr(shape, data, casacore::SHARE);
    casacore::IPosition location(itsShape.size(), 0);
    this->write(arr, location, accumulate);
}

void ImageWriter::write(float *data, const casacore::IPosition &shape,
                        const casacore::IPosition &loc, bool accumulate)
{
    ASKAPASSERT(shape.size() == itsShape.size());
    ASKAPASSERT(loc.size() == itsShape.size());
    casacore::Array<Float> arr(shape, data, casacore::SHARE);
    this->write(arr, loc, accumulate);
}

void ImageWriter::write(const casacore::Array<Float> &data, bool accumulate)
{
    ASKAPASSERT(data.ndim() == itsShape.size());
    casacore::IPosition location(itsShape.size(), 0);
    this->write(data, location, accumulate);
}

void ImageWriter::write(const casacore::Array<Float> &data,
                        const casacore::IPosition &loc, bool accumulate)
{
    ASKAPASSERT(data.ndim() == itsShape.size());
    ASKAPASSERT(loc.size() == itsShape.size());
    boost::shared_ptr<accessors::IImageAccess> imageAcc = accessors::imageAccessFactory(itsParset);
    // ASKAPLOG_DEBUG_STR(logger,
    //                    "Writing array of shape " << data.shape() <<
    //                    " to image " << itsImageName <<
    //                    " at location " << loc);
    if (accumulate) {
        casacore::Array<casacore::Float> newdata = data + this->read(loc, data.shape());
        imageAcc->write(itsImageName, newdata, loc);
    } else {
        imageAcc->write(itsImageName, data, loc);
    }

}

void ImageWriter::writeMask(const casacore::Array<bool> &mask,
                            const casacore::IPosition &loc)
{
    ASKAPASSERT(mask.ndim() == itsShape.size());
    ASKAPASSERT(loc.size() == itsShape.size());
    boost::shared_ptr<accessors::IImageAccess> imageAcc = accessors::imageAccessFactory(itsParset);
    imageAcc->makeDefaultMask(itsImageName);
    imageAcc->writeMask(itsImageName, mask, loc);

}

casacore::Array<casacore::Float>
ImageWriter::read(const casacore::IPosition& loc, const casacore::IPosition &shape)
{
    ASKAPASSERT(loc.size() == shape.size());
    boost::shared_ptr<accessors::IImageAccess> imageAcc = accessors::imageAccessFactory(itsParset);
    casacore::IPosition trc = loc;
    trc += shape-1;
    // ASKAPLOG_DEBUG_STR(logger, "About to read from " << itsImageName <<" at loc="<<loc << " and shape="<<shape<<" which means trc="<<trc);
    return imageAcc->read(itsImageName, loc, trc);
}



}

}

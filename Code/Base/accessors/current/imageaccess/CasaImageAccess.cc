/// @file CasaImageAccess.cc
/// @brief Access casa image
/// @details This class implements IImageAccess interface for CASA image
///
/// @copyright (c) 2007 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>
///

#include <askap_accessors.h>

#include <imageaccess/CasaImageAccess.h>

#include <askap/AskapLogging.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/images/Regions/ImageRegion.h>
#include <casacore/images/Regions/RegionHandler.h>

ASKAP_LOGGER(logger, ".casaImageAccessor");

using namespace askap;
using namespace askap::accessors;

// reading methods

/// @brief obtain the shape
/// @param[in] name image name
/// @return full shape of the given image
casacore::IPosition CasaImageAccess::shape(const std::string &name) const
{
    casacore::PagedImage<float> img(name);
    return img.shape();
}

/// @brief read full image
/// @param[in] name image name
/// @return array with pixels
casacore::Array<float> CasaImageAccess::read(const std::string &name) const
{
    ASKAPLOG_INFO_STR(logger, "Reading CASA image " << name);
    casacore::PagedImage<float> img(name);
    if (img.hasPixelMask()) {
        ASKAPLOG_INFO_STR(logger, " - setting unmasked pixels to zero");
        // generate an Array of zeros and copy the elements for which the mask is true
        casacore::Array<float> tempArray(img.get().shape(), 0.0);
        tempArray = casacore::MaskedArray<float>(img.get(), img.getMask(), casacore::True);
        return tempArray;
        // The following seems to avoid a copy but takes longer:
        //// Iterate over image array and set any unmasked pixels to zero
        //casacore::Array<float> tempArray = img.get();
        //const casacore::LogicalArray tempMask = img.getMask();
        //casacore::Array<float>::iterator iterArray = tempArray.begin();
        //casacore::LogicalArray::const_iterator iterMask = tempMask.begin();
        //for( ; iterArray != tempArray.end() ; iterArray++ ) {
        //    if (*iterMask == casacore::False) {
        //        *iterArray = 0.0;
        //    }
        //    iterMask++;
        //}
        //return tempArray;
    } else {
        return img.get();
    }
}

/// @brief read part of the image
/// @param[in] name image name
/// @param[in] blc bottom left corner of the selection
/// @param[in] trc top right corner of the selection
/// @return array with pixels for the selection only
casacore::Array<float> CasaImageAccess::read(const std::string &name, const casacore::IPosition &blc,
        const casacore::IPosition &trc) const
{
    ASKAPLOG_INFO_STR(logger, "Reading a slice of the CASA image " << name << " from " << blc << " to " << trc);
    casacore::PagedImage<float> img(name);
    if (img.hasPixelMask()) {
        ASKAPLOG_INFO_STR(logger, " - setting unmasked pixels to zero");
        // generate an Array of zeros and copy the elements for which the mask is true
        const casacore::Slicer slicer(blc, trc, casacore::Slicer::endIsLast);
        casacore::Array<float> tempSlice(img.getSlice(slicer).shape(), 0.0);
        tempSlice = casacore::MaskedArray<float>(img.getSlice(slicer), img.getMaskSlice(slicer), casacore::True);
        return tempSlice;
        // The following seems to avoid a copy but takes longer:
        //// Iterate over image array and set any unmasked pixels to zero
        //const casacore::Slicer slicer(blc,trc,casacore::Slicer::endIsLast);
        //casacore::Array<float> tempSlice = img.getSlice(slicer);
        //const casacore::LogicalArray tempMask = img.getMaskSlice(slicer);
        //casacore::Array<float>::iterator iterSlice = tempSlice.begin();
        //casacore::LogicalArray::const_iterator iterMask = tempMask.begin();
        //for( ; iterSlice != tempSlice.end() ; iterSlice++ ) {
        //    if (*iterMask == casacore::False) {
        //        *iterSlice = 0.0;
        //    }
        //    iterMask++;
        //}
        //return tempSlice;
    } else {
        return img.getSlice(casacore::Slicer(blc, trc, casacore::Slicer::endIsLast));
    }
}

/// @brief obtain coordinate system info
/// @param[in] name image name
/// @return coordinate system object
casacore::CoordinateSystem CasaImageAccess::coordSys(const std::string &name) const
{
    casacore::PagedImage<float> img(name);
    return img.coordinates();
}
casacore::CoordinateSystem CasaImageAccess::coordSysSlice(const std::string &name, const casacore::IPosition &blc,
        const casacore::IPosition &trc) const
{
    casacore::Slicer slc(blc, trc, casacore::Slicer::endIsLast);
    ASKAPLOG_INFO_STR(logger, " CasaImageAccess - Slicer " << slc);
    casacore::PagedImage<float> img(name);
    casacore::SubImage<casacore::Float> si = casacore::SubImage<casacore::Float>(img, slc, casacore::AxesSpecifier(casacore::True));
    return si.coordinates();


}
/// @brief obtain beam info
/// @param[in] name image name
/// @return beam info vector
casacore::Vector<casacore::Quantum<double> > CasaImageAccess::beamInfo(const std::string &name) const
{
    casacore::PagedImage<float> img(name);
    casacore::ImageInfo ii = img.imageInfo();
    return ii.restoringBeam().toVector();
}

std::string CasaImageAccess::getUnits(const std::string &name) const
{
    casacore::Table tmpTable(name);
    std::string units = tmpTable.keywordSet().asString("units");
    return units;
}

/// @brief Get a particular keyword from the image metadata (A.K.A header)
/// @details This reads a given keyword to the image metadata.
/// @param[in] name Image name
/// @param[in] keyword The name of the metadata keyword
std::string CasaImageAccess::getMetadataKeyword(const std::string &name,
        const std::string &keyword) const
{

    casacore::PagedImage<float> img(name);
    casacore::TableRecord miscinfo = img.miscInfo();
    std::string value = "";
    if (miscinfo.isDefined(keyword)) {
        value = miscinfo.asString(keyword);
    } else {
        ASKAPLOG_WARN_STR(logger, "Keyword " << keyword << " is not defined in metadata for image " << name);
    }
    return value;

}


// writing methods

/// @brief create a new image
/// @details A call to this method should preceed any write calls. The actual
/// image may be created only upon the first write call. Details depend on the
/// implementation.
/// @param[in] name image name
/// @param[in] shape full shape of the image
/// @param[in] csys coordinate system of the full image
void CasaImageAccess::create(const std::string &name, const casacore::IPosition &shape,
                             const casacore::CoordinateSystem &csys)
{
    ASKAPLOG_INFO_STR(logger, "Creating a new CASA image " << name << " with the shape " << shape);
    casacore::PagedImage<float> img(casacore::TiledShape(shape), csys, name);
}

/// @brief write full image
/// @param[in] name image name
/// @param[in] arr array with pixels
void CasaImageAccess::write(const std::string &name, const casacore::Array<float> &arr)
{
    ASKAPLOG_INFO_STR(logger, "Writing an array with the shape " << arr.shape() << " into a CASA image " << name);
    casacore::PagedImage<float> img(name);
    img.put(arr);
}

/// @brief write a slice of an image
/// @param[in] name image name
/// @param[in] arr array with pixels
/// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
void CasaImageAccess::write(const std::string &name, const casacore::Array<float> &arr,
                            const casacore::IPosition &where)
{
    ASKAPLOG_INFO_STR(logger, "Writing a slice with the shape " << arr.shape() << " into a CASA image " <<
                      name << " at " << where);
    casacore::PagedImage<float> img(name);
    img.putSlice(arr, where);
}
/// @brief write a slice of an image mask
/// @param[in] name image name
/// @param[in] arr array with pixels
/// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
void CasaImageAccess::writeMask(const std::string &name, const casacore::Array<bool> &mask,
                                const casacore::IPosition &where)
{
    ASKAPLOG_INFO_STR(logger, "Writing a slice with the shape " << mask.shape() << " into a CASA image " <<
                      name << " at " << where);
    casacore::PagedImage<float> img(name);
    img.pixelMask().putSlice(mask, where);
}

/// @brief write a slice of an image mask
/// @param[in] name image name
/// @param[in] arr array with pixels

void CasaImageAccess::writeMask(const std::string &name, const casacore::Array<bool> &mask)
{
    ASKAPLOG_INFO_STR(logger, "Writing a full mask with the shape " << mask.shape() << " into a CASA image " <<
                      name);
    casacore::PagedImage<float> img(name);
    img.pixelMask().put(mask);
}
/// @brief set brightness units of the image
/// @details
/// @param[in] name image name
/// @param[in] units string describing brightness units of the image (e.g. "Jy/beam")
void CasaImageAccess::setUnits(const std::string &name, const std::string &units)
{
    casacore::PagedImage<float> img(name);
    img.setUnits(casacore::Unit(units));
}

/// @brief set restoring beam info
/// @details For the restored image we want to carry size and orientation of the restoring beam
/// with the image. This method allows to assign this info.
/// @param[in] name image name
/// @param[in] maj major axis in radians
/// @param[in] min minor axis in radians
/// @param[in] pa position angle in radians
void CasaImageAccess::setBeamInfo(const std::string &name, double maj, double min, double pa)
{
    casacore::PagedImage<float> img(name);
    casacore::ImageInfo ii = img.imageInfo();
    ii.setRestoringBeam(casacore::Quantity(maj, "rad"), casacore::Quantity(min, "rad"), casacore::Quantity(pa, "rad"));
    img.setImageInfo(ii);
}

/// @brief apply mask to image
/// @details Deteails depend upon the implemenation - CASA images will have the pixel mask assigned
/// but FITS images will have it applied to the pixels ... which is an irreversible process
/// @param[in] name image name
/// @param[in] the mask

void CasaImageAccess::makeDefaultMask(const std::string &name)
{
    casacore::PagedImage<float> img(name);

    // Create a mask and make it default region.
    // need to assert sizes etc ...
    img.makeMask("mask", casacore::True, casacore::True);
    casacore::Array<casacore::Bool> mask(img.shape());
    mask = casacore::True;
    img.pixelMask().put(mask);

}


/// @brief Set a particular keyword for the metadata (A.K.A header)
/// @details This adds a given keyword to the image metadata.
/// @param[in] name Image name
/// @param[in] keyword The name of the metadata keyword
/// @param[in] value The value for the keyword, in string format
/// @param[in] desc A description of the keyword
void CasaImageAccess::setMetadataKeyword(const std::string &name, const std::string &keyword,
        const std::string value, const std::string &desc)
{

    casacore::PagedImage<float> img(name);
    casacore::TableRecord miscinfo = img.miscInfo();
    miscinfo.define(keyword, value);
    miscinfo.setComment(keyword, desc);
    img.setMiscInfo(miscinfo);

}

/// @brief Add a HISTORY message to the image metadata
/// @details Adds a string detailing the history of the image
/// @param[in] name Image name
/// @param[in] history History comment to add
void CasaImageAccess::addHistory(const std::string &name, const std::string &history)
{

    casacore::PagedImage<float> img(name);
    casacore::LogIO log = img.logSink();
    log << history << casacore::LogIO::POST;

}


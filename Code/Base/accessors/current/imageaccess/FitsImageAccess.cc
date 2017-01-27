/// @file FitsImageAccess.cc
/// @brief Access FITS image
/// @details This class implements IImageAccess interface for FITS image
///
/// @copyright (c) 2016 CSIRO
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
/// Foundation, Inc.,  59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Stephen Ord <stephen.ord@csiro.au>
/// @author Max Voronkov <maxim.voronkov@csiro.au>
///

#include <askap_accessors.h>




#include <askap/AskapLogging.h>

#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/TempImage.h>
#include <casacore/images/Images/ImageFITSConverter.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/lattices/Lattices/ArrayLattice.h>

#include <imageaccess/FITSImageRW.h>
#include <imageaccess/FitsImageAccess.h>

ASKAP_LOGGER(logger, ".fitsImageAccessor");

using namespace askap;
using namespace askap::accessors;

// reading methods

/// @brief obtain the shape
/// @param[in] name image name
/// @return full shape of the given image
casa::IPosition FitsImageAccess::shape(const std::string &name) const
{
    std::string fullname = name + ".fits";
    casa::FITSImage img(fullname);
    return img.shape();
}

/// @brief read full image
/// @param[in] name image name
/// @return array with pixels
casa::Array<float> FitsImageAccess::read(const std::string &name) const
{
    std::string fullname = name + ".fits";
    ASKAPLOG_INFO_STR(logger, "Reading FITS image " << fullname);

    casa::FITSImage img(fullname);

    const casa::IPosition shape = img.shape();
    ASKAPLOG_INFO_STR(logger," - Shape " << shape);

    casa::IPosition blc(shape.nelements(),0);
    casa::IPosition trc(shape);
    return this->read(name,blc,trc);


}

/// @brief read part of the image
/// @param[in] name image name
/// @param[in] blc bottom left corner of the selection
/// @param[in] trc top right corner of the selection
/// @return array with pixels for the selection only
casa::Array<float> FitsImageAccess::read(const std::string &name, const casa::IPosition &blc,
        const casa::IPosition &trc) const
{
    std::string fullname = name + ".fits";
    ASKAPLOG_INFO_STR(logger, "Reading a slice of the FITS image " << name << " from " << blc << " to " << trc);

    casa::FITSImage img(fullname);
    casa::Array<float> buffer;
    casa::Slicer slc(blc,trc,casa::Slicer::endIsLength);
    // std::cout << "Reading a slice of the FITS image " << name << " slice " << slc << std::endl;
    ASKAPCHECK(img.doGetSlice(buffer,slc) == casa::False, "Cannot read image");
    return buffer;

}

/// @brief obtain coordinate system info
/// @param[in] name image name
/// @return coordinate system object
casa::CoordinateSystem FitsImageAccess::coordSys(const std::string &name) const
{
    std::string fullname = name + ".fits";
    casa::FITSImage img(fullname);
    return img.coordinates();
}

/// @brief obtain beam info
/// @param[in] name image name
/// @return beam info vector
casa::Vector<casa::Quantum<double> > FitsImageAccess::beamInfo(const std::string &name) const
{
    std::string fullname = name + ".fits";
    casa::FITSImage img(fullname);
    casa::ImageInfo ii = img.imageInfo();
    return ii.restoringBeam().toVector();
}

// writing methods

/// @brief create a new image
/// @details Unlike the casaaccessor this is only called when there is something
/// to actually write.
/// image may be created only upon the first write call. Details depend on the
/// implementation.
/// @param[in] name image name
/// @param[in] shape full shape of the image
/// @param[in] csys coordinate system of the full image
void FitsImageAccess::create(const std::string &name, const casa::IPosition &shape,
                             const casa::CoordinateSystem &csys)
{

    ASKAPLOG_INFO_STR(logger, "Creating a new FITS image " << name << " with the shape " << shape);
    casa::String error;

    itsFITSImage.reset(new FITSImageRW(name,shape,csys));
    if (!itsFITSImage->create()) {
        casa::String error;
        error = casa::String("Failed to create FITSFile");
        ASKAPTHROW(AskapError,error);
    }
    itsFITSImage->print_hdr();
    // make an array
    // this requires that the whole array fits in memory
    // which may not in general be the case

    // casa::TempImage<casa::Float> image(casa::TiledShape(shape),csys,0);


    // Now write the fits file.
    // casa::ImageFITSConverter::ImageToFITS (error, image, name);

}

/// @brief write full image
/// @param[in] name image name (not used)
/// @param[in] arr array with pixels
void FitsImageAccess::write(const std::string &name, const casa::Array<float> &arr)
{
    ASKAPLOG_INFO_STR(logger, "Writing an array with the shape " << arr.shape() << " into a FITS image " << name);
    itsFITSImage->write(arr);


}

/// @brief write a slice of an image
/// @param[in] name image name (not used)
/// @param[in] arr array with pixels
/// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
void FitsImageAccess::write(const std::string &name, const casa::Array<float> &arr,
                            const casa::IPosition &where)
{
    ASKAPLOG_INFO_STR(logger, "Writing a slice with the shape " << arr.shape() << " into a FITS image " <<
                      name << " at " << where);
    casa::String error;

    if (!itsFITSImage->write(arr,where)) {
        error = casa::String("Failed to write slice");
        ASKAPTHROW(AskapError,error);
    }

}

/// @brief set brightness units of the image
/// @details
/// @param[in] name image name
/// @param[in] units string describing brightness units of the image (e.g. "Jy/beam")
void FitsImageAccess::setUnits(const std::string &name, const std::string &units)
{
    itsFITSImage->setUnits(units);
}

/// @brief set restoring beam info
/// @details For the restored image we want to carry size and orientation of the restoring beam
/// with the image. This method allows to assign this info.
/// @param[in] name image name
/// @param[in] maj major axis in radians
/// @param[in] min minor axis in radians
/// @param[in] pa position angle in radians
/// The values are stored in a FITS header - note the FITS standard requires degrees
/// so these arguments are converted.

void FitsImageAccess::setBeamInfo(const std::string &name, double maj, double min, double pa)
{

    itsFITSImage->setRestoringBeam(maj, min, pa);

}

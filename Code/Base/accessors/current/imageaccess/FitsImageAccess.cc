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
#include <casacore/casa/System/ProgressMeter.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/TempImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/images/Images/ImageFITSConverter.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/lattices/Lattices/ArrayLattice.h>

#include <imageaccess/FITSImageRW.h>
#include <imageaccess/FitsImageAccess.h>

#include <fitsio.h>

ASKAP_LOGGER(logger, ".fitsImageAccessor");

using namespace askap;
using namespace askap::accessors;

// reading methods

/// @brief obtain the shape
/// @param[in] name image name
/// @return full shape of the given image
casacore::IPosition FitsImageAccess::shape(const std::string &name) const
{
    std::string fullname = name + ".fits";
    casacore::FITSImage img(fullname);
    return img.shape();
}

/// @brief read full image
/// @param[in] name image name
/// @return array with pixels
casacore::Array<float> FitsImageAccess::read(const std::string &name) const
{
    std::string fullname = name + ".fits";
    ASKAPLOG_INFO_STR(logger, "Reading FITS image " << fullname);

    casacore::FITSImage img(fullname);

    const casacore::IPosition shape = img.shape();
    ASKAPLOG_INFO_STR(logger, " - Shape " << shape);

    casacore::IPosition blc(shape.nelements(), 0);
    casacore::IPosition trc(shape);

    trc[0] = trc[0] - 1;
    trc[1] = trc[1] - 1;
    trc[2] = trc[2] - 1;
    trc[3] = trc[3] - 1;

    return this->read(name, blc, trc);


}

/// @brief read part of the image
/// @param[in] name image name
/// @param[in] blc bottom left corner of the selection
/// @param[in] trc top right corner of the selection
/// @return array with pixels for the selection only
casacore::Array<float> FitsImageAccess::read(const std::string &name, const casacore::IPosition &blc,
        const casacore::IPosition &trc) const
{
    std::string fullname = name + ".fits";
    ASKAPLOG_INFO_STR(logger, "Reading a slice of the FITS image " << name << " from " << blc << " to " << trc);

    casacore::FITSImage img(fullname);
    casacore::Array<float> buffer;
    casacore::Slicer slc(blc, trc, casacore::Slicer::endIsLast);
    std::cout << "Reading a slice of the FITS image " << name << " slice " << slc << std::endl;
    ASKAPCHECK(img.doGetSlice(buffer, slc) == casacore::False, "Cannot read image");
    return buffer;

}

/// @brief obtain coordinate system info
/// @param[in] name image name
/// @return coordinate system object
casacore::CoordinateSystem FitsImageAccess::coordSys(const std::string &name) const
{
    std::string fullname = name + ".fits";
    casacore::FITSImage img(fullname);
    return img.coordinates();
}

casacore::CoordinateSystem FitsImageAccess::coordSysSlice(const std::string &name, const casacore::IPosition &blc,
        const casacore::IPosition &trc) const
{
    std::string fullname = name + ".fits";
    casacore::Slicer slc(blc, trc, casacore::Slicer::endIsLast);
    ASKAPLOG_INFO_STR(logger, " FITSImageAccess - Slicer " << slc);
    casacore::FITSImage img(fullname);
    casacore::SubImage<casacore::Float> si = casacore::SubImage<casacore::Float>(img, slc, casacore::AxesSpecifier(casacore::True));
    return si.coordinates();

}
/// @brief obtain beam info
/// @param[in] name image name
/// @return beam info vector
casacore::Vector<casacore::Quantum<double> > FitsImageAccess::beamInfo(const std::string &name) const
{
    std::string fullname = name + ".fits";
    casacore::FITSImage img(fullname);
    casacore::ImageInfo ii = img.imageInfo();
    return ii.restoringBeam().toVector();
}
std::string FitsImageAccess::getUnits(const std::string &name) const
{

    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    std::string fullname = name + ".fits";
    int status = 0;
    std::string units("");
    const std::string key("Brightness (pixel) unit");
    char comment[1024];
    if (fits_open_file(&fptr, fullname.c_str(), READONLY, &status))
        ASKAPCHECK(status == 0, "FITSImageAccess:: Cannot open FITS file");

    if (fits_read_key(fptr, TSTRING, "BUNIT", (void *)(units.c_str()), comment,  &status))
        ASKAPLOG_WARN_STR(logger, "FITSImageAccess:: Cannot find BUNIT keyword - fits_read_key returned status " << status);

    status=0;
    if (fits_close_file(fptr, &status))
        ASKAPCHECK(status == 0, "FITSImageAccess:: Error on closing file");

    return units;

}

/// @brief Get a particular keyword from the image metadata (A.K.A header)
/// @details This reads a given keyword to the image metadata.
/// @param[in] name Image name
/// @param[in] keyword The name of the metadata keyword
std::string FitsImageAccess::getMetadataKeyword(const std::string &name, const std::string &keyword) const
{

    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    std::string fullname = name + ".fits";
    int status = 0;
    char value[1024];
    char comment[1024];
    if (fits_open_file(&fptr, fullname.c_str(), READONLY, &status))
        ASKAPCHECK(status == 0, "FITSImageAccess:: Cannot open FITS file");
    status=0;
    
    if (fits_read_key(fptr, TSTRING, keyword.c_str(), value, comment,  &status))
        ASKAPLOG_WARN_STR(logger, "FITSImageAccess:: Cannot find keyword " << keyword << " - fits_read_key returned status " << status);
    status=0;
    if (fits_close_file(fptr, &status))
        ASKAPCHECK(status == 0, "FITSImageAccess:: Error on closing file, status="<<status);

    std::string valueStr(value);
    return valueStr;

}


void FitsImageAccess::connect(const std::string &name)
{
    std::string fullname = name + ".fits";
    itsFITSImage.reset(new FITSImageRW(fullname));
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
void FitsImageAccess::create(const std::string &name, const casacore::IPosition &shape,
                             const casacore::CoordinateSystem &csys)
{

    ASKAPLOG_INFO_STR(logger, "Creating a new FITS image " << name << " with the shape " << shape);
    casacore::String error;

    itsFITSImage.reset(new FITSImageRW());
    if (!itsFITSImage->create(name, shape, csys)) {
        casacore::String error;
        error = casacore::String("Failed to create FITSFile");
        ASKAPTHROW(AskapError, error);
    }
    itsFITSImage->print_hdr();
    // make an array
    // this requires that the whole array fits in memory
    // which may not in general be the case

    // casacore::TempImage<casacore::Float> image(casacore::TiledShape(shape),csys,0);


    // Now write the fits file.
    // casacore::ImageFITSConverter::ImageToFITS (error, image, name);

}

/// @brief write full image
/// @param[in] name image name (not used)
/// @param[in] arr array with pixels
void FitsImageAccess::write(const std::string &name, const casacore::Array<float> &arr)
{
    ASKAPLOG_INFO_STR(logger, "Writing an array with the shape " << arr.shape() << " into a FITS image " << name);
    connect(name);
    itsFITSImage->write(arr);


}

/// @brief write a slice of an image
/// @param[in] name image name (not used)
/// @param[in] arr array with pixels
/// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
void FitsImageAccess::write(const std::string &name, const casacore::Array<float> &arr,
                            const casacore::IPosition &where)
{
    ASKAPLOG_INFO_STR(logger, "Writing a slice with the shape " << arr.shape() << " into a FITS image " <<
                      name << " at " << where);
    casacore::String error;
    connect(name);
    if (!itsFITSImage->write(arr, where)) {
        error = casacore::String("Failed to write slice");
        ASKAPTHROW(AskapError, error);
    }

}
/// @brief write a slice of an image mask
/// @param[in] name image name
/// @param[in] arr array with pixels
/// @param[in] where bottom left corner where to put the slice to (trc is deduced from the array shape)
void FitsImageAccess::writeMask(const std::string &name, const casacore::Array<bool> &mask,
                                const casacore::IPosition &where)
{
    casacore::String error = casacore::String("FITS pixel mask not yet implemented");
    ASKAPLOG_INFO_STR(logger, error);
}
/// @brief write a slice of an image mask
/// @param[in] name image name
/// @param[in] arr array with pixels
void FitsImageAccess::writeMask(const std::string &name, const casacore::Array<bool> &mask)
{
    casacore::String error = casacore::String("FITS pixel mask not yet implemented");
    ASKAPLOG_INFO_STR(logger, error);
}
/// @brief set brightness units of the image
/// @details
/// @param[in] name image name
/// @param[in] units string describing brightness units of the image (e.g. "Jy/beam")
void FitsImageAccess::setUnits(const std::string &name, const std::string &units)
{
    connect(name);
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
    connect(name);
    itsFITSImage->setRestoringBeam(maj, min, pa);

}
/// @brief apply mask to image
/// @details Details depend upon the implemenation - CASA images will have the pixel mask assigned
/// but FITS images will have it applied to the pixels ... which is an irreversible process
/// In this mode we would either have to apply it to the array - or readback the array - mask
/// then write ...
void FitsImageAccess::makeDefaultMask(const std::string &name)
{

    casacore::String error = casacore::String("A default mask in FITS makes no sense");
    ASKAPLOG_INFO_STR(logger, error);

}

/// @brief Set a particular keyword for the metadata (A.K.A header)
/// @details This adds a given keyword to the image metadata.
/// @param[in] name Image name
/// @param[in] keyword The name of the metadata keyword
/// @param[in] value The value for the keyword, in string format
/// @param[in] desc A description of the keyword
void FitsImageAccess::setMetadataKeyword(const std::string &name, const std::string &keyword,
        const std::string value, const std::string &desc)
{
    connect(name);
    itsFITSImage->setHeader(keyword, value, desc);
}

/// @brief Add a HISTORY message to the image metadata
/// @details Adds a string detailing the history of the image
/// @param[in] name Image name
/// @param[in] history History comment to add
void FitsImageAccess::addHistory(const std::string &name, const std::string &history)
{

    connect(name);
    itsFITSImage->addHistory(history);
    
}

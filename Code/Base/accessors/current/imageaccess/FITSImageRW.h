/// @file FITSImageRW.h
/// @brief Read/Write FITS image class
/// @details This class implements the write methods that are absent
/// from the casacore FITSImage.
///
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
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Stephen Ord <stephen.ord@csiro.au
///
#ifndef ASKAP_ACCESSORS_FITS_IMAGE_RW_H
#define ASKAP_ACCESSORS_FITS_IMAGE_RW_H

#include <casacore/images/Images/FITSImage.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/Utilities/DataType.h>
#include <casacore/fits/FITS/fitsio.h>

#include "boost/scoped_ptr.hpp"

namespace askap {
namespace accessors {

/// @brief Extend FITSImage class functionality
/// @details It is made clear in the casacore implementation that there are
/// difficulties in writing general FITS access routines for writing.
/// I will implement what ASKAP needs here
/// @ingroup imageaccess


extern bool created;

class FITSImageRW {

public:

    FITSImageRW ();

    FITSImageRW (const std::string &name);

    /// @brief create a new FITS image
    /// @details A call to this method should preceed any write calls. The actual
    /// image may be created only upon the first write call. Details depend on the
    /// implementation.


    bool create (const std::string &name, const casa::IPosition &shape,\
        const casa::CoordinateSystem &csys,\
        uint memoryInMB = 64,\
        bool preferVelocity = false,\
        bool opticalVelocity = true,\
        int BITPIX=-32,\
        float minPix = 1.0,\
        float maxPix = -1.0,\
        bool degenerateLast=false,\
        bool verbose=true,\
        bool stokesLast=false,\
        bool preferWavelength=false,\
        bool airWavelength=false,\
        bool primHead=true,\
        bool allowAppend=false,\
        bool history=true);

    // Destructor does nothing
    virtual ~FITSImageRW();

    bool create();

    void print_hdr();
    void setUnits(const std::string &units);

    void setRestoringBeam(double,double,double);
    // write into a FITS image
    bool write(const casa::Array<float>& );
    bool write(const casa::Array<float> &arr,const casa::IPosition &where);
    private:



        std::string name;
        casa::IPosition shape;
        casa::CoordinateSystem csys;
        uint memoryInMB;
        bool preferVelocity;
        bool opticalVelocity;
        int BITPIX;
        float minPix;
        float maxPix;
        bool degenerateLast;
        bool verbose;
        bool stokesLast;
        bool preferWavelength;
        bool airWavelength;
        bool primHead;
        bool allowAppend;
        bool history;

        casa::FitsKeywordList theKeywordList;

};
}
}
#endif

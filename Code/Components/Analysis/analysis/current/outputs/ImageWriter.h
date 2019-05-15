/// @file
///
/// Utility class to easily write out a CASA image, with optional piece-wise writing
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

#ifndef ASKAP_ANALYSIS_IMAGE_WRITER_H_
#define ASKAP_ANALYSIS_IMAGE_WRITER_H_

#include <string>
#include <duchamp/Cubes/cubes.hh>
#include <casacore/casa/aipstype.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/images/Images/ImageInfo.h>
#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

class ImageWriter {
    public:
        ImageWriter() {};
        ImageWriter(const LOFAR::ParameterSet &parset,duchamp::Cube *cube, std::string imageName);
        virtual ~ImageWriter() {};

        void copyMetadata(duchamp::Cube *cube);

        std::string &imagename() {return itsImageName;};
        casacore::Unit &bunit() {return itsBunit;};
        casacore::CoordinateSystem &coordsys() {return itsCoordSys;};
        casacore::IPosition &shape() {return itsShape;};

        void setTileshapeFromShape(casacore::IPosition &shape);

        virtual void create();

        void write(float *data,
                   const casacore::IPosition &shape,
                   bool accumulate = false);

        void write(float *data,
                   const casacore::IPosition &shape,
                   const casacore::IPosition &loc,
                   bool accumulate = false);

        void write(const casacore::Array<casacore::Float> &data,
                   bool accumulate = false);

        virtual void write(const casacore::Array<casacore::Float> &data,
                           const casacore::IPosition &loc,
                           bool accumulate = false);

    void writeMask(const casacore::Array<bool> &mask,
                   const casacore::IPosition &loc);

        casacore::Array<casacore::Float> read(const casacore::IPosition& loc,
                                      const casacore::IPosition &shape);

    protected:
            /// @brief The defining parset
        LOFAR::ParameterSet itsParset;

        std::string itsImageName;
        casacore::Unit itsBunit;
        casacore::IPosition itsShape;
        casacore::IPosition itsTileshape;
        casacore::CoordinateSystem itsCoordSys;
        casacore::ImageInfo itsImageInfo;
};


}

}



#endif

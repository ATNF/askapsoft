/// @file SmsTypes.h
/// @brief Some simple types used in the Sky Model Service.
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#ifndef ASKAP_CP_SMS_SMSTYPES_H
#define ASKAP_CP_SMS_SMSTYPES_H

// System includes

// ASKAPsoft includes

// Ice interfaces
#include <SkyModelService.h>
#include <SkyModelServiceDTO.h>

// Local package includes
#include "Utility.h"

namespace askap {
namespace cp {
namespace sms {

// Create an alias for the Ice interfaces
namespace sms_interface = askap::interfaces::skymodelservice;

/// @brief Extents structure, used to define a region of interest about
/// a coordinate.
struct Extents {
    /// @brief Construct an Extents object
    ///
    /// @param width The width
    /// @param height The height
    inline Extents(double width, double height) :
        width(width),
        height(height)
    {
        ASKAPASSERT(width > 0);
        ASKAPASSERT(height > 0);
    }
    
    /// @brief Construct an Extents from the corresponding Ice struct
    inline Extents(const sms_interface::RectExtents& that) :
        width(that.width),
        height(that.height)
    {
    }

    double width;
    double height;
};

/// @brief A RA/Dec coordinate
struct Coordinate {
    /// @brief Default constructor required for instantiation inside STL
    /// containers.
    inline Coordinate() :
        ra(0),
        dec(0)
    {
    }

    /// @brief Construct a coordinate in J2000 decimal degrees
    ///
    /// @param ra J2000 Right-ascension in decimal degrees.
    /// @param dec J2000 declination in decimal degrees.
    inline Coordinate(double ra, double dec) :
        ra(ra),
        dec(dec)
    {
        ASKAPASSERT((ra >= 0.0) && (ra < 360.0));
        ASKAPASSERT((dec >= -90.0) && (dec <= 90.0));
    }

    /// @brief Construct a Coordinate from the corresponding Ice struct
    inline Coordinate(const sms_interface::Coordinate& that) :
        ra(that.rightAscension),
        dec(that.declination)
    {
    }

    double ra;
    double dec;
};

/// @brief A rectangular region specified as a centre coordinate plus extents.
struct Rect {

    /// @brief Constructs a Rect from centre and extents.
    inline Rect(Coordinate centre, Extents extents) :
        centre(centre),
        extents(extents)
    {
    }

    /// @brief Construct a Rect from the corresponding Ice struct
    inline Rect(const sms_interface::Rect& that) :
        centre(that.centre),
        extents(that.extents)
    {
    }

    /// @brief retrieve the top-left coordinate
    inline Coordinate topLeft() const
    {
        return Coordinate(
            utility::wrapAngleDegrees(centre.ra - extents.width / 2.0),
            centre.dec + extents.height / 2.0);
    }

    /// @brief retrieve the top-right coordinate
    inline Coordinate topRight() const
    {
        return Coordinate(
            utility::wrapAngleDegrees(centre.ra + extents.width / 2.0),
            centre.dec + extents.height / 2.0);
    }

    /// @brief retrieve the bottom-left coordinate
    inline Coordinate bottomLeft() const
    {
        return Coordinate(
            utility::wrapAngleDegrees(centre.ra - extents.width / 2.0),
            centre.dec - extents.height / 2.0);
    }

    /// @brief retrieve the bottom-right coordinate
    inline Coordinate bottomRight() const
    {
        return Coordinate(
            utility::wrapAngleDegrees(centre.ra + extents.width / 2.0),
            centre.dec - extents.height / 2.0);
    }

    Coordinate centre;
    Extents extents;
};


};
};
};

#endif

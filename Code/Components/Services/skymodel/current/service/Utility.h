/// @file Utility.h
/// @brief Utility functions outside of the primary data service.
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

#ifndef ASKAP_CP_SMS_UTILITY_H
#define ASKAP_CP_SMS_UTILITY_H

// System includes

// ASKAPsoft includes
#include <boost/noncopyable.hpp>
#include <boost/math/constants/constants.hpp>
#include <Common/ParameterSet.h>

// Local package includes


namespace askap {
namespace cp {
namespace sms {
namespace utility {

/// @brief Conversion from degrees to radians
///
/// @tparam Numeric The numeric type (double, float)
/// @param degrees The value in degrees to convert
///
/// @return The value in radians
template<class Numeric> inline Numeric degreesToRadians(Numeric degrees) {
    // This looks weird, but boost defines the degree constant as pi/180
    return degrees * boost::math::constants::degree<Numeric>();
}

inline double wrapAngleDegrees(double angle) {
    angle = std::fmod(angle, 360.0);
    if (angle < 0.0)
        angle += 360.0;
    return angle;
}

};
};
};
};

#endif

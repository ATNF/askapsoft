/// @file HealPixFacade.h
/// @brief HEALPix utility functions
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

#ifndef ASKAP_CP_SMS_HEALPIXFACADE_H
#define ASKAP_CP_SMS_HEALPIXFACADE_H

#include <vector>

// ASKAPsoft and 3rdParty includes
#include <askap/AskapError.h>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <Common/ParameterSet.h>
#include <healpix_base.h>
#include <pointing.h>

// Local package includes
#include "Utility.h"
#include "SmsTypes.h"


namespace askap {
namespace cp {
namespace sms {


class HealPixFacade : private boost::noncopyable {
    public:
        typedef boost::int64_t Index;
        typedef std::vector<Index> IndexList;
        typedef boost::shared_ptr<IndexList> IndexListPtr;
        /// @brief Constructor.
        ///
        /// @param order The HEALPix order.
        HealPixFacade(Index order);

        /// @brief Calculate the HEALPix index for a given RA and declination.
        ///
        /// @note   I will probably require a vectorisable version of this function
        ///         that operates on a vector of input coordinates, but this function
        ///         will suffice for the initial unit tests.
        /// @param[in] coordinate J2000 coordinate (decimal degrees)
        Index calcHealPixIndex(Coordinate coordinate) const;

        /// @brief Returns the set of all pixels which overlap with the disk
        /// defined by a centre and radius.
        ///
        /// @param[in] centre J2000 coordinate of the disk centre (decimal degrees)
        /// @param[in] radius Radius in decimal degrees of the disk.
        /// @param[in] fact Oversampling factor. The overlapping test will be done 
        ///         at the resolution fact*nside. Must be a power of 2.
        ///
        /// @return The vector of pixel indicies matching the query.
        IndexListPtr queryDisk(Coordinate centre, double radius, int fact=8) const;

        /// @brief Returns the set of all pixels which overlap with the rectangle
        ///
        /// @param[in] rect The rectangle
        /// @param[in] fact Oversampling factor. The overlapping test will be done 
        ///         at the resolution fact*nside. Must be a power of 2.
        ///
        /// @return The vector of pixel indicies matching the query.
        IndexListPtr queryRect(Rect rect, int fact=8) const;

        /// @brief Converts a J2000 coordinate to a pointing
        ///
        /// @param[in] coordinate J2000 coordinate (decimal degrees)
        ///
        /// @return The pointing.
        inline static pointing J2000ToPointing(Coordinate coordinate) {
            return pointing(
                    // theta = (90 - dec)
                    utility::degreesToRadians(90.0 - coordinate.dec),
                    // phi = ra
                    utility::degreesToRadians(coordinate.ra));
        }

    private:
        T_Healpix_Base<Index> itsHealPixBase;
        Index itsNSide;
};

};
};
};

#endif

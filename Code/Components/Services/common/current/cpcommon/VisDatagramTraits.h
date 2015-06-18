/// @file VisDatagramTraits.h
///
/// @copyright (c) 2010 CSIRO
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

#ifndef ASKAP_CP_VISDATAGRAM_TRAITS_H
#define ASKAP_CP_VISDATAGRAM_TRAITS_H

#include <boost/static_assert.hpp>

namespace askap {
    namespace cp {

        /// @brief generic traits class with no properties defined
        /// @details We use generic templated code, specialisation 
        /// of this class for a particular protocol allows to 
        /// pass protocol-specific traits (e.g. chunk size or number
        /// of channels or card numbers, etc).
        template<typename T>
        struct VisDatagramTraits {
           BOOST_STATIC_ASSERT_MSG(sizeof(T) == 0, 
                "Attempted a build for protocol without defined traits");
        };

    };
};

#endif // #ifndef ASKAP_CP_VISDATAGRAM_TRAITS_H

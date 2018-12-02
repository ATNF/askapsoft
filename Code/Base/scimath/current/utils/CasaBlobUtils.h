/// @file CasaBlobUtils.h
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
/// @author Daniel Mitchell <daniel.mitchell@csiro.au>
/// 
#ifndef SCIMATH_CASABLOBUTILS_H
#define SCIMATH_CASABLOBUTILS_H

// ASKAPsoft includes
#include "askap/AskapError.h"

// casacore's includes
#include <casacore/casa/aips.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/casa/Quanta/MVDirection.h"
#include "casacore/measures/Measures/Stokes.h"

// LOFAR includes
#include <Blob/BlobOStream.h>
#include <Blob/BlobIStream.h>

namespace LOFAR
{

    // casa::CoordinateSystem
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::CoordinateSystem& cSys);
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::CoordinateSystem& cSys);

    // MV: serialisation for measures-related casacore types

    /// @brief output operator for casa::Quantity
    /// @param[in] os output stream
    /// @param[in] q Quantity to serialise
    /// @return output stream for chaining
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::Quantity& q);

    /// @brief input operator for casa::Quantity
    /// @param[in] is input stream
    /// @param[in] q quantity object to populate
    /// @return input stream for chaining
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::Quantity& q);

    /// @brief output operator for casa::MDirection::Ref
    /// @param[in] os output stream
    /// @param[in] ref object to serialise
    /// @return output stream for chaining
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MDirection::Ref& ref);

    /// @brief input operator for casa::MDirection::Ref
    /// @param[in] is input stream
    /// @param[in] ref object to populate
    /// @return input stream for chaining
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MDirection::Ref& ref);

    /// @brief output operator for casa::MVDirection
    /// @param[in] os output stream
    /// @param[in] dir object to serialise
    /// @return output stream for chaining
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MVDirection& dir);

    /// @brief input operator for casa::MVDirection
    /// @param[in] is input stream
    /// @param[in] dir object to populate
    /// @return input stream for chaining
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MVDirection& dir);

    /// @brief output operator for casa::MDirection
    /// @param[in] os output stream
    /// @param[in] dir object to serialise
    /// @return output stream for chaining
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MDirection& dir);

    /// @brief input operator for casa::MDirection
    /// @param[in] is input stream
    /// @param[in] dir object to populate
    /// @return input stream for chaining
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MDirection& dir);

    /// @brief output operator for casa::Stokes::StokesTypes
    /// @param[in] os output stream
    /// @param[in] pol object to serialise
    /// @return output stream for chaining
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::Stokes::StokesTypes& pol);

    /// @brief input operator for casa::Stokes::StokesTypes
    /// @param[in] is input stream
    /// @param[in] pol object to populate
    /// @return input stream for chaining
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::Stokes::StokesTypes& pol);
}

#include "CasaBlobUtils.tcc"

#endif

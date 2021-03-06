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
/// @author Ben Humphreys <ben.humphreys@csiro.au>
/// 
#ifndef ASKAP_CP_COMMON_CASABLOBUTILS_H
#define ASKAP_CP_COMMON_CASABLOBUTILS_H

// ASKAPsoft includes
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/casa/Quanta/MVDirection.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/scimath/Mathematics/RigidVector.h"
#include "Blob/BlobOStream.h"
#include "Blob/BlobIStream.h"

namespace LOFAR {

        // casa::MVEpoch
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MVEpoch& obj);
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MVEpoch& obj);

        // casa::MVDirection
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MVDirection& obj);
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MVDirection& obj);

        // casa::MDirection
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MDirection& obj);
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MDirection& obj);

        // casa::Stokes::StokesTypes
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::Stokes::StokesTypes& obj);
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::Stokes::StokesTypes& obj);

        // casa::RigidVector<T, n>
        template<class T, int n>
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::RigidVector<T, n>& obj);
        template<class T, int n>
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::RigidVector<T, n>& obj);

        // casa::MDirection::Ref
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::MDirection::Ref& obj);
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::MDirection::Ref& obj);

        // casa::Quantity
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casa::Quantity& obj);
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casa::Quantity& obj);

} // end of namespace LOFAR

#endif

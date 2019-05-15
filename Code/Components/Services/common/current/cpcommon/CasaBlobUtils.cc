/// @file CasaBlobUtils.cc
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

#include "cpcommon/CasaBlobUtils.h"

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

        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::MVEpoch& obj)
        {
            os << obj.get();
            return os;
        }

        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::MVEpoch& obj)
        {
            casacore::Double time;
            is >> time;
            obj = casacore::MVEpoch(time);
            return is;
        }

        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::MDirection& obj)
        {
            os << obj.getAngle().getValue()(0);
            os << obj.getAngle().getValue()(1);
            os << obj.getAngle().getUnit();
            os << obj.getRefString();
            return os;
        }

        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::MDirection& obj)
        {
            casacore::Double coord1;
            casacore::Double coord2;
            casacore::String ref;
            casacore::String unit;
            is >> coord1;
            is >> coord2;
            is >> unit;
            is >> ref;
            casacore::MDirection dir(casacore::Quantity(coord1, unit),
                                 casacore::Quantity(coord2, unit));
            dir.setRefString(ref);
            obj = dir;
            return is;
        }

        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::MVDirection& obj)
        {
            os << obj.getLong() << obj.getLat();
            return os;
        }

        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::MVDirection& obj)
        {
            double longitude, latitude;
            is >> longitude >> latitude;
            casacore::MVDirection md(longitude, latitude);
            obj = md;
            return is;
        }

        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::Stokes::StokesTypes& obj)
        {
            os << static_cast<int>(obj);
            return os;
        }

        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::Stokes::StokesTypes& obj)
        {
            int stokes;
            is >> stokes;
            obj = static_cast<casacore::Stokes::StokesTypes>(stokes);

            return is;
        }

        template<class T, int n>
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::RigidVector<T, n>& obj)
        {
            for (int i = 0; i < n; ++i) {
            os << obj(i);
            }
            return os;
        }

        template<class T, int n>
        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::RigidVector<T, n>& obj)
        {
            for (int i = 0; i < n; ++i) {
                is >> obj(i);
            }
            return is;
        }

        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::MDirection::Ref& obj)
        {
            os << obj.getType();
            return os;
        }

        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::MDirection::Ref& obj)
        {
            casacore::uInt type;
            is >> type;
            casacore::MDirection::Ref ref(type);
            obj = ref;
            return is;
        }

        // casacore::Quantity
        LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os, const casacore::Quantity& obj)
        {
            os << obj.getValue()<< obj.getFullUnit().getName();
            return os;
        }

        LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is, casacore::Quantity& obj)
        {
            double value;
            casacore::String unit;
            is >> value >> unit;
            obj = casacore::Quantity(value, unit);
            return is;
        }

} // End namespace LOFAR

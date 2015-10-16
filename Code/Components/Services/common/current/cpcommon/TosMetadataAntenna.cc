/// @file TosMetadataAntenna.cc
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

// Include own header file first
#include "TosMetadataAntenna.h"

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "casa/aips.h"
#include "measures/Measures/MDirection.h"
#include "casa/Quanta/Quantum.h"

#include "cpcommon/CasaBlobUtils.h"

// Using
using namespace askap::cp;

TosMetadataAntenna::TosMetadataAntenna(const casa::String& name)
    : itsName(name), itsOnSource(false), itsFlagged(true)
{
}

casa::String TosMetadataAntenna::name(void) const
{
    return itsName;
}

casa::MDirection TosMetadataAntenna::actualRaDec(void) const
{
    return itsActualRaDec;
}

void TosMetadataAntenna::actualRaDec(const casa::MDirection& val)
{
    itsActualRaDec = val;
}

casa::MDirection TosMetadataAntenna::actualAzEl(void) const
{
    return itsActualAzEl;
}

void TosMetadataAntenna::actualAzEl(const casa::MDirection& val)
{
    itsActualAzEl = val;
}

casa::Quantity TosMetadataAntenna::actualPolAngle(void) const
{
    return itsPolAngle;
}

void TosMetadataAntenna::actualPolAngle(const casa::Quantity& q)
{
    itsPolAngle = q;
}

casa::Bool TosMetadataAntenna::onSource(void) const
{
    return itsOnSource;
}

void TosMetadataAntenna::onSource(const casa::Bool& val)
{
    itsOnSource = val;
}

casa::Bool TosMetadataAntenna::flagged(void) const
{
    return itsFlagged;
}

void TosMetadataAntenna::flagged(const casa::Bool& val)
{
    itsFlagged = val;
}

/// serialise TosMetadataAntenna
/// @param[in] os output stream
/// @param[in] obj object to serialise
/// @return output stream
LOFAR::BlobOStream& LOFAR::operator<<(LOFAR::BlobOStream& os, const askap::cp::TosMetadataAntenna& obj)
{
   os.putStart("TosMetadataAntenna", 1);
   os << obj.name() << obj.flagged() << obj.onSource() << obj.actualPolAngle() << 
         obj.actualAzEl() << obj.actualRaDec();
   os.putEnd();
   return os;
}

/// deserialise TosMetadataAntenna
/// @param[in] is input stream
/// @param[out] obj object to deserialise
/// @return input stream
LOFAR::BlobIStream& LOFAR::operator>>(LOFAR::BlobIStream& is, askap::cp::TosMetadataAntenna& obj)
{
   using namespace askap;
   const int version = is.getStart("TosMetadataAntenna");
   ASKAPASSERT(version == 1);
   casa::String name;
   is >> name;
   obj = TosMetadataAntenna(name);
   casa::Bool flag, onSource;
   is >> flag >> onSource;
   obj.flagged(flag);
   obj.onSource(onSource);
   casa::Quantity q;
   is >> q;
   obj.actualPolAngle(q);
   casa::MDirection dir;
   is >> dir;
   obj.actualAzEl(dir);
   is >> dir;
   obj.actualRaDec(dir);
   return is;
}


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
#include "askap/askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/casa/Quanta/Quantum.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/ArrayIO.h"

#include "cpcommon/CasaBlobUtils.h"

// 3rdParty
#include "Blob/BlobArray.h"
#include "Blob/BlobAipsIO.h"
#include "Blob/BlobOStream.h"
#include "Blob/BlobIStream.h"
// Using
using namespace askap::cp;

TosMetadataAntenna::TosMetadataAntenna(const casacore::String& name)
    : itsName(name), itsOnSource(false), itsFlagged(true)
{
}

/// @brief Copy constructor
/// @param[in] other input object
TosMetadataAntenna::TosMetadataAntenna(const TosMetadataAntenna &other) :
    itsName(other.itsName), itsActualRaDec(other.itsActualRaDec),
    itsActualAzEl(other.itsActualAzEl), itsPolAngle(other.itsPolAngle),
    itsOnSource(other.itsOnSource), itsFlagged(other.itsFlagged),
    itsUVW(other.itsUVW.copy())
{}

/// @brief assignment operator
/// @param[in] other input object
/// @return reference to the original object
TosMetadataAntenna& TosMetadataAntenna::operator=(const TosMetadataAntenna &other)
{
  if (this != &other) {
      itsName = other.itsName;
      itsActualRaDec = other.itsActualRaDec;
      itsActualAzEl = other.itsActualAzEl;
      itsPolAngle = other.itsPolAngle;
      itsOnSource = other.itsOnSource;
      itsFlagged = other.itsFlagged;
      itsUVW.assign(other.itsUVW.copy());
  }
  return *this;
}
    

casacore::String TosMetadataAntenna::name(void) const
{
    return itsName;
}

casacore::MDirection TosMetadataAntenna::actualRaDec(void) const
{
    return itsActualRaDec;
}

void TosMetadataAntenna::actualRaDec(const casacore::MDirection& val)
{
    itsActualRaDec = val;
}

casacore::MDirection TosMetadataAntenna::actualAzEl(void) const
{
    return itsActualAzEl;
}

void TosMetadataAntenna::actualAzEl(const casacore::MDirection& val)
{
    itsActualAzEl = val;
}

casacore::Quantity TosMetadataAntenna::actualPolAngle(void) const
{
    return itsPolAngle;
}

void TosMetadataAntenna::actualPolAngle(const casacore::Quantity& q)
{
    itsPolAngle = q;
}

casacore::Bool TosMetadataAntenna::onSource(void) const
{
    return itsOnSource;
}

void TosMetadataAntenna::onSource(const casacore::Bool& val)
{
    itsOnSource = val;
}

casacore::Bool TosMetadataAntenna::flagged(void) const
{
    return itsFlagged;
}

void TosMetadataAntenna::flagged(const casacore::Bool& val)
{
    itsFlagged = val;
}

/// @brief Get the values of the UVW vector
/// @return vector with UVWs, 3 values for each beam
const casacore::Vector<casacore::Double>& TosMetadataAntenna::uvw() const
{
    return itsUVW;
}

/// @brief Set the values of the UVW vector
/// @param[in] val the vector with UVWs
/// @note It is expected that we get 3 values per beam. An exception is thrown if the number of
/// elements is not divisable by 3.
void TosMetadataAntenna::uvw(const casacore::Vector<casacore::Double> &uvw)
{
   using namespace askap;
   ASKAPCHECK(uvw.nelements() % 3 == 0, "The uvw vector in the metadata is expected to have 3*Nbeam elements, you have "<<uvw.nelements());
   // to deal with reference semantics of CASA arrays
   itsUVW.assign(uvw.copy());
}


/// serialise TosMetadataAntenna
/// @param[in] os output stream
/// @param[in] obj object to serialise
/// @return output stream
LOFAR::BlobOStream& LOFAR::operator<<(LOFAR::BlobOStream& os, const askap::cp::TosMetadataAntenna& obj)
{
   os.putStart("TosMetadataAntenna", 2);
   os << obj.name() << obj.flagged() << obj.onSource() << obj.actualPolAngle() << 
         obj.actualAzEl() << obj.actualRaDec()<<obj.uvw();
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
   ASKAPASSERT(version == 2);
   casacore::String name;
   is >> name;
   obj = TosMetadataAntenna(name);
   casacore::Bool flag, onSource;
   is >> flag >> onSource;
   obj.flagged(flag);
   obj.onSource(onSource);
   casacore::Quantity q;
   is >> q;
   obj.actualPolAngle(q);
   casacore::MDirection dir;
   is >> dir;
   obj.actualAzEl(dir);
   is >> dir;
   obj.actualRaDec(dir);
   casacore::Vector<casacore::Double> uvwBuf;
   is >> uvwBuf;
   obj.uvw(uvwBuf);
   is.getEnd();
   return is;
}


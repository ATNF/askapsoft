/// @file
/// @brief A handler of  ANTENNA subtable
/// @details This class provides access to the ANTENNA subtable (which contains 
/// antenna mounts and positions for all antennas). It caches the whole table
/// in constructor and then returns cached values. 
///
/// @copyright (c) 2007 CSIRO
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
///

// own includes
#include <dataaccess/MemAntennaSubtableHandler.h>
#include <askap_accessors.h>

// enable logging when it is actually used
// #include <askap/AskapLogging.h>
// ASKAP_LOGGER(logger, "");

#include <askap/AskapError.h>
#include <dataaccess/DataAccessError.h>

// casa includes
#include <casacore/tables/Tables/ScalarColumn.h>
#include <casacore/tables/Tables/TableRecord.h>
#include <casacore/measures/TableMeasures/ScalarMeasColumn.h>
#include <casacore/casa/Arrays/Array.h>

using namespace askap;
using namespace askap::accessors;

/// read all required information from the ANTENNA subtable
/// @param[in] ms an input measurement set (a table which has an
/// ANTENNA subtable)
MemAntennaSubtableHandler::MemAntennaSubtableHandler(const casacore::Table &ms) : 
       itsAllEquatorial(true)
{
  casacore::Table antennaSubtable=ms.keywordSet().asTable("ANTENNA");
  if (!antennaSubtable.nrow()) {
      ASKAPTHROW(DataAccessError, "The ANTENNA subtable is empty");      
  }
  casacore::ROScalarColumn<casacore::String> mountCol(antennaSubtable,"MOUNT");
  casacore::ROScalarMeasColumn<casacore::MPosition> posCol(antennaSubtable,"POSITION");
  mountCol.getColumn(itsMounts,casacore::True);
  itsPositions.resize(itsMounts.nelements());
  casacore::Vector<casacore::MPosition>::iterator it=itsPositions.begin();
  casacore::Vector<casacore::String>::const_iterator cit=itsMounts.begin();
  for (casacore::uInt ant=0; it!=itsPositions.end(); ++it,++ant,++cit) {
       *it=posCol(ant);
       const casacore::String &cMount = *cit;
       if (cMount != "EQUATORIAL" && cMount != "equatorial") {
           itsAllEquatorial = false;
       }
  }  
}

/// @brief get the number of antennae
/// @details
/// This method returns the number of antennae (i.e. all antID indices
/// are expected to be less than this number). Following the general
/// assumptions about ANTENNA subtable, this number is assumed to be
/// fixed.
/// @return total number of antennae 
casacore::uInt MemAntennaSubtableHandler::getNumberOfAntennae() const
{
  return itsMounts.nelements();
}

  
/// @brief obtain the position of the given antenna
/// @details
/// @param[in] antID antenna ID to return the position for
/// @return a reference to the MPosition measure
const casacore::MPosition& MemAntennaSubtableHandler::getPosition(casacore::uInt antID) 
                           const
{
  ASKAPDEBUGASSERT(antID<itsPositions.nelements());
  return itsPositions[antID];
}                           
  
/// @brief obtain the mount type for the given antenna
/// @details
/// @param[in] antID antenna ID to return the position for
/// @return a string describing the mount type
const casacore::String& MemAntennaSubtableHandler::getMount(casacore::uInt antID) const
{
  ASKAPDEBUGASSERT(antID<itsMounts.nelements());
  return itsMounts[antID];
}

/// @brief check whether all antennae are equatorialy mounted
/// @details
/// This method checks the mount type for all antennas to be 
/// either EQUATORIAL or equatorial. This mount type doesn't require
/// parallactic angle rotation and can be trated separately.
/// @return true, if all antennae are equatorially mounted
bool MemAntennaSubtableHandler::allEquatorial() const throw()
{  
  return itsAllEquatorial;
}   

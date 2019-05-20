/// @file Antenna.cc
///
/// @copyright (c) 2011 CSIRO
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
#include "Antenna.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <vector>

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Arrays/Vector.h"

ASKAP_LOGGER(logger, ".Antenna");

using namespace askap;
using namespace askap::cp::ingest;
using namespace casacore;

Antenna::Antenna(const casacore::String& name,
                 const casacore::String& mount,
                 const casacore::Vector<casacore::Double>& position,
                 const casacore::Quantity& diameter, 
                 const casacore::Quantity& delay)
        : itsName(name), itsMount(mount), itsPosition(position),
        itsDiameter(diameter), itsDelay(delay)
{
    ASKAPCHECK(itsDiameter.isConform("m"),
               "Diameter must conform to meters");
    ASKAPCHECK(position.nelements() == 3,
               "Position vector must have three elements");
    ASKAPCHECK(itsDelay.isConform("s"),
               "Antenna delay must conform to seconds");
}

casacore::String Antenna::name(void) const
{
    return itsName;
}

casacore::String Antenna::mount(void) const
{
    return itsMount;
}

casacore::Vector<casacore::Double> Antenna::position(void) const
{
    return itsPosition;
}

casacore::Quantity Antenna::diameter(void) const
{
    return itsDiameter;
}

casacore::Quantity Antenna::delay(void) const
{
    return itsDelay;
}

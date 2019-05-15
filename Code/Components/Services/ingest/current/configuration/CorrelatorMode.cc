/// @file CorrelatorMode.cc
///
/// @copyright (c) 2014 CSIRO
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
#include "CorrelatorMode.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Quanta.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"

using namespace askap;
using namespace askap::cp::ingest;

CorrelatorMode::CorrelatorMode()
{
}

CorrelatorMode::CorrelatorMode(const std::string& modeName,
        const casacore::Quantity& chanWidth,
        const casacore::uInt nChan,
        const std::vector<casacore::Stokes::StokesTypes>& stokes,
        const casacore::uInt interval,
        const casacore::Quantity& freqOffset)
        : itsModeName(modeName), itsChanWidth(chanWidth), itsNChan(nChan),
        itsStokes(stokes), itsInterval(interval), itsFreqOffset(freqOffset)
{
    ASKAPCHECK(chanWidth.isConform("Hz"),
            "Channel width must conform to Hz");
    ASKAPCHECK(!stokes.empty(), "Stokes vector is empty");
}

const std::string& CorrelatorMode::name(void) const
{
    return itsModeName;
}

casacore::uInt CorrelatorMode::nChan(void) const
{
        return itsNChan;
}

const casacore::Quantity& CorrelatorMode::chanWidth(void) const
{
        return itsChanWidth;
}

/// @brief Frequency offset
/// @return bulk offset in frequency for the current configuration
const casacore::Quantity& CorrelatorMode::freqOffset() const
{
   return itsFreqOffset;
}

const std::vector<casacore::Stokes::StokesTypes>& CorrelatorMode::stokes(void) const
{
        return itsStokes;
}

casacore::uInt CorrelatorMode::interval(void) const
{
    return itsInterval;
}


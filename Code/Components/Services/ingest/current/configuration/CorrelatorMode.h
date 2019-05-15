/// @file CorrelatorMode.h
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

#ifndef ASKAP_CP_INGEST_CORRELATORMODE_H
#define ASKAP_CP_INGEST_CORRELATORMODE_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Quanta.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief This class encapsulates a "scan", a part of a larger observation.
class CorrelatorMode {
    public:
        /// @brief Constructor
        CorrelatorMode();

        /// @brief Constructor
        CorrelatorMode(const std::string& modeName,
             const casacore::Quantity& chanWidth,
             const casacore::uInt nChan,
             const std::vector<casacore::Stokes::StokesTypes>& stokes,
             const casacore::uInt interval,
             const casacore::Quantity& freqOffset);

        /// @brief Returns the correlator mode name
        const std::string& name(void) const;

        /// @brief The number of spectral channels
        casacore::uInt nChan(void) const;

        /// @brief The width (in Hz) of a single spectral channel.
        /// @note This may be a negative width in the case where increasing
        /// channel number corresponds to decreasing frequency.
        const casacore::Quantity& chanWidth(void) const;

        /// @brief The stokes types to be observed
        const std::vector<casacore::Stokes::StokesTypes>& stokes(void) const;

        /// @brief Returns, in microseconds, correlator integration interval.
        casacore::uInt interval(void) const;

        /// @brief Frequency offset
        /// @return bulk offset in frequency for the current configuration
        const casacore::Quantity& freqOffset() const;

    private:
        std::string itsModeName;
        casacore::Quantity itsChanWidth;
        casacore::uInt itsNChan;
        std::vector<casacore::Stokes::StokesTypes> itsStokes;
        casacore::uInt itsInterval;
        casacore::Quantity itsFreqOffset;
};

}
}
}

#endif

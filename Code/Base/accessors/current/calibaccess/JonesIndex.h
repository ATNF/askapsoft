/// @file JonesIndex.h
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

#ifndef ASKAP_ACCESSORS_JONES_INDEX_H
#define ASKAP_ACCESSORS_JONES_INDEX_H

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"

namespace askap {
namespace accessors {

/// @brief antenna/beam indices combined in a class to be used as an index 
/// @details Key type used for indexing into the calibration solution maps for the
/// GainSolution, LeakageSolution and BandpassSolution classes.
/// @ingroup calibaccess
class JonesIndex {

    public:
        /// Constructor
        ///
        /// @param[in] antenna  ID of the antenna. This must be the physical
        ///                     antenna ID.
        /// @param[in] beam     ID of the beam. Again, must map to an actual
        ///                     beam.
        JonesIndex(const casacore::Short antenna, casacore::Short beam);
        
        /// @brief constructor accepting uInt
        /// @param[in] antenna  ID of the antenna. This must be the physical
        ///                     antenna ID.
        /// @param[in] beam     ID of the beam. Again, must map to an actual
        ///                     beam.
        JonesIndex(const casacore::uInt antenna, casacore::uInt beam);
        

        /// Obtain the antenna ID
        /// @return the antenna ID
        casacore::Short antenna(void) const;

        /// Obtain the beam ID
        /// @return the beam ID
        casacore::Short beam(void) const;

        /// Operator...
        bool operator==(const JonesIndex& rhs) const;

        /// Operator...
        bool operator!=(const JonesIndex& rhs) const;

        /// Operator...
        bool operator<(const JonesIndex& rhs) const;

    private:
        casacore::Short itsAntenna;
        casacore::Short itsBeam;
};

};
};

#endif // #ifndef ASKAP_ACCESSORS_JONES_INDEX_H


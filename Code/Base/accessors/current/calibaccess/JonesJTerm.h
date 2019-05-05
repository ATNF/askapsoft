/// @file JonesJTerm.h
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

#ifndef ASKAP_ACCESSORS_JONESJTERM_H
#define ASKAP_ACCESSORS_JONESJTERM_H

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"
#include "casacore/casa/BasicSL/Complex.h"

namespace askap {
namespace accessors {

/// @brief JonesJTerm (parallel-hand gain)
/// @details This class is also used to represent bandpass (with a separate instance 
/// for each spectral channel)
/// @ingroup calibaccess
class JonesJTerm {

    public:
        /// @brief Constructor
        /// This default (no-args) constructor is needed by various containers,
        /// for instance to populate a vector or matrix with default values.
        /// This constructor will set g1Valid and g2Valid to false, indicating
        /// the data is not valid.
        JonesJTerm();

        /// @brief Constructor.
        /// @param[in] g1 gain for polarision 1;
        /// @param[in] g1Valid  flag indicating the validity of the data g1.
        ///                     Set this to true to indicate g1 contains a
        ///                     valid gain, otherwise false.
        /// @param[in] g2 gain for polarision 2;
        /// @param[in] g2Valid  flag indicating the validity of the data g2.
        ///                     Set this to true to indicate g1 contains a
        ///                     valid gain, otherwise false.
        JonesJTerm(const casacore::Complex& g1,
                   const casacore::Bool g1Valid,
                   const casacore::Complex& g2,
                   const casacore::Bool g2Valid);

        /// Returns the gain for polarisation 1.
        /// @return the gain for polarisation 1.
        casacore::Complex g1(void) const;

        /// Returns a flag indicating the validity of the data g1;
        /// @return true if g1 contains a valid gain, otherwise false.
        casacore::Bool g1IsValid(void) const;

        /// Returns the gain for polarisation 2.
        /// @return the gain for polarisation 2.
        casacore::Complex g2(void) const;

        /// Returns a flag indicating the validity of the data g2;
        /// @return true if g2 contains a valid gain, otherwise false.
        casacore::Bool g2IsValid(void) const;

    private:
        casacore::Complex itsG1;
        casacore::Bool itsG1Valid;
        casacore::Complex itsG2;
        casacore::Bool itsG2Valid;
};

};
};

#endif

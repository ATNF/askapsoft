/// @file JonesDTerm.cc
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
#include "JonesDTerm.h"

// Include package level header file
#include "askap_accessors.h"

// System includes

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"

// Using
using namespace askap::accessors;

JonesDTerm::JonesDTerm()
        : itsD12(-1.0, -1.0), itsD12Valid(false), itsD21(-1.0, -1.0), itsD21Valid(false)
{
}

/// @brief Constructor.
/// @param[in] d12 leakage from Y to X (element of the first row and second column of the Jones matrix) 
/// @param[in] d21 leakage from X to Y (element of the second row and first column of the Jones matrix)
/// @note validity flags are set (to true) for both d12 and d21
JonesDTerm::JonesDTerm(const casacore::Complex& d12,
                       const casacore::Complex& d21)
        : itsD12(d12), itsD12Valid(true), itsD21(d21), itsD21Valid(true)
{
}

/// @brief Constructor.
/// @param[in] d12 leakage from Y to X (element of the first row and second column of the Jones matrix) 
/// @param[in] d12Valid true, if d12 has a valid value
/// @param[in] d21 leakage from X to Y (element of the second row and first column of the Jones matrix)
/// @param[in] d21Valid true, if d21 has a valid value
JonesDTerm::JonesDTerm(const casacore::Complex& d12, const casacore::Bool d12Valid,
                       const casacore::Complex& d21, const casacore::Bool d21Valid) :
                       itsD12(d12), itsD12Valid(d12Valid), itsD21(d21), itsD21Valid(d21Valid) {}


casacore::Complex JonesDTerm::d12(void) const
{
    return itsD12;
}

casacore::Complex JonesDTerm::d21(void) const
{
    return itsD21;
}

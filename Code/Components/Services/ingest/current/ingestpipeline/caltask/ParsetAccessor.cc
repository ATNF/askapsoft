/// @file ParsetAccessor.cc
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
#include "ingestpipeline/caltask/ParsetAccessor.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>

// ASKAPsoft includes
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "Common/ParameterSet.h"
#include "casacore/casa/aipstype.h"
#include "casacore/casa/BasicSL/Complex.h"
#include "casacore/casa/Arrays/Vector.h"

using namespace askap::cp::ingest;

ParsetAccessor::ParsetAccessor(const LOFAR::ParameterSet& parset)
        : itsParset(parset)
{
}

casacore::Complex ParsetAccessor::getGain(casacore::uInt ant, casacore::uInt beam,
                                      ISolutionAccessor::Pol pol, casacore::Bool& valid) const
{
    std::string name("gain.");

    if (pol == XX) {
        name += "g11.";
    } else if (pol == YY) {
        name += "g22.";
    }

    name += askap::utility::toString<casacore::uInt>(ant) + "." + askap::utility::toString<casacore::uInt>(beam);

    // An exception is thrown in readComplex in the case the gain is not found,
    // so just set valid <- true here
    valid = true;
    return readComplex(name);
}

casacore::Complex ParsetAccessor::getLeakage(casacore::uInt ant, casacore::uInt beam,
        ISolutionAccessor::LeakageTerm term, casacore::Bool& valid) const
{
    ASKAPTHROW(AskapError, "ParsetAccessor::getLeakage() not implemented");
}

casacore::Complex ParsetAccessor::getBandpass(casacore::uInt ant, casacore::uInt beam,
        casacore::uInt chan, ISolutionAccessor::Pol pol,
        casacore::Bool& valid) const
{
    ASKAPTHROW(AskapError, "ParsetAccessor::getBandpass() not implemented");
}

/// @brief helper method to load complex parameter
/// @details It reads the value from itsParset and
/// forms a complex number.
/// @param[in] name parameter name
/// @return complex number
casacore::Complex ParsetAccessor::readComplex(const std::string& name) const
{
    casacore::Vector<casacore::Float> val = itsParset.getFloatVector(name);
    ASKAPCHECK(val.nelements() > 0, "Expect at least one element for " << name << " gain parameter");

    if (val.nelements() == 1) {
        return val[0];
    }

    ASKAPCHECK(val.nelements() == 2, "Expect either one or two elements to define complex value, you have: " << val);
    return casacore::Complex(val[0], val[1]);
}

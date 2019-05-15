/// @file DataserviceAccessor.cc
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
#include "cmodel/DataserviceAccessor.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes

// ASKAPsoft include
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"

// Using
using namespace casacore;
using namespace askap;
using namespace askap::cp::pipelinetasks;
using namespace askap::cp::sms::client;

ASKAP_LOGGER(logger, ".DataserviceAccessor");

DataserviceAccessor::DataserviceAccessor(const std::string& locatorHost,
        const std::string& locatorPort,
        const std::string& serviceName = "SkyModelService")
    : itsService(locatorHost, locatorPort, serviceName)
{
}

DataserviceAccessor::~DataserviceAccessor()
{
}

ComponentListPtr DataserviceAccessor::coneSearch(
    const casacore::Quantity& ra,
    const casacore::Quantity& dec,
    const casacore::Quantity& searchRadius,
    const casacore::Quantity& fluxLimit)
{
    // Pre-conditions
    ASKAPCHECK(ra.isConform("deg"), "ra must conform to degrees");
    ASKAPCHECK(dec.isConform("deg"), "dec must conform to degrees");
    ASKAPCHECK(searchRadius.isConform("deg"), "searchRadius must conform to degrees");
    ASKAPCHECK(fluxLimit.isConform("Jy"), "fluxLimit must conform to Jy");

    ASKAPLOG_DEBUG_STR(logger, "Cone search - ra: " << ra.getValue("deg")
                           << " deg, dec: " << dec.getValue("deg")
                           << " deg, radius: " << searchRadius.getValue("deg")
                           << " deg, Fluxlimit: " << fluxLimit.getValue("Jy") << " Jy");

    return itsService.coneSearch(ra, dec, searchRadius, fluxLimit);
}

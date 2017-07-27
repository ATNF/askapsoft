/// @file SkyModelServiceImpl.cc
///
/// @copyright (c) 2016 CSIRO
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
/// @author Daniel Collins <daniel.collins@csiro.au>

// Include own header file first
#include "SkyModelServiceImpl.h"

// Include package level header file
#include "askap_skymodel.h"

// System includes
#include <string>

// ASKAPsoft includes
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>

// Local package includes
#include "DataMarshalling.h"
#include "GlobalSkyModel.h"
#include "QueryBuilder.h"
#include "Utility.h"

ASKAP_LOGGER(logger, ".SkyModelService");

using namespace odb;
using namespace askap::cp::sms;
using namespace askap::interfaces::skymodelservice;


SkyModelServiceImpl* SkyModelServiceImpl::create(const LOFAR::ParameterSet& parset)
{
    ASKAPLOG_DEBUG_STR(logger, "factory");
    boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(parset));
    SkyModelServiceImpl* pImpl = new SkyModelServiceImpl(pGsm);
    ASKAPCHECK(pImpl, "SkyModelServiceImpl creation failed");
    return pImpl;
}

SkyModelServiceImpl::SkyModelServiceImpl(
    boost::shared_ptr<GlobalSkyModel> gsm)
    :
    itsGsm(gsm)
{
    ASKAPLOG_DEBUG_STR(logger, "ctor");
}

SkyModelServiceImpl::~SkyModelServiceImpl()
{
    ASKAPLOG_DEBUG_STR(logger, "dtor");
}

std::string SkyModelServiceImpl::getServiceVersion(const Ice::Current&)
{
    ASKAPLOG_DEBUG_STR(logger, "getServiceVersion");
    // TODO: should this return ASKAP_PACKAGE_VERSION? Or a semantic version?
    return "1.0";
}

ComponentSeq SkyModelServiceImpl::coneSearch(
    const sms_interface::Coordinate& centre,
    double radius,
    const sms_interface::SearchCriteria& criteria,
    const Ice::Current&)
{
    GlobalSkyModel::ComponentListPtr results = itsGsm->coneSearch(
        Coordinate(centre),
        radius,
        queryBuilder(criteria));
    return marshallComponentsToDTO(results);
}

ComponentSeq SkyModelServiceImpl::rectSearch(
    const sms_interface::Rect& roi,
    const sms_interface::SearchCriteria& criteria,
    const Ice::Current&)
{
    GlobalSkyModel::ComponentListPtr results = itsGsm->rectSearch(
        Rect(roi),
        queryBuilder(criteria));
    return marshallComponentsToDTO(results);
}

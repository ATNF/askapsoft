/// @file {{cookiecutter.service_class_name}}Impl.cc
///
/// @copyright (c) {{cookiecutter.year}} CSIRO
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
/// @author {{cookiecutter.user_name}} <{{cookiecutter.user_email}}>

// Include own header file first
#include "{{cookiecutter.service_class_name}}Impl.h"

// Include package level header file
#include "askap_{{cookiecutter.package_name}}.h"

// System includes
#include <string>

// ASKAPsoft includes
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>

// Local package includes
//#include ".h"

ASKAP_LOGGER(logger, ".{{cookiecutter.service_class_name}}");

using namespace askap::cp::{{cookiecutter.namespace}};
using namespace askap::interfaces::{{cookiecutter.ice_interface_namespace}};


{{cookiecutter.service_class_name}}Impl* {{cookiecutter.service_class_name}}Impl::create(const LOFAR::ParameterSet& parset)
{
    ASKAPLOG_DEBUG_STR(logger, "factory");
    {{cookiecutter.service_class_name}}Impl* pImpl = new {{cookiecutter.service_class_name}}Impl();
    ASKAPCHECK(pImpl, "{{cookiecutter.service_class_name}}Impl creation failed");
    return pImpl;
}

{{cookiecutter.service_class_name}}Impl::{{cookiecutter.service_class_name}}Impl()
{
    ASKAPLOG_DEBUG_STR(logger, "ctor");
}

{{cookiecutter.service_class_name}}Impl::~{{cookiecutter.service_class_name}}Impl()
{
    ASKAPLOG_DEBUG_STR(logger, "dtor");
}

std::string {{cookiecutter.service_class_name}}Impl::getServiceVersion(const Ice::Current&)
{
    ASKAPLOG_DEBUG_STR(logger, "getServiceVersion");
    // TODO: should this return ASKAP_PACKAGE_VERSION? Or a semantic version?
    return "1.0";
}

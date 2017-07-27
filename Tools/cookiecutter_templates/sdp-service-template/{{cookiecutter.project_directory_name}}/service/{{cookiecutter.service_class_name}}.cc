/// @file {{cookiecutter.service_class_name}}.cc
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
#include "{{cookiecutter.service_class_name}}.h"

// Include package level header file
#include "askap_{{cookiecutter.package_name}}.h"

// System includes
#include <string>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <Common/ParameterSet.h>
#include <iceutils/CommunicatorConfig.h>
#include <iceutils/CommunicatorFactory.h>
#include <iceutils/ServiceManager.h>

// Local includes
#include "{{cookiecutter.service_class_name}}Impl.h"

ASKAP_LOGGER(logger, ".{{cookiecutter.service_class_name}}");

using namespace askap;
using namespace askap::cp::{{cookiecutter.namespace}};
using namespace askap::cp::icewrapper;

{{cookiecutter.service_class_name}}::{{cookiecutter.service_class_name}}(const LOFAR::ParameterSet& parset) :
    itsParset(parset),
    itsComm(),
    itsServiceManager()
{
    ASKAPLOG_INFO_STR(logger, ASKAP_PACKAGE_VERSION);

    // grab Ice config from parset
    const LOFAR::ParameterSet& iceParset = parset.makeSubset("ice.");
    const string locatorHost = iceParset.get("locator_host");
    const string locatorPort = iceParset.get("locator_port");
    const string serviceName = iceParset.get("service_name");
    const string adapterName = iceParset.get("adapter_name");
    const string adapterEndpoints = iceParset.get("adapter_endpoints");
    ASKAPLOG_DEBUG_STR(logger, "locator host: " << locatorHost);
    ASKAPLOG_DEBUG_STR(logger, "locator port: " << locatorPort);
    ASKAPLOG_DEBUG_STR(logger, "service name: " << serviceName);
    ASKAPLOG_DEBUG_STR(logger, "adapter name: " << adapterName);
    ASKAPLOG_DEBUG_STR(logger, "adapter endpoints: " << adapterEndpoints);

    // instantiate communicator
    CommunicatorConfig cc(locatorHost, locatorPort);
    cc.setAdapter(adapterName, adapterEndpoints, true);
    CommunicatorFactory commFactory;
    itsComm = commFactory.createCommunicator(cc);

    // assemble the service manager
    itsServiceManager.reset(
        new ServiceManager(
            itsComm,
            {{cookiecutter.service_class_name}}Impl::create(parset),
            serviceName,
            adapterName));
}

{{cookiecutter.service_class_name}}::~{{cookiecutter.service_class_name}}()
{
    ASKAPLOG_INFO_STR(logger, "Shutting down");

    // stop the service manager
    if (itsServiceManager.get()) {
        itsServiceManager->stop();
        itsServiceManager.reset();
    }

    // destroy communicator
    if (itsComm.get())
        itsComm->destroy();
}

void {{cookiecutter.service_class_name}}::run(void)
{
    ASKAPLOG_INFO_STR(logger, "Running");
    itsServiceManager->start(true);
    std::cerr << "Pre-waitForShutdown\n";
    itsServiceManager->waitForShutdown();
    ASKAPLOG_INFO_STR(logger, "Post-waitForShutdown");
    std::cerr << "Post-waitForShutdown\n";
}

/// @file {{cookiecutter.cli_app_name}}.cc
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

// Include package level header file
#include <askap_{{cookiecutter.package_name}}.h>

// System includes
#include <string>
#include <fstream>
#include <sstream>

// Boost includes

// ASKAPsoft includes
#include <askap/Application.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askap/StatReporter.h>
#include <Common/ParameterSet.h>
#include <IceUtil/Exception.h>

// Local Package includes
#include "service/{{cookiecutter.service_class_name}}.h"

using namespace askap;
using namespace askap::cp::{{cookiecutter.namespace}};

ASKAP_LOGGER(logger, ".main");

class {{cookiecutter.cli_app_class_name}} : public askap::Application {
    public:
        virtual int run(int argc, char* argv[])
        {
            StatReporter stats;

            try {
                {{cookiecutter.service_class_name}} service(config());
                service.run();
            } catch (const askap::AskapError& e) {
                ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << e.what());
                return 1;
            } catch (const Ice::CommunicatorDestroyedException& e) {
                ASKAPLOG_FATAL_STR(logger, "Ice communicator destroyed " << argv[0] << ": " << e.what());
                return 2;
            } catch (const std::exception& e) {
                ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << e.what());
                return 3;
            }

            stats.logSummary();

            return 0;
        }
};

int main(int argc, char *argv[])
{
    {{cookiecutter.cli_app_class_name}} app;
    return app.main(argc, argv);
}

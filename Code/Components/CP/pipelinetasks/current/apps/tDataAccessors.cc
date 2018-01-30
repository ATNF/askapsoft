/// @file tDataAccessors.cc
///
/// @copyright (c) 2018 CSIRO
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

// System includes
#include <iostream>
#include <string>

// ASKAPsoft includes
#include "askap/Application.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"

// Local package includes

// Using
//using namespace askap::cp;
//using namespace askap::cp::icewrapper;

ASKAP_LOGGER(logger, ".tDataAccessors");

class TestDataAccessorsApp : public askap::Application
{
    public:
        virtual int run(int argc, char* argv[])
        {
            const std::string locatorHost = config().getString("ice.locator_host");
            const std::string locatorPort = config().getString("ice.locator_port");
            const std::string adapterName = config().getString("ice.adapter_name");

            std::cout << "locator host: " << locatorHost << "\n"
                << "locator port: " << locatorPort << "\n"
                << "adapter name: " << adapterName << "\n"
                << std::endl;

            return 0;
        }
};

int main(int argc, char *argv[])
{
    TestDataAccessorsApp app;
    return app.main(argc, argv);
}

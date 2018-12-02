/// @file tservice.cc
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

// ASKAPsoft includes
#include <AskapTestRunner.h>

// Test includes
#include "ServiceTest.h"

int main(int argc, char *argv[])
{
    // Set up the test runner
    askapdev::testutils::AskapTestRunner runner(argv[0]);

    // Add all the tests
    runner.addTest(askap::cp::{{cookiecutter.namespace}}::ServiceTest::suite());

    // Run
    const bool wasSucessful = runner.run();

    return wasSucessful ? 0 : 1;
}

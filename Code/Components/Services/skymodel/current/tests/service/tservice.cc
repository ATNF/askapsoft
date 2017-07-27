/// @file tstub.cc
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

// ASKAPsoft includes
#include <AskapTestRunner.h>

// Test includes
#include "HealpixTest.h"
#include "GlobalSkyModelTest.h"
#include "ServiceTest.h"
#include "SmsTypesTest.h"
#include "UtilityTest.h"
#include "VOTableDataTest.h"

int main(int argc, char *argv[])
{
    // Set up the test runner
    askapdev::testutils::AskapTestRunner runner(argv[0]);

    // Add all the tests
    runner.addTest(askap::cp::sms::VOTableDataTest::suite());
    runner.addTest(askap::cp::sms::GlobalSkyModelTest::suite());
    runner.addTest(askap::cp::sms::ServiceTest::suite());
    runner.addTest(askap::cp::sms::HealpixTest::suite());
    runner.addTest(askap::cp::sms::UtilityTest::suite());
    runner.addTest(askap::cp::sms::SmsTypesTest::suite());

    // Run
    const bool wasSucessful = runner.run();

    return wasSucessful ? 0 : 1;
}

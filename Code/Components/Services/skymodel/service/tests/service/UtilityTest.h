/// @file UtilityTest.cc
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

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <string>
#include <boost/math/constants/constants.hpp>

// Classes to test
#include "service/Utility.h"

using std::string;
using std::vector;

namespace askap {
namespace cp {
namespace sms {

class UtilityTest : public CppUnit::TestFixture {

        CPPUNIT_TEST_SUITE(UtilityTest);
        CPPUNIT_TEST(testDegreesToRadians_float);
        CPPUNIT_TEST(testDegreesToRadians_double);
        CPPUNIT_TEST(testWrapAngleInRange);
        CPPUNIT_TEST(testWrapAngleLarge);
        CPPUNIT_TEST(testWrapAngleNegative);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
        }

        void tearDown() {
        }

        void testDegreesToRadians_float() {
            float degrees = 90.0f;
            float expected = boost::math::float_constants::pi / 2.0f;
            float actual = utility::degreesToRadians<float>(degrees);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, actual, 0.000001);
        }

        void testDegreesToRadians_double() {
            double degrees = 293.7;
            double expected = degrees * boost::math::double_constants::pi / 180;
            double actual = utility::degreesToRadians<double>(degrees);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, actual, 0.000001);
        }

        void testWrapAngleInRange() {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                34.092,
                utility::wrapAngleDegrees(34.092),
                0.000001);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                127.999,
                utility::wrapAngleDegrees(127.999),
                0.000001);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                0.0,
                utility::wrapAngleDegrees(0.0),
                0.000001);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                359.999,
                utility::wrapAngleDegrees(359.999),
                0.000001);
        }

        void testWrapAngleLarge() {
            double expected = 43.9501;
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                expected,
                utility::wrapAngleDegrees(expected + 360.0),
                0.000001);

            expected = 343.9501;
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                expected,
                utility::wrapAngleDegrees(expected + 360.0 * 7),
                0.000001);

            expected = 0;
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                expected,
                utility::wrapAngleDegrees(expected + 360.0),
                0.000001);
        }

        void testWrapAngleNegative() {
            double expected = 359.5;
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                expected,
                utility::wrapAngleDegrees(expected - 360.0),
                0.000001);

            expected = 121.12;
            CPPUNIT_ASSERT_DOUBLES_EQUAL(
                expected,
                utility::wrapAngleDegrees(expected - 360.0 * 3),
                0.000001);
        }
    private:

};

}
}
}

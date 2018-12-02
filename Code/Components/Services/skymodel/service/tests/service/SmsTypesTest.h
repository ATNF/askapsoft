/// @file SmsTypesTest.cc
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
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
/// See the GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Daniel Collins <daniel.collins@csiro.au>

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Classes to test
#include "service/SmsTypes.h"

using std::string;
using std::vector;

namespace askap {
namespace cp {
namespace sms {

class SmsTypesTest : public CppUnit::TestFixture {

        CPPUNIT_TEST_SUITE(SmsTypesTest);
        CPPUNIT_TEST(testExtentsZero);
        CPPUNIT_TEST(testExtentsNegative);
        CPPUNIT_TEST(testCoordinateRangeChecks);
        CPPUNIT_TEST(testRectVerticesAroundZero);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
        }

        void tearDown() {
        }

        void testExtentsZero() {
            CPPUNIT_ASSERT_THROW(Extents(0, 9), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Extents(8, 0), askap::AskapError);
        }

        void testExtentsNegative() {
            CPPUNIT_ASSERT_THROW(Extents(-0.10, 9), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Extents(8, -20), askap::AskapError);
        }

        void testCoordinateRangeChecks() {
            CPPUNIT_ASSERT_THROW(Coordinate(-0.10, 0), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Coordinate(360.10, 0), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Coordinate(0, -90.10), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Coordinate(359.99, 90.001), askap::AskapError);
        }

        void testRectVerticesAroundZero() {
            Rect rect(Coordinate(0, 0), Extents(2,2));

            // expected vertices
            Coordinate tl(359.0,  1);
            Coordinate tr(  1.0,  1);
            Coordinate bl(359.0, -1);
            Coordinate br(  1.0, -1);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(tl.ra, rect.topLeft().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tl.dec, rect.topLeft().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tr.ra, rect.topRight().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tr.dec, rect.topRight().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bl.ra, rect.bottomLeft().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bl.dec, rect.bottomLeft().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(br.ra, rect.bottomRight().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(br.dec, rect.bottomRight().dec, 0.000001);
        }

        void testRectVerticesAllPositive() {
            Rect rect(Coordinate(10, 39), Extents(5, 10));

            // expected vertices
            Coordinate tl(7.5, 44.0);
            Coordinate tr(12.5, 44.0);
            Coordinate bl(7.5, 34.0);
            Coordinate br(12.5, 34.0);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(tl.ra, rect.topLeft().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tl.dec, rect.topLeft().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tr.ra, rect.topRight().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tr.dec, rect.topRight().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bl.ra, rect.bottomLeft().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bl.dec, rect.bottomLeft().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(br.ra, rect.bottomRight().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(br.dec, rect.bottomRight().dec, 0.000001);
        }

        void testRectVerticesNegativeDec() {
            Rect rect(Coordinate(10, -39), Extents(5, 10));

            // expected vertices
            Coordinate tl(7.5, -34.0);
            Coordinate tr(12.5, -34.0);
            Coordinate bl(7.5, -44.0);
            Coordinate br(12.5, -44.0);

            CPPUNIT_ASSERT_DOUBLES_EQUAL(tl.ra, rect.topLeft().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tl.dec, rect.topLeft().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tr.ra, rect.topRight().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(tr.dec, rect.topRight().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bl.ra, rect.bottomLeft().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(bl.dec, rect.bottomLeft().dec, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(br.ra, rect.bottomRight().ra, 0.000001);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(br.dec, rect.bottomRight().dec, 0.000001);
        }
};

}
}
}

/// @file AntennaTest.cc
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

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include "askap/askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Arrays/Vector.h"

// Classes to test
#include "configuration/Antenna.h"

using namespace std;

namespace askap {
namespace cp {
namespace ingest {

class AntennaTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(AntennaTest);
        CPPUNIT_TEST(testAll);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
        };

        void tearDown() {
        }

        void testAll() {
            casacore::Double dblTolerance = 1e-15;

            const casacore::String name = "ak01";
            const casacore::String mount = "equatorial";
            casacore::Vector<casacore::Double> position(3);
            position(0) = -2556084.669;
            position(1) = 5097398.337;
            position(2) = -2848424.133;
            const casacore::Quantity diameter(12, "m");
            const casacore::Quantity delay(-2.2, "ns");

            // Create instance
            Antenna instance(name, mount, position, diameter, delay);

            // Test instance
            CPPUNIT_ASSERT(name == instance.name());
            CPPUNIT_ASSERT(mount == instance.mount());
            CPPUNIT_ASSERT(position.nelements() == instance.position().nelements());
            for (size_t i = 0; i < position.nelements(); ++i) {
                CPPUNIT_ASSERT_DOUBLES_EQUAL(position(i), instance.position()(i), dblTolerance);
            }
            CPPUNIT_ASSERT(diameter == instance.diameter());
            CPPUNIT_ASSERT(delay == instance.delay());

            // Check exceptional inputs
            const casacore::Quantity badDiameter(12, "rad");
            CPPUNIT_ASSERT_THROW(Antenna(name, mount, position, badDiameter, delay), 
                    askap::AskapError);
            casacore::Vector<casacore::Double> badPosition(2);
            CPPUNIT_ASSERT_THROW(Antenna(name, mount, badPosition, diameter, delay),
                    askap::AskapError);
            const casacore::Quantity badDelay(12, "rad");
            CPPUNIT_ASSERT_THROW(Antenna(name, mount, position, diameter, badDelay), 
                    askap::AskapError);
        }
};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

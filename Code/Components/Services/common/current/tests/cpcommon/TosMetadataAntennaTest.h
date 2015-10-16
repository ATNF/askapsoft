/// @file TosMetadataAntennaTest.cc
///
/// @copyright (c) 2010 CSIRO
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
#include "casa/aips.h"
#include "boost/scoped_ptr.hpp"
#include "measures/Measures/MDirection.h"
#include "askap/AskapError.h"
#include "casa/Quanta/Quantum.h"
#include "Blob/BlobIStream.h"
#include "Blob/BlobIBufVector.h"
#include "Blob/BlobOStream.h"
#include "Blob/BlobOBufVector.h"

// Classes to test
#include "cpcommon/TosMetadataAntenna.h"

// Using
using namespace casa;
using namespace LOFAR;

namespace askap {
namespace cp {

class TosMetadataAntennaTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(TosMetadataAntennaTest);
        CPPUNIT_TEST(testName);
        CPPUNIT_TEST(testActualRaDec);
        CPPUNIT_TEST(testActualAzEl);
        CPPUNIT_TEST(testPolAngle);
        CPPUNIT_TEST(testOnSource);
        CPPUNIT_TEST(testHwError);
        CPPUNIT_TEST(testSerialise);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
            instance.reset(new TosMetadataAntenna("ak01"));
        }

        void tearDown() {
            instance.reset();
        }

        void testName() {
            const casa::String antennaName("ak01");
            CPPUNIT_ASSERT_EQUAL(antennaName, instance->name());
        };

        void testActualRaDec() {
            MDirection testDir(Quantity(20, "deg"),
                               Quantity(-10, "deg"),
                               MDirection::Ref(MDirection::J2000));

            instance->actualRaDec(testDir); // Set
            directionsEqual(testDir, instance->actualRaDec());
        }

        void testActualAzEl() {
            MDirection testDir(Quantity(90, "deg"),
                               Quantity(45, "deg"),
                               MDirection::Ref(MDirection::AZEL));

            instance->actualAzEl(testDir); // Set
            directionsEqual(testDir, instance->actualAzEl());
        }

        void testPolAngle() {
            const Quantity testVal = Quantity(1.123456, "rad");
            instance->actualPolAngle(testVal);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(testVal.getValue("rad"), instance->actualPolAngle().getValue("rad"),1e-6);
        }

        void testOnSource() {
            instance->onSource(true);
            CPPUNIT_ASSERT_EQUAL(true, instance->onSource());
            instance->onSource(false);
            CPPUNIT_ASSERT_EQUAL(false, instance->onSource());
        }

        void testHwError() {
            instance->flagged(true);
            CPPUNIT_ASSERT_EQUAL(true, instance->flagged());
            instance->flagged(false);
            CPPUNIT_ASSERT_EQUAL(false, instance->flagged());
        }

        void testSerialise() {
            // call test cases above as they fill 'instance' with some numbers
            CPPUNIT_ASSERT(instance);
            testActualRaDec();
            testActualAzEl();
            testPolAngle();
            instance->onSource(true);
            instance->flagged(false);
            
            TosMetadataAntenna received("none");

            // Encode
            std::vector<int8_t> buf;
            LOFAR::BlobOBufVector<int8_t> obv(buf, expandSize);
            LOFAR::BlobOStream out(obv);
            out.putStart("TosMetadataAntennaTest", 1);
            out << *instance;
            out.putEnd();

            // Decode
            LOFAR::BlobIBufVector<int8_t> ibv(buf);
            LOFAR::BlobIStream in(ibv);
            int version = in.getStart("TosMetadataAntennaTest");
            ASKAPASSERT(version == 1);
            in >> received;
            in.getEnd();

            
            // check the result
            CPPUNIT_ASSERT_EQUAL(false, received.flagged());
            CPPUNIT_ASSERT_EQUAL(true, received.onSource());
            CPPUNIT_ASSERT_DOUBLES_EQUAL(received.actualPolAngle().getValue("rad"), 
                    instance->actualPolAngle().getValue("rad"),1e-6);
            directionsEqual(received.actualAzEl(), instance->actualAzEl());
            directionsEqual(received.actualRaDec(), instance->actualRaDec());
            CPPUNIT_ASSERT_EQUAL(instance->name(),received.name());
        }

    private:

        // Compare two MDirection instances, returning true if they are
        // equal, otherwise false
        void directionsEqual(const MDirection& dir1, const MDirection& dir2) {
            for (int i = 0; i < 2; ++i) {
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(dir1.getAngle().getValue()(i),
                        dir2.getAngle().getValue()(i), 1e-6);
            }
            CPPUNIT_ASSERT_EQUAL(dir1.getRef().getType(), dir2.getRef().getType());
        }

        // Instance of class under test
        boost::scoped_ptr<TosMetadataAntenna> instance;

        // Expand size. Size of increment for Blob BufVector storage.
        // Too small and there is lots of overhead in expanding the vector.
        static const unsigned int expandSize = 4 * 1024 * 1024;
};

}   // End namespace cp
}   // End namespace askap

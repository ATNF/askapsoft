/// @file TosMetadataTest.cc
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
#include "boost/scoped_ptr.hpp"
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"

#include "Blob/BlobIStream.h"
#include "Blob/BlobIBufVector.h"
#include "Blob/BlobOStream.h"
#include "Blob/BlobOBufVector.h"

// Classes to test
#include "cpcommon/TosMetadata.h"

// Using
using namespace casacore;

namespace askap {
namespace cp {

class TosMetadataTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(TosMetadataTest);
        CPPUNIT_TEST(testConstructor);
        CPPUNIT_TEST(testCopy);
        CPPUNIT_TEST(testAssignment);
        CPPUNIT_TEST(testAddAntenna);
        CPPUNIT_TEST(testAddAntennaDuplicate);
        CPPUNIT_TEST(testTime);
        CPPUNIT_TEST(testScanId);
        CPPUNIT_TEST(testFlagged);
        CPPUNIT_TEST(testTargetName);
        CPPUNIT_TEST(testTargetDirection);
        CPPUNIT_TEST(testPhaseDirection);
        CPPUNIT_TEST(testCorrMode);
        CPPUNIT_TEST(testBeamOffsets);
        CPPUNIT_TEST_EXCEPTION(testBeamOffsetsException, AskapError);
        CPPUNIT_TEST(testAntennaAccess);
        CPPUNIT_TEST(testAntennaInvalid);
        CPPUNIT_TEST(testSerialisation);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
            instance.reset(new TosMetadata());
        }

        void tearDown() {
            instance.reset();
        }

        void testConstructor() {
            CPPUNIT_ASSERT_EQUAL(0u, instance->nAntenna());
            CPPUNIT_ASSERT_EQUAL(0ul, instance->time());
        }

        void testAddAntenna() {
            const casacore::uInt nAntenna = 36;

            for (casacore::uInt i = 0; i < nAntenna; ++i) {
                CPPUNIT_ASSERT_EQUAL(i, instance->nAntenna());
                TosMetadataAntenna ant("ak" + utility::toString(i));
                instance->addAntenna(ant);
            }

            CPPUNIT_ASSERT_EQUAL(nAntenna, instance->nAntenna());
        };

        void testAddAntennaDuplicate() {
            TosMetadataAntenna ant1("ak01");
            instance->addAntenna(ant1);
            CPPUNIT_ASSERT_EQUAL(1u, instance->nAntenna());
            CPPUNIT_ASSERT_THROW(instance->addAntenna(ant1), askap::AskapError);
            CPPUNIT_ASSERT_EQUAL(1u, instance->nAntenna());

            TosMetadataAntenna ant2("ak01"); // Different instance, same name
            CPPUNIT_ASSERT_THROW(instance->addAntenna(ant2), askap::AskapError);
            CPPUNIT_ASSERT_EQUAL(1u, instance->nAntenna());
        }

        void testTime() {
            const uLong testVal = 1234;
            instance->time(testVal);
            CPPUNIT_ASSERT_EQUAL(testVal, instance->time());
        }

        void testScanId() {
            for (casacore::Int i = -2; i < 10; ++i) {
                instance->scanId(i);
                CPPUNIT_ASSERT_EQUAL(i, instance->scanId());
            }
        }

        void testFlagged() {
            instance->flagged(true);
            CPPUNIT_ASSERT_EQUAL(true, instance->flagged());
            instance->flagged(false);
            CPPUNIT_ASSERT_EQUAL(false, instance->flagged());
        }

        void testTargetName() {
            CPPUNIT_ASSERT(instance->targetName() == "");
            instance->targetName("1934-638");
            CPPUNIT_ASSERT(instance->targetName() == "1934-638");
        }

        void testTargetDirection() {
            const MDirection dir(Quantity(187.5, "deg"),
                    MDirection::Ref(MDirection::J2000));
            instance->targetDirection(dir);
        }

        void testPhaseDirection() {
            const MDirection dir(Quantity(187.5, "deg"),
                    MDirection::Ref(MDirection::J2000));
            instance->phaseDirection(dir);
        }

        void testCorrMode() {
            CPPUNIT_ASSERT(instance->corrMode() == "");
            instance->corrMode("standard");
            CPPUNIT_ASSERT(instance->corrMode() == "standard");
        }

        void testBeamOffsets() {
            CPPUNIT_ASSERT_EQUAL(size_t(0u),instance->beamOffsets().nelements());
            casacore::Matrix<casacore::Double> beamOffsets(2, 5);
            for (size_t beam = 0; beam < beamOffsets.ncolumn(); ++beam) {
                 beamOffsets(0,beam) = casacore::C::pi / 30. * double(beam);
                 beamOffsets(1,beam) = -casacore::C::pi / 60. * double(beam);
            }
            instance->beamOffsets(beamOffsets);
            const int nBeam = beamOffsets.ncolumn();
            CPPUNIT_ASSERT_EQUAL(size_t(2u), instance->beamOffsets().nrow());
            CPPUNIT_ASSERT_EQUAL(size_t(nBeam), instance->beamOffsets().ncolumn());
            // this should restore the pristince state without triggering an exception
            instance->beamOffsets(casacore::Matrix<casacore::Double>());
            CPPUNIT_ASSERT_EQUAL(size_t(0u),instance->beamOffsets().nelements());
            CPPUNIT_ASSERT_EQUAL(size_t(0u), instance->beamOffsets().nrow());
            CPPUNIT_ASSERT_EQUAL(size_t(0u), instance->beamOffsets().ncolumn());
            // set the matrix back
            instance->beamOffsets(beamOffsets);
            // and invalidate the original matrix, so unhandled reference semantics would trigger an exception
            beamOffsets.resize(0,0);
            CPPUNIT_ASSERT_EQUAL(size_t(2u), instance->beamOffsets().nrow());
            CPPUNIT_ASSERT_EQUAL(size_t(nBeam), instance->beamOffsets().ncolumn());
            for (int beam = 0; beam < nBeam; ++beam) {
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(casacore::C::pi / 30. * double(beam), instance->beamOffsets()(0,beam), 1e-6);
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(-casacore::C::pi / 60. * double(beam), instance->beamOffsets()(1,beam), 1e-6);
            }
        }

        void testBeamOffsetsException() {
            // this will throw an exception as the number of coordinates should be exactly 2
            instance->beamOffsets(casacore::Matrix<casacore::Double>(3,5,0.));
        }

        void testAntennaAccess() {
            const casacore::String ant1Name = "ak01";
            const casacore::String ant2Name = "ak02";
            const TosMetadataAntenna a1(ant1Name);
            const TosMetadataAntenna a2(ant2Name);

            CPPUNIT_ASSERT_EQUAL(0u, instance->nAntenna());
            instance->addAntenna(a1);
            CPPUNIT_ASSERT_EQUAL(1u, instance->nAntenna());
            instance->addAntenna(a2);
            CPPUNIT_ASSERT_EQUAL(2u, instance->nAntenna());

            const TosMetadataAntenna& ant1 = instance->antenna(ant1Name);
            CPPUNIT_ASSERT_EQUAL(ant1Name, ant1.name());
            const TosMetadataAntenna& ant2 = instance->antenna(ant2Name);
            CPPUNIT_ASSERT_EQUAL(ant2Name, ant2.name());
        }

        void testAntennaInvalid() {
            TosMetadataAntenna ant("ak01");
            instance->addAntenna(ant);

            // Request an invalid antenna id (wrong name)
            CPPUNIT_ASSERT_THROW(instance->antenna(""), askap::AskapError);
            CPPUNIT_ASSERT_THROW(instance->antenna("ak2"), askap::AskapError);
        }

       
 
        void testCopy() {
            populateCurrentInstance();

            TosMetadata copy(*instance);
            // to ensure (lack of) reference semantics is covered
            instance.reset();
           
            verifyResult(copy);
        }

        void testAssignment() {
            populateCurrentInstance();

            TosMetadata copy;
            copy = *instance;
            // to ensure (lack of) reference semantics is covered
            instance.reset();
           
            verifyResult(copy);
        }
    
        void testSerialisation() {
            populateCurrentInstance();

            TosMetadata received;

            // Encode
            std::vector<int8_t> buf;
            LOFAR::BlobOBufVector<int8_t> obv(buf);
            LOFAR::BlobOStream out(obv);
            out.putStart("TosMetadataTest", 1);
            out << *instance;
            out.putEnd();

            // Decode
            LOFAR::BlobIBufVector<int8_t> ibv(buf);
            LOFAR::BlobIStream in(ibv);
            int version = in.getStart("TosMetadataTest");
            ASKAPASSERT(version == 1);
            in >> received;
            in.getEnd();

            // test content
            verifyResult(received);
        }
    protected:
        // fill current instance with test values (shared method between copy,asignment and serialisation tests)
        void populateCurrentInstance() {
            // use test methods defined above as they populate the "instance"
            CPPUNIT_ASSERT(instance);
            testAntennaAccess();
            testCorrMode();
            testPhaseDirection();
            testTargetDirection();
            testTargetName();
            testTime();
            testBeamOffsets();
            instance->flagged(true);
            instance->scanId(30);
        }

        // verify given metadata object - expected to match that produced by populateCurrentInstance
        void verifyResult(const TosMetadata &received) {
            // test content
            CPPUNIT_ASSERT_EQUAL(casacore::String("ak01"), received.antenna("ak01").name());
            CPPUNIT_ASSERT_EQUAL(casacore::String("ak02"), received.antenna("ak02").name());
            CPPUNIT_ASSERT_EQUAL(2u, received.nAntenna());
            CPPUNIT_ASSERT_EQUAL(std::string("standard"), received.corrMode());
            CPPUNIT_ASSERT_EQUAL(true, received.flagged());
            CPPUNIT_ASSERT_EQUAL(std::string("1934-638"), received.targetName());
            CPPUNIT_ASSERT_EQUAL(1234ul, received.time());
            CPPUNIT_ASSERT_EQUAL(30, received.scanId());
            CPPUNIT_ASSERT_EQUAL(size_t(2u), received.beamOffsets().nrow());
            const int nBeam = 5;
            CPPUNIT_ASSERT_EQUAL(size_t(nBeam), received.beamOffsets().ncolumn());
            for (int beam = 0; beam < nBeam; ++beam) {
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(casacore::C::pi / 30. * double(beam), received.beamOffsets()(0,beam), 1e-6);
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(-casacore::C::pi / 60. * double(beam), received.beamOffsets()(1,beam), 1e-6);
            }
            // add check of direction fields here when time permits
        }

    private:
        // Instance of class under test
        boost::scoped_ptr<TosMetadata> instance;
};

}   // End namespace cp

}   // End namespace askap

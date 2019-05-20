/// @file SkyModelServiceClientTest.h
///
/// @copyright (c) 2017 CSIRO
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
#include <limits>
#include "casacore/casa/aipstype.h"
#include "askap/askap/AskapError.h"
#include "casacore/casa/Quanta/Quantum.h"

// Boost includes
#include "boost/random/uniform_real.hpp"
#include "boost/random/variate_generator.hpp"
#include "boost/random/linear_congruential.hpp"

// Ice interfaces
#include <SkyModelService.h>
#include <SkyModelServiceDTO.h>

// Classes to test
#include "smsclient/Component.h"
#include "smsclient/SkyModelServiceClient.h"

namespace askap {
namespace cp {
namespace sms {
namespace client {

// Alias for the Ice type namespace
namespace ice_interfaces = askap::interfaces::skymodelservice;

class SkyModelServiceClientTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(SkyModelServiceClientTest);
    CPPUNIT_TEST(testPreconditions);
    CPPUNIT_TEST(testTransformDataResult);
    CPPUNIT_TEST(testTransformDataResultSize);
    CPPUNIT_TEST(testUnits);
    CPPUNIT_TEST(testValues);
    CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
            itsCount = 5;
            for (size_t i = 0; i < itsCount; i++) {
                ice_interfaces::ContinuumComponent c;
                c.ra = 14.93;
                c.dec = -18.1;
                c.fluxInt = 1010.1; // mJy
                c.spectralIndex = -0.1;
                c.spectralCurvature = 0.01;
                c.majAxisDeconv = 12.0;
                c.minAxisDeconv = 8.0;
                itsIceComponents.push_back(c);
            }

            itsClientComponents = itsSmsClient.transformData(itsIceComponents);
        };

        void tearDown() {
        }

        void testPreconditions() {
            CPPUNIT_ASSERT_EQUAL(itsCount, itsIceComponents.size());
            for (ice_interfaces::ComponentSeq::iterator it = itsIceComponents.begin();
                it != itsIceComponents.end();
                it++) {
                CPPUNIT_ASSERT(it->ra >= 0);
                CPPUNIT_ASSERT(it->ra < 360);
                CPPUNIT_ASSERT(it->dec >= -90);
                CPPUNIT_ASSERT(it->dec <= 90);
                CPPUNIT_ASSERT(it->fluxInt > 0);
            }
        }

        //ComponentListPtr transformData(const askap::interfaces::skymodelservice::ComponentSeq& ice_resultset) const;
        void testTransformDataResult() {
            CPPUNIT_ASSERT(itsClientComponents);
        }

        void testTransformDataResultSize() {
            CPPUNIT_ASSERT(itsClientComponents->size() == itsIceComponents.size());
        }

        void testUnits() {
            for (ComponentList::iterator it = itsClientComponents->begin();
                it != itsClientComponents->end();
                it++) {
                CPPUNIT_ASSERT(it->rightAscension().isConform("deg"));
                CPPUNIT_ASSERT(it->declination().isConform("deg"));
                CPPUNIT_ASSERT(it->positionAngle().isConform("rad"));
                CPPUNIT_ASSERT(it->majorAxis().isConform("arcsec"));
                CPPUNIT_ASSERT(it->minorAxis().isConform("arcsec"));
                CPPUNIT_ASSERT(it->i1400().isConform("Jy"));
            }
        }

        void testValues() {
            const double dblEpsilon = std::numeric_limits<double>::epsilon();

            for (size_t i = 0; i < itsIceComponents.size(); i++) {

                CPPUNIT_ASSERT_EQUAL(
                    (ComponentId)itsIceComponents[i].id,
                    (*itsClientComponents)[i].id());
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].ra,
                    (*itsClientComponents)[i].rightAscension().getValue(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].dec,
                    (*itsClientComponents)[i].declination().getValue(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].posAngDeconv,
                    (*itsClientComponents)[i].positionAngle().getValue(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].majAxisDeconv,
                    (*itsClientComponents)[i].majorAxis().getValue(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].minAxisDeconv,
                    (*itsClientComponents)[i].minorAxis().getValue(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].fluxInt,
                    (*itsClientComponents)[i].i1400().getValue(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].spectralIndex,
                    (*itsClientComponents)[i].spectralIndex(),
                    dblEpsilon);
                CPPUNIT_ASSERT_DOUBLES_EQUAL(
                    itsIceComponents[i].spectralCurvature,
                    (*itsClientComponents)[i].spectralCurvature(),
                    dblEpsilon);
            }
        }

    private:

        size_t itsCount;
        ice_interfaces::ComponentSeq itsIceComponents;
        SkyModelServiceClient itsSmsClient;
        ComponentListPtr itsClientComponents;
};

}
}
}
}

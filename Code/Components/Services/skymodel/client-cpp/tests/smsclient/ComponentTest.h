/// @file ComponentTest.h
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
#include <limits>
#include "casacore/casa/aipstype.h"
#include "askap/AskapError.h"
#include "casacore/casa/Quanta/Quantum.h"

// Classes to test
#include "smsclient/Component.h"

namespace askap {
namespace cp {
namespace sms {
namespace client {

class ComponentTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(ComponentTest);
        CPPUNIT_TEST(testConstructor);
        CPPUNIT_TEST(testGetters);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
            itsRA = casacore::Quantity(187.5, "deg");
            itsDec = casacore::Quantity(-45.0, "deg");
            itsPositionAngle = casacore::Quantity(1.0, "rad");
            itsMajorAxis = casacore::Quantity(12.0, "arcsec");
            itsMinorAxis = casacore::Quantity(8.0, "arcsec");
            itsI1400 = casacore::Quantity(0.1, "Jy");
            itsSpectralIndex = -0.1;
            itsSpectralCurvature = 0.01;
        };

        void tearDown() {
        }

        void testConstructor() {
            // Test with various non-conformant units
            casacore::Quantity conformJy(0.1, "Jy");
            casacore::Quantity conformDeg(187.5, "deg");
            CPPUNIT_ASSERT_THROW(Component(1l,conformJy, itsDec, itsPositionAngle,
                    itsMajorAxis, itsMinorAxis, itsI1400, itsSpectralIndex, itsSpectralCurvature), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Component(1l,itsRA, conformJy, itsPositionAngle,
                    itsMajorAxis, itsMinorAxis, itsI1400, itsSpectralIndex, itsSpectralCurvature), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Component(1l,itsRA, itsDec, conformJy,
                    itsMajorAxis, itsMinorAxis, itsI1400, itsSpectralIndex, itsSpectralCurvature), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Component(1l,itsRA, itsDec, itsPositionAngle,
                    conformJy, itsMinorAxis, itsI1400, itsSpectralIndex, itsSpectralCurvature), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Component(1l,itsRA, itsDec, itsPositionAngle,
                    itsMajorAxis, conformJy, itsI1400, itsSpectralIndex, itsSpectralCurvature), askap::AskapError);
            CPPUNIT_ASSERT_THROW(Component(1l,itsRA, itsDec, itsPositionAngle,
                    itsMajorAxis, itsMinorAxis, conformDeg, itsSpectralIndex, itsSpectralCurvature), askap::AskapError);
        }

        void testGetters() {
            ComponentId id = 34l;
            Component c(id, itsRA, itsDec,
                    itsPositionAngle, itsMajorAxis,
                    itsMinorAxis, itsI1400, itsSpectralIndex,
                    itsSpectralCurvature);

            const double dblEpsilon = std::numeric_limits<double>::epsilon();
            CPPUNIT_ASSERT(id == c.id());
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsRA.getValue(), c.rightAscension().getValue(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsDec.getValue(), c.declination().getValue(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsPositionAngle.getValue(), c.positionAngle().getValue(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsMajorAxis.getValue(), c.majorAxis().getValue(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsMinorAxis.getValue(), c.minorAxis().getValue(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsI1400.getValue(), c.i1400().getValue(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsSpectralIndex, c.spectralIndex(), dblEpsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(itsSpectralCurvature, c.spectralCurvature(), dblEpsilon);
        }

        private:
        casacore::Quantity itsRA;
        casacore::Quantity itsDec;
        casacore::Quantity itsPositionAngle;
        casacore::Quantity itsMajorAxis;
        casacore::Quantity itsMinorAxis;
        casacore::Quantity itsI1400;
        casacore::Double itsSpectralIndex;
        casacore::Double itsSpectralCurvature;
};

}
}
}
}

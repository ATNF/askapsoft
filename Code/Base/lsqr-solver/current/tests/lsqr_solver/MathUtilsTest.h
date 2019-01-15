/// @copyright (c) 2007 CSIRO
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
/// @author Vitaliy Ogarko <vogarko@gmail.com>

#include <lsqr_solver/MathUtils.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap
{
  namespace lsqr
  {
    class MathUtilsTest : public CppUnit::TestFixture
    {
      CPPUNIT_TEST_SUITE(MathUtilsTest);

      CPPUNIT_TEST(testGetNormUnitVecs);
      CPPUNIT_TEST(testGetNormNonUnitVecs);
      CPPUNIT_TEST(testMultiply);
      CPPUNIT_TEST(testAdd);
      CPPUNIT_TEST(testTransform);
      CPPUNIT_TEST(testNormalize);

      CPPUNIT_TEST_SUITE_END();

      public:

        // Test of GetNorm.
        // Unit vectors.
        void testGetNormUnitVecs()
        {
            Vector vec(3);
            vec[0] = 1.0;
            vec[1] = 0.0;
            vec[2] = 0.0;

            CPPUNIT_ASSERT_EQUAL(1.0, MathUtils::GetNorm(vec));

            vec[0] = 0.0;
            vec[1] = 1.0;
            vec[2] = 0.0;

            CPPUNIT_ASSERT_EQUAL(1.0, MathUtils::GetNorm(vec));

            vec[0] = 0.0;
            vec[1] = 0.0;
            vec[2] = 1.0;

            CPPUNIT_ASSERT_EQUAL(1.0, MathUtils::GetNorm(vec));
        }

        // Test of GetNorm.
        // Non-unit vectors.
        void testGetNormNonUnitVecs()
        {
            Vector vec(3);
            vec[0] = 3.0;
            vec[1] = 4.0;
            vec[2] = 12.0;

            // Exact result.
            CPPUNIT_ASSERT_EQUAL(13.0, MathUtils::GetNorm(vec));

            double epsilon = 1.e-10;

            vec[0] = 1.0;
            vec[1] = 2.0;
            vec[2] = 3.0;

            // Approximate result.
            CPPUNIT_ASSERT_DOUBLES_EQUAL(3.74165738677, MathUtils::GetNorm(vec), epsilon);
        }

        // Test of Multiply.
        void testMultiply()
        {
            Vector vec(3);
            vec[0] = 1.0;
            vec[1] = 2.0;
            vec[2] = 3.0;

            Vector vec2(3);
            vec2 = vec;

            double scalar = 5.4321;

            MathUtils::Multiply(vec2, scalar);

            CPPUNIT_ASSERT_EQUAL(scalar * vec[0], vec2[0]);
            CPPUNIT_ASSERT_EQUAL(scalar * vec[1], vec2[1]);
            CPPUNIT_ASSERT_EQUAL(scalar * vec[2], vec2[2]);
        }

        // Test of Add.
        void testAdd()
        {
            Vector x(3);
            x[0] = 1.0;
            x[1] = 2.0;
            x[2] = 3.0;

            Vector y(3);
            y[0] = 4.0;
            y[1] = 5.0;
            y[2] = 6.0;

            MathUtils::Add(x, y);

            CPPUNIT_ASSERT_EQUAL(5.0, x[0]);
            CPPUNIT_ASSERT_EQUAL(7.0, x[1]);
            CPPUNIT_ASSERT_EQUAL(9.0, x[2]);
        }

        // Test of Transform.
        void testTransform()
        {
            Vector x(3);
            x[0] = 1.0;
            x[1] = 2.0;
            x[2] = 3.0;

            Vector y(3);
            y[0] = 4.0;
            y[1] = 5.0;
            y[2] = 6.0;

            double a = 2.0;
            double b = 3.0;

            MathUtils::Transform(a, x, b, y);

            CPPUNIT_ASSERT_EQUAL(14.0, x[0]);
            CPPUNIT_ASSERT_EQUAL(19.0, x[1]);
            CPPUNIT_ASSERT_EQUAL(24.0, x[2]);
        }

        // Test of Normalize.
        // Non-parallel.
        void testNormalize()
        {
            Vector vec(3);
            vec[0] = 1.0;
            vec[1] = 2.0;
            vec[2] = 3.0;

            Vector vec2(3);
            vec2 = vec;

            double norm = 0.0;

            CPPUNIT_ASSERT(MathUtils::Normalize(vec2, norm, false));
            CPPUNIT_ASSERT_EQUAL(1.0, MathUtils::GetNorm(vec2));
            CPPUNIT_ASSERT_EQUAL(norm, MathUtils::GetNorm(vec));
        }
    };
  }
}

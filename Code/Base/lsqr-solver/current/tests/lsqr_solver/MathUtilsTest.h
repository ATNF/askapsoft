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

      CPPUNIT_TEST(testGetNorm);

      CPPUNIT_TEST_SUITE_END();

      public:

        // Test of GetNorm.
        // Unit vectors.
        void testGetNorm()
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
    };
  }
}

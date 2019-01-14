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

#include <lsqr_solver/SparseMatrix.h>
#include <askap/AskapError.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap
{
  namespace lsqr
  {
    class SparseMatrixTest : public CppUnit::TestFixture
    {
      CPPUNIT_TEST_SUITE(SparseMatrixTest);
      CPPUNIT_TEST(testConstructors);
      CPPUNIT_TEST_SUITE_END();

      public:

        void testConstructors()
        {
            SparseMatrix *matrix = new SparseMatrix(1, 1);
            CPPUNIT_ASSERT(matrix != NULL);
            delete matrix;
        }
    };

  }
}

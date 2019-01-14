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
#include <stdexcept>

#include <cppunit/extensions/HelperMacros.h>

namespace askap
{
  namespace lsqr
  {
    class SparseMatrixTest : public CppUnit::TestFixture
    {
      CPPUNIT_TEST_SUITE(SparseMatrixTest);

      CPPUNIT_TEST(testConstructors);
      CPPUNIT_TEST(testAdd);
      CPPUNIT_TEST_EXCEPTION(testInvalidAdd, std::runtime_error);
      CPPUNIT_TEST_EXCEPTION(testInvalidAdd2, std::runtime_error);
      CPPUNIT_TEST(testGetNumberRows);
      CPPUNIT_TEST(testFinalizeZeroColumns);
      CPPUNIT_TEST(testFinalizeNonZeroColumns);
      CPPUNIT_TEST_EXCEPTION(testInvalidFinalize, std::runtime_error);
      CPPUNIT_TEST_EXCEPTION(testInvalidFinalize2, std::runtime_error);
      CPPUNIT_TEST_EXCEPTION(testInvalidNewRow, std::runtime_error);
      CPPUNIT_TEST(testGetValueAllNonZero);
      CPPUNIT_TEST(testGetValueZeroDiag);
      CPPUNIT_TEST(testMultVectorAllNonZero);
      CPPUNIT_TEST(testMultVectorZeroDiag);
      CPPUNIT_TEST(testMultVectorDiag);
      CPPUNIT_TEST(testMultVector1x3);
      CPPUNIT_TEST(testMultVector3x1);
      CPPUNIT_TEST(testMultVectorOneNonZero);
      CPPUNIT_TEST(testTransMultVectorAllNonZero3x3);
      CPPUNIT_TEST(testTransMultVectorAllNonZero2x3);
      CPPUNIT_TEST(testTransMultVectorAllNonZero3x2);
      CPPUNIT_TEST(testReset);

      CPPUNIT_TEST_SUITE_END();

      public:

        void testConstructors()
        {
            SparseMatrix *matrix = new SparseMatrix(1, 1);
            CPPUNIT_ASSERT(matrix != NULL);
            delete matrix;
        }

        // Test of Add.
        void testAdd()
        {
            SparseMatrix matrix(1, 3);

            matrix.NewRow();

            CPPUNIT_ASSERT_EQUAL(size_t(0), matrix.GetNumberElements());

            matrix.Add(0.0, 0);
            CPPUNIT_ASSERT_EQUAL(size_t(0), matrix.GetNumberElements());

            matrix.Add(10.0, 0);
            CPPUNIT_ASSERT_EQUAL(size_t(1), matrix.GetNumberElements());

            matrix.Add(20.0, 1);
            CPPUNIT_ASSERT_EQUAL(size_t(2), matrix.GetNumberElements());

            matrix.Add(0.0, 2);
            CPPUNIT_ASSERT_EQUAL(size_t(2), matrix.GetNumberElements());

            matrix.Add(30.0, 2);
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetNumberElements());
        }

        // Test of Add.
        // Add more elements than reserved number (nnz).
        void testInvalidAdd()
        {
            SparseMatrix matrix(1, 3);

            matrix.NewRow();
            matrix.Add(1.0, 0);
            matrix.Add(2.0, 0);
            matrix.Add(3.0, 0);
            // Will throw std::runtime_error.
            matrix.Add(4.0, 0);
        }

        // Test of Add.
        // Add elements before the first row has been added.
        void testInvalidAdd2()
        {
            SparseMatrix matrix(1, 3);
            matrix.Add(1.0, 0);
        }

        // Test of GetCurrentNumberRows, and GetTotalNumberRows.
        void testGetNumberRows()
        {
            SparseMatrix matrix(3, 9);

            CPPUNIT_ASSERT_EQUAL(size_t(0), matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetTotalNumberRows());

            matrix.NewRow();
            CPPUNIT_ASSERT_EQUAL(size_t(1), matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetTotalNumberRows());

            matrix.NewRow();
            CPPUNIT_ASSERT_EQUAL(size_t(2), matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetTotalNumberRows());

            matrix.NewRow();
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetTotalNumberRows());
        }

        // Test of Finalize.
        // Zero number of columns.
        void testFinalizeZeroColumns()
        {
            SparseMatrix matrix(1, 1);
            matrix.NewRow();
            CPPUNIT_ASSERT(matrix.Finalize(0));
        }

        // Test of Finalize.
        // Non-zero number of columns.
        void testFinalizeNonZeroColumns()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(5.0, 1);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);
            matrix.Add(9.0, 2);

            CPPUNIT_ASSERT(matrix.Finalize(3));
        }

        // Test of Finalize.
        // Finalize matrix with wrong number of rows.
        void testInvalidFinalize()
        {
            SparseMatrix matrix(1, 1);
            // Will throw std::runtime_error.
            matrix.Finalize(0);
        }

        // Test of Finalize.
        // Pass wrong number of matrix columns to Finalize.
        void testInvalidFinalize2()
        {
            SparseMatrix matrix(1, 1);
            matrix.Add(1.0, 2);
            // Will throw std::runtime_error.
            matrix.Finalize(1);
        }

        void testInvalidNewRow()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);
            // Will throw std::runtime_error.
            matrix.NewRow();
        }

        // Test of GetValue.
        // Matrix with all non-zero elements.
        void testGetValueAllNonZero()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(5.0, 1);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);
            matrix.Add(9.0, 2);

            matrix.Finalize(3);

            CPPUNIT_ASSERT_EQUAL(1.0, matrix.GetValue(0, 0));
            CPPUNIT_ASSERT_EQUAL(2.0, matrix.GetValue(1, 0));
            CPPUNIT_ASSERT_EQUAL(3.0, matrix.GetValue(2, 0));

            CPPUNIT_ASSERT_EQUAL(4.0, matrix.GetValue(0, 1));
            CPPUNIT_ASSERT_EQUAL(5.0, matrix.GetValue(1, 1));
            CPPUNIT_ASSERT_EQUAL(6.0, matrix.GetValue(2, 1));

            CPPUNIT_ASSERT_EQUAL(7.0, matrix.GetValue(0, 2));
            CPPUNIT_ASSERT_EQUAL(8.0, matrix.GetValue(1, 2));
            CPPUNIT_ASSERT_EQUAL(9.0, matrix.GetValue(2, 2));
        }

        // Test of GetValue.
        // Do not add diagonal elements,
        // so GetValue() should return zero when querying for them.
        void testGetValueZeroDiag()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);

            matrix.Finalize(3);

            CPPUNIT_ASSERT_EQUAL(0.0, matrix.GetValue(0, 0));
            CPPUNIT_ASSERT_EQUAL(2.0, matrix.GetValue(1, 0));
            CPPUNIT_ASSERT_EQUAL(3.0, matrix.GetValue(2, 0));

            CPPUNIT_ASSERT_EQUAL(4.0, matrix.GetValue(0, 1));
            CPPUNIT_ASSERT_EQUAL(0.0, matrix.GetValue(1, 1));
            CPPUNIT_ASSERT_EQUAL(6.0, matrix.GetValue(2, 1));

            CPPUNIT_ASSERT_EQUAL(7.0, matrix.GetValue(0, 2));
            CPPUNIT_ASSERT_EQUAL(8.0, matrix.GetValue(1, 2));
            CPPUNIT_ASSERT_EQUAL(0.0, matrix.GetValue(2, 2));
        }

        // Test of MultVector.
        // 3x3 matrix with all non-zero elements.
        void testMultVectorAllNonZero()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(5.0, 1);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);
            matrix.Add(9.0, 2);

            matrix.Finalize(3);

            Vector x(3);
            Vector b(3);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            matrix.MultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(14.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(32.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(50.0, b[2]);
        }

        // Test of MultVector.
        // 3x3 matrix with no diagonal elements.
        void testMultVectorZeroDiag()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);

            matrix.Finalize(3);

            Vector x(3);
            Vector b(3);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            matrix.MultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(13.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(22.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(23.0, b[2]);
        }

        // Test of MultVector.
        // Diagonal 3x3 matrix.
        void testMultVectorDiag()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();
            matrix.Add(1.0, 0);

            matrix.NewRow();
            matrix.Add(2.0, 1);

            matrix.NewRow();
            matrix.Add(3.0, 2);

            matrix.Finalize(3);

            Vector x(3);
            Vector b(3);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            matrix.MultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(1.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(4.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(9.0, b[2]);
        }

        // Test of MultVector.
        // 1x3 matrix.
        void testMultVector1x3()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();
            matrix.Add(1.0, 0);

            matrix.NewRow();
            matrix.Add(2.0, 0);

            matrix.NewRow();
            matrix.Add(3.0, 0);

            matrix.Finalize(1);

            Vector x(1);
            Vector b(3);

            x[0] = 2;

            matrix.MultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(2.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(4.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(6.0, b[2]);
        }

        // Test of MultVector.
        // 3x1 matrix.
        void testMultVector3x1()
        {
            SparseMatrix matrix(1, 9);

            matrix.NewRow();
            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.Finalize(3);

            Vector x(3);
            Vector b(1);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            matrix.MultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(14.0, b[0]);
        }

        // Test of MultVector.
        // 3x3 matrix with one non-zero element a11.
        void testMultVectorOneNonZero()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();
            matrix.Add(5.0, 0);

            matrix.NewRow();
            matrix.NewRow();

            matrix.Finalize(3);

            Vector x(3);
            Vector b(3);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            b[0] = 10;
            b[1] = 10;
            b[2] = 10;

            matrix.MultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(5.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(0.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(0.0, b[2]);
        }

        // Test of TransMultVector.
        // 3x3 matrix with all non-zero elements.
        void testTransMultVectorAllNonZero3x3()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(5.0, 1);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);
            matrix.Add(9.0, 2);

            matrix.Finalize(3);

            Vector x(3);
            Vector b(3);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            matrix.TransMultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(30.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(36.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(42.0, b[2]);
        }

        // Test of TransMultVector.
        // 2x3 matrix with all non-zero elements.
        void testTransMultVectorAllNonZero2x3()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);

            matrix.NewRow();

            matrix.Add(3.0, 0);
            matrix.Add(4.0, 1);

            matrix.NewRow();

            matrix.Add(5.0, 0);
            matrix.Add(6.0, 1);

            matrix.Finalize(2);

            Vector x(3);
            Vector b(2);

            x[0] = 1;
            x[1] = 2;
            x[2] = 3;

            matrix.TransMultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(22.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(28.0, b[1]);
        }

        // Test of TransMultVector.
        // 3x2 matrix with all non-zero elements.
        void testTransMultVectorAllNonZero3x2()
        {
            SparseMatrix matrix(2, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(5.0, 1);
            matrix.Add(6.0, 2);

            matrix.Finalize(3);

            Vector x(2);
            Vector b(3);

            x[0] = 2;
            x[1] = 3;

            matrix.TransMultVector(x, b);

            CPPUNIT_ASSERT_EQUAL(14.0, b[0]);
            CPPUNIT_ASSERT_EQUAL(19.0, b[1]);
            CPPUNIT_ASSERT_EQUAL(24.0, b[2]);
        }

        // Test of Reset.
        void testReset()
        {
            SparseMatrix matrix(3, 9);

            matrix.NewRow();

            matrix.Add(1.0, 0);
            matrix.Add(2.0, 1);
            matrix.Add(3.0, 2);

            matrix.NewRow();

            matrix.Add(4.0, 0);
            matrix.Add(5.0, 1);
            matrix.Add(6.0, 2);

            matrix.NewRow();

            matrix.Add(7.0, 0);
            matrix.Add(8.0, 1);
            matrix.Add(9.0, 2);

            matrix.Finalize(3);

            CPPUNIT_ASSERT_EQUAL(size_t(9), matrix.GetNumberElements());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetTotalNumberRows());

            matrix.Reset();

            CPPUNIT_ASSERT_EQUAL(size_t(0), matrix.GetNumberElements());
            CPPUNIT_ASSERT_EQUAL(size_t(0), matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(size_t(3), matrix.GetTotalNumberRows());
        }
    };

  }
}

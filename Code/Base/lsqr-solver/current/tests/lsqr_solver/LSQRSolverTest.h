///
/// @author Vitaliy Ogarko <vogarko@gmail.com>
///

#include <lsqr_solver/LSQRSolver.h>
#include <lsqr_solver/ModelDamping.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap
{
  namespace lsqr
  {
    class LSQRSolverTest : public CppUnit::TestFixture
    {
      CPPUNIT_TEST_SUITE(LSQRSolverTest);

      CPPUNIT_TEST(testConstructors);
      CPPUNIT_TEST(testUnderdeterminedNonDamped);
      CPPUNIT_TEST(testUnderdeterminedDamped);
      CPPUNIT_TEST(testUnderdeterminedSeveralDampings);
      CPPUNIT_TEST(testOverdetermined);
      CPPUNIT_TEST(testNoElements);

      CPPUNIT_TEST_SUITE_END();

      private:

        /*
        * Define the following underdetermined system:
        *
        * x1 + x2 = 1,
        * 2x1 + x2 - q = 0.
        *
        * Which has the minimum norm underdetermined solution x1 = 0, x2 = 1, q = 1.
        * (See Carl Wunsch, The ocean circulation inverse problem, Eq.(3.4.120).)
        */
        struct WunschFixture
        {
          size_t ncols, nrows;
          SparseMatrix* matrix;
          Vector* b_RHS;

          WunschFixture()
          {
              ncols = 3;
              nrows = 2;

#ifdef HAVE_MPI
              matrix = new SparseMatrix(nrows, MPI_COMM_WORLD);
#else
              matrix = new SparseMatrix(nrows);
#endif
              b_RHS = new Vector(nrows, 0.0);

              double a[3][2];

              a[0][0] = 1.0;
              a[1][0] = 1.0;
              a[2][0] = 0.0;

              a[0][1] = 2.0;
              a[1][1] = 1.0;
              a[2][1] = - 1.0;

              // Right hand side.
              b_RHS->at(0) = 1.0;
              b_RHS->at(1) = 0.0;

              for (size_t j = 0; j < nrows; ++j)
              {
                  matrix->NewRow();

                  for (size_t i = 0; i < ncols; ++i)
                  {
                      matrix->Add(a[i][j], i);
                  }
              }
              matrix->Finalize(ncols);
          }

          ~WunschFixture()
          {
              delete matrix;
              delete b_RHS;
          }
        };

      public:

        // Test of constructor.
        void testConstructors()
        {
            LSQRSolver *solver = new LSQRSolver(3, 9);
            CPPUNIT_ASSERT(solver != NULL);
            delete solver;
        }

        // Testing underdetermined non-damped system (defined in WunschFixture).
        void testUnderdeterminedNonDamped()
        {
            double rmin = 1.e-13;
            size_t niter = 100;

            WunschFixture system;

            LSQRSolver solver(system.nrows, system.ncols);

            Vector x(system.ncols, 0.0);
            solver.Solve(niter, rmin, *system.matrix, *system.b_RHS, x, true);

            double epsilon = 1.e-15;

            CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, x[0], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, x[1], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, x[2], epsilon);
        }

        //------------------------------------------------------------------------------
        // Testing underdetermined damped system (defined in WunschFixture).
        //
        // This damped version (with model_ref = 0.5) finds another solution x1 = 1/6, x2 = 1-1/6, q = 1+1/6,
        // which is an exact solution and lies closer to the reference model, than the undamped solution.
        // this solution is stable for many orders of alpha: 1.e-3 <= alpha <= 1.e-12
        //------------------------------------------------------------------------------
        void testUnderdeterminedDamped()
        {
            int myrank = 0;
            int nbproc = 1;

            double rmin = 1.e-13;
            size_t niter = 100;

            double alpha = 1.e-12;
            double normPower = 2.0;
            double modelRefValue = 0.5;

            WunschFixture system;

            size_t nelements = system.ncols;

            Vector model(nelements, 0.0);
            Vector modelRef(nelements, modelRefValue);

            ModelDamping damping(nelements);

            // Add damping to the system.
            damping.Add(alpha, normPower, *system.matrix, *system.b_RHS, &model, &modelRef, NULL, myrank, nbproc);

            LSQRSolver solver(system.matrix->GetTotalNumberRows(), nelements);

            Vector x(system.ncols, 0.0);
            solver.Solve(niter, rmin, *system.matrix, *system.b_RHS, x, true);

            double epsilon = 1.e-5;

            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 6.0, x[0], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 - 1.0 / 6.0, x[1], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 + 1.0 / 6.0, x[2], epsilon);
        }

        //------------------------------------------------------------------------------
        // Test of adding several damping terms.
        // Solving the same system as in testUnderdeterminedDamped.
        // With alpha = 1.e-13 the test testUnderdeterminedDamped fails, but with alpha = 1.e-12 passes.
        // Therefore, it should also pass when 10 damping terms added with alpha = 1.e-13.
        //------------------------------------------------------------------------------
        void testUnderdeterminedSeveralDampings()
        {
            int myrank = 0;
            int nbproc = 1;

            double rmin = 1.e-13;
            int niter = 100;

            double alpha = 1.e-13;
            double normPower = 2.0;
            double modelRefValue = 0.5;

            size_t ndamping = 10;

            WunschFixture system;

            size_t nelements = system.ncols;

            Vector model(nelements, 0.0);
            Vector modelRef(nelements, modelRefValue);

            ModelDamping damping(nelements);

            // Add damping terms to the system.
            for (size_t i = 0; i < ndamping; ++i)
            {
                damping.Add(alpha, normPower, *system.matrix, *system.b_RHS, &model, &modelRef, NULL, myrank, nbproc);
            }

            LSQRSolver solver(system.matrix->GetTotalNumberRows(), nelements);

            Vector x(system.ncols, 0.0);
            solver.Solve(niter, rmin, *system.matrix, *system.b_RHS, x, true);

            double epsilon = 1.e-5;

            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 6.0, x[0], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 - 1.0 / 6.0, x[1], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 + 1.0 / 6.0, x[2], epsilon);
        }

        //---------------------------------------------------------------------
        // Testing overdetermined system.
        //---------------------------------------------------------------------
        // Consider a regression with constant, linear and quadratic terms:
        // f(x) = b1 + b2 * x + b3 * x^2
        //
        // We build a matrix 3 x N:
        //
        //   1  x1  x1^2
        //   1  x2  x2^2
        //   ...
        //   1  xn  xn^2,
        //
        // with x_i = i/N, i = 1, ..., N.
        //
        // Apply LSQR method and compare to the solution x = (b1, b2, b3).
        // An example from [1].
        // [1] Least Squares Estimation, Sara A. van de Geer, Volume 2, pp. 1041-1045,
        //     in Encyclopedia of Statistics in Behavioral Science, 2005.
        //---------------------------------------------------------------------
        void testOverdetermined()
        {
            size_t nelements_total = 3;
            size_t nrows = 1000;
            double rmin = 1.e-14;
            size_t niter = 100;

            size_t nelements = nelements_total;

#ifdef HAVE_MPI
            SparseMatrix matrix(nrows, MPI_COMM_WORLD);
#else
            SparseMatrix matrix(nrows);
#endif

            Vector b_RHS(nrows, 0.0);

            Vector b(3);
            b[0] = 1.0;
            b[1] = - 3.0;
            b[2] = 0.0;

            // Building the matrix with right hand side.
            for (size_t i = 0; i < nrows; ++i)
            {
                matrix.NewRow();

                double xi = double(i) / double(nrows);

                matrix.Add(1.0, 0);
                matrix.Add(xi, 1);
                matrix.Add(xi * xi, 2);

                b_RHS[i] = b[0] + b[1] * xi + b[2] * xi * xi;
            }
            matrix.Finalize(nelements);

            LSQRSolver solver(nrows, nelements);

            Vector x(nelements, 0.0);
            solver.Solve(niter, rmin, matrix, b_RHS, x, true);

            double epsilon = 1.e-14;

            CPPUNIT_ASSERT_DOUBLES_EQUAL(b[0], x[0], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(b[1], x[1], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(b[2], x[2], epsilon);
        }

        /*
         * Testing matrix without elements. Solver should not run and solution should not change.
         */
        void testNoElements()
        {
            size_t ncols = 3;
            size_t nrows = 3;

#ifdef HAVE_MPI
            SparseMatrix matrix(nrows, MPI_COMM_WORLD);
#else
            SparseMatrix matrix(nrows);
#endif

            matrix.NewRow();
            matrix.NewRow();
            matrix.NewRow();

            matrix.Finalize(ncols);

            Vector b_RHS(nrows, 0.0);
            b_RHS[0] = 1.0;
            b_RHS[1] = 2.0;
            b_RHS[2] = 3.0;

            Vector x(ncols, 0.0);

            double rmin = 1.e-14;
            size_t niter = 100;

            LSQRSolver solver(nrows, ncols);

            solver.Solve(niter, rmin, matrix, b_RHS, x, true);

            CPPUNIT_ASSERT_EQUAL(0.0, x[0]);
            CPPUNIT_ASSERT_EQUAL(0.0, x[1]);
            CPPUNIT_ASSERT_EQUAL(0.0, x[2]);
        }
    };
  }
}

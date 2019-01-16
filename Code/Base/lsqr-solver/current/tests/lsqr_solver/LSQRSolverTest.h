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

              matrix = new SparseMatrix(nrows, ncols * nrows);
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
            int myrank = 0;

            double rmin = 1.e-13;
            size_t niter = 100;

            WunschFixture system;

            LSQRSolver solver(system.nrows, system.ncols);

            Vector x(system.ncols, 0.0);
            solver.Solve(niter, rmin, *system.matrix, *system.b_RHS, x, myrank, true);

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
            damping.Add(alpha, normPower, *system.matrix, *system.b_RHS, &model, &modelRef, NULL);

            LSQRSolver solver(system.matrix->GetTotalNumberRows(), nelements);

            Vector x(system.ncols, 0.0);
            solver.Solve(niter, rmin, *system.matrix, *system.b_RHS, x, myrank, false);

            double epsilon = 1.e-5;

            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 6.0, x[0], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 - 1.0 / 6.0, x[1], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 + 1.0 / 6.0, x[2], epsilon);
        }

        //------------------------------------------------------------------------------
        // Test of adding several damping terms.
        // Solving the same system as in solve_underdetermined_2.
        // With alpha = 1.e-13 the test solve_underdetermined_2 fails, but with alpha = 1.e-13 passes.
        // Therefore, it should also pass when added 10 damping terms with alpha = 1.e-13.
        //------------------------------------------------------------------------------
        void testUnderdeterminedSeveralDampings()
        {
            int myrank = 0;

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
                damping.Add(alpha, normPower, *system.matrix, *system.b_RHS, &model, &modelRef, NULL);
            }

            LSQRSolver solver(system.matrix->GetTotalNumberRows(), nelements);

            Vector x(system.ncols, 0.0);
            solver.Solve(niter, rmin, *system.matrix, *system.b_RHS, x, myrank, false);

            double epsilon = 1.e-5;

            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 / 6.0, x[0], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 - 1.0 / 6.0, x[1], epsilon);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0 + 1.0 / 6.0, x[2], epsilon);
        }
    };
  }
}

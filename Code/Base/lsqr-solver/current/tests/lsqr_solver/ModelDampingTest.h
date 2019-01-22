///
/// @author Vitaliy Ogarko <vogarko@gmail.com>
///

#include <lsqr_solver/ModelDamping.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap
{
  namespace lsqr
  {
    class ModelDampingTest : public CppUnit::TestFixture
    {
      CPPUNIT_TEST_SUITE(ModelDampingTest);

      CPPUNIT_TEST(testConstructors);
      CPPUNIT_TEST(testAdd);

      CPPUNIT_TEST_SUITE_END();

      public:

        // Test of constructor.
        void testConstructors()
        {
            ModelDamping *damp = new ModelDamping(3);
            CPPUNIT_ASSERT(damp != NULL);
            delete damp;
        }

        // Test of Add.
        void testAdd()
        {
            double alpha = 0.1;
            double normPower = 2.0;
            size_t nelements = 3;

            SparseMatrix matrix(0, 0);
            Vector b(0);

            matrix.Finalize(0);

            Vector model(nelements);

            for (size_t i = 0; i < nelements; ++i)
            {
                model[i] = double(i + 1);
            }

            ModelDamping damping(nelements);

            damping.Add(alpha, normPower, matrix, b, &model, NULL, NULL);

            CPPUNIT_ASSERT_EQUAL(nelements, matrix.GetCurrentNumberRows());
            CPPUNIT_ASSERT_EQUAL(nelements, matrix.GetNumberElements());

            for (size_t i = 0; i < nelements; ++i)
            {
                // Check matrix diagonal.
                CPPUNIT_ASSERT_EQUAL(alpha, matrix.GetValue(i, i));

                // Check the right-hand side.
                CPPUNIT_ASSERT_EQUAL(- alpha * model[i], b[i]);
            }
        }
    };
  }
}

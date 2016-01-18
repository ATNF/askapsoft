/// @file
/// 
/// @brief Tests of CompositeStep class
/// @details The unit test covers non-MPI part of the logic in the CompositeStep,
/// e.g. rank allocation and communicator creation  
///
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#ifndef ASKAP_ASKAPPARALLEL_COMPOSITE_STEP_TEST_H
#define ASKAP_ASKAPPARALLEL_COMPOSITE_STEP_TEST_H

#include "processingsteps/CompositeStep.h"
#include <askap/AskapError.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap {

namespace askapparallel {

class CompositeStepTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(CompositeStepTest);
   CPPUNIT_TEST(testAddSubStep);
   CPPUNIT_TEST_SUITE_END();
public:
   void testAddSubStep();   
};

void CompositeStepTest::testAddSubStep() {
   CompositeStep cs;
   // empty processing step to add to the composite
   boost::shared_ptr<ProcessingStep> ps(new ProcessingStep);
   CPPUNIT_ASSERT(ps);
   cs.addSubStep(ps, 1, 1);
   CPPUNIT_ASSERT_EQUAL(size_t(1u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(1u, cs.itsSteps[0].id().nRanks());
}

} // end of namespace askapparallel
} // end of namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_COMPOSITE_STEP_TEST_H

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
#include <askap/AskapUtil.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap {

namespace askapparallel {

class CompositeStepTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(CompositeStepTest);
   CPPUNIT_TEST(testAddSubStep);
   CPPUNIT_TEST(testAddSubStepFlexMultiRank);
   CPPUNIT_TEST_EXCEPTION(testAddSubStepTwoFlex, AskapError);
   CPPUNIT_TEST_EXCEPTION(testTagMultiRank, AskapError);
   CPPUNIT_TEST(testTagRank);
   CPPUNIT_TEST_SUITE_END();
public:
   void testAddSubStep();   
   void testAddSubStepFlexMultiRank();   
   void testAddSubStepTwoFlex();   
   void testTagMultiRank();
   void testTagRank();
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

   // add 3 groups of 2 ranks each
   cs.addSubStep(ps, 2, 3);
   CPPUNIT_ASSERT_EQUAL(size_t(2u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(1, cs.itsSteps[1].id().first());
   CPPUNIT_ASSERT_EQUAL(6, cs.itsSteps[1].id().last());
   CPPUNIT_ASSERT_EQUAL(2u, cs.itsSteps[1].id().nRanks());

   // add flexible allocation, single rank per process
   cs.addSubStep(ps, 1, CompositeStep::USE_ALL_AVAILABLE);
   CPPUNIT_ASSERT_EQUAL(size_t(3u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(7, cs.itsSteps[2].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[2].id().last());
   CPPUNIT_ASSERT_EQUAL(1u, cs.itsSteps[2].id().nRanks());

   // fixed allocation following a flexible allocation
   cs.addSubStep(ps, 2, 1);
   CPPUNIT_ASSERT_EQUAL(size_t(4u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(-2, cs.itsSteps[3].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[3].id().last());
   CPPUNIT_ASSERT_EQUAL(-3, cs.itsSteps[2].id().last());
   CPPUNIT_ASSERT_EQUAL(7, cs.itsSteps[2].id().first());
   CPPUNIT_ASSERT_EQUAL(2u, cs.itsSteps[3].id().nRanks());

   // another fixed allocation following a flexible one,
   // this time - single rank but two groups
   cs.addSubStep(ps, 1, 2);
   CPPUNIT_ASSERT_EQUAL(size_t(5u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(-2, cs.itsSteps[4].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[4].id().last());
   CPPUNIT_ASSERT_EQUAL(-4, cs.itsSteps[3].id().first());
   CPPUNIT_ASSERT_EQUAL(-3, cs.itsSteps[3].id().last());
   CPPUNIT_ASSERT_EQUAL(-5, cs.itsSteps[2].id().last());
   CPPUNIT_ASSERT_EQUAL(7, cs.itsSteps[2].id().first());
   CPPUNIT_ASSERT_EQUAL(2u, cs.itsSteps[3].id().nRanks());
}

void CompositeStepTest::testAddSubStepFlexMultiRank() {
   CompositeStep cs;
   // empty processing step to add to the composite
   boost::shared_ptr<ProcessingStep> ps(new ProcessingStep);
   CPPUNIT_ASSERT(ps);

   // add flexible allocation, 3 ranks per process
   // default number of groups mean the flexible allocation
   cs.addSubStep(ps, 3);
   CPPUNIT_ASSERT_EQUAL(size_t(1u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(3u, cs.itsSteps[0].id().nRanks());

   // fixed allocation following the flexible one
   cs.addSubStep(ps, 1, 1);
   CPPUNIT_ASSERT_EQUAL(size_t(2u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(-2, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(3u, cs.itsSteps[0].id().nRanks());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[1].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[1].id().last());
   CPPUNIT_ASSERT_EQUAL(1u, cs.itsSteps[1].id().nRanks());
}


void CompositeStepTest::testAddSubStepTwoFlex() {
   CompositeStep cs;
   // empty processing step to add to the composite
   boost::shared_ptr<ProcessingStep> ps(new ProcessingStep);
   CPPUNIT_ASSERT(ps);

   // flexible
   cs.addSubStep(ps, 1);
   CPPUNIT_ASSERT_EQUAL(size_t(1u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(1u, cs.itsSteps[0].id().nRanks());
   // fixed
   cs.addSubStep(ps, 1,1);
   CPPUNIT_ASSERT_EQUAL(size_t(2u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(-2, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(1u, cs.itsSteps[0].id().nRanks());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[1].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[1].id().last());
   CPPUNIT_ASSERT_EQUAL(1u, cs.itsSteps[1].id().nRanks());

   // attempting to have another flexible allocation - this should
   // throw an exception
   cs.addSubStep(ps, 1);
}

void CompositeStepTest::testTagMultiRank() {
   CompositeStep cs;
   // empty processing step to add to the composite
   boost::shared_ptr<ProcessingStep> ps(new ProcessingStep);
   CPPUNIT_ASSERT(ps);
   
   // add a substep with multi-rank groups, flexible allocation (although it doesn't matter)
   const StepIDProxy idp = cs.addSubStep(ps, 10);
   CPPUNIT_ASSERT_EQUAL(size_t(1u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(10u, cs.itsSteps[0].id().nRanks());

   CPPUNIT_ASSERT(!idp.isSingleRank());

   // the following should throw an exception as we attempt to
   // tag a multi-rank processing step
   cs.tagRank("flex", idp);
};

void CompositeStepTest::testTagRank() {
   CompositeStep cs;
   // empty processing step to add to the composite
   boost::shared_ptr<ProcessingStep> ps(new ProcessingStep);
   CPPUNIT_ASSERT(ps);
   
   // add a step, flexible allocation
   const StepIDProxy idp = cs.addSubStep(ps, 5);
   CPPUNIT_ASSERT_EQUAL(size_t(1u), cs.itsSteps.size());
   CPPUNIT_ASSERT_EQUAL(0, cs.itsSteps[0].id().first());
   CPPUNIT_ASSERT_EQUAL(-1, cs.itsSteps[0].id().last());
   CPPUNIT_ASSERT_EQUAL(5u, cs.itsSteps[0].id().nRanks());
   
   CPPUNIT_ASSERT(!idp.isSingleRank());
   // take a slice - second group, first element
   const StepIDProxy idpSliced = idp(2, 1);
   CPPUNIT_ASSERT(idpSliced.isSingleRank());

   CPPUNIT_ASSERT(cs.itsTaggedRanks.begin() == cs.itsTaggedRanks.end());
   cs.tagRank("example", idpSliced);
   const std::map<std::string, StepIDProxy>::const_iterator ci = cs.itsTaggedRanks.find("example");
   CPPUNIT_ASSERT(ci != cs.itsTaggedRanks.end());
   CPPUNIT_ASSERT(ci->second.isSingleRank());
   CPPUNIT_ASSERT(boost::shared_ptr<CompositeStep>(&cs, utility::NullDeleter()) == ci->second.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(0u), ci->second.index());
}

} // end of namespace askapparallel
} // end of namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_COMPOSITE_STEP_TEST_H

/// @file
/// 
/// @brief Tests of ComplexDiff autodifferentiation class
/// @details See ComplexDiff for description of what this class  
/// is supposed to do. This file contains appropriate unit tests.
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

#ifndef ASKAP_ASKAPPARALLEL_STEP_ID_TEST_H
#define ASKAP_ASKAPPARALLEL_STEP_ID_TEST_H

#include "processingsteps/StepID.h"

#include <cppunit/extensions/HelperMacros.h>

namespace askap {

namespace askapparallel {

class StepIDTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(StepIDTest);
   CPPUNIT_TEST(testConstruction);
   CPPUNIT_TEST_SUITE_END();
public:
   void testConstruction();   
};

void StepIDTest::testConstruction() {
   // creation with the default constructor
   StepID dummy;
   CPPUNIT_ASSERT(dummy.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(0, dummy.first());
   CPPUNIT_ASSERT_EQUAL(0, dummy.last());
   CPPUNIT_ASSERT_EQUAL(1u, dummy.nRanks());

   CPPUNIT_ASSERT(dummy(0).isSingleRank());
   CPPUNIT_ASSERT_EQUAL(0, dummy(0).first());
   CPPUNIT_ASSERT_EQUAL(0, dummy(0).last());
   CPPUNIT_ASSERT_EQUAL(1u, dummy(0).nRanks());
   
   // explicit creation of a single rank step
   // negative rank number corresponds to flexible allocation
   StepID srStep(-3);
   CPPUNIT_ASSERT(srStep.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(-3, srStep.first());
   CPPUNIT_ASSERT_EQUAL(-3, srStep.last());
   CPPUNIT_ASSERT_EQUAL(1u, srStep.nRanks());
   
   CPPUNIT_ASSERT(srStep(0).isSingleRank());
   CPPUNIT_ASSERT_EQUAL(-3, srStep(0).first());
   CPPUNIT_ASSERT_EQUAL(-3, srStep(0).last());
   CPPUNIT_ASSERT_EQUAL(1u, srStep(0).nRanks());
   
   StepID srGroup(0,5);
   CPPUNIT_ASSERT(!srGroup.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(0, srGroup.first());
   CPPUNIT_ASSERT_EQUAL(5, srGroup.last());
   CPPUNIT_ASSERT_EQUAL(1u, srGroup.nRanks());
   
   // test each of the 6 single rank groups
   for (unsigned int grp = 0; grp <= 5u; ++grp) {
        StepID testStep = srGroup(grp);
        CPPUNIT_ASSERT(testStep.isSingleRank());
        CPPUNIT_ASSERT_EQUAL(int(grp), testStep.first());
        CPPUNIT_ASSERT_EQUAL(int(grp), testStep.last());
        CPPUNIT_ASSERT_EQUAL(1u, testStep.nRanks());        
   } 
}

} // end of namespace askapparallel
} // end of namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_STEP_ID_TEST_H

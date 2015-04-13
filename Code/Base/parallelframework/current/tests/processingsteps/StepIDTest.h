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
#include <askap/AskapError.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap {

namespace askapparallel {

class StepIDTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(StepIDTest);
   CPPUNIT_TEST(testConstruction);
   CPPUNIT_TEST(testProjectFixed);
   CPPUNIT_TEST(testProjectFlex);
   CPPUNIT_TEST_EXCEPTION(testTooFewRanks, AskapError);
   CPPUNIT_TEST_EXCEPTION(testTooFewRanksFlex, AskapError);
   CPPUNIT_TEST_EXCEPTION(testUnevenRanks, AskapError);
   CPPUNIT_TEST(testCopy);
   CPPUNIT_TEST_SUITE_END();
public:
   void testConstruction();   
   void testProjectFixed();   
   void testProjectFlex();   
   void testTooFewRanks();
   void testTooFewRanksFlex();
   void testUnevenRanks();
   void testCopy();
   
protected:
   /// @brief test that given StepID objects have expected values
   /// @param[in] srGroup - group of single rank steps occupying ranks from 0 to 5 inclusive
   /// @param[in] mrGroup - group of 2-rank steps occupying ranks from 0 to 5 inclusive   
   void inspectGroups(const StepID &srGroup, const StepID &mrGroup);    
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
   
   // group of single rank steps
   StepID srGroup(0,5);
   
   // group of two-rank steps
   StepID mrGroup(0,5,2);
   
   inspectGroups(srGroup, mrGroup);   
}

/// @brief test that given StepID objects have expected values
/// @param[in] srGroup - group of single rank steps occupying ranks from 0 to 5 inclusive
/// @param[in] mrGroup - group of 2-rank steps occupying ranks from 0 to 5 inclusive   
void StepIDTest::inspectGroups(const StepID &srGroup, const StepID &mrGroup)
{
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

   CPPUNIT_ASSERT(!mrGroup.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(0, mrGroup.first());
   CPPUNIT_ASSERT_EQUAL(5, mrGroup.last());
   CPPUNIT_ASSERT_EQUAL(2u, mrGroup.nRanks());
    
   // test each of the 3 groups
   for (unsigned int grp = 0; grp < 3u; ++grp) {
        for (unsigned int elem = 0; elem < mrGroup.nRanks(); ++elem) {
             StepID testStep = mrGroup(grp, elem);
             CPPUNIT_ASSERT(testStep.isSingleRank());
             CPPUNIT_ASSERT_EQUAL(int(grp * 2 + elem), testStep.first());
             CPPUNIT_ASSERT_EQUAL(int(grp * 2 + elem), testStep.last());
             CPPUNIT_ASSERT_EQUAL(1u, testStep.nRanks());
        }   
   }
}

void StepIDTest::testTooFewRanks()
{
   // group of single rank steps
   StepID srGroup(0,5);
   // the following should generate the exception because the group requiring 6 ranks cannot be mapped to 5 available
   srGroup.project(5);
}

void StepIDTest::testTooFewRanksFlex()
{
   // group of single rank steps
   StepID srGroup(-6,-1);
   // the following should generate the exception because the group requiring 6 ranks cannot be mapped to 5 available
   srGroup.project(5);  
}

void StepIDTest::testUnevenRanks()
{
   // group of 2-rank steps
   StepID mrGroup(0,-1,2);
   // the following should generate the exception because an even number of ranks is required to map this allocation
   mrGroup.project(5);
}

    
void StepIDTest::testProjectFixed() {
   // fixed allocation should remain unchanged by projection operation
   StepID srStep(3);
   srStep.project(5);
   CPPUNIT_ASSERT(srStep.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(3, srStep.first());
   CPPUNIT_ASSERT_EQUAL(3, srStep.last());
   CPPUNIT_ASSERT_EQUAL(1u, srStep.nRanks());

   // group of single rank steps
   StepID srGroup(0,5);
   srGroup.project(6);

   // group of two-rank steps
   StepID mrGroup(0,5,2);
   mrGroup.project(6);
   
   inspectGroups(srGroup, mrGroup);   
}

void StepIDTest::testProjectFlex() {
   // flexible allocation "-2" maps to penultimate rank which is zero-based rank number 3 for 5 ranks in total 
   StepID srStep(-2);
   srStep.project(5);
   CPPUNIT_ASSERT(srStep.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(3, srStep.first());
   CPPUNIT_ASSERT_EQUAL(3, srStep.last());
   CPPUNIT_ASSERT_EQUAL(1u, srStep.nRanks());   

   // group of single rank steps
   StepID srGroup(-6,-1);
   srGroup.project(6);

   // group of two-rank steps
   StepID mrGroup(-6,-1,2);
   mrGroup.project(6);
   
   inspectGroups(srGroup, mrGroup);   

   // group of single rank steps
   StepID srGroup2(0,-1);
   srGroup2.project(6);

   // group of two-rank steps
   StepID mrGroup2(0,-1,2);
   mrGroup2.project(6);

   inspectGroups(srGroup2, mrGroup2);   
}

void StepIDTest::testCopy()
{
   // group of single rank steps
   StepID srGroup(0,-1);
   StepID srGroup2(srGroup);
   srGroup.project(6);
   srGroup2.project(6);

   // group of two-rank steps
   StepID mrGroup(0,-2,2);
   mrGroup.project(7);
   StepID mrGroup2(mrGroup);
   
   inspectGroups(srGroup, mrGroup);
   inspectGroups(srGroup2, mrGroup2);      
}


} // end of namespace askapparallel
} // end of namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_STEP_ID_TEST_H

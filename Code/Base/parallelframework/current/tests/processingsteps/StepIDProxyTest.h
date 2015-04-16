/// @file
/// 
/// @brief Tests of StepIDProxy class
/// @details See StepIDProxy  for description of what this class  
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

#ifndef ASKAP_ASKAPPARALLEL_STEP_ID_PROXY_TEST_H
#define ASKAP_ASKAPPARALLEL_STEP_ID_PROXY_TEST_H

#include "processingsteps/StepID.h"
#include "processingsteps/StepIDProxy.h"
#include "processingsteps/CompositeStep.h"
#include <askap/AskapError.h>

#include <cppunit/extensions/HelperMacros.h>

namespace askap {

namespace askapparallel {

class StepIDProxyTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(StepIDProxyTest);
   CPPUNIT_TEST(testConstruction);
   CPPUNIT_TEST(testProcess);
   CPPUNIT_TEST_EXCEPTION(testWrongSlicing, AskapError);   
   CPPUNIT_TEST_SUITE_END();
public:
   void testConstruction();   
   void testProcess();
   void testWrongSlicing();   
};

void StepIDProxyTest::testConstruction()
{
   boost::shared_ptr<CompositeStep> csPtr(new CompositeStep);
   StepIDProxy proxy1(5u, csPtr, false);
   CPPUNIT_ASSERT_EQUAL(csPtr, proxy1.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(5u), proxy1.index());
   CPPUNIT_ASSERT(!proxy1.isSingleRank());
      
   StepIDProxy proxy2(5u, csPtr, 2, 1);
   CPPUNIT_ASSERT_EQUAL(csPtr, proxy2.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(5u), proxy2.index());
   CPPUNIT_ASSERT(proxy2.isSingleRank());
   
   
   StepIDProxy proxy3(5u, boost::shared_ptr<CompositeStep>(), true);
   CPPUNIT_ASSERT(!proxy3.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(5u), proxy3.index());
   CPPUNIT_ASSERT(proxy3.isSingleRank());      
}

void StepIDProxyTest::testProcess()
{
   StepIDProxy proxy1(0u, boost::shared_ptr<CompositeStep>(), false);
   CPPUNIT_ASSERT(!proxy1.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(0u), proxy1.index());

   StepID mrGroup(0,5,2);
   const StepID test1 = proxy1.process(mrGroup);
   CPPUNIT_ASSERT(!test1.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(0, test1.first());
   CPPUNIT_ASSERT_EQUAL(5, test1.last());
   CPPUNIT_ASSERT_EQUAL(2u, test1.nRanks());

   StepIDProxy proxy2(0u, boost::shared_ptr<CompositeStep>(), 2, 1);
   CPPUNIT_ASSERT(!proxy2.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(0u), proxy2.index());

   const StepID test2 = proxy2.process(mrGroup);
   CPPUNIT_ASSERT(test2.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(5, test2.first());
   CPPUNIT_ASSERT_EQUAL(5, test2.last());
   CPPUNIT_ASSERT_EQUAL(1u, test2.nRanks());
   
   StepIDProxy proxy3 = proxy1(2, 1);
   CPPUNIT_ASSERT(!proxy3.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(0u), proxy3.index());
   CPPUNIT_ASSERT(proxy3.isSingleRank());
   
   const StepID test3 = proxy3.process(mrGroup);
   CPPUNIT_ASSERT(test3.isSingleRank());
   CPPUNIT_ASSERT_EQUAL(5, test3.first());
   CPPUNIT_ASSERT_EQUAL(5, test3.last());
   CPPUNIT_ASSERT_EQUAL(1u, test3.nRanks());      
}

void StepIDProxyTest::testWrongSlicing()
{
   StepIDProxy proxy(0u, boost::shared_ptr<CompositeStep>(), 2, 1);
   CPPUNIT_ASSERT(!proxy.composite());
   CPPUNIT_ASSERT_EQUAL(size_t(0u), proxy.index());
   StepID srGroup(0,3);
   // the following should trigger the exception because we don't have group 2 element 1 here
   proxy.process(srGroup);   
}

} // end of namespace askapparallel
} // end of namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_STEP_ID_PROXY_TEST_H

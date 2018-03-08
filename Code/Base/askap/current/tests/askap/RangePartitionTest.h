/// @file
///
/// @brief Test of the range partition class        
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
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>

#ifndef ASKAP_UTILITY_RANGE_PARTITION_TEST_H
#define ASKAP_UTILITY_RANGE_PARTITION_TEST_H

#include <cppunit/extensions/HelperMacros.h>
#include <askap/RangePartition.h>

#include <iostream>

namespace askap {

namespace utility {

class RangePartitionTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(RangePartitionTest);
  //CPPUNIT_TEST(tempTest);

  CPPUNIT_TEST(testEqualPartition);
  CPPUNIT_TEST(testUnequalPartition);
  CPPUNIT_TEST(testOneItemPerGroup);
  CPPUNIT_TEST(testMoreGroupsThanItems);
  CPPUNIT_TEST_EXCEPTION(testVoidGroupAccess1,CheckError);
  CPPUNIT_TEST_EXCEPTION(testVoidGroupAccess2,CheckError);
  CPPUNIT_TEST(testSpecificSettings);
  
  CPPUNIT_TEST_SUITE_END();
public:
  void checkConsistency(const RangePartition &rp) {
     unsigned int itemCounter = 0u;
     for (unsigned int grp = 0; grp < rp.nGroups(); ++grp) {
          CPPUNIT_ASSERT(rp.voidGroup(grp) == (grp >= rp.nNonVoidGroups()));
          if (rp.voidGroup(grp)) {
              CPPUNIT_ASSERT_EQUAL(0u, rp.nItemsThisGroup(grp));
          } else {
              CPPUNIT_ASSERT(rp.nItemsThisGroup(grp) <= rp.nItems());
              CPPUNIT_ASSERT(rp.nItemsThisGroup(grp) > 0u);
              CPPUNIT_ASSERT_EQUAL(rp.last(grp) + 1u, rp.first(grp) + rp.nItemsThisGroup(grp));
              if (grp == 0) {
                  CPPUNIT_ASSERT_EQUAL(0u, rp.first(grp));
              } else {
                  CPPUNIT_ASSERT_EQUAL(rp.first(grp), rp.last(grp - 1) + 1);
              }
          }
          itemCounter += rp.nItemsThisGroup(grp);
     }
     CPPUNIT_ASSERT_EQUAL(itemCounter, rp.nItems()); 
  }

  void testEqualPartition() {
     // 10 elements into 2 groups
     RangePartition rp(10,2);
     CPPUNIT_ASSERT_EQUAL(10u, rp.nItems());
     CPPUNIT_ASSERT_EQUAL(2u, rp.nGroups());

     checkConsistency(rp);
     // groups 0..4 and 5..9
     CPPUNIT_ASSERT_EQUAL(5u, rp.first(1u));
     CPPUNIT_ASSERT_EQUAL(4u, rp.last(0u));
     CPPUNIT_ASSERT_EQUAL(9u, rp.last(1u));
     CPPUNIT_ASSERT_EQUAL(5u, rp.nItemsThisGroup(0u));
     CPPUNIT_ASSERT_EQUAL(5u, rp.nItemsThisGroup(1u));
  }

  void testUnequalPartition() {
     // 13 elements into 3 groups
     RangePartition rp(13,3);
     CPPUNIT_ASSERT_EQUAL(13u, rp.nItems());
     CPPUNIT_ASSERT_EQUAL(3u, rp.nGroups());

     checkConsistency(rp);
     // groups 0..4, 5..9, 10..12
     CPPUNIT_ASSERT_EQUAL(5u, rp.first(1u));
     CPPUNIT_ASSERT_EQUAL(4u, rp.last(0u));
     CPPUNIT_ASSERT_EQUAL(9u, rp.last(1u));
     CPPUNIT_ASSERT_EQUAL(10u, rp.first(2u));
     CPPUNIT_ASSERT_EQUAL(12u, rp.last(2u));
     CPPUNIT_ASSERT_EQUAL(5u, rp.nItemsThisGroup(0u));
     CPPUNIT_ASSERT_EQUAL(5u, rp.nItemsThisGroup(1u));
     CPPUNIT_ASSERT_EQUAL(3u, rp.nItemsThisGroup(2u));
  }
  
  void testOneItemPerGroup() {
     // 13 elements into 13 groups
     RangePartition rp(13,13);
     CPPUNIT_ASSERT_EQUAL(13u, rp.nItems());
     CPPUNIT_ASSERT_EQUAL(13u, rp.nGroups());

     checkConsistency(rp);

     for (unsigned int grp = 0; grp < rp.nGroups(); ++grp) {
          CPPUNIT_ASSERT_EQUAL(1u, rp.nItemsThisGroup(grp));
     }
  }

  void testMoreGroupsThanItems() {
     // 3 elements into 13 groups
     RangePartition rp(3,13);
     CPPUNIT_ASSERT_EQUAL(3u, rp.nItems());
     CPPUNIT_ASSERT_EQUAL(13u, rp.nGroups());

     checkConsistency(rp);
     CPPUNIT_ASSERT_EQUAL(3u, rp.nNonVoidGroups());
     for (unsigned int grp = 0; grp < rp.nGroups(); ++grp) {
          if (grp < rp.nNonVoidGroups()) {
              CPPUNIT_ASSERT_EQUAL(1u, rp.nItemsThisGroup(grp));
          } else {
              CPPUNIT_ASSERT_EQUAL(0u, rp.nItemsThisGroup(grp));
          }
     }
  }

  void testVoidGroupAccess1() {
     // 3 elements into 13 groups
     RangePartition rp(3,13);
     CPPUNIT_ASSERT_EQUAL(3u, rp.nItems());
     CPPUNIT_ASSERT_EQUAL(13u, rp.nGroups());

     // this will generate exception
     rp.first(4u);
  }

  void testVoidGroupAccess2() {
     // 3 elements into 13 groups
     RangePartition rp(3,13);
     CPPUNIT_ASSERT_EQUAL(3u, rp.nItems());
     CPPUNIT_ASSERT_EQUAL(13u, rp.nGroups());

     // this will generate exception
     rp.last(4u);
  }

  void tempTest() {
     RangePartition rp(16200, 319);
     checkConsistency(rp);
  }
  
  void testSpecificSettings() {
     // test specific settings which arise in using of actual scientific code, see ASKAPSDP-2962
     const unsigned int nTrials = 7;
     const unsigned int nWorkers[nTrials] = {9, 19, 39, 79, 810, 319, 639};
   
     for (unsigned int trial = 0; trial < nTrials-2; ++trial) {
          // 16200 channels distributed across the given number of workers
          RangePartition rp(16200, nWorkers[trial]);
          CPPUNIT_ASSERT_EQUAL(16200u, rp.nItems());
          CPPUNIT_ASSERT_EQUAL(nWorkers[trial], rp.nGroups());

          checkConsistency(rp);
          CPPUNIT_ASSERT_EQUAL(nWorkers[trial], rp.nNonVoidGroups());
     }
  }
  
};

} // namespace utility

} // namespace askap

#endif // #ifndef ASKAP_UTILITY_RANGE_PARTITION_TEST_H


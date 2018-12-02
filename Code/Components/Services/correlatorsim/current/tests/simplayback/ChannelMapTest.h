/// @file BaselineMapTest.cc
///
/// @copyright (c) 2015 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <iostream>
#include <limits>
#include <stdint.h>
#include "askap/AskapError.h"

// Classes to test
#include "simplayback/ChannelMap.h"

using namespace std;

namespace askap {
namespace cp {

class ChannelMapTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(ChannelMapTest);
        CPPUNIT_TEST(testForward);
        CPPUNIT_TEST(testBackward);
        CPPUNIT_TEST_SUITE_END();

    public:

        const static uint32_t chanMin = 0;
        const static uint32_t nChan = 216;

        void setUp() {
        };

        void tearDown() {
        }

        // forward: contiguous to non-contiguous back to contiguoys
        void testForward() {
            ChannelMap cmap;
            for (uint32_t contChan = chanMin; contChan < nChan; ++contChan) {
                uint32_t nonContChan = cmap.toCorrelator(contChan);
                uint32_t contChanCheck = cmap.fromCorrelator(nonContChan);
                //cout << contChan << " : " << nonContChan << " : " <<
                //        contChanCheck << endl;
                CPPUNIT_ASSERT_EQUAL(contChan, contChanCheck);
            }
        }

        // backward: non-contiguous to contiguous back to non-contiguoys
        void testBackward() {
            ChannelMap cmap;
            for (uint32_t nonContChan = chanMin; nonContChan < nChan; 
                    ++nonContChan) {
                uint32_t contChan = cmap.fromCorrelator(nonContChan);
                uint32_t nonContChanCheck = cmap.toCorrelator(contChan);
                //cout << nonContChan << " : " << contChan << " : " <<
                //        nonContChanCheck << endl;
                CPPUNIT_ASSERT_EQUAL(nonContChan, nonContChanCheck);
            }
        }

    private:

};

}   // End namespace cp
}   // End namespace askap

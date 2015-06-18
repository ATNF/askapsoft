/// @file VisDatagramTest.cc
///
/// @copyright (c) 2010 CSIRO
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

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>

// Classes to test
#include "cpcommon/VisDatagram.h"


namespace askap {
namespace cp {

// we can only test compilation of templates here
// nothing else makes sense for interfaces
class VisDatagramTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(VisDatagramTest);
        CPPUNIT_TEST(testBETA);
        CPPUNIT_TEST(testADE);
        CPPUNIT_TEST_SUITE_END();

    public:

        void testBETA() {
          VisDatagramBETA vd;
          VisDatagramTraits<VisDatagramBETA> dt;
          CPPUNIT_ASSERT_EQUAL(1u, dt.VISPAYLOAD_VERSION);
          const uint32_t version = protocolVersion(vd);
          CPPUNIT_ASSERT_EQUAL(1u, version);

          const uint32_t sliceSize = dt.N_CHANNELS_PER_SLICE;
          CPPUNIT_ASSERT_EQUAL(sliceSize, VisDatagramTraits<VisDatagramBETA>::N_CHANNELS_PER_SLICE);
          CPPUNIT_ASSERT(sliceSize != 0);

          CPPUNIT_ASSERT_EQUAL(2014u, yearCommissioned(vd));
        }

        void testADE() {
          VisDatagramADE vd;
          VisDatagramTraits<VisDatagramADE> dt;
          CPPUNIT_ASSERT_EQUAL(2u, dt.VISPAYLOAD_VERSION);
          const uint32_t version = protocolVersion(vd);
          CPPUNIT_ASSERT_EQUAL(2u, version);

          const uint32_t maxSliceSize = dt.MAX_BASELINES_PER_SLICE;
          CPPUNIT_ASSERT_EQUAL(maxSliceSize, VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE);

          CPPUNIT_ASSERT_EQUAL(2016u, yearCommissioned(vd));
        }

    private:
        // obtain version of the protocol
        template<typename T>
        static uint32_t protocolVersion(const T &) {
           return VisDatagramTraits<T>::VISPAYLOAD_VERSION;
        }

        // demonstration of protocol-specific actions through SFINAE
        // (although it can also be done via specialisation)
        // Specialisation makes sense if we have 

        // the following method is compiled only for datagram protocols
        // which have BETA trait defined 
        // NB: second template argument of enable_if is the return type
        template<typename T>
        static typename boost::enable_if<boost::is_class<typename
           VisDatagramTraits<T>::BETA>, uint32_t>::type 
                  yearCommissioned(const T&) {

            // it is safe to access BETA-specific fields in
            // VisDatagramTraits<T> here.

            return 2014;
        }

        // the following method is compiled only for datagram protocols
        // which have ADE trait defined 
        // NB: second template argument of enable_if is the return type
        template<typename T>
        static typename boost::enable_if<boost::is_class<typename
           VisDatagramTraits<T>::ADE>, uint32_t>::type 
                  yearCommissioned(const T&) {

            // it is safe to access ADE-specific fields in
            // VisDatagramTraits<T> here.

            return 2016;
        }
 
};

}   // End namespace cp

}   // End namespace askap


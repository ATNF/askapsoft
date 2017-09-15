/// @file VisDatagramTestHelper.h
/// @brief helper class to encapsulate protocol-specific operations
/// @details This class contains static methods which are conditionally
/// compiled via SFINAE and allow protocol-specific actions. It is used
/// in testing only.
///
/// @copyright (c) 2013 CSIRO
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

#ifndef ASKAP_CP_INGEST_TESTS_VISDATAGRAM_TEST_HELPER_H
#define ASKAP_CP_INGEST_TESTS_VISDATAGRAM_TEST_HELPER_H

// Support classes
#include "boost/shared_ptr.hpp"
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include "cpcommon/VisDatagram.h"
#include <boost/static_assert.hpp>

// std includes
#include <algorithm>

namespace askap {
namespace cp {
namespace ingest {

/// @brief helper class to encapsulate protocol-specific operations
template<typename T, typename Enable = void>
struct VisDatagramTestHelper {
    BOOST_STATIC_ASSERT_MSG(sizeof(T) == 0, 
         "Attempted to use VisDatagramTestHelper with unspecified protocol");
};


// specialisation for BETA-style protocol
template<typename T>
struct VisDatagramTestHelper<T, typename boost::enable_if<boost::is_class<typename
        VisDatagramTraits<T>::BETA> >::type>  {

   /// @return number of channels carried in a single datagram
   static uint32_t nChannelsPerDatagram() {
       // for BETA, data are sliced in frequency - just retun the slice size
       return VisDatagramTraits<T>::N_CHANNELS_PER_SLICE;
   }

   /// @return number of channels simulated in the test
   static uint32_t nChannelsForTest() {
       return nChannelsPerDatagram();
   }

   /// @brief populate protocol-specific fields
   /// @param[in] vis visibility datagram to work with
   static void fillProtocolSpecificInfo(T& vis) {
       // we simulate only the first correlation product
       vis.baselineid = 1;
       // initialise visibility array with zeros - although we don't check
       // visibilities as part of the test, NaNs, etc can fail the integrity check logic and
       // cause incorrect results
       std::fill_n(vis.vis, VisDatagramTraits<T>::N_CHANNELS_PER_SLICE, FloatComplex(0.,0.));
   }     

   /// @brief test whether given accessor channel and baseline product are expected to be defined
   /// @param[in] chan 0-based channel in the resulting accessor (from 0 to nChannelsForTest()-1)
   /// @param[in] product 1-based correlator product
   /// @return true, if channel and product are expected to be valid
   static bool validChannelAndProduct(uint32_t chan, uint32_t product) {
       const uint32_t slice = 0;
       const uint32_t startChan = slice * nChannelsPerDatagram(); //inclusive
       const uint32_t endChan = (slice + 1) * nChannelsPerDatagram(); //exclusive
       return (product == 1) && (chan >= startChan) && (chan < endChan);
   }
};

// specialisation for ADE-style protocol
template<typename T>
struct VisDatagramTestHelper<T, typename boost::enable_if<boost::is_class<typename
        VisDatagramTraits<T>::ADE> >::type>  {


   /// @return number of channels carried in a single datagram
   static uint32_t nChannelsPerDatagram() {
      // for ADE, there is one channel in the datagram
      return 1u;
   }

   /// @return number of channels simulated in the test
   static uint32_t nChannelsForTest() {
       // for ADE simulate a set of channels one card is responsible for
       return 216u;
   }

   /// @brief populate protocol-specific fields
   /// @param[in] vis visibility datagram to work with
   static void fillProtocolSpecificInfo(T& vis) {
       // we simulate only the first correlation product
       vis.baseline1 = 1;
       vis.baseline2 = 21;
       vis.block = 1;
       vis.card = 1;
       vis.channel = 11;
       vis.freq = 1e3;
       // initialise visibility array with zeros - although we don't check
       // visibilities as part of the test, NaNs, etc can fail the integrity check logic and
       // cause incorrect results
       std::fill_n(vis.vis, VisDatagramTraits<T>::MAX_BASELINES_PER_SLICE, FloatComplex(0.,0.));
   }     
        
   /// @brief test whether given accessor channel and baseline product are expected to be defined
   /// @param[in] chan 0-based channel in the resulting accessor (from 0 to nChannelsForTest()-1)
   /// @param[in] product 1-based correlator product
   /// @return true, if channel and product are expected to be valid
   static bool validChannelAndProduct(uint32_t chan, uint32_t product) {
        // 1-based channel 11 in the datagram translates to zero-based channel 55 in the accessor
        // (see VisConverterADE).
        const uint32_t expectedChan = 55;

        // ADE system has a single channel per datagram, but several baselines
        const uint32_t startProduct = 1; // inclusive
        const uint32_t endProduct = 21; // inclusive
        return (chan == expectedChan) && (product >= startProduct) && (product <= endProduct);
   }

};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

#endif // #ifndef ASKAP_CP_INGEST_TESTS_VISDATAGRAM_TEST_HELPER_H


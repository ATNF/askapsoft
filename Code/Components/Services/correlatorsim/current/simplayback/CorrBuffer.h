/// @file CorrBuffer.h
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
///
/// Buffer unit that provides convenient intermediate data format during
/// conversion from measurement set to datagram

#ifndef ASKAP_CP_CORRBUFFER_H
#define ASKAP_CP_CORRBUFFER_H 

#include <stdint.h>
#include <vector>
#include <string>
#include <boost/optional.hpp>
#include "cpcommon/FloatComplex.h"
#include "CorrBufferUnit.h"
#include "FreqIndex.h"

using std::vector;
using std::string;


namespace askap {
namespace cp {

class CorrBuffer
{
    public :

        /// Constructor
        CorrBuffer();

        /// Destructor
        virtual ~CorrBuffer();

        /// Initialize
        /// @param[in] Total number of correlation products
        /// @param[in] Total number of channels
        void init(uint32_t nCorrProd, uint32_t nChannel);

        /// Reset the buffer
        void reset();

		/// Insert data into buffer. Note that range is checked.
		void insert(uint32_t cp, uint32_t chan, const FloatComplex& vis);

        /// Find the next correlation product that has no data.
#ifdef NEW_BUFFER
        /// @param[in] Starting correlation product for search.
		/// Note that any negative number forces search at the top.
		/// @param[in] Search in this channel.
        /// @return The next empty correlation product
        boost::optional<uint32_t> findNextEmptyCorrProd(uint32_t startCP, 
                uint32_t chan) const;
#else
        /// @param[in] Starting point for search (-1 to search from beginning)
        /// @return The next empty correlation product (-1 if none found)
        int32_t findNextEmptyCorrProd(int32_t startCP) const;
#endif

#ifdef NEW_BUFFER
        /// Find the next correlation product that has data.
        /// @param[in] Starting correlation product for search.
		/// Note that any negative number forces search at the top.
		/// @param[in] Search in this channel.
        /// @return The next correlation product that has data
        boost::optional<uint32_t> findNextFullCorrProd(uint32_t startCP, 
                uint32_t chan) const;
#else
        /// Find the next correlation product with original data (not copy)
        /// @param[in] Starting point for search (-1 to search from beginning).
        /// @return The next correlation product with original data
        /// (-1 if none found)
        int32_t findNextOriginalCorrProd(int32_t startCP) const;
#endif

#ifdef NEW_BUFFER
		/// Find the next channel that has no data
        boost::optional<uint32_t> findNextEmptyChannel(uint32_t cp, 
                uint32_t startChan) const;

		/// Find the next channel that has data
        boost::optional<uint32_t> findNextFullChannel(uint32_t cp, 
                uint32_t startChan) const;

        /// Fill all empty correlation products of a given channel
        void fillOneChannel(uint32_t chan);

        /// Fill all empty channels of a given correlation product
        void fillOneCorrProduct(uint32_t cp);
#endif

        /// Copy one correlation product to another (for all channels).
		/// Note that range is checked.
        /// @param[in] Correlation product index used as source
        /// @param[in] Correlation product index used as destination
        void copyCorrProd(int32_t source, int32_t target);

        /// Copy one channel to another (for all correlation products)
		/// Note that range is checked.
        /// @param[in] Channel index used as source
        /// @param[in] Channel index used as destination
        void copyChannel(int32_t source, int32_t target);

		/// Count rows of the buffer
		uint32_t getRowCount() const;

		/// Count columns of the buffer
		uint32_t getColumnCount() const;

		/// Count the same visibility data in the buffer
		uint32_t countSameVisibility(const CorrBuffer& buffer2,
				const float small) const;

        /// Print the buffer
        void print() const;

		void print(const string& option) const;

        // data
        
        /// Time stamp of this buffer
        uint64_t timeStamp;

        /// Beam index of this buffer
        uint32_t beam;

        /// True if the data is ready to use
        bool ready;

        /// Channel count in measurement set (original data)
        uint32_t nChanMeas;

        /// Card count (each card contains a limited number of channels)
        uint32_t nCard;

        /// 2D array of data containing original data from measurement set
        /// and dummy data (derived from measurement set)
        /// Row   : correlation products as specified in parset
        /// Column: channels as specified in parset
        vector<vector<CorrBufferUnit> > data;

#ifndef NEW_BUFFER
        /// Array of correlation product status
        /// True if correlation product is filled with data
		// TO BE DEPRECATED
        vector<bool> corrProdIsFilled;

        /// Array of correlation product status
        /// True if correlation product is original data (not copy) 
		// TO BE DEPRECATED
        vector<bool> corrProdIsOriginal;
#endif

        /// Array of frequency index, which includes block, card, channel
        /// and frequency itself
        vector<FreqIndex> freqId;
};

};
};

#endif

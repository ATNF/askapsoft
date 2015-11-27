/// @file Permutation.h
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>
///
/// Map channel ID from and to correlator.
/// Note that the channels are contiguous in ingest pipeline, but 
/// not contiguous in the correlator.

#ifndef ASKAP_CP_CHANNELMAP_H
#define ASKAP_CP_CHANNELMAP_H

#include <stdint.h>

namespace askap {
namespace cp {

class ChannelMap {
	
	public :
	
		ChannelMap();
		
		virtual ~ChannelMap();

        /// Convert channel ordering from correlator. 
        /// The resulting channels are contiguous.
        /// Note that the channels are 0-based
        /// @param[in] channelID     channel ID in correlator
        /// @return channel ID in ingest (contiguous)
		uint32_t fromCorrelator(uint32_t channelID);
		
        /// Convert channel ordering to correlator.
        /// Note that the channels are 0-based
        /// @param[in] channel ID   channel ID in ingest (contiguous)
        /// @return channel ID in correlator
		uint32_t toCorrelator(uint32_t channelID);
};
 
};
};

#endif

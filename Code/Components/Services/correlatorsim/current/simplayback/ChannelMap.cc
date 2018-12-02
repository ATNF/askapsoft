/// @file ChannelMap.cc
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


// Include own header file first
#include "ChannelMap.h"

// System includes
#include <iostream>
#include <stdint.h>

#include "askap/AskapError.h"

using namespace std;

using namespace askap;
using namespace askap::cp;


ChannelMap::ChannelMap() {;}



// Copied from Max

uint32_t ChannelMap::fromCorrelator(uint32_t channelID) const {
	ASKAPDEBUGASSERT(channelID < 216);
	const uint32_t fineOffset = channelID % 9;
	const uint32_t group = channelID / 9;
	//ASKAPDEBUGASSERT(group < 24);
	const uint32_t chip = group / 4; 
	const uint32_t coarseChannel = group % 4;
	return fineOffset + chip * 9 + coarseChannel * 54;
}


// The reverse mapping

uint32_t ChannelMap::toCorrelator(uint32_t channelID) const {
	ASKAPDEBUGASSERT(channelID < 216);
	const uint32_t fineOffset = channelID % 9;
	const uint32_t group = channelID / 9;
	const uint32_t chip = group % 6;
	const uint32_t coarseChannel = group / 6;
	return fineOffset + 9 * (coarseChannel + chip * 4);
}


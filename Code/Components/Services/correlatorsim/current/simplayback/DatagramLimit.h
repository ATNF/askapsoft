/// @file DatagramLimit.h
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
/// @author Paulus Lahur <Paulus.Lahur@csiro.au>
///
/// This file contains the constants that define the number of members,
/// minimum number (base number, usually 0 or 1), and maximum number
/// in datagram arrays.
/// Note that frequency channels are grouped in a card,
/// and cards are grouped in a block.
//
#ifndef ASKAP_CP_DATAGRAMLIMIT_H
#define ASKAP_CP_DATAGRAMLIMIT_H

#include <stdint.h>

namespace askap {
namespace cp {

    // For the blocks 
    const uint32_t DATAGRAM_NBLOCK = 8;     // total count
    const uint32_t DATAGRAM_BLOCKMIN = 1;   // base number
    const uint32_t DATAGRAM_BLOCKMAX = 8;   // largest number

    // Cards in a block
    const uint32_t DATAGRAM_NCARD = 12;     // total count
    const uint32_t DATAGRAM_CARDMIN = 1;    // base number
    const uint32_t DATAGRAM_CARDMAX = 12;   // largest number

    // Channels in a card
    // Each channel is associated with a certain frequency
    const uint32_t DATAGRAM_NCHANNEL = 216;     // total count
    const uint32_t DATAGRAM_CHANNELMIN = 1;     // base number
    const uint32_t DATAGRAM_CHANNELMAX = 216;   // largest number

    // Beam's base number
    const uint32_t DATAGRAM_BEAMMIN = 0;

    // Baseline's base number
    const uint32_t DATAGRAM_BASELINEMIN = 0;
};
};

#endif 

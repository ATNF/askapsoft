/// @file VisDatagramADE.h
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

#ifndef ASKAP_CP_VISDATAGRAM_ADE_H
#define ASKAP_CP_VISDATAGRAM_ADE_H

// this header shouldn't be included directly, include VisDatagram.h
// instead

#include "cpcommon/FloatComplex.h"
#include "cpcommon/VisDatagramTraits.h"
#include <stdint.h>

namespace askap {
    namespace cp {

        // forward declaration
        struct VisDatagramADE;  

        /// @brief specialisation for ADE-specific datagram
        template<>
        struct VisDatagramTraits<VisDatagramADE> {
           /// @brief Version number for the VisDatagramADE.
           static const uint32_t VISPAYLOAD_VERSION = 0x2;

           /// @brief Max number of baselines per slice in the VisDatagramADE. 
           /// One VisDatagram will then contain data for up to 
           /// MAX_BASELINES_PER_SLICE baselines.
           /// This is hardcoded so fixed size buffers can be used.
           static const uint32_t MAX_BASELINES_PER_SLICE = 657;

           /// @brief to enable conditional compilation via SFINAE
           struct ADE {};
        };

        /// @brief This structure specifies the UDP datagram which is sent from
        /// the correlator to the central processor. 
        struct VisDatagramADE
        {
            /// A version number for this structure. Also doubles as a magic
            /// number which can be used to verify if the datagram is of this
            /// type.
            uint32_t version;

            /// Slice number. tbd - document (ask Euan) what it means
            uint32_t slice;

            /// Timestamp - Binary Atomic Time (BAT). The number of microseconds
            /// since Modified Julian Day (MJD) = 0
            uint64_t timestamp;
            
            /// @brief block number 
            /// @details Indicates which block of 12 correlator cards. 
            /// Allowed values are from 1 to 8
            /// Part of frequency index
            uint32_t block;
            
            /// @brief card number
            /// @details Indicates which card in the block. 
            /// Allowed values are from 1 to 12
            /// Part of frequency index
            uint32_t card;
            
            /// @brief channel
            /// @details frequency channel index. 
            /// Allowed values are from 0 to 215
            /// Part of frequency index
            uint32_t channel;
            
            /// Frequency - sky frequency in MHz (originated in firmware)
            float freq;

            /// Beam ID
            /// This identified which synthesised beam this datagram corresponds to.
            /// This need not be contiguous and does not have to begin at zero or one.
            uint32_t beamid;
            
            /// Baseline IDs
            /// This identified which antenna pairs and polarisation products this datagram
            /// corresponds to.
            /// These are contiguous and baseline1 is the first baseline in this datagram
            /// and baseline2 is the last. #visibilities = baseline2 - baseline1
            uint32_t baseline1;
            uint32_t baseline2;


            /// Visibilities
            FloatComplex vis[VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE];
        } __attribute__((__packed__));

    };
};

#endif // #ifndef ASKAP_CP_VISDATAGRAM_ADE_H

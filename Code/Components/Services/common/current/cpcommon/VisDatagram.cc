/// @file VisDatagram.cc
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

#include "cpcommon/VisDatagram.h"

namespace askap {
    namespace cp {

       // definition of const statics for the sake of completeness
       // although this could've been skipped as these fields are
       // integral types and we never access them directly rather than
       // through type.

       // @brief Version number for the VisDatagramBETA.
       const uint32_t VisDatagramTraits<VisDatagramBETA>::VISPAYLOAD_VERSION;

       // @brief Number of channels per slice in the VisDatagramiBETA.
       const uint32_t VisDatagramTraits<VisDatagramBETA>::N_CHANNELS_PER_SLICE;

       // @brief Version number for the VisDatagramADE.
       const uint32_t VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION;

       // @brief Max number of baselines in VisDatagramADE. 
       const uint32_t VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE;
    } // namespace cp

} // namespace askap


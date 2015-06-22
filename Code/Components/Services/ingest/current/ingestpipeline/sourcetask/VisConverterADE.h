/// @file VisConverterADE.h
/// @brief converter of visibility stream to vis chunks
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

#ifndef ASKAP_CP_VISCONVERTER_ADE_H
#define ASKAP_CP_VISCONVERTER_ADE_H

// standard includes
#include <stdint.h>
#include <set>

// 3rd party
#include "Common/ParameterSet.h"
#include "casa/aipstype.h"
#include "boost/tuple/tuple.hpp"

// local includes
#include "ingestpipeline/sourcetask/VisConverterDef.h"
#include "ingestpipeline/sourcetask/VisConverterBase.h"
#include "configuration/Configuration.h"
#include "configuration/CorrelatorMode.h"
#include "cpcommon/VisDatagram.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief converter for VisDatagramADE 
/// @details This is specialisation using distribution of 
/// datagrams per baselines which we planned to use for ADE
template<>
class VisConverter<VisDatagramADE> : public VisConverterBase {
public:
   /// @param[in] params parameters specific to the associated source task
   ///                   used to set up mapping, etc
   /// @param[in] config configuration
   /// @param[in] id rank of the given ingest process
   VisConverter(const LOFAR::ParameterSet& params,
                const Configuration& config, int id);

   /// @brief create a new VisChunk
   /// @details This method initialises itsVisChunk with a new buffer.
   /// It is intended to be used when the first datagram of a new 
   /// integration is processed.
   /// @param[in] timestamp BAT corresponding to this new chunk
   /// @param[in] corrMode correlator mode parameters (determines shape, etc)
   void initVisChunk(const casa::uLong timestamp, const CorrelatorMode &corrMode);

   /// @brief main method add datagram to the current chunk
   /// @details This method processes one datagram and adds it to 
   /// the current chunk (assumed to be already initialised)
   /// @param[in] vis datagram to process
   void add(const VisDatagramADE &vis);

private:
   /// Identifies a datagram based on beam, block, card, channel, slice
   /// This is used for duplicate detection
   typedef boost::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t> DatagramIdentity;

   std::set<DatagramIdentity> itsReceivedDatagrams;

   // temporary
   uint32_t itsNDuplicates;
};

} // namespace ingest
} // namespace cp
} // namespace askap

//autoinstantiation has been disabled (see VisConverter.cc for manual
//instantiation).
//#include "ingestpipeline/sourcetask/VisConverterADE.tcc"

#endif // #ifndef ASKAP_CP_VISCONVERTER_ADE_H


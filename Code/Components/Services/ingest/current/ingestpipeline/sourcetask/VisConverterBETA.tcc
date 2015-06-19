/// @file VisConverterBETA.tcc
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

#ifndef ASKAP_CP_VISCONVERTER_BETA_TCC
#define ASKAP_CP_VISCONVERTER_BETA_TCC

// standard includes

// 3rd party
#include "boost/bind.hpp"
#include "boost/tuple/tuple_comparison.hpp"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"

ASKAP_LOGGER(logger, ".VisConverterBETA");

namespace askap {
namespace cp {
namespace ingest {

/// @param[in] params parameters specific to the associated source task
///                   used to set up mapping, etc
/// @param[in] config configuration
/// @param[in] id rank of the given ingest process
VisConverter<VisDatagramBETA>::VisConverter(const LOFAR::ParameterSet& params,
       const Configuration& config, int id) : 
       VisConverterBase(params, config, id) {}

/// @brief create a new VisChunk
/// @details This method initialises itsVisChunk with a new buffer.
/// It is intended to be used when the first datagram of a new 
/// integration is processed.
/// @param[in] timestamp BAT corresponding to this new chunk
/// @param[in] corrMode correlator mode parameters (determines shape, etc)
void VisConverter<VisDatagramBETA>::initVisChunk(const casa::uLong timestamp, 
               const CorrelatorMode &corrMode)
{
   itsReceivedDatagrams.clear();
   VisConverterBase::initVisChunk(timestamp, corrMode);
}

/// @brief main method add datagram to the current chunk
/// @details This method processes one datagram and adds it to 
/// the current chunk (assumed to be already initialised)
/// @param[in] vis datagram to process
void VisConverter<VisDatagramBETA>::add(const VisDatagramBETA &vis)
{
   ASKAPCHECK(vis.slice < 16, "Slice index is invalid");

   // Detect duplicate datagrams
   const DatagramIdentity identity(vis.baselineid, vis.slice, vis.beamid);
   if (itsReceivedDatagrams.find(identity) != itsReceivedDatagrams.end()) {
       ASKAPLOG_WARN_STR(logger, "Duplicate VisDatagram - BaselineID: " << 
            vis.baselineid << ", Slice: " << vis.slice << ", Beam: " << 
            vis.beamid);
      
       return;
   }
   itsReceivedDatagrams.insert(identity);
}

} // namespace ingest
} // namespace cp
} // namespace askap

#endif // #ifndef ASKAP_CP_VISCONVERTER_BETA_TCC


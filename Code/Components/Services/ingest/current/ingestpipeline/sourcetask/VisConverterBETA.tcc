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
#include "askap/AskapError.h"


namespace askap {
namespace cp {
namespace ingest {

/// @param[in] params parameters specific to the associated source task
///                   used to set up mapping, etc
/// @param[in] config configuration
/// @param[in] id rank of the given ingest process
VisConverter<VisDatagramBETA>::VisConverter(const LOFAR::ParameterSet& params,
       const Configuration& config, int id) : 
       VisConverterBase(params, config, id) 
{
   ASKAPLOG_INFO_STR(logger, "Initialised BETA-style visibility stream converter, id="<<id);
}

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
   const casa::uInt nChannels = channelManager().localNChannels(id());
    
   ASKAPCHECK(nChannels % VisDatagramTraits<VisDatagramBETA>::N_CHANNELS_PER_SLICE == 0,
        "Number of channels must be divisible by N_CHANNELS_PER_SLICE");
   const casa::uInt datagramsExpected = nCorrProducts() * maxNumberOfBeams() * 
          (nChannels / VisDatagramTraits<VisDatagram>::N_CHANNELS_PER_SLICE);
   setNumberOfExpectedDatagrams(datagramsExpected);
}

/// @brief main method add datagram to the current chunk
/// @details This method processes one datagram and adds it to 
/// the current chunk (assumed to be already initialised)
/// @param[in] vis datagram to process
void VisConverter<VisDatagramBETA>::add(const VisDatagramBETA &vis)
{
   common::VisChunk::ShPtr chunk = visChunk();
   ASKAPASSERT(chunk);

   // map correlator product to the row and polarisation index
   const boost::optional<std::pair<casa::uInt, casa::uInt> > prod = 
          mapCorrProduct(vis.baselineid, vis.beamid);

   if (!prod) {
       // warning has already been given inside mapCorrProduct
       countDatagramAsIgnored();
       return;
   }

   ASKAPCHECK(vis.slice < 16, "Slice index is invalid");

   const casa::uInt row = prod->first;
   const casa::uInt polidx = prod->second;
   ASKAPDEBUGASSERT(row < chunk->nRow());
   ASKAPDEBUGASSERT(polidx < chunk->nPol());
   ASKAPCHECK(chunk->nPol() == 4, "Currently only support full polarisation case");

   // Detect duplicate datagrams
   const DatagramIdentity identity(vis.baselineid, vis.slice, vis.beamid);
   if (itsReceivedDatagrams.find(identity) != itsReceivedDatagrams.end()) {
       ASKAPLOG_WARN_STR(logger, "Duplicate VisDatagram - BaselineID: " << 
            vis.baselineid << ", Slice: " << vis.slice << ", Beam: " << 
            vis.beamid);
       countDatagramAsIgnored();
       return;
   }
   itsReceivedDatagrams.insert(identity);

   // insert data into accessor, unflag if good
   const casa::uInt antenna1 = chunk->antenna1()(row);
   const casa::uInt antenna2 = chunk->antenna2()(row);
   const bool rowIsValid = isAntennaGood(antenna1) && isAntennaGood(antenna2);
   const bool isAutoCorr = antenna1 == antenna2;
   const casa::uInt chanOffset = (vis.slice) * VisDatagramTraits<VisDatagramBETA>::N_CHANNELS_PER_SLICE;
   for (casa::uInt chan = 0; chan < VisDatagramTraits<VisDatagramBETA>::N_CHANNELS_PER_SLICE; ++chan) {
        casa::Complex sample(vis.vis[chan].real, vis.vis[chan].imag);
        ASKAPCHECK((chanOffset + chan) <= chunk->nChannel(), "Channel index overflow");

        // note, always copy the data even if the row is flagged -
        // data could still be of interest
        chunk->visibility()(row, chanOffset + chan, polidx) = sample;

        // Unflag the sample 
        if (rowIsValid) {
            chunk->flag()(row, chanOffset + chan, polidx) = false;
        }

        if (isAutoCorr) {
            // For auto-correlations we duplicate cross-pols as index 2 should always be missing
            ASKAPDEBUGASSERT(polidx != 2);

            if (polidx == 1) {
                chunk->visibility()(row, chanOffset + chan, 2) = conj(sample);
                // Unflag the sample
                if (rowIsValid) {
                    chunk->flag()(row, chanOffset + chan, 2) = false;
                }
            }
        }
   }
  
   countDatagramAsUseful();
}

} // namespace ingest
} // namespace cp
} // namespace askap

#endif // #ifndef ASKAP_CP_VISCONVERTER_BETA_TCC


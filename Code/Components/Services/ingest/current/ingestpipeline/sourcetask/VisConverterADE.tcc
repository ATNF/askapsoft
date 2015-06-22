/// @file VisConverterADE.tcc
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

#ifndef ASKAP_CP_VISCONVERTER_ADE_TCC
#define ASKAP_CP_VISCONVERTER_ADE_TCC

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
VisConverter<VisDatagramADE>::VisConverter(const LOFAR::ParameterSet& params,
       const Configuration& config, int id) : 
       VisConverterBase(params, config, id), itsNDuplicates(0u)
{
   ASKAPLOG_INFO_STR(logger, "Initialised ADE-style visibility stream converter, id="<<id);
}

/// @brief create a new VisChunk
/// @details This method initialises itsVisChunk with a new buffer.
/// It is intended to be used when the first datagram of a new 
/// integration is processed.
/// @param[in] timestamp BAT corresponding to this new chunk
/// @param[in] corrMode correlator mode parameters (determines shape, etc)
void VisConverter<VisDatagramADE>::initVisChunk(const casa::uLong timestamp, 
               const CorrelatorMode &corrMode)
{
   itsReceivedDatagrams.clear();
   if (itsNDuplicates > 0) {
       ASKAPLOG_DEBUG_STR(logger, "Received "<<itsNDuplicates<<" duplicate datagram in the previous VisChunk");
       itsNDuplicates = 0u;
   }
   VisConverterBase::initVisChunk(timestamp, corrMode);
   const casa::uInt nChannels = channelManager().localNChannels(id());

   // the code below is experimental

   ASKAPCHECK(nChannels % 216 == 0, "Bandwidth should be multiple of 4-MHz");
    
   //const casa::uInt nSlices = 4;
   const casa::uInt nSlices = 1;
   const casa::uInt nBeams = 36;
   const casa::uInt datagramsExpected = nSlices * nBeams * nChannels;
   setNumberOfExpectedDatagrams(datagramsExpected);
}

/// @brief main method add datagram to the current chunk
/// @details This method processes one datagram and adds it to 
/// the current chunk (assumed to be already initialised)
/// @param[in] vis datagram to process
void VisConverter<VisDatagramADE>::add(const VisDatagramADE &vis)
{
   common::VisChunk::ShPtr chunk = visChunk();
   ASKAPASSERT(chunk);

   // the code below is experimental

   // check that the hardware sents sensible data
   ASKAPCHECK(vis.channel < 216, "vis.channel = "<<vis.channel<<" exceeds the limit of 216");
   ASKAPCHECK((vis.block > 0) && (vis.block <= 8), "vis.block = "<<vis.block<<" is outside [1,8] range");
   ASKAPCHECK((vis.card > 0) && (vis.card <= 12), "vis.card = "<<vis.card<<" is outside [1,12] range");
   ASKAPCHECK(vis.slice < 4, "Slice index is invalid");
   ASKAPCHECK(vis.beamid <= 36, "Currently support only up to 36 beams");

   
   // Detect duplicate datagrams
   const DatagramIdentity identity(vis.beamid, vis.block, vis.card, vis.channel, vis.slice);
   if (itsReceivedDatagrams.find(identity) != itsReceivedDatagrams.end()) {
       /*
       ASKAPLOG_WARN_STR(logger, "Duplicate VisDatagram - Block: " << 
            vis.block << ", Card: " << vis.card << ", Channel: " << 
            vis.channel<<", Beam: "<<vis.beamid << ", Slice: "<<vis.slice);
       */
       ++itsNDuplicates;
       countDatagramAsIgnored();
       return;
   }
   itsReceivedDatagrams.insert(identity);
   

   bool atLeastOneUseful = false;
   for (uint32_t product = vis.baseline1, item = 0; product <= vis.baseline2; ++product, ++item) {

        // check that sensible data received from hardware
        ASKAPCHECK(product > 0, "Expect product (baseline) number to be positive");
        ASKAPCHECK(product <= 2592, "Expect product (baseline) number to be 2592 or less, you have "<<product);
        ASKAPCHECK(item < VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE, 
              "Product "<<product<<" between baseline1="<<vis.baseline1<<
              " and baseline2="<<vis.baseline2<<" exceeds buffer size of "<<
              VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE);

        // can skip products here to avoid unnecessary warnings
        if (product > 78) { 
            break;
        }

        // map correlator product to the row and polarisation index
        const boost::optional<std::pair<casa::uInt, casa::uInt> > mp = 
             mapCorrProduct(product, vis.beamid);

        if (!mp) {
            // warning has already been given inside mapCorrProduct
            continue;
        }


        const casa::uInt row = mp->first;
        const casa::uInt polidx = mp->second;

        ASKAPDEBUGASSERT(row < chunk->nRow());
        ASKAPDEBUGASSERT(polidx < chunk->nPol());

        // for now
        if (vis.channel >= chunk->nChannel()) {
            ASKAPLOG_WARN_STR(logger, "Got channel outside bounds: "<<vis.channel);
            break;
        }
        const casa::uInt channel = vis.channel;
        ASKAPASSERT(channel < chunk->nChannel())
        //
        atLeastOneUseful = true;
        casa::Complex sample(vis.vis[product].real, vis.vis[product].imag);

        const casa::uInt antenna1 = chunk->antenna1()(row);
        const casa::uInt antenna2 = chunk->antenna2()(row);
        const bool rowIsValid = isAntennaGood(antenna1) && isAntennaGood(antenna2);
        const bool isAutoCorr = antenna1 == antenna2;
       
        // note, always copy the data even if the row is flagged -
        // data could still be of interest
        chunk->visibility()(row, channel, polidx) = sample;

        // Unflag the sample 
        if (rowIsValid) {
            chunk->flag()(row, channel, polidx) = false;
        }

        if (isAutoCorr) {
            // For auto-correlations we duplicate cross-pols as 
            // index 2 should always be missing
            ASKAPDEBUGASSERT(polidx != 2);
            ASKAPASSERT(chunk->nPol() == 4);

            if (polidx == 1) {
                // don't really need conjugation here as the sample should
                // be real
                chunk->visibility()(row, channel, 2) = conj(sample);
                // Unflag the sample
                if (rowIsValid) {
                    chunk->flag()(row, channel, 2) = false;
                }
            }
        }
   }

   if (atLeastOneUseful) {
       countDatagramAsUseful();
   } else {
       countDatagramAsIgnored();
   }
}

} // namespace ingest
} // namespace cp
} // namespace askap

#endif // #ifndef ASKAP_CP_VISCONVERTER_ADE_TCC


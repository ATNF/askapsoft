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
// just to access static methods
#include "ingestpipeline/sourcetask/VisSource.h"

// std includes
#include <iomanip>

namespace askap {
namespace cp {
namespace ingest {

/// @param[in] params parameters specific to the associated source task
///                   used to set up mapping, etc
/// @param[in] config configuration
/// @param[in] id rank of the given ingest process
VisConverter<VisDatagramADE>::VisConverter(const LOFAR::ParameterSet& params,
       const Configuration& config, int id) : 
       VisConverterBase(params, config, id), itsNSlices(4u), itsNDuplicates(0u),
       // unmapped products will be cached and ignored in the future processing
       // if product number falls beyond the bounds of this vector, full processing is done
       // This type of caching buys us about 0.3 seconds 
       itsInvalidProducts(2628u, false)
{
   ASKAPLOG_INFO_STR(logger, "Initialised ADE-style visibility stream converter, id="<<id);
   // by default, we have 4 data slices. This number can be reduced by rejecting some
   // datagrams before they are even put in the buffer
   const uint32_t maxNSlices = VisSource::getMaxSlice(params) + 1;
   if (maxNSlices < itsNSlices) {
       itsNSlices = maxNSlices;
   }
   ASKAPLOG_INFO_STR(logger, "Expecting "<<itsNSlices<<" slices of the correlator product space");
}

/// @brief helper (and probably temporary) method to remap channels
/// @details Maps [0..215] channel index into [0..215] channel number, per card.
/// We can expose this function via parset reusing index mapper (as for beams), but
/// for now just use hard-coded logic
/// @param[in] channelId input channel ID
/// @return physical channel number
uint32_t VisConverter<VisDatagramADE>::mapChannel(uint32_t channelId)
{
  ASKAPDEBUGASSERT(channelId < 216);
  const uint32_t fineOffset = channelId % 9;
  const uint32_t group = channelId / 9;
  ASKAPDEBUGASSERT(group < 24);
  const uint32_t chip = group / 4; 
  const uint32_t coarseChannel = group % 4;
  return fineOffset + chip * 9 + coarseChannel * 54;
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

   ASKAPCHECK(nChannels % 216 == 0, "Bandwidth should be multiple of 4-MHz");
    
   // by default, we have 4 data slices
   const casa::uInt datagramsExpected = itsNSlices * nBeamsToReceive() * nChannels;
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
   ASKAPCHECK((vis.channel > 0) && (vis.channel <= 216), "vis.channel = "<<vis.channel<<" is outside [1,216] range");
   ASKAPCHECK((vis.block > 0) && (vis.block <= 8), "vis.block = "<<vis.block<<" is outside [1,8] range");
   ASKAPCHECK((vis.card > 0) && (vis.card <= 12), "vis.card = "<<vis.card<<" is outside [1,12] range");
   ASKAPCHECK(vis.slice < 4, "Slice index is invalid");
   ASKAPCHECK((vis.beamid > 0) && (vis.beamid <= 36), "vis.beamid = "<<vis.beamid<<" is outside [1,36] range");

   
   // Detect duplicate datagrams
   const DatagramIdentity identity(vis.beamid, vis.block, vis.card, vis.channel, vis.slice);
   if (itsReceivedDatagrams.find(identity) != itsReceivedDatagrams.end()) {
       if (!itsNDuplicates) {
           ASKAPLOG_WARN_STR(logger, "Duplicate VisDatagram - Block: " << 
               vis.block << ", Card: " << vis.card << ", Channel: " << 
               vis.channel<<", Beam: "<<vis.beamid << ", Slice: "<<vis.slice);
           ASKAPLOG_WARN_STR(logger, "Futher messages about duplicated datagrams suspended till the end of the cycle");
       }
       ++itsNDuplicates;
       countDatagramAsIgnored();
       return;
   }
   itsReceivedDatagrams.insert(identity);

   // for now
   if (vis.channel > chunk->nChannel()) {
       ASKAPLOG_WARN_STR(logger, "Got channel outside bounds: "<<vis.channel);
       countDatagramAsIgnored();
       return;
   }
   // channel id to physical channel mapping is dependent on hardware configuration
   // it is not clear yet what modes we want to expose to end user via parset
   // for now have some mapping hard-coded
   //const casa::uInt channel = vis.channel;
   const casa::uInt channel = mapChannel(vis.channel - 1);

   ASKAPASSERT(channel < chunk->nChannel())
   //

   /*
   // this is a commissioning hack
   if ((channel == 108) && (vis.beamid == 1)) {
       ASKAPLOG_DEBUG_STR(logger, "Channel "<<channel<<" frequency in MHz: metadata/parset="<<std::setprecision(15)<<
             chunk->frequency()[channel]/1e6<<" datagram="<<std::setprecision(15)<<vis.freq);
   }
   //
   */
   ASKAPCHECK(fabs(chunk->frequency()[channel] / 1e6 - vis.freq) < 1e-5, "Detected frequency mismatch for channel="<<
            vis.channel<<" card="<<vis.card<<" block="<<vis.block<<" slice="<<vis.slice<<" beam="<<vis.beamid<<
            " hardware reports "<<std::setprecision(15)<<vis.freq<<" MHz, expected "<<std::setprecision(15)<<chunk->frequency()[channel]/1e6<<" MHz");

   bool atLeastOneUseful = false;
   for (uint32_t product = vis.baseline1, item = 0; product <= vis.baseline2; ++product, ++item) {

        // check that sensible data received from hardware
        ASKAPCHECK(product > 0, "Expect product (baseline) number to be positive");
        ASKAPCHECK(product <= 2628, "Expect product (baseline) number to be 2628 or less, you have "<<product);
        ASKAPCHECK(item < VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE, 
              "Product "<<product<<" between baseline1="<<vis.baseline1<<
              " and baseline2="<<vis.baseline2<<" exceeds buffer size of "<<
              VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE);

        if ((product < itsInvalidProducts.size()) && itsInvalidProducts[product]) {
            continue;
        }

        /*
        // this is a commissioning hack. To be removed in production system

        // can skip products here to avoid unnecessary warnings
        // this needs to be removed for the production system
        if (product > 300) { 
            if (!atLeastOneUseful) {
                ASKAPLOG_WARN_STR(logger, "Rejecting the whole datagram, slice="<<vis.slice<<
                          " block="<<vis.block << ", card=" << vis.card << ", channel=" << 
                          vis.channel<<", baseline1="<<vis.baseline1<<
                          " and baseline2="<<vis.baseline2<<", product=" <<product); 
            }
            break;
        }
        */


        // map correlator product to the row and polarisation index
        const boost::optional<std::pair<casa::uInt, casa::uInt> > mp = 
             mapCorrProduct(product, vis.beamid);

        if (!mp) {
            // kind of a hack to cache invalid products - beamid of 1 should always be present
            // mapping is static and is not expected to change from datagram to datagram, so can cache
            if ((vis.beamid == 1) && (product < itsInvalidProducts.size())) {
                itsInvalidProducts[product] = true;
            }
            continue;
        }

        const casa::uInt row = mp->first;
        const casa::uInt polidx = mp->second;

        ASKAPDEBUGASSERT(row < chunk->nRow());
        ASKAPDEBUGASSERT(polidx < chunk->nPol());

        atLeastOneUseful = true;
        casa::Complex sample(vis.vis[item].real, vis.vis[item].imag);

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
                chunk->visibility()(row, channel, 2) = conj(sample);
                // Unflag the sample
                if (rowIsValid) {
                    chunk->flag()(row, channel, 2) = false;
                }
            }
        }

        // temporary - debugging frequency mapping/values received from the ioc
        //chunk->visibility().yzPlane(row).row(channel).set(casa::Complex(vis.freq,0.));
        //chunk->visibility().yzPlane(row).row(channel).set(casa::Complex(vis.freq - chunk->frequency()[channel]/1e6,0.));
        //chunk->visibility().yzPlane(row).row(channel).set(casa::Complex(antenna1 + 10.*config().rank(),0.));
        //
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


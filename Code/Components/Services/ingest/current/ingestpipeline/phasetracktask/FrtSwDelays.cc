/// @file FrtSwDelays.cc
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

// Local package includes
#include "ingestpipeline/phasetracktask/FrtSwDelays.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"

// std includes
#include <string>
#include <map>

ASKAP_LOGGER(logger, ".FrtSwDelay");

namespace askap {
namespace cp {
namespace ingest {

// simplest fringe rotation method, essentially just a proof of concept

/// @brief Constructor.
/// @param[in] parset the configuration parameter set.
// @param[in] config configuration
FrtSwDelays::FrtSwDelays(const LOFAR::ParameterSet& parset, const Configuration& config)  
{
   ASKAPLOG_INFO_STR(logger, "Software-based fringe rotation");
   const std::vector<Antenna> antennas = config.antennas();
   const size_t nAnt = antennas.size();
   const casa::String refName = casa::downcase(parset.getString("refant"));
   itsRefAntIndex = nAnt;
   for (casa::uInt ant=0; ant<nAnt; ++ant) {
        if (casa::downcase(antennas.at(ant).name()) == refName) {
            itsRefAntIndex = ant;
            break;
        }
   }  
   ASKAPCHECK(itsRefAntIndex < nAnt, "Reference antenna "<<refName<<" is not found in the configuration");
   ASKAPLOG_INFO_STR(logger, "Will use "<<refName<<" (antenna index "<<itsRefAntIndex<<") as a reference antenna");
}

/// Process a VisChunk.
///
/// This method is called once for each correlator integration.
/// 
/// @param[in] chunk    a shared pointer to a VisChunk object. The
///             VisChunk contains all the visibilities and associated
///             metadata for a single correlator integration. This method
///             is expected to correct visibilities in this VisChunk 
///             as required (some methods may not need to do any correction at all)
/// @param[in] delays matrix with delays for all antennas (rows) and beams (columns) in seconds
/// @param[in] rates matrix with phase rates for all antennas (rows) and 
///                  beams (columns) in radians per second
/// @param[in] effLO effective LO frequency in Hz. Note, this is BETA-specific. It is probably
/// better to derive this from chunk if necessary.
void FrtSwDelays::process(const askap::cp::common::VisChunk::ShPtr& chunk, 
                           const casa::Matrix<double> &delays,
                           const casa::Matrix<double> &/*rates*/,
                           const double /*effLO*/)
{
  ASKAPDEBUGASSERT(delays.ncolumn() > 0);
  ASKAPDEBUGASSERT(itsRefAntIndex < delays.nrow());

  // this class doesn't talk to hardware at all, report ideal delays just for debugging
  for (casa::uInt ant = 0; ant < delays.nrow(); ++ant) {
       // negate the sign here because we want to compensate the delay
       const double diffDelay = (delays(itsRefAntIndex,0) - delays(ant,0));
       // ideal delay
       ASKAPLOG_INFO_STR(logger, "delays between "<<ant<<" and ref="<<itsRefAntIndex
               <<" are "<<diffDelay*1e9<<" ns");
  }

  //
  for (casa::uInt row = 0; row < chunk->nRow(); ++row) {
       // slice to get this row of data
       const casa::uInt ant1 = chunk->antenna1()[row];
       const casa::uInt ant2 = chunk->antenna2()[row];
       ASKAPDEBUGASSERT(ant1 < delays.nrow());
       ASKAPDEBUGASSERT(ant2 < delays.nrow());

       casa::Matrix<casa::Complex> thisRow = chunk->visibility().yzPlane(row);

       // attempt to correct for residual delays in software
       const casa::uInt beam1 = chunk->beam1()[row];
       const casa::uInt beam2 = chunk->beam2()[row];
       ASKAPDEBUGASSERT(beam1 < delays.ncolumn());
       ASKAPDEBUGASSERT(beam2 < delays.ncolumn());
       // actual delay, note the sign is flipped because we're correcting the delay here
       const double thisRowDelay = delays(ant1,beam1) - delays(ant2,beam2);
               
       const double phaseDueToAppliedDelay = 0.;
               //2. * casa::C::pi * effLO * appliedDelay;
       const casa::Vector<casa::Double>& freq = chunk->frequency();
       ASKAPDEBUGASSERT(freq.nelements() == thisRow.nrow());
       for (casa::uInt chan = 0; chan < thisRow.nrow(); ++chan) {
            const float phase = static_cast<float>(phaseDueToAppliedDelay + 
                           2. * casa::C::pi * freq[chan] * thisRowDelay);
            const casa::Complex phasor(cos(phase), sin(phase));

            // actual rotation (same for all polarisations)
            for (casa::uInt pol = 0; pol < thisRow.ncolumn(); ++pol) {
                 thisRow(chan,pol) *= phasor;
            }
       }
  }
}

} // namespace ingest 
} // namespace cp 
} // namespace askap 

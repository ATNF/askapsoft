/// @file FrtHWAde.cc
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
#include "ingestpipeline/phasetracktask/FrtHWAde.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include <casa/Arrays/Vector.h>

// std includes
#include <string>
#include <map>

ASKAP_LOGGER(logger, ".FrtHWAde");

namespace askap {
namespace cp {
namespace ingest {

/// @brief Constructor.
/// @param[in] parset the configuration parameter set.
// @param[in] config configuration
FrtHWAde::FrtHWAde(const LOFAR::ParameterSet& parset, const Configuration& config) : 
       itsFrtComm(parset, config), 
       itsDelayTolerance(static_cast<int>(parset.getUint32("delaystep",0u))), 
       itsFRPhaseRateTolerance(static_cast<int>(parset.getUint32("frratestep",20u))),
       itsTm(config.antennas().size(),0.),
       itsPhases(config.antennas().size(),0.),
       itsUpdateTimeOffset(static_cast<int32_t>(parset.getInt32("updatetimeoffset")))
{
   if (itsDelayTolerance == 0) {
       ASKAPLOG_INFO_STR(logger, "Delays will be updated every time the delay changes by 0.206 ns");
   } else {
       ASKAPLOG_INFO_STR(logger, "Delays will be updated when the required delay diverges more than "
               << itsDelayTolerance << " 0.206ns steps");
   } 
 
   if (itsFRPhaseRateTolerance == 0) {
       ASKAPLOG_INFO_STR(logger, "FR phase rate will be updated every time the rate changes by 0.0248 deg/s");
   } else {
       ASKAPLOG_INFO_STR(logger, "FR phase rate will be updated every time the setting diverges more than "
               << itsFRPhaseRateTolerance <<" 0.0248 deg/s steps");
   }

   if (itsUpdateTimeOffset == 0) {
      ASKAPLOG_INFO_STR(logger, "The reported BAT of the fringe rotator parameter update will be used as is without any adjustment");
   } else {
      ASKAPLOG_INFO_STR(logger, "The reported BAT of the fringe rotator parameter update will be shifted by "<<itsUpdateTimeOffset<<" microseconds");
   }

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
   ASKAPLOG_INFO_STR(logger, "Will use "<<refName<<" (antenna index "<<itsRefAntIndex<<") as the reference antenna");
   // can set up the reference antenna now to save time later
   itsFrtComm.setFRParameters(itsRefAntIndex, 0, 0, 0);
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
/// @param[in] effLO effective LO frequency in Hz
/// @note this interface stems from BETA, in particular effLO doesn't fit well with ADE
void FrtHWAde::process(const askap::cp::common::VisChunk::ShPtr& chunk, 
              const casa::Matrix<double> &delays, const casa::Matrix<double> &rates, const double effLO)
{
  ASKAPDEBUGASSERT(delays.ncolumn() > 0);
  ASKAPDEBUGASSERT(itsRefAntIndex < delays.nrow());
  ASKAPDEBUGASSERT(delays.ncolumn() == rates.ncolumn());
  ASKAPDEBUGASSERT(delays.nrow() == rates.nrow());
  // signal about new timestamp (there is no much point to mess around with threads
  // as actions are tide down to correlator cycles
  itsFrtComm.newTimeStamp(chunk->time());

  // HW phase rate units are 2^{-28} turns per FFB sample of 54 microseconds
  // note - units need to be checked, see discussions on ADESCOM-74
  const double phaseRateUnit = 2. * casa::C::pi / 268435456. / 54e-6;
  
  // HW delay unit corresponds to phase sloping up to pi / 2^{17} / fine channel
  // this gives about 0.206 ns per step
  const double delayUnit = 54. / 262144. / 1e6;

  const double integrationTime = chunk->interval();
  ASKAPASSERT(integrationTime > 0);

  for (casa::uInt ant = 0; ant < delays.nrow(); ++ant) {
       // negate the sign here because we want to compensate the delay
       const double diffDelay = (delays(itsRefAntIndex,0) - delays(ant,0))/delayUnit;
       // ideal delay
       ASKAPLOG_INFO_STR(logger, "delays between "<<ant<<" and ref="<<itsRefAntIndex<<" are "
               <<diffDelay*delayUnit*1e9<<" ns");

       casa::Int hwDelay = -static_cast<casa::Int>(diffDelay);

       // differential rate, negate the sign because we want to compensate here
       casa::Int diffRate = static_cast<casa::Int>((rates(itsRefAntIndex,0) - rates(ant,0))/phaseRateUnit);
       
       /*
       if (ant == 0) {
           const double interval = itsTm[ant]>0 ? (chunk->time().getTime("s").getValue() - itsTm[ant]) : 0;
           //diffRate = (int(interval/240) % 2 == 0 ? +1. : -1) * static_cast<casa::Int>(casa::C::pi / 100. / phaseRateUnit);
           const casa::Int rates[11] = {-10, -8, -6, -4, -2, 0, 2, 4, 6, 8,10}; 
           const double addRate = rates[int(interval/180) % 11];
           diffRate = addRate;
           if (int((interval - 5.)/180) % 11 != int(interval/180) % 11) {
               ASKAPLOG_DEBUG_STR(logger,"Invalidating ant="<<ant);
               itsFrtComm.invalidate(ant);
           }
           
           ASKAPLOG_DEBUG_STR(logger, "Interval = "<<interval<<" seconds, rate = "<<diffRate<<" for ant = "<<ant<<" addRate="<<addRate);
       }  else { diffRate = 0.;}
       if (itsTm[ant]<=0) {
           itsTm[ant] = chunk->time().getTime("s").getValue();
       }
       */
           
       if ((abs(diffRate - itsFrtComm.requestedFRPhaseRate(ant)) > itsFRPhaseRateTolerance) || (abs(hwDelay - itsFrtComm.requestedFRPhaseSlope(ant)) > itsDelayTolerance) || itsFrtComm.isUninitialised(ant)) {
           ASKAPLOG_INFO_STR(logger, "Set delays for antenna "<<ant<<" to "<<hwDelay * delayUnit *1e9<<" ns  and phase rate to "<<diffRate * phaseRateUnit * 180. / casa::C::pi<<" deg/s");

           ASKAPLOG_INFO_STR(logger, "Set phase rate for antenna "<<ant<<" to "<<diffRate);
           ASKAPLOG_DEBUG_STR(logger, "   in hw units: rate="<<diffRate<<" delay="<<hwDelay);
           itsFrtComm.setFRParameters(ant,diffRate,hwDelay,0);
           itsPhases[ant] = 0.;
       } 
       ASKAPDEBUGASSERT(ant < itsTm.size())
       /*
       if (itsTm[ant] > 0) {
           itsPhases[ant] += (chunk->time().getTime("s").getValue() - itsTm[ant]) * phaseRateUnit * itsFrtComm.requestedFRPhaseRate(ant);
       }
       */
       if (itsFrtComm.hadFRUpdate(ant)) {
           // 25000 microseconds is the offset before event trigger and the application of phase rates/accumulator reset (specified in the osl script)
           // on top of this we have a user defined fudge offset (see #5736)
           // to be tested whether this offset applies to ADE - don't do it for now
           const int32_t triggerOffset = /*25000 +*/ itsUpdateTimeOffset;
           const uint64_t lastReportedFRUpdateBAT = itsFrtComm.lastFRUpdateBAT(ant);

           ASKAPCHECK(static_cast<int64_t>(lastReportedFRUpdateBAT) > static_cast<int64_t>(triggerOffset), "The FR trigger offset "<<triggerOffset<<
                  " microseconds is supposed to be small compared to BAT="<<lastReportedFRUpdateBAT<<", ant="<<ant);

           const uint64_t lastFRUpdateBAT = lastReportedFRUpdateBAT + triggerOffset;
           const uint64_t currentBAT = epoch2bat(casa::MEpoch(chunk->time(),casa::MEpoch::UTC));

           if (currentBAT > lastFRUpdateBAT) {
               const uint64_t elapsedTime = currentBAT - lastFRUpdateBAT; 
               const double etInCycles = double(elapsedTime + itsUpdateTimeOffset) / integrationTime / 1e6;
           
               ASKAPLOG_DEBUG_STR(logger, "Antenna "<<ant<<": elapsed time since last FR update "<<double(elapsedTime)/1e6<<" s ("<<etInCycles<<" cycles)");
       
               itsPhases[ant] = double(elapsedTime) * 1e-6 * phaseRateUnit * itsFrtComm.requestedFRPhaseRate(ant);
           } else {
              ASKAPLOG_DEBUG_STR(logger, "Still processing old data before FR update event trigger for antenna "<<ant);
           }
       } // if FR had an update for a given antenna
  } // loop over antennas
  //
  for (casa::uInt row = 0; row < chunk->nRow(); ++row) {
       // slice to get this row of data
       const casa::uInt ant1 = chunk->antenna1()[row];
       const casa::uInt ant2 = chunk->antenna2()[row];
       ASKAPDEBUGASSERT(ant1 < delays.nrow());
       ASKAPDEBUGASSERT(ant2 < delays.nrow());

       if (itsFrtComm.isValid(ant1) && itsFrtComm.isValid(ant2)) {
           // desired delays are set and applied, do phase rotation
           casa::Matrix<casa::Complex> thisRow = chunk->visibility().yzPlane(row);
           const double appliedDelay = delayUnit * (itsFrtComm.requestedFRPhaseSlope(ant1)-itsFrtComm.requestedFRPhaseSlope(ant2));

           // attempt to correct for residual delays in software
           const casa::uInt beam1 = chunk->beam1()[row];
           const casa::uInt beam2 = chunk->beam2()[row];
           ASKAPDEBUGASSERT(beam1 < delays.ncolumn());
           ASKAPDEBUGASSERT(beam2 < delays.ncolumn());
           // actual delay, note the sign is flipped because we're correcting the delay here
           const double thisRowDelay = delays(ant1,beam1) - delays(ant2,beam2);
           const double residualDelay = thisRowDelay - appliedDelay;
           ASKAPLOG_DEBUG_STR(logger, "Residual delay for ant1="<<ant1<<" ant2="<<ant2<<
                  " is "<<residualDelay*1e9<<" ns thisRowDelay="<<thisRowDelay*1e9<<
                  " appliedDelay="<<appliedDelay*1e9);

           // actual rate
           //const double thisRowRate = rates(ant1,beam1) - rates(ant2,beam2);
              
           
           //const double phaseDueToAppliedDelay = 2. * casa::C::pi * effLO * appliedDelay;
           const double phaseDueToAppliedDelay = 0.;//2. * casa::C::pi * 24e6 * appliedDelay;
           const double phaseDueToAppliedRate = itsPhases[ant1] - itsPhases[ant2];
           const casa::Vector<casa::Double>& freq = chunk->frequency();
           ASKAPDEBUGASSERT(freq.nelements() == thisRow.nrow());
           for (casa::uInt chan = 0; chan < thisRow.nrow(); ++chan) {
                //casa::Vector<casa::Complex> thisChan = thisRow.row(chan);
                const float phase = static_cast<float>(phaseDueToAppliedDelay + phaseDueToAppliedRate +
                             2. * casa::C::pi * freq[chan] * residualDelay);
                const casa::Complex phasor(cos(phase), sin(phase));

                // actual rotation (same for all polarisations)
                for (casa::uInt pol = 0; pol < thisRow.ncolumn(); ++pol) {
                     thisRow(chan,pol) *= phasor;
                }
                //thisChan *= phasor;
           }
           
       } else {
         // the parameters for these antennas are being changed, flag the data
         casa::Matrix<casa::Bool> thisFlagRow = chunk->flag().yzPlane(row);
         thisFlagRow.set(casa::True); 
       }
  }
}

} // namespace ingest 
} // namespace cp 
} // namespace askap 

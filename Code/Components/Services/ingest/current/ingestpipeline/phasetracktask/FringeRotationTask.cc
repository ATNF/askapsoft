/// @file FringeRotationTask.cc
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

#include "ingestpipeline/phasetracktask/FringeRotationTask.h"
#include "ingestpipeline/phasetracktask/FrtDrxDelays.h"
#include "ingestpipeline/phasetracktask/FrtHWAndDrx.h"
#include "ingestpipeline/phasetracktask/FrtSwDelays.h"
#include "ingestpipeline/phasetracktask/FrtHWAde.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"

// boost include
#include <boost/shared_ptr.hpp>

// casa includes
#include <casacore/measures/Measures/MeasFrame.h>
#include <casacore/measures/Measures/MEpoch.h>
#include <casacore/measures/Measures/MCEpoch.h>
#include <casacore/measures/Measures/MDirection.h>
#include <casacore/measures/Measures/MCDirection.h>
#include <casacore/measures/Measures/MeasConvert.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/MatrixMath.h>
#include <casacore/measures/Measures/UVWMachine.h>

// name substitution should get the same name for all ranks, we need MPI for that
// it would be nicer to get all MPI-related stuff to a single (top-level?) file eventually
#include <mpi.h>


ASKAP_LOGGER(logger, ".FringeRotationTask");

using namespace casacore;
using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;


/// @brief Constructor.
/// @param[in] parset the configuration parameter set.
/// @param[in] config configuration
FringeRotationTask::FringeRotationTask(const LOFAR::ParameterSet& parset,
                               const Configuration& config)
    : CalcUVWTask(parset, config), itsConfig(config),
      itsParset(parset), itsToBeInitialised(true),
      itsCalcUVW(parset.getBool("calcuvw", true))
{
    //ASKAPLOG_DEBUG_STR(logger, "constructor of the generalised fringe rotation task");
    if (itsCalcUVW) {
        ASKAPLOG_INFO_STR(logger, "This task will also calculate UVW, replacing any pre-existing value");
    }
    ASKAPCHECK(!parset.isDefined("fixeddelays"), "Parset has old-style fixeddelay keyword defined - correct it in fcm before proceeding further");

    const std::vector<Antenna> antennas = config.antennas();
    const size_t nAnt = antennas.size();
    // comment the following code to use the old-style parset definition of fixed delays
    itsFixedDelays.resize(nAnt);
    for (size_t id = 0; id < nAnt; ++id) {
         itsFixedDelays[id] = antennas.at(id).delay().getValue("ns");
    }
    // end of the code treating the new parset definition of fixed delays

    if (config.rank() == 0) {
        ASKAPLOG_INFO_STR(logger, "The fringe rotation will apply fixed delays in addition to phase rotation");
        ASKAPLOG_INFO_STR(logger, "The following fixed delays are specified:");
        ASKAPCHECK(itsFixedDelays.size() == nAnt, "Fixed delays do not seem to be specified for all configured antennas");

        for (size_t id = 0; id < casacore::min(casacore::uInt(nAnt),casacore::uInt(itsFixedDelays.size())); ++id) {
             ASKAPLOG_INFO_STR(logger, "    antenna: " << antennas.at(id).name()<<" (id="<<id << ") delay: "
                    << itsFixedDelays[id] << " ns");
        }
    }
}

/// @brief should this task be executed for inactive ranks?
/// @details If a particular rank is inactive, process method is
/// not called unless this method returns true. This class has the
/// following behavior.
///   - Returns true initially to allow collective operations if
///     number of ranks is more than 1.
///   - After the first call to process method, inactive ranks are
///     identified and false is returned for them.
/// @return true, if process method should be called even if
/// this rank is inactive (i.e. uninitialised chunk pointer
/// will be passed to process method).
bool FringeRotationTask::isAlwaysActive() const
{
  // the first iteration should be done on all ranks, then only on ranks with data.
  return itsToBeInitialised;
}

/// @brief initialise fringe rotation approach
/// @details This method uses the factory method to initialise 
/// the fringe rotation approach on the rank which has data. It is
/// checked using MPI collectives, that only one rank has a valid data stream
/// @param[in] hasData true if this rank has data, false otherwise
void FringeRotationTask::initialise(bool hasData)
{
   ASKAPDEBUGASSERT(itsToBeInitialised);
   ASKAPDEBUGASSERT(itsConfig.rank() < itsConfig.nprocs());
   std::vector<int> activityFlags(itsConfig.nprocs(), 0);
   if (hasData) {
       activityFlags[itsConfig.rank()] = 1;
   }
   const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)activityFlags.data(),
        activityFlags.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
  
   // now activityFlags should be consistent across all ranks - figure out the role for
   // this particular rank
   const int numInputs = std::accumulate(activityFlags.begin(), activityFlags.end(), 0);

   // most methods talk to hardware, therefore only one data stream is
   // supported. Technically, we could exclude 'swdelays' because it
   // doesn't care. However, we're not using it on independent chunks
   // anyway and it is better to have the same guard applied to avoid
   // relying on untested case.
   ASKAPCHECK(numInputs == 1, "Exactly one input stream is expected by FringeRotationTask, use it after merge");
   if (hasData) {
       ASKAPLOG_INFO_STR(logger, "This rank ("<<itsConfig.rank()<<") will handle fringe rotation and residual correction");
       itsFrtMethod = fringeRotationMethod(itsParset, itsConfig);
   } 

   itsToBeInitialised = false;
}

/// @brief process one VisChunk 
/// @details Perform fringe tracking, correct residual effects on visibilities in the
/// specified VisChunk.
///
/// @param[in,out] chunk  the instance of VisChunk for which the
///                       phase factors will be applied.
void FringeRotationTask::process(askap::cp::common::VisChunk::ShPtr& chunk)
{
    if (itsToBeInitialised) {
        initialise(static_cast<bool>(chunk));
        if (!chunk) {
             return;
        }
    } else {
        ASKAPCHECK(itsFrtMethod && chunk, 
             "Parallel data streams are not supported; use fringe rotation task after Merge");
    }

    casacore::Matrix<double> delays(nAntennas(), nBeams(),0.);
    casacore::Matrix<double> rates(nAntennas(),nBeams(),0.);

    // geocentric U and V per antenna/beam; we need those only if
    // itsCalcUVW is true, so in principle we could've added more logic to 
    // avoid allocating unnecessary memory. But it looks like premature optimisation. 
    casacore::Matrix<double> antUs(nAntennas(), nBeams(), 0.);
    casacore::Matrix<double> antVs(nAntennas(), nBeams(), 0.);
    casacore::Matrix<double> antWs(nAntennas(), nBeams(), 0.);
    std::vector<boost::shared_ptr<casacore::UVWMachine> > uvwMachines(nBeams());

    // calculate delays (in seconds) and rates (in radians per seconds) for each antenna
    // and beam the values are absolute per antenna w.r.t the Earth centre

    // Determine Greenwich Apparent Sidereal Time
    //const double gast = calcGAST(chunk->time());
    const casacore::MEpoch epoch(chunk->time(), casacore::MEpoch::UTC);
    const double effLOFreq = getEffectiveLOFreq(*chunk);
    const double siderealRate = casacore::C::_2pi / 86400. / (1. - 1./365.25);

    for (casacore::uInt ant = 0; ant < nAntennas(); ++ant) {
         ASKAPASSERT(chunk->phaseCentre().nelements() > 0);
         const casacore::MDirection dishPnt = casacore::MDirection(chunk->phaseCentre()[0],chunk->directionFrame());
         // fixed delay in seconds
         const double fixedDelay = ant < itsFixedDelays.size() ? itsFixedDelays[ant]*1e-9 : 0.;

         // antenna coordinates in ITRF
         const casacore::Vector<double> xyz = antXYZ(ant);
         ASKAPDEBUGASSERT(xyz.nelements() == 3);
         const casacore::MPosition antPos(casacore::MVPosition(xyz), casacore::MPosition::ITRF);
         /*
         const casacore::MPosition antPos(casacore::MVPosition(casacore::Quantity(370.81, "m"),
                            casacore::Quantity(116.6310372795, "deg"),
                            casacore::Quantity(-26.6991531922, "deg")),
                            casacore::MPosition::Ref(casacore::MPosition::WGS84));
         */
         const casacore::MeasFrame frame(epoch, antPos);

         for (casacore::uInt beam = 0; beam < nBeams(); ++beam) {
              // Current APP phase center
              const casacore::MDirection fpc = casacore::MDirection::Convert(phaseCentre(dishPnt, beam),
                                    casacore::MDirection::Ref(casacore::MDirection::TOPO, frame))();
              const casacore::MDirection hadec = casacore::MDirection::Convert(phaseCentre(dishPnt, beam),
                                    casacore::MDirection::Ref(casacore::MDirection::HADEC, frame))();
              if (itsCalcUVW && (ant == 0)) {
                  // for optional uvw rotation
                  // HADEC frame doesn't seem to work correctly, even apart from inversion of the first coordinate
                  // For details see ADESCOM-342.
                  //uvwMachines[beam].reset(new casacore::UVWMachine(casacore::MDirection::Ref(casacore::MDirection::J2000), hadec, frame));
                  uvwMachines[beam].reset(new casacore::UVWMachine(casacore::MDirection::Ref(casacore::MDirection::J2000), fpc, frame));
              }
              //const double ra = fpc.getAngle().getValue()(0);
              const double dec = hadec.getValue().getLat(); //fpc.getAngle().getValue()(1);
              // hour angle at latitude zero
              const double H0 = hadec.getValue().getLong() - antPos.getValue().getLong();

              // Transformation from antenna position to the geocentric delay
              //const double H0 = gast - ra;
              const double sH0 = sin(H0);
              const double cH0 = cos(H0);
              const double cd = cos(dec);
              const double sd = sin(dec);

              // APP delay is a scalar, so transformation matrix is just a vector
              // we could probably use the matrix math to process all antennas at once, however
              // do it explicitly for now for simplicity
              const casacore::Vector<double> xyz = antXYZ(ant);
              ASKAPDEBUGASSERT(xyz.nelements() == 3);
              const double delayInMetres = -cd * cH0 * xyz(0) + cd * sH0 * xyz(1) - sd * xyz(2);
              delays(ant,beam) = fixedDelay + delayInMetres / casacore::C::c;
              rates(ant,beam) = (cd * sH0 * xyz(0) + cd * cH0 * xyz(1)) * siderealRate * casacore::C::_2pi / casacore::C::c * effLOFreq;

              // optional uvw calculation
              if (itsCalcUVW) {
                  antUs(ant, beam) = -sH0 * xyz(0) - cH0 * xyz(1);
                  antVs(ant, beam) = sd * cH0 * xyz(0) - sd * sH0 * xyz(1) - cd * xyz(2);
                  antWs(ant, beam) = delayInMetres;
              }
         }
    }

    if (itsCalcUVW) {
        casacore::Vector<double> uvwvec(3);
        for (casacore::uInt row = 0; row<chunk->nRow(); ++row) {
             const casacore::uInt ant1 = chunk->antenna1()(row);
             const casacore::uInt ant2 = chunk->antenna2()(row);
             const casacore::uInt beam = chunk->beam1()(row);
             ASKAPASSERT(ant1 < nAntennas());
             ASKAPASSERT(ant2 < nAntennas());
             ASKAPASSERT(beam < nBeams());

             uvwvec(0) = antUs(ant2, beam) - antUs(ant1, beam);
             uvwvec(1) = antVs(ant2, beam) - antVs(ant1, beam);
             uvwvec(2) = antWs(ant2, beam) - antWs(ant1, beam);

             ASKAPASSERT(beam < uvwMachines.size());
             ASKAPDEBUGASSERT(uvwMachines[beam]);
             uvwMachines[beam]->convertUVW(uvwvec);
             ASKAPDEBUGASSERT(uvwvec.nelements() == 3);
             chunk->uvw()(row) = uvwvec;
             /* 
                // code for cross-check with calcUVWTask
                // note, now it doesn't quite match as we using the frame related to each
                // particular antenna rather than antenna 0. This gives about 0.1mm error
             casacore::Vector<double> diff = uvwvec.copy();
             diff(0) -= chunk->uvw()(row)(0);
             diff(1) -= chunk->uvw()(row)(1);
             diff(2) -= chunk->uvw()(row)(2);
             ASKAPCHECK(sqrt(diff[0]*diff[0]+diff[1]*diff[1]+diff[2]*diff[2]) < 1e-6, 
                 "Mismatch in UVW for row="<<row<<": uvwvec="<<uvwvec<<" chunk: "<<chunk->uvw()(row));
             */
             
        }
    }

    itsFrtMethod->process(chunk, delays, rates, effLOFreq);
}


/// @brief factory method for the fringe rotation approach classes
/// @details This class is used to create implementations of 
/// IFrtApproach interface based on the parset. These classes do
/// actual work on application of delays and rates
/// @param[in] parset the configuration parameter set.
// @param[in] config configuration
IFrtApproach::ShPtr FringeRotationTask::fringeRotationMethod(const LOFAR::ParameterSet& parset, 
               const Configuration & config)
{
  const std::string name = parset.getString("method");
  ASKAPLOG_INFO_STR(logger, "Selected fringe rotation method: "<<name);

  IFrtApproach::ShPtr result;
  if (name == "drxdelays") {
      result.reset(new FrtDrxDelays(parset,config));
  } else if (name == "hwanddrx") {
      result.reset(new FrtHWAndDrx(parset,config));
  } else if (name == "swdelays") {
      result.reset(new FrtSwDelays(parset,config));
  } else if (name == "hwade") {
      result.reset(new FrtHWAde(parset,config));
  }
  ASKAPCHECK(result, "Method "<<name<<" is unknown");
  return result;
}

/// @brief helper method to obtain effective LO frequency
/// @details This was a BETA-specific method by design. The result is not
/// used for ADE-specific fringe rotation code. However, while we experiment with
/// fringe rotation for different frequency setups, it is handy to keep the associated
/// interface. It may be removed later. The code was moved into this class to be able
/// to retire PhaseTrackTask which becomes harder to maintain as the interfaces are 
/// changing for ADE.
/// ------------------------
/// The effective LO frequency is deduced from the sky frequency as
/// BETA has a simple conversion chain (the effective LO and the sky frequency of
/// the first channel always have a fixed offset which is hard coded).
/// It is handy to encapsulate the formula in one method as it is used by more
/// than one class.
/// @param[in] chunk the visibility chunk for this integration cycle
/// @return Effective LO frequency in Hz
double FringeRotationTask::getEffectiveLOFreq(const askap::cp::common::VisChunk& chunk)
{
   // the following code is BETA specific. Leave it here for now, it is unused for ADE
 
   // here we need the effective LO frequency, we can deduce it from the start frequency of the very first
   // channel (global, not local for this rank)
   // Below we hardcoded the formula derived from the BETA simple conversion chain (note, it may change
   // for ADE - need to check)
   //
   // BETA has 3 frequency conversions with effective LO being TunableLO - 4432 MHz - 768 MHz
   // (the last one is because digitisation acts like another LO). As a result, the spectrum is always inverted.
   // The start frequency corresponds to the top of the band and is a fixed offset from TunableLO which we need
   // to calculate the effective LO frequency. Assuming that the software correlator got the bottom of the band,
   // i.e. the last 16 of 304 channels, the effective LO is expected to be 40 MHz below the bottom of the band or
   // 344 MHz below the top of the band. This number needs to be checked when we get the actual system observing
   // an astronomical source. 
   //
   // Investigations in January 2014 revealed that the effective LO is 343.5 MHz below the top of the band which
   // is the centre of the first fine channel. The correct frequency mapping is realised if 0.5 MHz is added to
   // the centre the top coarse channel (the tunable LO corresponds to the centre of the coarse channel in the
   // middle of the band, we probably wrongfully assumed the adjacent channel initially therefore there is a 
   // correction of 1 MHz one way and 0.5 MHz the other). The tunable LO of 5872 MHz corresponds to 
   // the top fine channel frequency of 1015.5 MHz. The 343.5 MHz offset for the effective LO has been verified
   // with the 3h track on the Galactic centre and DRx delay update tolerance of 51 steps (the phase didn't jump
   // within the uncertainty of the measurement when DRx delay was updated). Note the accuracy of the measurement
   // is equivalent to a few fine channels, but there doesn't seem to be any reason why such small offset might be
   // present.
   return chunk.frequency()(0) - 343.5e6;
}




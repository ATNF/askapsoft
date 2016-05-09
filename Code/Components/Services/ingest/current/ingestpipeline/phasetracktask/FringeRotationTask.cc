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
#include "ingestpipeline/phasetracktask/PhaseTrackTask.h"
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

ASKAP_LOGGER(logger, ".FringeRotationTask");

using namespace casa;
using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;


/// @brief Constructor.
/// @param[in] parset the configuration parameter set.
/// @param[in] config configuration
FringeRotationTask::FringeRotationTask(const LOFAR::ParameterSet& parset,
                               const Configuration& config)
    : CalcUVWTask(parset, config), itsConfig(config),
      itsFixedDelays(parset.getDoubleVector("fixeddelays", std::vector<double>())),
      itsFrtMethod(fringeRotationMethod(parset,config)), itsCalcUVW(parset.getBool("calcuvw", true))
{
    ASKAPLOG_DEBUG_STR(logger, "constructor of the generalised fringe rotation task");
    ASKAPLOG_INFO_STR(logger, "This is a specialised version of fringe rotation tasks used for debugging; use data on your own risk");
    if (itsCalcUVW) {
        ASKAPLOG_INFO_STR(logger, "This task will also calculate UVW, replacing any pre-existing value");
    }

    if (itsFixedDelays.size() != 0) {
        ASKAPLOG_INFO_STR(logger, "The phase tracking task will apply fixed delays in addition to phase rotation");
        ASKAPLOG_INFO_STR(logger, "Fixed delays specified for " << itsFixedDelays.size() << " antennas:");

        const std::vector<Antenna> antennas = config.antennas();
        const size_t nAnt = antennas.size();
        for (size_t id = 0; id < casa::min(casa::uInt(nAnt),casa::uInt(itsFixedDelays.size())); ++id) {
             ASKAPLOG_INFO_STR(logger, "    antenna: " << antennas.at(id).name()<<" (id="<<id << ") delay: "
                     << itsFixedDelays[id] << " ns (temp: fcm has "<<antennas.at(id).delay().getValue("ns")<<")");
        }
        if (nAnt < itsFixedDelays.size()) {
            ASKAPLOG_INFO_STR(logger,  "    other fixed delays are ignored");
        }
    } else {
            ASKAPLOG_INFO_STR(logger, "No fixed delay specified");
    }
}

/// @brief process one VisChunk 
/// @details Perform fringe tracking, correct residual effects on visibilities in the
/// specified VisChunk.
///
/// @param[in,out] chunk  the instance of VisChunk for which the
///                       phase factors will be applied.
void FringeRotationTask::process(askap::cp::common::VisChunk::ShPtr& chunk)
{
    ASKAPCHECK(itsFrtMethod && chunk, 
           "Parallel data streams are not supported; use fringe rotation task after Merge");

    casa::Matrix<double> delays(nAntennas(), nBeams(),0.);
    casa::Matrix<double> rates(nAntennas(),nBeams(),0.);

    // geocentric U and V per antenna/beam; we need those only if
    // itsCalcUVW is true, so in principle we could've added more logic to 
    // avoid allocating unnecessary memory. But it looks like premature optimisation. 
    casa::Matrix<double> antUs(nAntennas(), nBeams(), 0.);
    casa::Matrix<double> antVs(nAntennas(), nBeams(), 0.);
    casa::Matrix<double> antWs(nAntennas(), nBeams(), 0.);
    std::vector<boost::shared_ptr<casa::UVWMachine> > uvwMachines(nBeams());

    // calculate delays (in seconds) and rates (in radians per seconds) for each antenna
    // and beam the values are absolute per antenna w.r.t the Earth centre

    // Determine Greenwich Apparent Sidereal Time
    const double gast = calcGAST(chunk->time());
    casa::MeasFrame frame(casa::MEpoch(chunk->time(), casa::MEpoch::UTC));
    const double effLOFreq = getEffectiveLOFreq(*chunk);
    const double siderealRate = casa::C::_2pi / 86400. / (1. - 1./365.25);

    for (casa::uInt ant = 0; ant < nAntennas(); ++ant) {
         ASKAPASSERT(chunk->phaseCentre().nelements() > 0);
         const casa::MDirection dishPnt = casa::MDirection(chunk->phaseCentre()[0],chunk->directionFrame());
         // fixed delay in seconds
         const double fixedDelay = ant < itsFixedDelays.size() ? itsFixedDelays[ant]*1e-9 : 0.;
         for (casa::uInt beam = 0; beam < nBeams(); ++beam) {
              // Current APP phase center
              const casa::MDirection fpc = casa::MDirection::Convert(phaseCentre(dishPnt, beam),
                                    casa::MDirection::Ref(casa::MDirection::TOPO, frame))();
              if (itsCalcUVW && (ant == 0)) {
                  // for optional uvw rotation
                  uvwMachines[beam].reset(new casa::UVWMachine(casa::MDirection::Ref(casa::MDirection::J2000), fpc));
              }
              const double ra = fpc.getAngle().getValue()(0);
              const double dec = fpc.getAngle().getValue()(1);

              // Transformation from antenna position to the geocentric delay
              const double H0 = gast - ra;
              const double sH0 = sin(H0);
              const double cH0 = cos(H0);
              const double cd = cos(dec);
              const double sd = sin(dec);

              // APP delay is a scalar, so transformation matrix is just a vector
              // we could probably use the matrix math to process all antennas at once, however
              // do it explicitly for now for simplicity
              const casa::Vector<double> xyz = antXYZ(ant);
              ASKAPDEBUGASSERT(xyz.nelements() == 3);
              const double delayInMetres = -cd * cH0 * xyz(0) + cd * sH0 * xyz(1) - sd * xyz(2);
              delays(ant,beam) = fixedDelay + delayInMetres / casa::C::c;
              rates(ant,beam) = (cd * sH0 * xyz(0) + cd * cH0 * xyz(1)) * siderealRate * casa::C::_2pi / casa::C::c * effLOFreq;

              // optional uvw calculation
              if (itsCalcUVW) {
                  antUs(ant, beam) = -sH0 * xyz(0) - cH0 * xyz(1);
                  antVs(ant, beam) = sd * cH0 * xyz(0) - sd * sH0 * xyz(1) - cd * xyz(2);
                  antWs(ant, beam) = delayInMetres;
              }
         }
    }

    if (itsCalcUVW) {
        casa::Vector<double> uvwvec(3);
        for (casa::uInt row = 0; row<chunk->nRow(); ++row) {
             const casa::uInt ant1 = chunk->antenna1()(row);
             const casa::uInt ant2 = chunk->antenna2()(row);
             const casa::uInt beam = chunk->beam1()(row);
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
             casa::Vector<double> diff = uvwvec.copy();
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

  // most methods talk to hardware, therefore only one data stream is
  // supported. Technically, we could exclude 'swdelays' because it
  // doesn't care. However, we're not using it on independent chunks
  // anyway and it is better to have the same guard applied to avoid
  // relying on untested case.
  if (config.nprocs() > 1 && config.rank() != 0) {
      ASKAPLOG_INFO_STR(logger, 
         "Parallel streams are not supported, this task is not supposed to be used for rank="<<
         config.rank());
      return IFrtApproach::ShPtr();
  }

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

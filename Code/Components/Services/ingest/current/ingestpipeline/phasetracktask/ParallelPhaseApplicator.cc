/// @file ParallelPhaseApplicator.cc
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
#include "ingestpipeline/phasetracktask/ParallelPhaseApplicator.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"

#include "boost/thread/thread.hpp"
#include "boost/tuple/tuple.hpp"

#include "askap/CircularBuffer.h"

ASKAP_LOGGER(logger, ".ParallelPhaseApplicator");

namespace askap {
namespace cp {
namespace ingest {


ParallelPhaseApplicator::ParallelPhaseApplicator(const casacore::Vector<double> &freq, casacore::Cube<casacore::Complex> &vis, size_t nThreads) : 
                           itsFreq(freq), itsCube(vis), itsBuffer(nThreads), itsInterrupted(false) {
   ASKAPASSERT(freq.nelements() == vis.ncolumn());
   for (size_t th = 0; th <nThreads; ++th) {
        itsThreadGroup.create_thread(boost::bind(&ParallelPhaseApplicator::run, this));
   }
}

ParallelPhaseApplicator::~ParallelPhaseApplicator() {
   itsInterrupted = true;
   itsThreadGroup.join_all();
}

void ParallelPhaseApplicator::run() {
   const int32_t ONE_SECOND=1000000;
   while (!itsInterrupted) {
          boost::shared_ptr<boost::tuple<casacore::uInt, double, double> > item = itsBuffer.next(ONE_SECOND);
          if (!item) {
              continue;
          }
          const casacore::uInt row = item->get<0>();
          const double phaseOffset = item->get<1>();
          const double residualDelay = item->get<2>();
          const casacore::IPosition &shape = itsCube.shape();
          casacore::Cube<casacore::Complex> tmpBuf;
          tmpBuf.takeStorage(shape, itsCube.data(), casacore::SHARE);
          casacore::Matrix<casacore::Complex> thisRow = tmpBuf.yzPlane(row);
          for (casacore::uInt chan = 0; chan < shape[1]; ++chan) {
               const float phase = static_cast<float>(phaseOffset +
                        2. * casacore::C::pi * itsFreq[chan] * residualDelay);
               const casacore::Complex phasor(cos(phase), sin(phase));

               // actual rotation (same for all polarisations)
               for (casacore::uInt pol = 0; pol < thisRow.ncolumn(); ++pol) {
                    thisRow(chan,pol) *= phasor;
               }
          }
   }
}
      
void ParallelPhaseApplicator::add(casacore::uInt row, double phaseOffset, double residualDelay) {
   ASKAPDEBUGASSERT(row < itsCube.nrow());
   boost::shared_ptr<boost::tuple<casacore::uInt, double, double> > item(new boost::tuple<casacore::uInt, double, double>(row,phaseOffset,residualDelay));
   itsBuffer.addWhenThereIsSpace(item);
};

void ParallelPhaseApplicator::complete() {
     const int32_t ONE_SECOND=1000000;
     if (itsBuffer.size() > 0 && !itsInterrupted) {
         itsBuffer.waitUntilEmpty(ONE_SECOND);
     }
}


} // namespace ingest 
} // namespace cp 
} // namespace askap 


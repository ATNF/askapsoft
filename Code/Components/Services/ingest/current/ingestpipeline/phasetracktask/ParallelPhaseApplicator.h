/// @file ParallelPhaseApplicator.h
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

// Include package level header file

#ifndef ASKAP_CP_INGEST_PARALLELPHASEAPPLICATOR_H
#define ASKAP_CP_INGEST_PARALLELPHASEAPPLICATOR_H


// ASKAPsoft includes
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"

// boost includes
#include "boost/thread/thread.hpp"
#include "boost/tuple/tuple.hpp"

// ASKAPsoft includes
#include "askap/CircularBuffer.h"


namespace askap {
namespace cp {
namespace ingest {

/// @brief helper class to apply phase gradient in parallel
/// @details Due to general non-thread-safety of casa containers, it is handy to dea; with shared-memory parallelism
/// explicitly rather than via openmp, etc. In addition, this code is considered temporary anyway given the direction
/// ingest is going. At this stage, I (MV) just tidied up the code a bit and extract it to this cass to be able to
/// commit ingest tree in the same state as it was used in the last few months. 
class ParallelPhaseApplicator {
public:
      /// @brief constructor
      /// @details It is intended that an object of this type should never be stored in a container or otherwise be 
      /// accessible outside a single method. Therefore it accesses frequencies and the chunk of data to work
      /// with via reference. The user should ensure that these references continue to be valid until this object
      /// goes out of scope or complete method is called.
      /// @param[in] freq reference to the frequency vector (number of elements is the number of channels
      ParallelPhaseApplicator(const casacore::Vector<double> &freq, casacore::Cube<casacore::Complex> &vis, size_t nThreads);

      /// @brief destructor - stops and joins parallel threads
      /// @note The user has to call complete method to ensure all work is finished. Destructor just terminates the 
      /// execution immediately.
      ~ParallelPhaseApplicator();

      /// @brief main execution method in the parallel threads
      void run();
      
      /// @brief add new job to the worklist
      /// @details This method adds a new work item = gradient to apply for the given row
      /// @param[in] row row number to work with
      /// @param[in] phaseOffset constant additive term for the phase gradient to be applied to this row
      /// @param[in] residualDelay the slope of the phase gradient to be applied to this row
      void add(casacore::uInt row, double phaseOffset, double residualDelay);

      /// @brief Wait until all submitted jobs are finished
      void complete();

private:
      /// @brief reference to the frequency vector
      const casacore::Vector<double> &itsFreq;
      /// @brief reference to the cube to work with
      casacore::Cube<casacore::Complex> &itsCube;
      /// @brief thread group
      boost::thread_group itsThreadGroup;
      /// @brief buffer to hold submitted work units
      utility::CircularBuffer<boost::tuple<casacore::uInt, double, double> > itsBuffer;
      /// @brief true of interruption is requested
      bool itsInterrupted;
};


} // namespace ingest 
} // namespace cp 
} // namespace askap 

#endif // ASKAP_CP_INGEST_PARALLELPHASEAPPLICATOR_H

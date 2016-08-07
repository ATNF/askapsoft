/// @file BeamScatterTask.h
///
/// Scatter beams between parallel ranks. Note, this task is written for
/// experiments. This is not how the ingest pipeline was designed to operate.
/// Most likely, this approach will not scale to full ASKAP, but may be handy
/// for early science.
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
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>

// Include own header file first
#include "BeamScatterTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"
#include "cpcommon/VisChunk.h"

// boost includes
#include "boost/shared_array.hpp"

// Local package includes
#include "configuration/Configuration.h"
#include "monitoring/MonitoringSingleton.h"

ASKAP_LOGGER(logger, ".BeamScatterTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

BeamScatterTask::BeamScatterTask(const LOFAR::ParameterSet& parset,
        const Configuration& config) :
    itsNStreams(static_cast<int>(parset.getUint32("nstreams", config.nprocs()))),
    itsCommunicator(MPI_COMM_WORLD)
{
    ASKAPLOG_DEBUG_STR(logger, "Constructor");
    ASKAPCHECK(config.nprocs() > 1,
            "This task is intended to be used in parallel mode only");
    ASKAPCHECK(itsNStreams > 1, "Beam scatter task doesn't make sense for a single output data stream");
    ASKAPLOG_INFO_STR(logger, "Will split beam space into "<<itsNStreams<<" data streams");
}

BeamScatterTask::~BeamScatterTask()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
    if (itsCommunicator != MPI_COMM_WORLD) {
        const int response = MPI_Comm_free(&itsCommunicator);
        ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_free = "<<response);
    }
}

void BeamScatterTask::process(VisChunk::ShPtr& chunk)
{
   if (chunk) {
       ASKAPLOG_DEBUG_STR(logger, "This rank is active");
   } else {
       ASKAPLOG_DEBUG_STR(logger, "This rank is inactive");
   }
}

/// @brief should this task be executed for inactive ranks?
/// @details If a particular rank is inactive, process method is
/// not called unless this method returns true. Possible use cases:
///   - Splitting the datastream expanding parallelism, i.e
///     inactive rank(s) become active after this task.
///   - Need for collective operations 
/// @return true, if process method should be called even if
/// this rank is inactive (i.e. uninitialised chunk pointer
/// will be passed to process method).
/// @note Currently, return true before the first call and then as needed
/// given the state of the input streams (i.e. it assumes that activity/inactivity
/// state doesn't change throughout the observation).
bool BeamScatterTask::isAlwaysActive() const
{
   return true;
}


/// @brief local rank in the group
/// @details Returns the rank against the local communicator, i.e.
/// the process number in the group of processes contributing to the
/// single output stream.
/// @return rank against itsCommunicator
int BeamScatterTask::localRank() const
{
   int rank;
   const int response = MPI_Comm_rank(itsCommunicator, &rank);
   
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_rank = "<<response);
   return rank;
}




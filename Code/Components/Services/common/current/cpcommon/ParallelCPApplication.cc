/// @file ParallelCPApplication.cc
/// @brief generic class for application expected to use MPI directly
/// @details For "light-weight" parallelism we want to do some MPI-specific
/// operations, logging configuration, etc in a consistent way but on the other
/// hand without pulling the whole framework into dependency list. This class
/// encapsulates some common code and is used for various central processor
/// applications instead of the basic askap::Application class.
/// @note We may eventually converge to use the standard framework everywhere (when
/// it is sufficiently developed). For now, this class is the way to avoid code
/// duplication.
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
/// @note Code based on the original cpingest.cc by Ben Humphreys

// Must be included first
#include "askap_cpcommon.h"

// System includes
#include <stdexcept>
#include <unistd.h>
#include <mpi.h>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"

// Local package includes
#include "cpcommon/ParallelCPApplication.h"

// Using
using namespace askap;
using namespace askap::cp::common;

ASKAP_LOGGER(logger, ".parallelcpapplication");

/// @brief access to rank of this process
/// @details Unlike direct call to the appropriate MPI routine,
/// this method returns 0 if the application is called in the
/// stand-alone mode (even if it is in MPI environment)
/// @return rank of the process
int ParallelCPApplication::rank() const
{
   return itsRank;
}

/// @brief stand-alone mode?
/// @return true in stand-alone mode, false otherwise
bool ParallelCPApplication::isStandAlone() const
{
   return itsStandAlone;
}

/// @brief access to the number of processes
/// @details Unlike direct call to the appropriate MPI routine,
/// this method returns 1, if the application is called in the
/// stand-alone mode (even if it is in MPI environment)
/// @return number of available processes
int ParallelCPApplication::numProcs() const
{
   return itsNumProcs;
}

/// @brief obtain node name for logging
/// @return node name
std::string ParallelCPApplication::nodeName()
{
    char name[MPI_MAX_PROCESSOR_NAME];
    int resultlen;
    MPI_Get_processor_name(name, &resultlen);
    std::string nodename(name);
    std::string::size_type idx = nodename.find_first_of('.');
    if (idx != std::string::npos) {
        // Extract just the hostname part
        nodename = nodename.substr(0, idx);
    }
    return nodename;
}

/// @brief obtain raw mpi rank
/// @return rank as returned by the appropriate MPI call
int ParallelCPApplication::mpiRank()
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
}

/// @brief obtain raw number of mpi processes/tasks
/// @return number of ranks available returned by the appropriate MPI call
int ParallelCPApplication::mpiNumProcs()
{
    int ntasks;
    MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
    return ntasks;
}

/// @brief constructor
ParallelCPApplication::ParallelCPApplication() : 
   itsRank(-1), itsNumProcs(-1), itsStandAlone(false) {}

int ParallelCPApplication::run(int argc, char* argv[])
{
   itsStandAlone = parameterExists("standalone");
   if (!itsStandAlone) {
       MPI_Init(&argc, &argv);
   }

   int error = 0;
   try {
       if (!itsStandAlone) {
           // MPI mode
           ASKAPLOG_REMOVECONTEXT("mpirank");
           ASKAPLOG_PUTCONTEXT("mpirank", utility::toString(mpiRank()));
           ASKAPLOG_REMOVECONTEXT("hostname");
           ASKAPLOG_PUTCONTEXT("hostname", nodeName());
           itsRank = mpiRank();
           itsNumProcs = mpiNumProcs();
       } else {
           // Standalone/Single-process mode
           itsRank = 0;
           itsNumProcs = 1;
           ASKAPLOG_REMOVECONTEXT("mpirank");
           ASKAPLOG_PUTCONTEXT("mpirank", utility::toString(-1));
       }

       ASKAPCHECK(rank() >= 0, "Problems with initialisation: rank seems to be negative");
       ASKAPCHECK(numProcs() >= 0, "Problems with initialisation: number of processes seems to be negative");

       // call pure virtual method to do the actual work
       run();

   } catch (const askap::AskapError& e) {
       ASKAPLOG_ERROR_STR(logger, "Askap error in " << argv[0] << ": " << e.what());
       std::cerr << "Askap error in " << argv[0] << ": " << e.what() << std::endl;
       error = 1;
   } catch (const std::exception& e) {
       ASKAPLOG_ERROR_STR(logger, "Unexpected exception in " << argv[0] << ": " << e.what());
       std::cerr << "Unexpected exception in " << argv[0] << ": " << e.what()
            << std::endl;
       error = 1;
   }

   if (!itsStandAlone) {
       if (error) {
           MPI_Abort(MPI_COMM_WORLD, error);
       } else {
           MPI_Finalize();
       }
   }

   return error;
}


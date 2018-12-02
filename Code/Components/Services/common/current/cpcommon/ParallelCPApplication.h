/// @file ParallelCPApplication.h
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

#ifndef ASKAP_CP_PARALLEL_CP_APPLICATION_H
#define ASKAP_CP_PARALLEL_CP_APPLICATION_H

#include <string>

// ASKAPsoft includes
#include "askap/Application.h"

namespace askap {
namespace cp {
namespace common {

/// @brief generic class for application expected to use MPI directly
/// @details For "light-weight" parallelism we want to do some MPI-specific
/// operations, logging configuration, etc in a consistent way but on the other
/// hand without pulling the whole framework into dependency list. This class
/// encapsulates some common code and is used for various central processor
/// applications instead of the basic askap::Application class.
/// @note We may eventually converge to use the standard framework everywhere (when
/// it is sufficiently developed). For now, this class is the way to avoid code
/// duplication
class ParallelCPApplication : public askap::Application
{
public:
   /// @brief constructor
   ParallelCPApplication();

   /// @brief entry point from the base class
   /// @details This is the override of the pure virtual method
   /// of the base class which is called when the application is 
   /// started. Command line arguments (e.g. required for proper
   /// MPI initialisation are passed as arguments)
   /// @param[in] argc number of command line arguments
   /// @param[in] argv vector of command line arguments
   /// @return error code (or zero for a successful run)
   virtual int run(int argc, char* argv[]);

   /// @brief method to override in derived classes
   /// @details Command line parameters can be accessed via
   /// methods of askap::Application class. Throw an exception
   /// in the case of unsuccessful execution.
   virtual void run() = 0;
   
   /// @brief access to rank of this process
   /// @details Unlike direct call to the appropriate MPI routine,
   /// this method returns 0 if the application is called in the
   /// stand-alone mode (even if it is in MPI environment)
   /// @return rank of the process
   int rank() const;

   /// @brief stand-alone mode?
   /// @return true in stand-alone mode, false otherwise
   bool isStandAlone() const;

   /// @brief access to the number of processes
   /// @details Unlike direct call to the appropriate MPI routine,
   /// this method returns 1, if the application is called in the
   /// stand-alone mode (even if it is in MPI environment)
   /// @return number of available processes
   int numProcs() const;

protected:

   /// @brief obtain node name for logging
   /// @return node name
   static std::string nodeName();

   /// @brief obtain raw mpi rank
   /// @return rank as returned by the appropriate MPI call
   static int mpiRank();

   /// @brief obtain raw number of mpi processes/tasks
   /// @return number of ranks available returned by the appropriate MPI call
   static int mpiNumProcs();

private:
   /// @brief rank of the given process
   /// @details Zero in the stand-alone mode
   int itsRank;

   /// @brief number of processes
   /// @details One in the stand-alone mode
   int itsNumProcs;

   /// @brief true in the stand-alone mode
   bool itsStandAlone;
};

} // namespace common
} // namespace cp
} // namespace askap

#endif // #ifndef ASKAP_CP_PARALLEL_CP_APPLICATION_H


/// @file ParallelMetadataSource.cc
/// @brief An adapter to use MetadataSource in parallel environment
/// @details When ingest pipeline is running under TOS we want to align all streams to the
/// same metadata. This adapter wraps around a MetadataSource object instantiated on one
/// of the ranks and distributes the metadata to all other ranks by broadcast.
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
#include "ingestpipeline/sourcetask/ParallelMetadataSource.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"

// boost includes
#include "boost/shared_array.hpp"

// I am not very happy to have MPI includes here, we may abstract this interaction
// eventually. This task is specific for the parallel case, so there is no reason to
// hide MPI. Leave it here for now.
#include <mpi.h>

ASKAP_LOGGER(logger, ".ParallelMetadataSource");

using namespace askap;
using namespace askap::cp::ingest;

/// @brief constructor
/// @details The adapter is constructed in slave or master mode depending
/// on the passed shared pointer with the actual metadata source object.
/// If non-zero pointer is passed, this rank is considered to be master rank.
/// Otherwise, it is a slave rank (i.e. null shared pointer) which will receive
/// a copy of the metadata. This class uses MPI collective calls, therefore all
/// ranks should call the constructor and 'next' method.
/// @param[in] msrc metadata source doing the actual work
ParallelMetadataSource::ParallelMetadataSource(const boost::shared_ptr<IMetadataSource> &msrc) :
          itsMetadataSource(msrc), itsMasterRank(-1)
{
    // 1) get the number of available ranks

    // we could've got the number of ranks from configuration, but it seems easier
    // not to overburden the interface with an extra parameter and obtain number of
    // ranks locally
    int nRanks = 0;
    int response = MPI_Comm_size(MPI_COMM_WORLD, &nRanks);
    ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_size = "<<response);
    ASKAPCHECK(nRanks > 1, "ParallelMetadataSource is supposed to be used only in parallel mode");

    // 2) aggregate initialisation state of the wrapped metadata source for each rank
    //    we do it as a cross-check and to find out the master rank on all slave ranks
    //    (this information could've been given as a parameter, but this way the interface is simpler)
    ASKAPDEBUGASSERT(sizeof(bool) == sizeof(char));
    bool sendBuf = msrc; // true, if this is the master rank
    boost::shared_array<bool> receiveBuf(new bool[nRanks]);

    response = MPI_Allgather((char*)(&sendBuf), 1, MPI_CHAR, (char*)(receiveBuf.get()), 1, MPI_CHAR, MPI_COMM_WORLD);
    ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allgather = "<<response);

    // 3) check that only one rank is the master
    ASKAPDEBUGASSERT(itsMasterRank < 0);
    for (int rank = 0; rank < nRanks; ++rank) {
         if (receiveBuf[rank]) {
             ASKAPCHECK(itsMasterRank < 0, "Two ranks "<<rank<<" and "<<itsMasterRank<<
                        " were defined as master ranks simultaneously");
             itsMasterRank = rank;
         }
    }
    if (sendBuf) {
        ASKAPLOG_INFO_STR(logger, "This rank ("<<itsMasterRank<<") will obtain metadata and broadcast to "<<nRanks - 1<<" slave ranks");
    } else {
        ASKAPLOG_DEBUG_STR(logger, "This is a slave rank, will receive metadata from master rank="<<itsMasterRank);
    }
}

/// @brief Returns the next TosMetadata object.
/// This call can be blocking, it will not return until an object is
/// available to return.
///
/// @param[in] timeout how long to wait for data before returning
///         a null pointer, in the case where the
///         buffer is empty. The timeout is in microseconds,
///         and anything less than zero will result in no
///         timeout (i.e. blocking functionality).
///
/// @return a shared pointer to a TosMetadata object.
boost::shared_ptr<askap::cp::TosMetadata> ParallelMetadataSource::next(const long timeout)
{
  boost::shared_ptr<askap::cp::TosMetadata> result;
  if (itsMetadataSource) {
      // this is the master rank - obtain metadata
      result = itsMetadataSource->next(timeout);
      // broadcast the result
  } else {
      // this is the slave rank - receive metadata
  }
  return result;
}



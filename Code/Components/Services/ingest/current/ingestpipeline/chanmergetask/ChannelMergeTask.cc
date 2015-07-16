/// @file ChannelMergeTask.cc
///
/// Merge of channel space handled by adjacent ranks
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

// Include own header file first
#include "ChannelMergeTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casa/aips.h"
#include "casa/Arrays/Vector.h"
#include "casa/Arrays/Cube.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "configuration/Configuration.h"

ASKAP_LOGGER(logger, ".ChannelMergeTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

ChannelMergeTask::ChannelMergeTask(const LOFAR::ParameterSet& parset,
        const Configuration& config) :
    itsRanksToMerge(static_cast<int>(parset.getUint32("ranks2merge")))
{
    ASKAPLOG_DEBUG_STR(logger, "Constructor");
    ASKAPCHECK(config.nprocs() > 1,
            "This task is intended to be used in parallel mode only");
    ASKAPLOG_INFO_STR(logger, "Will aggregate data handled by "<<itsRanksToMerge<<
            " consecutive ranks");
    ASKAPCHECK(itsRanksToMerge > 1, "Number of aggregated data chunks should be more than 1!");
    ASKAPCHECK(config.nprocs() % itsRanksToMerge == 0, "Total number of MPI ranks ("<<config.nprocs()<<
               ") should be an integral multiple of selected number of ranks to merge ("<<
               itsRanksToMerge<<")");
    //const int response = MPI_Bcast((void*)&tbuf, sizeof(tbuf), MPI_INTEGER, 0, MPI_COMM_WORLD);
    const int response = MPI_Comm_split(MPI_COMM_WORLD, config.rank() / itsRanksToMerge, 
               config.rank(), &itsCommunicator);
    ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Comm_split = "<<response);
}

ChannelMergeTask::~ChannelMergeTask()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
}

void ChannelMergeTask::process(VisChunk::ShPtr chunk)
{
    ASKAPCHECK(chunk, "ChannelMergeTask currently does not support idle input streams (inactive ranks)");
    //const casa::uInt nChanOriginal = chunk->nChannel();

    /*
    if (itsStart + itsNChan > nChanOriginal) {
        ASKAPLOG_WARN_STR(logger, "Channel selection task got chunk with " << nChanOriginal
                 << " channels, unable to select " << itsNChan
                 << " channels starting from " << itsStart);
        chunk->flag().set(true);
        return;
    }

    // extract required frequencies - don't take const reference to be able to
    // take slice (although we don't change the chunk yet)
    casa::Vector<casa::Double>& origFreq = chunk->frequency();
    casa::Vector<casa::Double> newFreq = origFreq(casa::Slice(itsStart,itsNChan));
    ASKAPDEBUGASSERT(newFreq.nelements() == itsNChan);

    // Extract slices from vis and flag cubes
    const casa::uInt nRow = chunk->nRow();
    const casa::uInt nPol = chunk->nPol();
    casa::Cube<casa::Complex>& origVis = chunk->visibility();
    casa::Cube<casa::Bool>& origFlag = chunk->flag();
    const casa::IPosition start(3, 0, itsStart, 0);
    const casa::IPosition length(3, nRow, itsNChan, nPol);
    const casa::Slicer slicer(start,length);

    casa::Cube<casa::Complex> newVis = origVis(slicer);
    casa::Cube<casa::Bool> newFlag = origFlag(slicer);
    ASKAPDEBUGASSERT(newVis.shape() == length);
    ASKAPDEBUGASSERT(newFlag.shape() == length);

    chunk->resize(newVis, newFlag, newFreq);
    */
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
/// @note Currently, always return true, but throw an exception if
/// any input data stream is inactive (full implementation would involve
/// setting up MPI communicators dynamcally, rather than in the constructor)
bool ChannelMergeTask::isAlwaysActive() const
{
   return true;
}



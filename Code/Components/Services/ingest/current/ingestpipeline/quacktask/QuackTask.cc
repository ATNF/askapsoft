/// @file QuackTask.cc
///
/// @copyright (c) 2014 CSIRO
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

// ASKAPsoft includes

// Include package level header file
#include "askap_cpingest.h"

// Local package includes
#include "ingestpipeline/quacktask/QuackTask.h"
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"


ASKAP_LOGGER(logger, ".QuackTask");

using namespace casacore;
using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;


/// @brief Constructor
/// @param[in] parset the configuration parameter set.
/// @param[in] config configuration
QuackTask::QuackTask(const LOFAR::ParameterSet& parset, const Configuration& config) :
       itsNCycles(parset.getUint32("ncycles",2u)), itsNCyclesThisScan(0u),
       itsCountedScanNumber(0u), itsFirstChunk(true), 
       // note, the following implies that this task is executed for receivers. It will work
       // on non-receiving ranks, but will not publish anything with WARNING severity
       itsVerboseRank(config.receiverId() == 0)
{
   if (itsNCycles == 0) {
       ASKAPLOG_DEBUG_STR(logger, "QuackTask is executed, but setup not to drop any cycles - essentially no operation");
   } else {
       if (itsVerboseRank) {
           ASKAPLOG_WARN_STR(logger, "Will flag "<<itsNCycles<<" cycle(s) following scan number change");
       } else {
           ASKAPLOG_DEBUG_STR(logger, "Will flag "<<itsNCycles<<" cycle(s) following scan number change");
       }
   }
}

/// @brief Flag visibilities in the specified VisChunk.
///
/// @param[in,out] chunk  the instance of VisChunk for which the
///                       flags will be applied, if necessary
void QuackTask::process(askap::cp::common::VisChunk::ShPtr& chunk)
{
   ASKAPDEBUGASSERT(chunk);
   if (itsFirstChunk || (chunk->scan() != itsCountedScanNumber)) {
       itsFirstChunk = false;
       itsCountedScanNumber = chunk->scan();
       itsNCyclesThisScan = 0u;
       ASKAPLOG_DEBUG_STR(logger, "Scan change detected, new scan id: "<<itsCountedScanNumber);
   } else {
       ++itsNCyclesThisScan;
   }
   if (itsNCyclesThisScan < itsNCycles) {
       if (itsVerboseRank) {
          ASKAPLOG_WARN_STR(logger, "Cycle "<<itsNCyclesThisScan + 1<<" of scan "<<itsCountedScanNumber<<" - flagging all the data");
       } else {
          ASKAPLOG_DEBUG_STR(logger, "Cycle "<<itsNCyclesThisScan + 1<<" of scan "<<itsCountedScanNumber<<" - flagging all the data");
       }
       chunk->flag().set(casacore::True);
   } else {
       if (itsNCycles != 0 && (itsNCyclesThisScan == itsNCycles)) {
           if (itsVerboseRank) {
               ASKAPLOG_WARN_STR(logger, "Unflagging data: scan "<<itsCountedScanNumber<<" got more than "<<itsNCycles<<" cycles");
           } else {
               ASKAPLOG_DEBUG_STR(logger, "Unflagging data: scan "<<itsCountedScanNumber<<" got more than "<<itsNCycles<<" cycles");
           }
       }
       // don't need to do anything here, unflagging means "not flagging" on top of existing flags for this task
   }
}



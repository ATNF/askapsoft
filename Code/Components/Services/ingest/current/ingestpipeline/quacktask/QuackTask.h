/// @file QuackTask.h
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

#ifndef ASKAP_CP_INGEST_QUACKTASK_H
#define ASKAP_CP_INGEST_QUACKTASK_H

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

namespace askap {
namespace cp {
namespace ingest {

/// @brief Helper flagging task to exclude setup cycles
///
/// This class is a helper task to flag a given number of cycles following
/// any scan change. It is named 'quack task' after AIPS task created for the
/// similar purpose. Hopefully, it is only needed temporary to assist commissioning and
/// will not be used in real operations.
///
/// This class implements the ITask interface which specified the process()
/// method. These "tasks" are treated polymorphically by the ingest pipeline.
/// Once data is sourced into the pipeline, the process() method is called
/// for each task (in a specific sequence), the VisChunk is read and/or modified
/// by each task.
///
/// The parameter set can contain the parameter controlling how many cycles are flagged
/// following each scan change (two is the default). We could've also detected the change 
/// in phase centre, but there are conditions where it would still generate corrupted data.
/// Therefore, it is better to keep this task simple.
///
/// @verbatim
/// ncycles = 2
/// @endverbatim
///
class QuackTask : public askap::cp::ingest::ITask {
    public:

        /// @brief Constructor
        /// @param[in] parset the configuration parameter set.
        /// @param[in] config configuration
        QuackTask(const LOFAR::ParameterSet& parset,
                        const Configuration& config);

        /// @brief Flag visibilities in the specified VisChunk.
        ///
        /// @param[in,out] chunk  the instance of VisChunk for which the
        ///                       flags will be applied, if necessary
        virtual void process(askap::cp::common::VisChunk::ShPtr& chunk);

    private:

        /// @brief number of cycles to flag
        casacore::uInt itsNCycles;

        /// @brief number of cycles since the last scan start
        casacore::uInt itsNCyclesThisScan;

        /// @brief scan id for which the cycles are being counted
        casacore::uInt itsCountedScanNumber;
  
        /// @brief true if it is the first chunk
        bool itsFirstChunk;

        /// @brief true if this rank should publish some messages with WARNING priority 
        /// (these will make it to the observing log)
        bool itsVerboseRank;
};

} // ingest
} // cp
} // askap

#endif


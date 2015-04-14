/// @file 
/// @brief Composite processing step
/// @details This is a composite constructed with a number of processing steps
/// executed in parallel. Individual jobs are represented by objects implementing
/// IProcessingStep interface..
///
/// @copyright (c) 2007 CSIRO
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
///

// own includes
#include "processingsteps/CompositeStep.h"

// for logging
#include "askap_parallelframework.h"
#include <askap/AskapLogging.h>
ASKAP_LOGGER(logger, ".parallelframework");

namespace askap {

namespace askapparallel {

/// @brief an empty constructor to create unnamed composite object
/// @details  Upon creation, no parallel jobs are associated with this composite. 
/// So if used before any add methods are called, it would effectively be an empty operation.
CompositeStep::CompositeStep() : ProcessingStep("composite") {}

/// @brief construct a composite object and assign a name
/// @details Upon creation, no parallel jobs are associated with this composite. 
/// So if used before any add methods are called, it would effectively be an empty operation.
/// @param[in] name name to assign
CompositeStep::CompositeStep(const std::string &name) : ProcessingStep(name) {}


// StepInfo helper class

/// @brief default constructor - empty shared pointer and shape
CompositeStep::StepInfo::StepInfo() {}
      
/// @brief constructor setting all details
/// @details
/// @param[in] step shared pointer to the processing step object
/// @param[in] id step ID (determines the rank assignment details)
/// @param[in] shape shape of the iteration domain 
CompositeStep::StepInfo::StepInfo(const boost::shared_ptr<IProcessingStep> &step, const StepID &id, const casa::IPosition & shape) :
    itsStepID(id), itsShape(shape), itsCode(step)  {}


} // namespace askapparallel

} // namespace askap

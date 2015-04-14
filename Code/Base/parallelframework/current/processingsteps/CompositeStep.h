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

#ifndef ASKAP_ASKAPPARALLEL_COMPOSITE_PROCESSING_STEP_H
#define ASKAP_ASKAPPARALLEL_COMPOSITE_PROCESSING_STEP_H

// own includes
#include "processingsteps/IProcessingStep.h"
#include "processingsteps/ProcessingStep.h"
#include "processingsteps/StepID.h"

// casa includes
#include <casa/Arrays/IPosition.h>

// std includes
#include <string>
#include <vector>

namespace askap {

namespace askapparallel {

/// @brief Composite processing step
/// @details This is a composite constructed with a number of processing steps
/// executed in parallel. Individual jobs are represented by objects implementing
/// IProcessingStep interface..
class CompositeStep : public ProcessingStep,
                      virtual public IProcessingStep
{
public:
	
   /// @brief an empty constructor to create unnamed composite object
   /// @details  Upon creation, no parallel jobs are associated with this composite. 
   /// So if used before any add methods are called, it would effectively be an empty operation.
   CompositeStep();

   /// @brief construct a composite object and assign a name
   /// @details Upon creation, no parallel jobs are associated with this composite. 
   /// So if used before any add methods are called, it would effectively be an empty operation.
   /// @param[in] name name to assign
   explicit CompositeStep(const std::string &name);
private:
   /// @brief helper type represending info for each child step
   struct StepInfo {
      
      /// @brief default constructor - empty shared pointer and shape
      StepInfo();
      
      /// @brief constructor setting all details
      /// @details
      /// @param[in] step shared pointer to the processing step object
      /// @param[in] id step ID (determines the rank assignment details)
      /// @param[in] shape shape of the iteration domain 
      StepInfo(const boost::shared_ptr<IProcessingStep> &step, const StepID &id, const casa::IPosition & shape = casa::IPosition());
   
      /// @brief rank assignment
      StepID itsStepID;
      
      /// @brief iteration domain, empty vector means no iteration required
      casa::IPosition itsShape;
      
      /// @brief shared pointer to the object representing this processing step
      boost::shared_ptr<IProcessingStep> itsCode;      
   };
  
   /// @brief details for individual steps
   std::vector<StepInfo> itsSteps;   
};

} // namespace askapparallel

} // namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_COMPOSITE_PROCESSING_STEP_H


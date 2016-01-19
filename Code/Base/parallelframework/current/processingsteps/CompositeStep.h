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
#include "processingsteps/StepInfo.h"
#include "processingsteps/StepIDProxy.h"

// casa includes
#include <casa/Arrays/IPosition.h>

// boost includes
#include <boost/shared_ptr.hpp>


// std includes
#include <string>
#include <vector>
#include <map>

namespace askap {

namespace askapparallel {

// forward declaration for unit testing
class CompositeStepTest;

/// @brief Composite processing step
/// @details This is a composite constructed with a number of processing steps
/// executed in parallel. Individual jobs are represented by objects implementing
/// IProcessingStep interface..
class CompositeStep : public ProcessingStep,
                      virtual public IProcessingStep
{
public:
   // flag to use all ranks available for the given processing step
   enum {
       USE_ALL_AVAILABLE = -1
   };
	
   /// @brief an empty constructor to create unnamed composite object
   /// @details  Upon creation, no parallel jobs are associated with this composite. 
   /// So if used before any add methods are called, it would effectively be an empty operation.
   CompositeStep();

   /// @brief construct a composite object and assign a name
   /// @details Upon creation, no parallel jobs are associated with this composite. 
   /// So if used before any add methods are called, it would effectively be an empty operation.
   /// @param[in] name name to assign
   explicit CompositeStep(const std::string &name);

   // methods to add substeps


   /// @brief add a substep without an associated iterator
   /// @details This method adds a given number of copies (by default to fill all the rank space) of the 
   /// given substep allocating a group of nRanks ranks for each one
   /// @param[in] substep shared pointer to the processing step to run
   /// @param[in] nRanks number of ranks in the group to allocate for the given processing step. Default is 1, but
   ///            multirank processing steps are allowed (e.g. MFS working with Taylor terms in parallel)
   /// @param[in] count number of processing steps to instantiate (in parallel). By default, instantiate as many as
   ///            one could fit in the available rank space
   /// @return proxy object for the given processing step or steps. This is used to setup connections between 
   /// steps via communicators. For multi-rank steps "local" communicator is created automatically.
   StepIDProxy addSubStep(const boost::shared_ptr<IProcessingStep> &substep, int nRanks = 1, int count = USE_ALL_AVAILABLE);

   /// @brief add a substep with an associated iterator
   /// @details This method adds a given number of copies (by default to fill all the rank space) of the 
   /// given substep allocating a group of nRanks ranks for each one
   /// @param[in] substep shared pointer to the processing step to run
   /// @param[in] shape dimensions for the associated iterator to traverse. If more than one rank allocated 
   ///                  (see count parameter), the iteration is split between available ranks. However, groups
   ///                  of nRanks ranks will receive the same iteration subspace.
   /// @param[in] nRanks number of ranks in the group to allocate for the given processing step. Default is 1, but
   ///            multirank processing steps are allowed (e.g. MFS working with Taylor terms in parallel)
   /// @param[in] count number of processing steps to instantiate (in parallel). By default, instantiate as many as
   ///            one could fit in the available rank space
   /// @return proxy object for the given processing step or steps. This is used to setup connections between 
   /// steps via communicators. For multi-rank steps "local" communicator is created automatically.
   StepIDProxy addSubStep(const boost::shared_ptr<IProcessingStep> &substep, const casa::IPosition &shape, 
                   int nRanks = 1, int count = USE_ALL_AVAILABLE);

   // optional communicators - calls to these methods just register the request, actual communicators
   // are created when execution starts
  
   /// @brief set up communicator between same element of all groups
   /// @details A call to this method enables creation of a custom communicator between the same
   /// element of all groups (i.e. 0th element of group 0, 1,... nGroup-1 if accessed from  
   /// rank which belongs to  the 0th element of any group). This is handy for collective operations, e.g.
   /// those used to add visibilities together in the MFS case. By default, the communicator is created
   /// for all elements. However, it is possible to restrict operation to only chosen element. In this case,
   /// the call will result in no operation for the ranks which do not correspond to the chosen element
   /// @param[in] name chosen name for the communicator. Note, global and local are reserved names.
   /// @param[in] step proxy for the child processing step to create the communicator for
   /// @param[in] element chosen element to create the communicator for. By default (negative value), 
   /// communicator between the same elements of all groups is created for all elements, but it is possible
   /// to restrict the operation to only chosen element.
   void createInterGroupCommunicator(const std::string &name, const StepIDProxy &step, int element = USE_ALL_AVAILABLE);

   /// @brief set up custom communicator
   /// @details A call to this method enables creation of a custom communicator between ranks listed
   /// explicitly. Unsliced step ID proxy class covers all groups and elements together.
   /// @param[in] name chosen name for the communicator. Note, global and local are reserved names.
   /// @param[in] steps vector of ID proxies to create the communicator for
   void createCommunicator(const std::string &name, const std::vector<StepIDProxy> &steps);

   // optional rank tagging for point to point communication (as opposed to collective operations 
   // via custom communicators). It may be required for some collective operations too.

   /// @brief associate rank with name
   /// @details A call to this method tags a chosen single rank (either a single rank step or 
   /// a single rank selected out of a multi-rank processing step). 
   /// @param[in] name chosen name for the rank to be tagged.
   /// @param[in] step processing step ID proxy to tag, it should correspond to single rank.
   void tagRank(const std::string &name, const StepIDProxy &step);
   
protected:

   /// @brief reserve part of rank space
   /// @details This call reserves some part of rank space (exact allocation is only known after initialisation)
   /// and returns an instance of StepID describing that rank space
   /// @param[in] nRanks number of ranks in the group to allocate for the given processing step. Default is 1, but
   ///            multirank processing steps are allowed (e.g. MFS working with Taylor terms in parallel)
   /// @param[in] count number of processing steps to instantiate (in parallel). By default, instantiate as many as
   ///            one could fit in the available rank space
   /// @return an instance of StepID describing the allocation
   /// @note this method may change StepID details already stored in itsSteps
   StepID reserveRankSpace(int nRanks = 1, int count = USE_ALL_AVAILABLE);
  

private:
  
   /// @brief details for individual child steps
   std::vector<StepInfo> itsSteps;   

   /// @brief rank tags
   /// @details Actual ranks are only known at run time (not at setup). This map
   /// stores all rank tags to create in the initialise method.
   std::map<std::string, StepIDProxy> itsTaggedRanks;

friend CompositeStepTest;

};

} // namespace askapparallel

} // namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_COMPOSITE_PROCESSING_STEP_H


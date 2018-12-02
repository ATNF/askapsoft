/// @file 
/// @brief Proxy class for an unfilled StepID object
/// @details The framework sets up relations between different parallel steps
/// through communicators. The rank allocation has some flexibility and is not
/// known until initialise method of the composite step. Moreover, more than
/// one rank can be allocated to a single processing step. The StepID class helps to
/// keep track the range of ranks and acts as an ID to identify a particular step.
/// This proxy class allows the user to set up element access relations without
/// the details stored in StepID (which may change if new processing steps are added).
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

#include "processingsteps/StepIDProxy.h"
#include "processingsteps/CompositeStep.h"

// ASKAP includes
#include "askap/AskapError.h"

namespace askap {

namespace askapparallel {

/// @brief construct an unsliced object 
/// @details This variant of the constructor creates an object in the state 
/// prior to operator() call
/// @param[in] index index of the step to deal with (processing steps are stored in a vector)
/// @param[in] composite shared pointer to the CompositeStep containing the processing step 
/// described by this object. This is used to identify rank space with a particular composite as,
/// in principle, we could have a nested case. No access is done using this shared pointer inside 
/// this class. It can safely be empty, if the user wishes so. However, in the future we might 
/// extend the framework to more complex connections between independent composite steps. Then
/// the actual shared pointer should be important.
/// @param[in] singleRank single rank flag (true, if the step is single rank)
StepIDProxy::StepIDProxy(size_t index, const boost::shared_ptr<CompositeStep> &composite, bool singleRank) :
      itsIndex(index), itsComposite(composite), itsSingleRank(singleRank), itsHasBeenSliced(false), 
      itsGroup(0u), itsElement(0u) 
{
}
   
/// @brief construct a sliced object
/// @details This variant of the constructor creates an object in the state 
/// after the call to operator()
/// @param[in] index index of the step to deal with (processing steps are stored in a vector)
/// @param[in] composite shared pointer to the CompositeStep containing the processing step 
/// described by this object. This is used to identify rank space with a particular composite as,
/// in principle, we could have a nested case
/// @param[in] group group index passed to operator()
/// @param[in] element element index passed to operator()   
StepIDProxy::StepIDProxy(size_t index, const boost::shared_ptr<CompositeStep> &composite, unsigned int group, unsigned int element) :
      itsIndex(index), itsComposite(composite), itsSingleRank(true), itsHasBeenSliced(true), 
      itsGroup(group), itsElement(element) 
{
}     

/// @brief default constructor
/// @details needed to store this proxy object in containers
StepIDProxy::StepIDProxy() : itsIndex(0u), itsSingleRank(true), itsHasBeenSliced(false) 
{
}

/// @brief extract single rank slice
/// @details The whole rank space can be represented as a 
/// number of groups each containing a number of elements.
/// This operator returns a single rank StepIDProxy corresponding
/// to the given group and element.
/// @param[in] group zero-based group number to choose
/// @param[in] element zero-based element number to choose
/// @return object describing single rank slice
StepIDProxy StepIDProxy::operator()(unsigned int group, unsigned int element) const
{
  ASKAPASSERT(itsComposite);
  return StepIDProxy(itsIndex, itsComposite, group, element);
}
   
/// @brief check that this object represents a single rank slice
/// @return true, if this object represents a single rank slice
bool StepIDProxy::isSingleRank() const
{
  ASKAPASSERT(itsComposite);
  return itsSingleRank;
}
   
/// @brief obtain index
/// @return index of the step
size_t StepIDProxy::index() const
{
  ASKAPASSERT(itsComposite);
  return itsIndex;
}
   
/// @brief obtain shared pointer to the composite holding the step
/// @return shared pointer to composite (just returns whatever is passed in the constructor, 
/// this class uses shared pointer merely as a tag to distinguish different instances of the 
/// composite step).
const boost::shared_ptr<CompositeStep>& StepIDProxy::composite() const
{
  return itsComposite;
}
   
/// @brief slice StepID if neccessary
/// @details This method takes the slice from the given StepID object if itsHasSliced is true,
/// otherwise the object is copied unchanged. This is used for delayed application of operator(),
/// when the appropriate StepID is ready.
/// @param[in] id input StepID
/// @return sliced or original StepID
StepID StepIDProxy::process(const StepID &id) const
{
  ASKAPASSERT(itsComposite);
  if (itsHasBeenSliced) {
      return id(itsGroup, itsElement);
  }
  return id;
}



} // namespace askapparallel

} // namespace askap

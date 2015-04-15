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

#ifndef ASKAP_ASKAPPARALLEL_STEP_ID_PROXY_H
#define ASKAP_ASKAPPARALLEL_STEP_ID_PROXY_H

// own includes
#include "processingsteps/StepID.h"

// boost includes
#include <boost/shared_ptr.hpp>

namespace askap {

namespace askapparallel {

// forward declaration
class CompositeStep;

/// @brief Proxy class for an unfilled StepID object
/// @details The framework sets up relations between different parallel steps
/// through communicators. The rank allocation has some flexibility and is not
/// known until initialise method of the composite step. Moreover, more than
/// one rank can be allocated to a single processing step. The StepID class helps to
/// keep track the range of ranks and acts as an ID to identify a particular step.
/// This proxy class allows the user to set up element access relations without
/// the details stored in StepID (which may change if new processing steps are added).
class StepIDProxy {
public:
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
   StepIDProxy(size_t index, const boost::shared_ptr<CompositeStep> &composite, bool singleRank);
   
   /// @brief construct a sliced object
   /// @details This variant of the constructor creates an object in the state 
   /// after the call to operator()
   /// @param[in] index index of the step to deal with (processing steps are stored in a vector)
   /// @param[in] composite shared pointer to the CompositeStep containing the processing step 
   /// described by this object. This is used to identify rank space with a particular composite as,
   /// in principle, we could have a nested case
   /// @param[in] group group index passed to operator()
   /// @param[in] element element index passed to operator()   
   StepIDProxy(size_t index, const boost::shared_ptr<CompositeStep> &composite, unsigned int group, unsigned int element);

   /// @brief extract single rank slice
   /// @details The whole rank space can be represented as a 
   /// number of groups each containing a number of elements.
   /// This operator returns a single rank StepIDProxy corresponding
   /// to the given group and element.
   /// @param[in] group zero-based group number to choose
   /// @param[in] element zero-based element number to choose
   /// @return object describing single rank slice
   StepIDProxy operator()(unsigned int group, unsigned int element = 0) const;
   
   /// @brief check that this object represents a single rank slice
   /// @return true, if this object represents a single rank slice
   bool isSingleRank() const;
   
   /// @brief obtain index
   /// @return index of the step
   size_t index() const;
   
   /// @brief obtain shared pointer to the composite holding the step
   /// @return shared pointer to composite (just returns whatever is passed in the constructor, 
   /// this class uses shared pointer merely as a tag to distinguish different instances of the 
   /// composite step).
   const boost::shared_ptr<CompositeStep>& composite() const;
   
   /// @brief slice StepID if neccessary
   /// @details This method takes the slice from the given StepID object if itsHasSliced is true,
   /// otherwise the object is copied unchanged. This is used for delayed application of operator(),
   /// when the appropriate StepID is ready.
   /// @param[in] id input StepID
   /// @return sliced or original StepID
   StepID process(const StepID &id) const;
      
private:
    
   /// @brief index of the step to deal with (processing steps are stored in a vector)
   size_t itsIndex;
   
   /// @brief shared pointer to the CompositeStep containing the processing step
   boost::shared_ptr<CompositeStep> itsComposite;
   
   /// @brief true, if this step is a single rank step
   bool itsSingleRank;
   
   /// @brief true, if slicing was done
   bool itsHasBeenSliced;
   
   /// @brief group index (meaningful only if slicing was done)
   unsigned int itsGroup;
   
   /// @brief element index (meaningful only if slicing was done)
   unsigned int itsElement; 
};
	
} // end of namespace askapparallel
} // end of namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_STEP_ID_PROXY_H


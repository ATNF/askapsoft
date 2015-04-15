/// @file 
/// @brief helper type represending info for each child step
/// @details The structure gathering all info describing child processing steps.
/// It is used by implementation of CompositeStep storing this info in a container.
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

#ifndef ASKAP_ASKAPPARALLEL_STEP_INFO_H
#define ASKAP_ASKAPPARALLEL_STEP_INFO_H

// own includes
#include "processingsteps/IProcessingStep.h"
#include "processingsteps/StepID.h"

// casa includes
#include <casa/Arrays/IPosition.h>

// boost includes
#include <boost/shared_ptr.hpp>

namespace askap {

namespace askapparallel {


/// @brief helper type represending info for each child step
/// @details The structure gathering all info describing child processing steps.
/// It is used by implementation of CompositeStep storing this info in a container.
struct StepInfo {
      
   /// @brief default constructor - empty shared pointer and shape
   StepInfo();
      
   /// @brief constructor setting all details
   /// @details
   /// @param[in] step shared pointer to the processing step object
   /// @param[in] id step ID (determines the rank assignment details)
   /// @param[in] shape shape of the iteration domain 
   StepInfo(const boost::shared_ptr<IProcessingStep> &step, const StepID &id, const casa::IPosition & shape = casa::IPosition());
   
   /// @brief const access to step id object
   /// @return const reference to StepID object corresponding to this processing step
   const StepID& id() const;
   
   /// @brief non-const access to step id object
   /// @return non-const reference to StepID object corresponding to this processing step
   StepID& id();
   
   /// @brief const access to the shape of interation domain
   /// @return const reference to the shape of iteration domain setup for this processing step
   /// @note empty IPosition object means no iteration
   const casa::IPosition& shape() const;
      
   /// @brief non-const access to the shape of interation domain
   /// @return non-const reference to the shape of iteration domain setup for this processing step
   /// @note empty IPosition object means no iteration
   casa::IPosition& shape();
   
   /// @brief const access to the shared pointer of the processing step
   /// @return const reference to the shared pointer to the processing step object
   const boost::shared_ptr<IProcessingStep>& step() const;

   /// @brief non-const access to the shared pointer of the processing step
   /// @return non-const reference to the shared pointer to the processing step object
   boost::shared_ptr<IProcessingStep>& step();    

private:   
   /// @brief step id object - describes rank assignment
   StepID itsID;
      
   /// @brief iteration domain, empty vector means no iteration required
   casa::IPosition itsShape;
      
   /// @brief shared pointer to the object representing this processing step
   boost::shared_ptr<IProcessingStep> itsStep;      
};

} // namespace askapparallel

} // namespace askap

#endif // #ifndef ASKAP_ASKAPPARALLEL_STEP_INFO_H


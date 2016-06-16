/// @file
///
/// Support for parallel statistics accululation to advise on imaging parameters
///
/// @copyright (c) 2016 CSIRO
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
/// @author Stephen Ord <stephen.ord@csiro.au>
///

#ifndef ASKAP_IMAGER_ADVISE_DI_H
#define ASKAP_IMAGER_ADVISE_DI_H

#include <Common/ParameterSet.h>
#include <parallel/MEParallelApp.h>
#include <measurementequation/VisMetaDataStats.h>
#include <casacore/casa/Quanta/MVDirection.h>
#include <parallel/AdviseParallel.h>

#include <boost/shared_ptr.hpp>
#include <string>

namespace askap {

namespace synthesis {

/// @brief parallel helper for the advise utility
/// @details This class does the core operation to run statistics estimators on every measurement set
/// and aggregating the result. Most non-trivial actions happen in the parallel mode.
/// @note It may be a bit untidy to derive this class from MEParallelApp just to reuse a bunch of existing code,
/// but some subtle features like frequency conversion setup may come handy in the future. The goal is that it should
/// work with only the single parameter present in the parset which describes the measurement set(s). 
/// @ingroup parallel
class AdviseDI : public AdviseParallel
{
public:
   /// @brief Constructor from ParameterSet
   /// @details The parset is used to construct the internal state. We could
   /// also support construction from a python dictionary (for example).
   /// The command line inputs are needed solely for MPI - currently no
   /// application specific information is passed on the command line.
   /// @param comms communication object 
   /// @param parset ParameterSet for inputs
   AdviseDI(askap::askapparallel::AskapParallel& comms, const LOFAR::ParameterSet& parset);

   /// @brief Add the missing parameters
   /// @details Add whatever details we require for both master and
   /// worker implementations
   
   void addMissingParameters();
   
   LOFAR::ParameterSet getParset() { return itsParset; };
   
protected:
   std::vector<std::string> getDatasets();
  
        
private:
   
   LOFAR::ParameterSet itsParset;
  
   /// The parameters we need to set.
   
   int channel;
   
   double minFrequency;
   
   double maxFrequency;
   
   casa::MVDirection itsTangent;
   
   
};

} // namespace synthesis

} // namespace askap

#endif // #ifndef SYNTHESIS_ADVISE_PARALLEL_H


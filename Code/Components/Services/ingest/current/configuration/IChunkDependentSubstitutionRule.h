/// @file IChunkDependentSubstitutionRule.h
///
/// @copyright (c) 2012 CSIRO
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

#ifndef ASKAP_CP_INGEST_ICHUNKDEPENDENTSUBSTITUTIONRULE_H
#define ASKAP_CP_INGEST_ICHUNKDEPENDENTSUBSTITUTIONRULE_H

// boost include 
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

// ASKAPsoft include
#include "configuration/ISubstitutionRule.h"
#include "cpcommon/VisChunk.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Interface for substitution rule which depends on VisChunk
/// @details More advanced substitution rules may depend on the content of current 
/// VisChunk. This interface adds an additional method to pass VisChunk for use, 
/// either to setup the rule or to check that the result still conforms to the state of
/// things in the first sighted VisChunk. 
class IChunkDependentSubstitutionRule : virtual public ISubstitutionRule {
   public:

   /// @brief constructor
   IChunkDependentSubstitutionRule();

   /// @brief initialise the object
   /// @details This is the main entry point supported by the base interface.
   /// Implementation does necessary operations with the chunk shared pointer and
   /// calls the variant of initialise passing the shared pointer necessary for the
   /// setup. 
   virtual void initialise();

   /// @brief pass chunk to work with
   /// @details The shared pointer to the chunk is stored in a weak pointer and is 
   /// expected to be valid until the call to initialise method. 
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The design is a bit ugly, but this is largely to contain MPI calls in a single
   /// place and don't have FAT interfaces. An exception is thrown if initialise method is
   /// called without setting up the chunk
   void setupFromChunk(const boost::shared_ptr<common::VisChunk> &chunk);

   /// @brief verify that the chunk conforms
   /// @details The class is setup once, at the time when MPI calls are allowed. This
   /// method allows to check that another (new) chunk still conforms with the original setup.
   /// The method exists only for cross-checks, it is not required to be called for correct
   /// operation of the whole framework.
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The chunk itself is unchanged. An exception is expected to be thrown 
   /// if the chunk doesn't conform.
   virtual void verifyChunk(const boost::shared_ptr<common::VisChunk> &chunk) = 0;

protected:

   /// @brief initialise the object
   /// @details This is the only place where MPI calls may happen. 
   /// In this method, the implementations are expected to provide a
   /// mechanism to obtain values for all keywords handled by this object.
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The chunk itself is unchanged
   virtual void initialise(const boost::shared_ptr<common::VisChunk> &chunk) = 0;

   /// @return true, if this rank is unused
   /// @note the result is only valid after a call to setupFromChunk
   inline bool unusedRank() const { return itsUnusedRank;}

private:
   /// @brief temporary buffer for chunk
   /// @details The weak pointer is expected to be valid only between the calls to 
   /// setupFromChunk and intialise methods.
   boost::weak_ptr<common::VisChunk> itsChunkBuf;

   /// @brief true, if this rank is not participating in substitution
   /// @details We pass zero shared pointer to the chunk as a mark that
   /// a particular rank is unused. Therefore, chunk-dependent classes should be
   /// able to deal with void chunk scenario. This flag is used to check whether this
   /// is the case for the given rank.
   /// @note This method is only valid after a call to setupFromChunk
   bool itsUnusedRank;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_ICHUNKDEPENDENTSUBSTITUTIONRULE_H


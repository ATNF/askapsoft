/// @file BeamSubstitutionRule.h
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

#ifndef ASKAP_CP_INGEST_BEAMSUBSTITUTIONRULE_H
#define ASKAP_CP_INGEST_BEAMSUBSTITUTIONRULE_H

// ASKAPsoft includes
#include "ingestpipeline/mssink/ChunkDependentSubstitutionRuleImpl.h"
#include "configuration/Configuration.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Substitute keyword by beam Id
/// @details This is an example of data dependent substituion method, The beam number is 
/// the result of the substitution. Substitution fails if there is more than one beam in 
/// the accessor.
class BeamSubstitutionRule : public ChunkDependentSubstitutionRuleImpl {
   public:

   /// @brief constructor
   /// @details
   /// @param[in] kw keyword string to represent
   /// @param[in] config configuration class
   BeamSubstitutionRule(const std::string &kw, const Configuration &config);

   // implementation of remaining interface mentods

   /// @brief verify that the chunk conforms
   /// @details The class is setup once, at the time when MPI calls are allowed. This
   /// method allows to check that another (new) chunk still conforms with the original setup.
   /// The method exists only for cross-checks, it is not required to be called for correct
   /// operation of the whole framework.
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The chunk itself is unchanged. An exception is expected to be thrown 
   /// if the chunk doesn't conform. All checks are ignored if the rule is not in use
   virtual void verifyChunk(const boost::shared_ptr<common::VisChunk> &chunk);

protected:

   /// @brief initialise the object
   /// @details This is the only place where MPI calls may happen. 
   /// In this method, the implementations are expected to provide a
   /// mechanism to obtain values for all keywords handled by this object.
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The chunk itself is unchanged
   virtual void initialise(const boost::shared_ptr<common::VisChunk> &chunk);

   /// @brief verify that all values in the integer array are the same
   /// @details This method also returns the value. Note, empty array as well as
   /// array with different numbers cause an exception.
   /// @param[in] vec vector with elements
   /// @return the value this vector has
   static casa::uInt checkAllValuesAreTheSame(const casa::Vector<casa::uInt> &vec);
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_BEAMSUBSTITUTIONRULE_H


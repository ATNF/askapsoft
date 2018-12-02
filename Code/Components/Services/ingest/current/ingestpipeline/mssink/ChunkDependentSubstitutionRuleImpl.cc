/// @file ChunkDependentSubstitutionRuleImpl.cc
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

// ASKAPsoft includes
#include "ingestpipeline/mssink/ChunkDependentSubstitutionRuleImpl.h"
#include "askap/AskapUtil.h"
#include "askap/AskapError.h"

// it would be nice to get all MPI stuff in a single place, but ingest is already MPI-heavy throughout
#include <mpi.h>

// std includes
#include <vector>
#include <algorithm>
#include <functional>

namespace askap {
namespace cp {
namespace ingest {

/// @brief constructor
/// @details
/// @param[in] kw keyword string to represent
/// @param[in] rank this rank
/// @param[in] nprocs number of ranks
ChunkDependentSubstitutionRuleImpl::ChunkDependentSubstitutionRuleImpl(const std::string &kw, int rank, int nprocs) :
       itsKeyword(kw), itsValue(-1), itsNProcs(nprocs), itsRank(rank), itsRankIndependent(true), 
       itsHasBeenInitialised(false), itsUnusedRanks(nprocs, 0)
{
   ASKAPASSERT(itsRank < itsNProcs);
   ASKAPASSERT(itsNProcs > 0);
}

// implementation of interface mentods

/// @brief obtain keywords handled by this object
/// @details This method returns a set of string keywords
/// (without leading % sign in our implementation, but in general this 
/// can be just logical full-string keyword, we don't have to limit ourselves
/// to particular single character tags) which this class recognises. Any of these
/// keyword can be passed to operator() once the object is initialised
/// @return set of keywords this object recognises
std::set<std::string> ChunkDependentSubstitutionRuleImpl::keywords() const
{
   std::set<std::string> result;
   result.insert(itsKeyword);
   return result;
}

/// @brief obtain value of a particular keyword
/// @details This is the main access method which is supposed to be called after
/// initialise(). 
/// @param[in] kw keyword to access, must be from the set returned by keywords
/// @return value of the requested keyword
/// @note  An exception may be thrown if the initialise() method is not called
/// prior to an attempt to access the value.
std::string ChunkDependentSubstitutionRuleImpl::operator()(const std::string &kw) const
{
   ASKAPCHECK(kw == itsKeyword, "Attempted to obtain keyword '"<<kw<<"' out of a substitution rule set up with '"<<itsKeyword<<"'");
   return utility::toString(itsValue);
}

/// @brief check if values are rank-independent
/// @details The implementation of this interface should evaluate a flag and return it
/// in this method to show whether the value for a particular keyword is
/// rank-independent or not. This is required to encapsulate all MPI related calls in
/// the initialise. Sometimes, the value of the flag can be known up front, e.g. if
/// the value is the result of gather-scatter operation or if it is based on rank number.
/// @param[in] kw keyword to check the flag for
/// @return true, if the given keyword has the same value for all ranks
bool ChunkDependentSubstitutionRuleImpl::isRankIndependent() const
{
   return itsRankIndependent;
};

/// @brief check that the rule is in use
/// @return true, if this particular rule has been initialised and, therefore, is in use
bool ChunkDependentSubstitutionRuleImpl::inUse() const
{
   return itsHasBeenInitialised; 
}

/// @brief get rank
/// @return rank passed via constructor
int ChunkDependentSubstitutionRuleImpl::rank() const
{
   return itsRank;
}

/// @brief get number of ranks
/// @return number of ranks passed via constructor
int ChunkDependentSubstitutionRuleImpl::nprocs() const
{
   return itsNProcs;
}

/// @brief initialise the object
/// @details This overrides implementation in base class to set in use flag and to
/// aggregate values after initialisation to set up rank dependence flags.
/// For all practical purposes, that implementation is used and the actual entry point
/// is via the pure virtual initialise method which accepts chunk as arguments (to be
/// defined in derived methods).
void ChunkDependentSubstitutionRuleImpl::initialise()
{
   ASKAPCHECK(!itsHasBeenInitialised, "The chunk-dependent rule has already been initialised");

   // aggregate idle rank flags - it is possible to do this here as chunk/activity flag should be set before initialisation
   ASKAPDEBUGASSERT(itsRank < static_cast<int>(itsUnusedRanks.size()));
   itsUnusedRanks[itsRank] = unusedRank();
   
   if (itsNProcs > 1) {
       const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)itsUnusedRanks.data(),
                                 itsUnusedRanks.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
   }
   
   IChunkDependentSubstitutionRule::initialise(); 

   // it is important to set the flag *after* initialisation, otherwise setValue,
   // which is the only way to set value field in derived classes, would abort with exception
   itsHasBeenInitialised = true;
   
   // aggregate values if necessary to set rank-dependency flag
   if (itsNProcs > 1) {
       // distributed case, need to aggregate values. Otherwise, the field has already been setup with true
       std::vector<int> individualValues(itsNProcs, 0);
       individualValues[itsRank] = itsValue;
       const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)individualValues.data(),
                                 individualValues.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);

       const int stubValue = individualValues[firstActiveRank()];
       for (size_t index = 0; index < individualValues.size(); ++index) {
            if (unusedRank(static_cast<int>(index))) {
                individualValues[index] = stubValue;
            }
       }
            
       // now individualValues are consistent on all ranks, so check the values
       itsRankIndependent = (std::find_if(individualValues.begin(), individualValues.end(), 
                              std::bind2nd(std::not_equal_to<int>(), itsValue)) == individualValues.end());
   }
   
}

/// @brief set the value represented by this class
/// @param[in] val value to set
/// @note An exception is raised if the value is set after the class has been initialised
void ChunkDependentSubstitutionRuleImpl::setValue(int val)
{
   ASKAPCHECK(!itsHasBeenInitialised, "setValue is used outside of initialisation, this should't happen");
   itsValue = val;
}

/// @brief check that the given rank is unused
/// @details This method check idle status for the given rank, the result is valid
/// only after call to initialise method 
/// @param[in] rank rank to check
/// @return true if the given rank is unused
bool ChunkDependentSubstitutionRuleImpl::unusedRank(int rank) const 
{
   ASKAPASSERT(rank < static_cast<int>(itsUnusedRanks.size()));
   return itsUnusedRanks[rank] > 0;
}

/// @brief return first active rank
/// @details this is valid only after initialise method. An exception is
/// thrown if all ranks are idle
/// @return the first rank which is active
int ChunkDependentSubstitutionRuleImpl::firstActiveRank() const
{
   for (int rank = 0; rank < static_cast<int>(itsUnusedRanks.size()); ++rank) {
        if (itsUnusedRanks[rank] == 0) {
            return rank;
        }
   }
   ASKAPTHROW(AskapError, "All ranks are inactive");
}

};
};
};


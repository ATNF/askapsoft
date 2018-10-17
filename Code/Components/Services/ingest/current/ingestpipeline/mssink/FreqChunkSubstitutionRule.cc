/// @file FreqChunkSubstitutionRule.cc
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
#include "ingestpipeline/mssink/FreqChunkSubstitutionRule.h"
#include "askap/AskapUtil.h"
#include "askap/AskapError.h"
#include "askap/IndexedCompare.h"

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
/// @param[in] config configuration class
FreqChunkSubstitutionRule::FreqChunkSubstitutionRule(const std::string &kw, const Configuration &config) :
       itsKeyword(kw), itsFreq(0.), itsFreqChunkId(0), itsNProcs(config.nprocs()), itsRank(config.rank()), itsRankIndependent(true) 
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
std::set<std::string> FreqChunkSubstitutionRule::keywords() const
{
   std::set<std::string> result;
   result.insert(itsKeyword);
   return result;
}

/// @brief verify that the chunk conforms
/// @details The class is setup once, at the time when MPI calls are allowed. This
/// method allows to check that another (new) chunk still conforms with the original setup.
/// The method exists only for cross-checks, it is not required to be called for correct
/// operation of the whole framework.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged. An exception is expected to be thrown 
/// if the chunk doesn't conform.
void FreqChunkSubstitutionRule::verifyChunk(const boost::shared_ptr<common::VisChunk> &chunk)
{
   ASKAPASSERT(chunk);
   ASKAPASSERT(chunk->frequency().nelements() > 0);
   const casa::Double curFreq = chunk->frequency()[0];
   ASKAPCHECK(casa::near(itsFreq, curFreq, 1e-6), 
             "Frequency axis appears to have changed, this is incompatible with the frequency chunk substitution rule");
}

/// @brief initialise the object
/// @details This is the only place where MPI calls may happen. 
/// In this method, the implementations are expected to provide a
/// mechanism to obtain values for all keywords handled by this object.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged
void FreqChunkSubstitutionRule::initialise(const boost::shared_ptr<common::VisChunk> &chunk)
{
   ASKAPASSERT(chunk);
   ASKAPASSERT(chunk->frequency().nelements() > 0);
   itsFreq = chunk->frequency()[0];

   if (itsNProcs > 1) {
       // distributed case, need to aggregate values. Otherwise, the field has already been setup correctly
       std::vector<double> individualFreqs(itsNProcs, 0);
       individualFreqs[itsRank] = itsFreq;
       int response = MPI_Allreduce(MPI_IN_PLACE, (void*)individualFreqs.data(),
                                 individualFreqs.size(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       std::vector<int> indices(itsNProcs);
       for (size_t index = 0; index < static_cast<size_t>(itsNProcs); ++index) {
            indices[index] = static_cast<int>(index);
       }
       std::sort(indices.begin(), indices.end(), utility::indexedCompare<int>(individualFreqs.begin()));
       itsFreqChunkId = 0;
       // going through ranks until the current one incrementing chunk Id if frequency changes significantly
       std::vector<int>::const_iterator ci = indices.begin();
       ASKAPDEBUGASSERT(ci != indices.end());
       ASKAPDEBUGASSERT(static_cast<size_t>(*ci) < individualFreqs.size());
       for (double curFreq = individualFreqs[*ci]; ci != indices.end(); ++ci) {
            ASKAPDEBUGASSERT(*ci < static_cast<int>(individualFreqs.size()));
            ASKAPCHECK(!isnan(curFreq), "Frequency axis contains NaNs! This is not expected");
            // Using 1 Hz frequency tolerance
            if (casa::abs(curFreq - individualFreqs[*ci]) > 1.) {
                curFreq = individualFreqs[*ci]; 
                ++itsFreqChunkId;
            }
            if (*ci == itsRank) {
                // current value of itsFreqChunkId is the correct value for this rank
                break; 
            }
       }
       ASKAPDEBUGASSERT(ci != indices.end());
 
       // check indices for rank-dependence
       std::vector<int> individualValues(itsNProcs, 0);
       individualValues[itsRank] = itsFreqChunkId;
       response = MPI_Allreduce(MPI_IN_PLACE, (void*)individualValues.data(),
                                 individualValues.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       // now individualValues are consistent on all ranks, so check the values
       itsRankIndependent = (std::find_if(individualValues.begin(), individualValues.end(), 
                              std::bind2nd(std::not_equal_to<int>(), itsFreqChunkId)) == individualValues.end());
   }
}

/// @brief obtain value of a particular keyword
/// @details This is the main access method which is supposed to be called after
/// initialise(). 
/// @param[in] kw keyword to access, must be from the set returned by keywords
/// @return value of the requested keyword
/// @note  An exception may be thrown if the initialise() method is not called
/// prior to an attempt to access the value.
std::string FreqChunkSubstitutionRule::operator()(const std::string &kw) const
{
   ASKAPCHECK(kw == itsKeyword, "Attempted to obtain keyword '"<<kw<<"' out of FreqChunkSubstitutionRule set up with '"<<itsKeyword<<"'");
   return utility::toString(itsFreqChunkId);
}

/// @brief check if values are rank-independent
/// @details The implementation of this interface should evaluate a flag and return it
/// in this method to show whether the value for a particular keyword is
/// rank-independent or not. This is required to encapsulate all MPI related calls in
/// the initialise. Sometimes, the value of the flag can be known up front, e.g. if
/// the value is the result of gather-scatter operation or if it is based on rank number.
/// @param[in] kw keyword to check the flag for
/// @return true, if the given keyword has the same value for all ranks
bool FreqChunkSubstitutionRule::isRankIndependent() const
{
   return itsRankIndependent;
};


};
};
};


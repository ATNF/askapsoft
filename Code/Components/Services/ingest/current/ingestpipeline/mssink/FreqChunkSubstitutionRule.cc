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
       ChunkDependentSubstitutionRuleImpl(kw, config.rank(), config.nprocs()), 
       itsFreq(0.) 
{
   ASKAPDEBUGASSERT(rank() < nprocs());
   ASKAPDEBUGASSERT(nprocs() > 0);
}

// implementation of remaining interface mentods

/// @brief verify that the chunk conforms
/// @details The class is setup once, at the time when MPI calls are allowed. This
/// method allows to check that another (new) chunk still conforms with the original setup.
/// The method exists only for cross-checks, it is not required to be called for correct
/// operation of the whole framework.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged. An exception is expected to be thrown 
/// if the chunk doesn't conform. If the rule is not in use, this method does nothing.
void FreqChunkSubstitutionRule::verifyChunk(const boost::shared_ptr<common::VisChunk> &chunk)
{
   if (inUse()) {
       ASKAPASSERT(chunk);
       ASKAPASSERT(chunk->frequency().nelements() > 0);
       const casa::Double curFreq = chunk->frequency()[0];
       ASKAPCHECK(casa::near(itsFreq, curFreq, 1e-6), 
                 "Frequency axis appears to have changed, this is incompatible with the frequency chunk substitution rule");
   }
}

/// @brief initialise the object
/// @details This is the only place where MPI calls may happen. 
/// In this method, the implementations are expected to provide a
/// mechanism to obtain values for all keywords handled by this object.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged
void FreqChunkSubstitutionRule::initialise(const boost::shared_ptr<common::VisChunk> &chunk)
{
   if (!unusedRank()) {
       ASKAPASSERT(chunk);
       ASKAPASSERT(chunk->frequency().nelements() > 0);
       itsFreq = chunk->frequency()[0];
   }

   if (nprocs() > 1) {
       // distributed case, need to aggregate values. Otherwise, the field has already been setup correctly
       std::vector<double> individualFreqs(nprocs(), 0);
       individualFreqs[rank()] = itsFreq;
       const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)individualFreqs.data(),
                                       individualFreqs.size(), MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       const double stubFreq = individualFreqs[firstActiveRank()];
       std::vector<int> indices(nprocs());
       for (size_t index = 0; index < static_cast<size_t>(nprocs()); ++index) {
            indices[index] = static_cast<int>(index);
            // this is necessary to get consistent picture for ranks which are active
            // including rank-dependence or otherwise
            // we don't care about result on inactive ones
            if (unusedRank(static_cast<int>(index))) {
                individualFreqs[index] = stubFreq;
            }
       }
       std::sort(indices.begin(), indices.end(), utility::indexedCompare<int>(individualFreqs.begin()));
       int freqChunkId = 0;
       // going through ranks until the current one incrementing chunk Id if frequency changes significantly
       std::vector<int>::const_iterator ci = indices.begin();
       ASKAPDEBUGASSERT(ci != indices.end());
       ASKAPDEBUGASSERT(static_cast<size_t>(*ci) < individualFreqs.size());
       for (double curFreq = individualFreqs[*ci]; ci != indices.end(); ++ci) {
            ASKAPDEBUGASSERT(*ci < static_cast<int>(individualFreqs.size()));
            ASKAPCHECK(!std::isnan(curFreq), "Frequency axis contains NaNs! This is not expected");
            // Using 1 Hz frequency tolerance
            if (casa::abs(curFreq - individualFreqs[*ci]) > 1.) {
                curFreq = individualFreqs[*ci]; 
                ++freqChunkId;
            }
            if (*ci == rank()) {
                // current value of freqChunkId is the correct value for this rank
                break; 
            }
       }
       ASKAPDEBUGASSERT(ci != indices.end());
       setValue(freqChunkId);
   }
}


};
};
};


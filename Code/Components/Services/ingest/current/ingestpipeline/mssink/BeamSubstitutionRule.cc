/// @file BeamSubstitutionRule.cc
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
#include "ingestpipeline/mssink/BeamSubstitutionRule.h"
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
/// @param[in] config configuration class
BeamSubstitutionRule::BeamSubstitutionRule(const std::string &kw, const Configuration &config) :
       itsKeyword(kw), itsBeam(-1), itsNProcs(config.nprocs()), itsRank(config.rank()), itsRankIndependent(true) 
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
std::set<std::string> BeamSubstitutionRule::keywords() const
{
   std::set<std::string> result;
   result.insert(itsKeyword);
   return result;
}

/// @brief verify that all values in the integer array are the same
/// @details This method also returns the value. Note, empty array as well as
/// array with different numbers cause an exception.
/// @param[in] vec vector with elements
/// @return the value this vector has
casa::uInt BeamSubstitutionRule::checkAllValuesAreTheSame(const casa::Vector<casa::uInt> &vec)
{
   ASKAPCHECK(vec.nelements() > 0, "BeamSubstitutionRule is not supposed to be used with empty data chunk");
   const casa::uInt res = vec[0];
   for (casa::uInt index = 1; index < vec.nelements(); ++index) {
        ASKAPCHECK(res == vec[index], "Different beam indices are encountered in the data chunk while beam substitution rule is used");
   }
   return res;
}

/// @brief verify that the chunk conforms
/// @details The class is setup once, at the time when MPI calls are allowed. This
/// method allows to check that another (new) chunk still conforms with the original setup.
/// The method exists only for cross-checks, it is not required to be called for correct
/// operation of the whole framework.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged. An exception is expected to be thrown 
/// if the chunk doesn't conform.
void BeamSubstitutionRule::verifyChunk(const boost::shared_ptr<common::VisChunk> &chunk)
{
   ASKAPASSERT(chunk);
   const casa::uInt beam = checkAllValuesAreTheSame(chunk->beam1());
   const casa::uInt beam2 = checkAllValuesAreTheSame(chunk->beam2());
   ASKAPCHECK(beam == beam2, "Beam1 and Beam2 in the visibility chunks are expected to be the same, you have "<<beam<<" and "<<beam2);
   ASKAPCHECK(itsBeam == static_cast<int>(beam), "Beam substitution rule for this rank setup to require beam "<<itsBeam<<
              ", while data chunk has beam "<<beam<<" in it!");
}

/// @brief initialise the object
/// @details This is the only place where MPI calls may happen. 
/// In this method, the implementations are expected to provide a
/// mechanism to obtain values for all keywords handled by this object.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged
void BeamSubstitutionRule::initialise(const boost::shared_ptr<common::VisChunk> &chunk)
{
   ASKAPASSERT(chunk);
   const casa::uInt beam = checkAllValuesAreTheSame(chunk->beam1());
   const casa::uInt beam2 = checkAllValuesAreTheSame(chunk->beam2());
   ASKAPCHECK(beam == beam2, "Beam1 and Beam2 in the visibility chunks are expected to be the same, you have "<<beam<<" and "<<beam2);
   ASKAPDEBUGASSERT(itsBeam < 0);
   itsBeam = static_cast<int>(beam);
   
   if (itsNProcs > 1) {
       // distributed case, need to aggregate values. Otherwise, the field has already been setup with true
       std::vector<int> individualValues(itsNProcs, 0);
       individualValues[itsRank] = itsBeam;
       const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)individualValues.data(),
                                 individualValues.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);
       // now individualValues are consistent on all ranks, so check the values
       itsRankIndependent = (std::find_if(individualValues.begin(), individualValues.end(), 
                              std::bind2nd(std::not_equal_to<int>(), itsBeam)) == individualValues.end());
   }
}

/// @brief obtain value of a particular keyword
/// @details This is the main access method which is supposed to be called after
/// initialise(). 
/// @param[in] kw keyword to access, must be from the set returned by keywords
/// @return value of the requested keyword
/// @note  An exception may be thrown if the initialise() method is not called
/// prior to an attempt to access the value.
std::string BeamSubstitutionRule::operator()(const std::string &kw) const
{
   ASKAPCHECK(kw == itsKeyword, "Attempted to obtain keyword '"<<kw<<"' out of GenericSubstitutionRule set up with '"<<itsKeyword<<"'");
   return utility::toString(itsBeam);
}

/// @brief check if values are rank-independent
/// @details The implementation of this interface should evaluate a flag and return it
/// in this method to show whether the value for a particular keyword is
/// rank-independent or not. This is required to encapsulate all MPI related calls in
/// the initialise. Sometimes, the value of the flag can be known up front, e.g. if
/// the value is the result of gather-scatter operation or if it is based on rank number.
/// @param[in] kw keyword to check the flag for
/// @return true, if the given keyword has the same value for all ranks
bool BeamSubstitutionRule::isRankIndependent() const
{
   return itsRankIndependent;
};


};
};
};


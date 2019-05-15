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

namespace askap {
namespace cp {
namespace ingest {

/// @brief constructor
/// @details
/// @param[in] kw keyword string to represent
/// @param[in] config configuration class
BeamSubstitutionRule::BeamSubstitutionRule(const std::string &kw, const Configuration &config) :
       ChunkDependentSubstitutionRuleImpl(kw, config.rank(), config.nprocs())
{
}

/// @brief verify that all values in the integer array are the same
/// @details This method also returns the value. Note, empty array as well as
/// array with different numbers cause an exception.
/// @param[in] vec vector with elements
/// @return the value this vector has
casacore::uInt BeamSubstitutionRule::checkAllValuesAreTheSame(const casacore::Vector<casacore::uInt> &vec)
{
   ASKAPCHECK(vec.nelements() > 0, "BeamSubstitutionRule is not supposed to be used with empty data chunk");
   const casacore::uInt res = vec[0];
   for (casacore::uInt index = 1; index < vec.nelements(); ++index) {
        ASKAPCHECK(res == vec[index], "Different beam indices are encountered in the data chunk while beam substitution rule is used");
   }
   return res;
}

// implementation of remaining interface mentods

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
   if (inUse()) {
       ASKAPASSERT(chunk);
       const casacore::uInt beam = checkAllValuesAreTheSame(chunk->beam1());
       const casacore::uInt beam2 = checkAllValuesAreTheSame(chunk->beam2());
       ASKAPCHECK(beam == beam2, "Beam1 and Beam2 in the visibility chunks are expected to be the same, you have "<<beam<<" and "<<beam2);
       ASKAPCHECK(value() == static_cast<int>(beam), "Beam substitution rule for this rank setup to require beam "<<value()<<
                  ", while data chunk has beam "<<beam<<" in it!");
   }
}

/// @brief initialise the object
/// @details This is the only place where MPI calls may happen. 
/// In this method, the implementations are expected to provide a
/// mechanism to obtain values for all keywords handled by this object.
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The chunk itself is unchanged
void BeamSubstitutionRule::initialise(const boost::shared_ptr<common::VisChunk> &chunk)
{
   if (unusedRank()) {
       return;
   }
   ASKAPASSERT(chunk);
   const casacore::uInt beam = checkAllValuesAreTheSame(chunk->beam1());
   const casacore::uInt beam2 = checkAllValuesAreTheSame(chunk->beam2());
   ASKAPCHECK(beam == beam2, "Beam1 and Beam2 in the visibility chunks are expected to be the same, you have "<<beam<<" and "<<beam2);
   ASKAPDEBUGASSERT(!inUse());
   setValue(static_cast<int>(beam));
}

};
};
};


/// @file IChunkDependentSubstitutionRule.cc
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

// ASKAPSoft includes
#include "configuration/IChunkDependentSubstitutionRule.h"
#include "askap/AskapError.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief initialise the object
/// @details This is the main entry point supported by the base interface.
/// Implementation does necessary operations with the chunk shared pointer and
/// calls the variant of initialise passing the shared pointer necessary for the
/// setup. 
void IChunkDependentSubstitutionRule::initialise()
{
   ASKAPCHECK(itsChunkBuf, "setupFromChunk method is not called prior to initialisation of chunk-dependent substitution rule");
   initialise(itsChunkBuf);
   // the chunk is not necessary anymore
   itsChunkBuf.reset();
}

/// @brief pass chunk to work with
/// @details The shared pointer to the chunk is stored until the call to initialise
/// method, and then released to avoid holding up too much memory unnecessarily. 
/// @param[in] chunk shared pointer to VisChunk to work with
/// @note The design is a bit ugly, but this is largely to contain MPI calls in a single
/// place and don't have FAT interfaces. An exception is thrown if initialise method is
/// called without setting up the chunk
void IChunkDependentSubstitutionRule::setupFromChunk(const boost::shared_ptr<common::VisChunk> &chunk)
{
   ASKAPCHECK(!itsChunkBuf, "setupFromChunk and initialisation are supposed to be called only once!");
   itsChunkBuf = chunk;
}


};
};
};


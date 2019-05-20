/// @file FeedConfig.cc
///
/// @copyright (c) 2011 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "FeedConfig.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Vector.h"

// Local package includes

ASKAP_LOGGER(logger, ".FeedConfig");

using namespace askap;
using namespace askap::cp::ingest;

FeedConfig::FeedConfig(const casacore::Matrix<casacore::Quantity>& offsets,
                       const casacore::Vector<casacore::String>& pols) :
        itsOffsets(offsets), itsPols(pols)
{
    ASKAPCHECK(offsets.ncolumn() == 2,
               "Offset matrix should have two columns");
    ASKAPCHECK(offsets.nrow() > 0,
               "Offsets should have at least one row");
    ASKAPCHECK(offsets.nrow() == pols.nelements(),
               "shape of offsets matrix and polarisations vector not consistent");

    // Ensure all offsets conform to radians
    casacore::Matrix<casacore::Quantity>::const_iterator it;

    for (it = itsOffsets.begin(); it != itsOffsets.end(); ++it) {
        ASKAPCHECK(it->isConform("rad"), "Offset must conform to radians");
    }
}

/// @brief copy constructor
/// @details It is necessary to have copy constructor due to reference semantics of 
/// casacore arrays. 
/// @param[in] other object to copy from
FeedConfig::FeedConfig(const FeedConfig &other) : itsOffsets(other.itsOffsets.copy()), itsPols(other.itsPols.copy()) {}

/// @brief assignment operator
/// @details It is necessary to have copy constructor due to reference semantics of 
/// casacore arrays. 
/// @param[in] other object to copy from
FeedConfig& FeedConfig::operator=(const FeedConfig &other)
{
   if (&other != this) {
       itsOffsets.reference(other.itsOffsets.copy());
       itsPols.reference(other.itsPols.copy());
   }
   return *this;
}


casacore::Quantity FeedConfig::offsetX(casacore::uInt i) const
{
    ASKAPCHECK(i < itsOffsets.nrow(),
               "Feed index out of bounds");
    return itsOffsets(i, 0);
}

casacore::Quantity FeedConfig::offsetY(casacore::uInt i) const
{
    ASKAPCHECK(i < itsOffsets.nrow(),
               "Feed index out of bounds");
    return itsOffsets(i, 1);
}

casacore::String FeedConfig::pol(casacore::uInt i) const
{
    ASKAPCHECK(i < itsPols.nelements(),
               "Feed index out of bounds");
    return itsPols(i);
}

casacore::uInt FeedConfig::nFeeds(void) const
{
    return itsOffsets.nrow();
}

/// @brief Obtain X and Y offsets for all beams
/// @details This is a helper method to extract all offsets at once in the 
/// format of the VisChunk buffer (i.e. 2 x nBeam matrix with offsets in radians).
/// It is not clear whether this method is going to be useful long term
/// @param[in] buffer the matrix to fill. It is resized, if necessary.
void FeedConfig::fillMatrix(casacore::Matrix<casacore::Double> &buffer) const
{
   if (buffer.nrow() != 2 || buffer.ncolumn() != nFeeds()) {
       buffer.resize(2, nFeeds());
   }
   for (casacore::uInt beam = 0; beam < buffer.ncolumn(); ++beam) {
        buffer(0, beam) = itsOffsets(beam, 0).getValue("rad");
        buffer(1, beam) = itsOffsets(beam, 1).getValue("rad");
   }
}


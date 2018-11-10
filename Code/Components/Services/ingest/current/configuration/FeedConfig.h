/// @file FeedConfig.h
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

#ifndef ASKAP_CP_CPINGEST_FEEDCONFIG_H
#define ASKAP_CP_CPINGEST_FEEDCONFIG_H

// ASKAPsoft includes
#include "casacore/casa/aips.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Vector.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief This class encapsulated the configuratin of a feed package such
/// as a single-pixel feed or a PAF.
class FeedConfig {
    public:

        /// @brief Constructor
        /// @param[in] offsets  Feeds (or synthesised beam) offsets in radians. The
        ///                     Matrix is sized Matrix(nFeeds,2). I.E. An offset in
        ///                     X and in Y for each feed. The first column is the
        ///                     offset in X and the second the offset in Y.
        ///
        /// @param[in] pols    Polarisations, size if nFeeds.
        FeedConfig(const casa::Matrix<casa::Quantity>& offsets,
                   const casa::Vector<casa::String>& pols);

        /// @brief copy constructor
        /// @details It is necessary to have copy constructor due to reference semantics of 
        /// casacore arrays. 
        /// @param[in] other object to copy from
        FeedConfig(const FeedConfig &other);

        /// @brief assignment operator
        /// @details It is necessary to have copy constructor due to reference semantics of 
        /// casacore arrays. 
        /// @param[in] other object to copy from
        FeedConfig& operator=(const FeedConfig &other);

        /// @brief Number of reciever elements. This may be for example two for
        /// a single pixel feed, or 36 for a PAF with 36 synthetic beams.
        casa::uInt nFeeds(void) const;

        /// @brief The X-offset of the feed given by "i".
        casa::Quantity offsetX(casa::uInt i) const;

        /// @brief The Y-offset of the feed given by "i".
        casa::Quantity offsetY(casa::uInt i) const;

        /// @brief The polarisation of the feed given by "i".
        casa::String pol(casa::uInt i) const;

        /// @brief Obtain X and Y offsets for all beams
        /// @details This is a helper method to extract all offsets at once in the 
        /// format of the VisChunk buffer (i.e. 2 x nBeam matrix with offsets in radians).
        /// It is not clear whether this method is going to be useful long term
        /// @param[in] buffer the matrix to fill. It is resized, if necessary.
        void fillMatrix(casa::Matrix<casa::Double> &buffer) const;

    private:
        casa::Matrix<casa::Quantity> itsOffsets;
        casa::Vector<casa::String> itsPols;
};

}
}
}

#endif

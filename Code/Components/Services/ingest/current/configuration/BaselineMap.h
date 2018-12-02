/// @file BaselineMap.h
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
/// Original code by Ben Humphreys

#ifndef ASKAP_CP_INGEST_BASELINEMAP_H
#define ASKAP_CP_INGEST_BASELINEMAP_H

// System include
#include <map>
#include <stdint.h>

// boost includes
#include "boost/tuple/tuple.hpp"
#include "boost/optional.hpp"

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "casacore/measures/Measures/Stokes.h"
#include "askap/AskapError.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Maps the baseline id, as is supplied in the VisDatagram by the
/// Correlator IOC, to a pair of antennas and a correlation product.
///
/// Below is the complete entry for an example 3-antenna system:
/// <PRE>
/// baselinemap.baselineids            = [1..21]
///
/// baselinemap.1                      = [0, 0, XX]
/// baselinemap.2                      = [0, 0, XY]
/// baselinemap.3                      = [0, 1, XX]
/// baselinemap.4                      = [0, 1, XY]
/// baselinemap.5                      = [0, 2, XX]
/// baselinemap.6                      = [0, 2, XY]
/// baselinemap.7                      = [0, 0, YY]
/// baselinemap.8                      = [0, 1, YX]
/// baselinemap.9                      = [0, 1, YY]
/// baselinemap.10                     = [0, 2, YX]
/// baselinemap.11                     = [0, 2, YY]
///
/// baselinemap.12                     = [1, 1, XX]
/// baselinemap.13                     = [1, 1, XY]
/// baselinemap.14                     = [1, 2, XX]
/// baselinemap.15                     = [1, 2, XY]
/// baselinemap.16                     = [1, 1, YY]
/// baselinemap.17                     = [1, 2, YX]
/// baselinemap.18                     = [1, 2, YY]
///
/// baselinemap.19                     = [2, 2, XX]
/// baselinemap.20                     = [2, 2, XY]
/// baselinemap.21                     = [2, 2, YY]
/// </PRE>
///
/// @note (MV): I think this class needs to be redesigned when we have
/// ASKAP going with a decent number of antennas. It seems better to
/// keep it as it is for now and it helps with sparse arrays during the
/// ADE roll out
class BaselineMap {
    public:

        /// @brief Constructor.
        /// @param[in] parset   a parset (i.e. a map from string to string)
        ///     describing the range of entries and the contents of the entries.
        ///     An example is shown in the class comments.
        explicit BaselineMap(const LOFAR::ParameterSet& parset);

        /// @brief Empty Constructor
        BaselineMap();

        /// Given a baseline is, return antenna 1
        ///
        /// @param[in] id   the baseline id.
        /// @return antennaid, or -1 in the case the baseline id mapping
        ///         does not exist.
        int32_t idToAntenna1(const int32_t id) const;

        /// Given a baseline is, return antenna 2
        ///
        /// @param[in] id   the baseline id.
        /// @return antennaid, or -1 in the case the baseline id mapping
        ///         does not exist.
        int32_t idToAntenna2(const int32_t id) const;

        /// Given a baseline is, return the stokes type
        ///
        /// @param[in] id   the baseline id.
        /// @return stokes type, or Stokes::Undefined in the case the baseline
        ///         id mapping does not exist.
        casa::Stokes::StokesTypes idToStokes(const int32_t id) const;

        /// Returns the number of entries in the map
        size_t size() const;

        /// @brief obtain largest id
        /// @details This is required to initialise a flat array buffer holding
        /// derived per-id information because the current implementation does not
        /// explicitly prohibits sparse ids.
        /// @return the largest id setup in the map
        int32_t maxID() const;

        /// @brief find an id matching baseline/polarisation description
        /// @details This is the reverse look-up operation.
        /// @param[in] ant1 index of the first antenna
        /// @param[in] ant2 index of the second antenna
        /// @param[in] pol polarisation product
        /// @return the index of the selected baseline/polarisation, or -1 if
        ///         the selected baseline/polarisation does not exist in the map.
        int32_t getID(const int32_t ant1, const int32_t ant2, const casa::Stokes::StokesTypes pol) const;


        /// @brief correlator produces lower triangle?
        /// @return true if ant2<=ant1 for all ids
        bool isLowerTriangle() const;

        /// @brief correlator produces upper triangle?
        /// @return true if ant1<=ant2 for all ids
        bool isUpperTriangle() const;

        /// @brief take slice for a subset of indices
        /// @details This method is probably temporary as it is primarily intended for ADE
        /// commissioning. When we have a decent number of ASKAP antennas ready, this
        /// additional layer of mapping needs to be removed as it is a complication. 
        /// This method produces a sparse map which includes only selected antenna indices.
        /// @param[in] ids vector with antenna ids to keep
        /// @note For simplicity, indices should always be given in increasing order. This
        /// ensures that no need for data conjugation arises at the user side (i.e. upper
        /// and lower triangles will remain such). 
        void sliceMap(const std::vector<int32_t> &ids);

    protected:

        /// @brief add one product to the map
        /// @param[in] id product number to add the mapping for
        /// @param[in] ant1 first antenna index
        /// @param[in] ant2 second antenna index
        /// @param[in] pol stokes parameter
        void add(int32_t id, int32_t ant1, int32_t ant2, casa::Stokes::StokesTypes pol);

        /// @brief populate map for ADE correlator
        /// @details To avoid carrying the map for 2628 products explicitly in fcm, 
        /// we use this method to define the full map analytically. In the future,
        /// we might even have a polymorphic class which does mapping analytically.
        /// This could even speed things up. However, at this stage, an option to
        /// support sparse arrays is more useful, so we keep the full map functionality
        /// in and just generate the map algorithmically.
        /// @param[in] nAnt number of antennas to generate the map for
        void defaultMapADE(uint32_t nAnt = 36);

    private:

        size_t itsSize;

        // correlator product descriptor, i.e. ant1, ant2 and polarisation type
        typedef boost::tuple<int32_t, int32_t, casa::Stokes::StokesTypes> ProductDesc;

        /// @brief mao of correlator product (baselineid) to descriptor
        std::map<int32_t, ProductDesc> itsMap;
 
        /// @brief cached iterator for a faster access
        mutable boost::optional<std::map<int32_t, ProductDesc>::const_iterator> itsCachedProduct;

        /// @brief caching method for itsCachedProduct
        /// @detail Calling this method sets itsCachedProduct
        /// @param[in] id   the product (baseline) id
        void syncProductCache(int32_t id) const;


        /// @brief flag that antenna1 index is not greater than antenna2 index for all products
        /// @return true, if ant1<=ant2 for all defined "baseline tuples", i.e. if the correlator
        /// produces upper triangle of products (considering products as mathematical matrix)
        /// @note this method is used to set up optimal row layout to avoid the need to conjugate data
        bool itsUpperTriangle;

        /// @brief flag that antenna2 index is not greater than antenna1 index for all products
        /// @return true, if ant2<=ant1 for all defined "baseline tuples", i.e. if the correlator
        /// produces lower triangle of products (considering products as mathematical matrix)
        /// @note this method is used to set up optimal row layout to avoid the need to conjugate data
        bool itsLowerTriangle;
        
};


};
};
};
#endif

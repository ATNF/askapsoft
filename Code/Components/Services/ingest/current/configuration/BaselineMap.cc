/// @file BaselineMap.cc
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

// Include own header file first
#include "BaselineMap.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <map>
#include <stdint.h>

// boost include
#include <boost/tuple/tuple_comparison.hpp>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "Common/ParameterSet.h"
#include "casacore/measures/Measures/Stokes.h"

// Using
using namespace askap;
using namespace askap::utility;
using namespace askap::cp;
using namespace askap::cp::ingest;
using namespace casa;

BaselineMap::BaselineMap()
{
}

BaselineMap::BaselineMap(const LOFAR::ParameterSet& parset) : itsUpperTriangle(true), itsLowerTriangle(true)
{
    const std::string configName = parset.getString("name", "");
    if (configName == "standard") {
        ASKAPCHECK(!parset.isDefined("baselineids"), 
              "The baseline map has to be either defined explicitly via baselineids keyword or via name, you have both");
        defaultMapADE();
    } else {
        ASKAPCHECK(configName == "", "Only 'standard' implicit baseline map is currently supported; you have="<<configName);
       
        const vector<int32_t> ids = parset.getInt32Vector("baselineids", true);
        itsSize = ids.size();

        for (vector<int32_t>::const_iterator it = ids.begin();
                it != ids.end(); ++it) {
            const int32_t id = *it;
            if (!parset.isDefined(toString(id))) {
                ASKAPTHROW(AskapError, "Baseline mapping for id " << id << " not present");
            } 

            const vector<string> tuple = parset.getStringVector(toString(id));
            if (tuple.size() != 3) {
                ASKAPTHROW(AskapError, "Baseline mapping for id " << id << " is malformed");
            }

            const int32_t ant1 = fromString<int32_t>(tuple[0]); 
            const int32_t ant2 = fromString<int32_t>(tuple[1]); 

            add(id, ant1, ant2, Stokes::type(tuple[2]));
        }
    } 
    ASKAPCHECK(itsMap.size() == itsSize, "Failed to initialise baseline map");
}

/// @brief populate map for ADE correlator
/// @details To avoid carrying the map for 2628 products explicitly in fcm, 
/// we use this method to define the full map analytically. In the future,
/// we might even have a polymorphic class which does mapping analytically.
/// This could even speed things up. However, at this stage, an option to
/// support sparse arrays is more useful, so we keep the full map functionality
/// in and just generate the map algorithmically.
/// @param[in] nAnt number of antennas to generate the map for
void BaselineMap::defaultMapADE(uint32_t nAnt) 
{
   ASKAPASSERT(itsMap.size() == 0); 
   ASKAPDEBUGASSERT(nAnt > 0);
   // we have 1-based product indices, so use prefix increment
   itsSize = 0;
   for (int32_t ant2 = 0; ant2 < static_cast<int32_t>(nAnt); ++ant2) {
        for (int32_t ant1 = 0; ant1 < ant2; ++ant1) {
             add(++itsSize, ant1, ant2, casacore::Stokes::XX);
             add(++itsSize, ant1, ant2, casacore::Stokes::YX);
        }
        add(++itsSize, ant2, ant2, casacore::Stokes::XX);
        for (int32_t ant1 = 0; ant1 <= ant2; ++ant1) {
             add(++itsSize, ant1, ant2, casacore::Stokes::XY);
             add(++itsSize, ant1, ant2, casacore::Stokes::YY);
        }
   }
   // itsSize should be set to the number of products here
}

/// @brief add one product to the map
/// @param[in] id product number to add the mapping for
/// @param[in] ant1 first antenna index
/// @param[in] ant2 second antenna index
/// @param[in] pol stokes parameter
void BaselineMap::add(int32_t id, int32_t ant1, int32_t ant2, casacore::Stokes::StokesTypes pol)
{
   ASKAPCHECK(ant1 >= 0, "Antenna 1 index is supposed to be non-negative");
   ASKAPCHECK(ant2 >= 0, "Antenna 2 index is supposed to be non-negative");
   ASKAPCHECK(id >= 0, "Product id is supposed to be non-negative");

   if (ant1 > ant2) {
       itsUpperTriangle = false;
   }

   if (ant2 > ant1) {
       itsLowerTriangle = false;
   }

   itsMap[id] = boost::make_tuple(ant1,ant2,pol);
}

/// @brief caching method for itsCachedProduct
/// @detail Calling this method sets itsCachedProduct
/// @param[in] id   the product (baseline) id
void BaselineMap::syncProductCache(int32_t id) const 
{
    if (itsCachedProduct && ((*itsCachedProduct)->first == id)) {
        return;
    }
        
    itsCachedProduct = itsMap.find(id);
    
    if ((*itsCachedProduct) == itsMap.end()) {
        itsCachedProduct = boost::none;
    } 
}

int32_t BaselineMap::idToAntenna1(const int32_t id) const
{
    syncProductCache(id);
    return itsCachedProduct ? (*itsCachedProduct)->second.get<0>() : -1;
}

int32_t BaselineMap::idToAntenna2(const int32_t id) const
{
    syncProductCache(id);
    return itsCachedProduct ? (*itsCachedProduct)->second.get<1>() : -1;
}

casacore::Stokes::StokesTypes BaselineMap::idToStokes(const int32_t id) const
{
    syncProductCache(id);
    return itsCachedProduct ? (*itsCachedProduct)->second.get<2>() : Stokes::Undefined;
}

size_t BaselineMap::size() const
{
    return itsSize;
}

/// @brief obtain largest id
/// @details This is required to initialise a flat array buffer holding
/// derived per-id information because the current implementation does not
/// explicitly prohibits sparse ids.
/// @return the largest id setup in the map
int32_t BaselineMap::maxID() const
{
  int32_t result = 0;
  for (std::map<int32_t, ProductDesc>::const_iterator ci = itsMap.begin(); 
       ci != itsMap.end(); ++ci) {
       ASKAPCHECK(ci->first >= 0, "Encountered negative id="<<ci->first);
       result = max(result, ci->first);
  }
  return result;
}

/// @brief find an id matching baseline/polarisation description
/// @details This is the reverse look-up operation.
/// @param[in] ant1 index of the first antenna
/// @param[in] ant2 index of the second antenna
/// @param[in] pol polarisation product
/// @return the index of the selected baseline/polarisation
/// @note an exception is thrown if there is no match
int32_t BaselineMap::getID(const int32_t ant1, const int32_t ant2, const casacore::Stokes::StokesTypes pol) const
{
  const ProductDesc product(ant1, ant2, pol);
  for (std::map<int32_t, ProductDesc>::const_iterator ci = itsMap.begin(); ci!=itsMap.end(); ++ci) {
       if (ci->second == product) {
           return ci->first;
       }
  }
  return -1;
}

/// @brief correlator produces lower triangle?
/// @return true if ant2<=ant1 for all ids
bool BaselineMap::isLowerTriangle() const
{
   return itsLowerTriangle && (itsSize != 0); 
}

/// @brief correlator produces upper triangle?
/// @return true if ant1<=ant2 for all ids
bool BaselineMap::isUpperTriangle() const
{
   return itsUpperTriangle && (itsSize != 0);
}

/// @brief take slice for a subset of indices
/// @details This method is probably temporary as it is primarily intended for ADE
/// commissioning. When we have a decent number of ASKAP antennas ready, this
/// additional layer of mapping needs to be removed as it is a complication. 
/// This method produces a sparse map which includes only selected antenna indices.
/// @param[in] ids vector with antenna ids to keep
/// @note For simplicity, indices should always be given in increasing order. This
/// ensures that no need for data conjugation arises at the user side (i.e. upper
/// and lower triangles will remain such). 
void BaselineMap::sliceMap(const std::vector<int32_t> &ids)
{
   // sanity check on supplied indices
   int32_t largestAntID = -1;

   for (std::map<int32_t, ProductDesc>::const_iterator ci = itsMap.begin(); 
        ci != itsMap.end(); ++ci) {
        
        if (largestAntID < ci->second.get<0>()) {
            largestAntID = ci->second.get<0>();
        }
        if (largestAntID < ci->second.get<1>()) {
            largestAntID = ci->second.get<1>();
        }
        
   }
   ASKAPCHECK(largestAntID >= 0, "Attempting to slice an empty map");


   int32_t previousID = -1;
   for (std::vector<int32_t>::const_iterator ci = ids.begin(); ci != ids.end(); ++ci) {
        ASKAPCHECK(*ci <= largestAntID, "sliceMap received antenna ID="<<*ci<<
                   " which exceeds the largest antenna index encountered in the map = "<<largestAntID);
        ASKAPCHECK(*ci >= 0, "Negative antenna ID encountered in sliceMap call");
        ASKAPCHECK(*ci > previousID, 
            "Antenna indices passed to sliceMap are expected to be in increasing order; you have = "<<ids);
        previousID = *ci;
   }
    
   // taking the slice
   std::map<int32_t, ProductDesc> newMap;

   size_t newSize = 0;  
   
   for (std::map<int32_t, ProductDesc>::const_iterator ci = itsMap.begin(); ci != itsMap.end(); ++ci) {

        //ASKAPDEBUGASSERT(ciAnt2 != itsAntenna2Map.end());
        //ASKAPDEBUGASSERT(ciPol != itsStokesMap.end());

        // indices should match
        const int32_t productID = ci->first;

        ProductDesc newProduct = ci->second;

        int32_t newIndex1 = -1;
        int32_t newIndex2 = -1;
        for (size_t i = 0; i < ids.size(); ++i) {
             if (ids[i] == newProduct.get<0>()) {
                 ASKAPDEBUGASSERT(newIndex1 < 0);
                 newIndex1 = static_cast<int32_t>(i);
             }
             if (ids[i] == newProduct.get<1>()) {
                 ASKAPDEBUGASSERT(newIndex2 < 0);
                 newIndex2 = static_cast<int32_t>(i);
             }
        }
        
        if ((newIndex1 >= 0) && (newIndex2 >= 0)) {
             newProduct.get<0>() = newIndex1;
             newProduct.get<1>() = newIndex2;
             newMap[productID] = newProduct;
             ++newSize;
        }
   }
   
   ASKAPCHECK(newSize > 0, "Taking slice rejected all correlation products in the map from "<<
                           itsSize<<" products available in the map");  

   // assign new maps to this class' data members
   itsMap = newMap;
   itsSize = newSize;
}


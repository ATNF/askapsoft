/// @file CorrProdMap.h
///
/// @copyright (c) 2015 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>
///
/// The file contains functions to map correlation products
/// (also known as "baselines"):
///   correlation product index <-> antenna1, antenna2, polarization product
/// The base number of correlation product and antenna indices are changeable
/// (default = 0).
/// Values for polarization product: 0:XX, 1:XY, 2:YX, 3:YY.
/// The index numbering is according to "revtriangle.txt"

#ifndef ASKAP_CP_CORRPRODMAP_H
#define ASKAP_CP_CORRPRODMAP_H

#include "Permutation.h"

// System include
#include <stdint.h>

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "measures/Measures/Stokes.h"

namespace askap {
namespace cp {

/// @brief Class for correlation product map.

class CorrProdMap 
{
    public:
	
		/// Constructor using default values: 
		/// - antenna base number = 0
		/// - index base number = 0
		CorrProdMap();
		
		/// Constructor using input values.
		/// @param[in] Antenna base number: 0 (default) or 1
		/// @param[in] Correlation product index base number: 0 (default) or 1
		CorrProdMap(const uint32_t antBaseIn, const uint32_t indexBaseIn);
		
		virtual ~CorrProdMap();

		// Set antenna base number.
		// @param[in] Antenna base number: 0 (default) or 1
		//void setAntennaBase (const uint32_t antBase);

		/// @return Antenna base number: 0 (default) or 1
		uint32_t getAntennaBase();

		// Set the base number of correlation product index.
		// @param[in] Correlation product index base number: 0 (default) or 1
		//void setIndexBase (const uint32_t indexBase);

		/// @return Correlation product index base number: 0 (default) or 1
		uint32_t getIndexBase();

		/// @param[in] Antenna count
		/// @return Total number of correlation products
		uint32_t getTotal(const uint32_t nAntenna);

		/// Get correlation product index
		/// @param[in] Antenna 1 (<= antenna 2)
		/// @param[in] Antenna 2 
		/// @param[in] Polarisation product: 0:XX, 1:XY, 2:YX, 3:YY
		/// @return Correlation product index
		uint32_t getIndex(const uint32_t ant1, const uint32_t ant2, 
				const uint32_t polProd);

		/// @param[in] Correlation product index
		/// @return Antenna 1 and 2 (antenna 1 <= antenna 2)
		std::pair<uint32_t,uint32_t> getAntennas(const uint32_t index);
		
		/// @param[in] Correlation product index
		/// @return Polarisation product: 0:XX, 1:XY, 2:YX, 3:YY
		uint32_t getPolarisationProduct(const uint32_t index);
		
		/// @param[in] Correlation product
		/// @param[out] Antenna 1 (<= antenna 2)
		/// @param[out] Antenna 2
		/// @param[out] Polarization product: 0:XX, 1:XY, 2:YX, 3:YY
		/// TO BE DEPRECATED
		int getAntennaAndPolarisationProduct(const uint32_t index, 
				uint32_t& ant1, uint32_t& ant2, uint32_t& polProd);
			
	private:
	
		const uint32_t antBase;		// Antenna base number: 0 (default) or 1
		const uint32_t indexBase;	// Index base number: 0 (default) or 1
		Permutation perm;
		
		// Given antenna and polarity indices, return composite index
		// Antenna index is 0-based
		// Polarity index: 0 = X, 1 = Y
		uint32_t getCompositeIndex(const uint32_t ant, const uint32_t pol);

		// Given composite index, return antenna index.
		// Both are 0-based.
		uint32_t getAntenna(const uint32_t comp);

		// Given composite index, return polarisation index.
		// Both are 0-based.
		uint32_t getPolarisation(const uint32_t comp);

		// Return the product of 2 polarisation.
		// Polarisation: 0=X, Y=1
		// Polarisation product: 0=XX, 1=XY, 2=YX, 3=YY
		uint32_t convertPolarisationToProduct(const uint32_t pol1, 
				const uint32_t pol2);

		// Return the polarisation elements from their product.
		std::pair<uint32_t,uint32_t> convertPolarisationToElements 
				(const uint32_t polProd);
		
#ifdef OUTSIDEASKAP
		void alertAntennaValue(const uint32_t ant);
		
		void alertIndexValue(const uint32_t index);

		void alertPolarisationProductValue(const uint32_t polProd);

		void alertValueOutsideRange(const uint32_t value, 
                const uint32_t minValue, const uint32_t maxValue);
		
		void alertWrongAntennaOrder(const uint32_t ant1, const uint32_t ant2);
#endif

};

};
};

#endif

/// @file CorrProdMap.cc
///
/// @copyright (c) 2010 CSIRO
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


// Allow coding & testing outside ASKAP before deployment
#define ASKAP_DEPLOY

// Include own header file first
#include "CorrProdMap.h"

// Include package level header file
#include "askap_correlatorsim.h"

// System includes
#include <stdint.h>
#include <cmath>

#include "Permutation.h"


#ifdef ASKAP_DEPLOY

#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
using namespace askap;
using namespace askap::utility;
using namespace askap::cp;

#else

#include <iostream>
using namespace std;

#endif


CorrProdMap::CorrProdMap () {
	// base numbers for antenna and index
	ANTBASE = 0;
	INDEXBASE = 0;
}
	
	
CorrProdMap::~CorrProdMap () {;}


/// The namespace "corrProd" contains functions to map correlation products
/// (also known as "baselines"):
///   correlation product index <-> antenna1, antenna2, coupled polarization
/// The base number of correlation product and antenna indices are changeable
/// (default = 0).
/// Values for coupled polarization: 0:XX, 1:XY, 2:YX, 3:YY.


/// Set antenna base number (default = 0).

void CorrProdMap::setAntennaBase (uint32_t antBase) {ANTBASE = antBase;}

uint32_t CorrProdMap::antennaBase () {return (ANTBASE);}


/// Set the base number of correlation product index (default = 0).

void CorrProdMap::setIndexBase (uint32_t indexBase) {INDEXBASE = indexBase;}

uint32_t CorrProdMap::indexBase () {return (INDEXBASE);}


/// Given the number of antennas, return the number of correlator products.

uint32_t CorrProdMap::total (uint32_t nantenna) {
	Permutation perm;
	return (perm.total (nantenna * 2));
}


/// Given the indices of antennas and coupled polarization, 
/// return correlator product index.

int CorrProdMap::getIndex (uint32_t ant1, uint32_t ant2, uint32_t coupledPol, 
        uint32_t& index) {
			
	if ((ant1 < ANTBASE) || (ant2 < ANTBASE)) {
#ifdef ASKAP_DEPLOY
        ASKAPTHROW (AskapError, 
                "Illegal antenna: " << ant1 << ", " << ant2);
#else
		cout << "ERROR in corrProd::getIndex: illegal antenna: " <<
				ant1 << ", " << ant2 << '\n';
#endif
        return -1;
	}
	if (ant1 > ant2) {
#ifdef ASKAP_DEPLOY
        ASKAPTHROW (AskapError, 
		        "antenna1 > antenna2: " << ant1 << " > " << ant2);
#else
		cout << "ERROR in corrProd::getIndex: antenna1 > antenna2: " <<
				ant1 << " > " << ant2 << '\n';
#endif
        return -1;
	}
	if ((coupledPol < 0) || (coupledPol > 3)) {
#ifdef ASKAP_DEPLOY
        ASKAPTHROW (AskapError, 
		        "Illegal coupled polarization: " << coupledPol);
#else
		cout << "ERROR in corrProd::getIndex: illegal coupled polarization: " 
				<< coupledPol << '\n';
#endif
        return -2;
	}
	
	uint32_t pol1, pol2;
	uint32_t comp1, comp2;
	
	// form a composite index from antenna and coupled polarization 
	polarDecouple (coupledPol, pol1, pol2);
	comp1 = compositeIndex (ant1-ANTBASE, pol1);
	comp2 = compositeIndex (ant2-ANTBASE, pol2);
	
	Permutation perm;
	index = perm.index (comp1, comp2) + INDEXBASE;
	
	return 0;
}


/// Given correlator product, return the indices of antennas & coupled 
/// polarization. Values for coupled polarization: 0:XX, 1:XY, 2:YX, 3:YY

int CorrProdMap::getAntennaAndCoupledPolar (uint32_t index, 
		uint32_t& ant1, uint32_t& ant2, uint32_t& coupledPol) {
			
	if (index < INDEXBASE) {
#ifdef ASKAP_DEPLOY
        ASKAPTHROW (AskapError, 
                "Illegal index: " << index);
#else
		cout << "ERROR in corrProd::getIndex: illegal index: " << index << 
                '\n';
#endif
        return -1;
	}
	
	uint32_t comp1, comp2;
	Permutation perm;
	perm.getMembers (index-INDEXBASE, comp1, comp2);
	
	// convert composite index to antenna and coupled polarization 
	ant1 = antenna (comp1) + ANTBASE;
	ant2 = antenna (comp2) + ANTBASE;
	coupledPol = polarCouple (polar(comp1),polar(comp2));
	return 0;
}


// Internal functions ---------------------------------------------------


// Given antenna and polarity indices, return composite index
// Antenna index is 0-based
// Polarity index: 0 = X, 1 = Y

uint32_t CorrProdMap::compositeIndex (uint32_t ant, uint32_t pol) {
	return (2*ant + pol);
}


// Given composite index, return antenna index.
// Both are 0-based.

uint32_t CorrProdMap::antenna (uint32_t comp) {
	return (int(comp/2));
}


// Given composite index, return polarization index.
// Both are 0-based.

uint32_t CorrProdMap::polar (uint32_t comp) {
	return (comp % 2);
}


// Return the coupled index of 2 polarization.
// Polarization: 0=X, Y=1
// Coupled polarization: 0=XX, 1=XY, 2=YX, 3=YY

uint32_t CorrProdMap::polarCouple (uint32_t pol1, uint32_t pol2) {
	return (2*pol1 + pol2);
}


// Return the polarization indices from their coupled index.
 
void CorrProdMap::polarDecouple (uint32_t couple, 
        uint32_t& pol1, uint32_t& pol2) {
	pol1 = int(couple/2);
	pol2 = couple % 2;
}

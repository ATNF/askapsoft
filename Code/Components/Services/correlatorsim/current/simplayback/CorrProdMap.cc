/// @file CorrProdMap.cc
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


// Include own header file first
#include "CorrProdMap.h"

// System includes
#include <stdint.h>
#include <cmath>
#include <iostream>

#include "Permutation.h"

// Include package level header file
#include "askap_correlatorsim.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "askap/AskapError.h"

using namespace askap;
using namespace askap::utility;
using namespace askap::cp;

using namespace std;


CorrProdMap::CorrProdMap() : antBase(0), indexBase(1) 
{
}


CorrProdMap::CorrProdMap(const uint32_t antBaseIn, const uint32_t indexBaseIn) :
		antBase(antBaseIn), indexBase(indexBaseIn) 
{
#ifndef OUTSIDEASKAP
	ASKAPCHECK((antBaseIn == 0) || (antBaseIn == 1), 
			"Illegal antenna base number: " << antBaseIn);
	ASKAPCHECK((indexBaseIn == 0) || (indexBaseIn == 1), 
			"Illegal index base number: " << indexBaseIn);
#endif
}
	
	
CorrProdMap::~CorrProdMap() 
{
}


// Set antenna base number (default = 0). Usual value is either 0 or 1

// void CorrProdMap::setAntennaBase (uint32_t antBaseIn) 
// {
// #ifndef OUTSIDEASKAP
	// ASKAPCHECK ((antBaseIn == 0) || (antBaseIn == 1), 
			// "Illegal antenna base number: " << antBaseIn);
// #endif
	// antBase = antBaseIn;
// }


uint32_t CorrProdMap::getAntennaBase() 
{
	return antBase;
}


// Set the base number of correlation product index (default = 0).
// Usual value is either 0 or 1

// void CorrProdMap::setIndexBase (uint32_t indexBaseIn) 
// {
// #ifndef OUTSIDEASKAP
	// ASKAPCHECK ((indexBaseIn == 0) || (indexBaseIn == 1), 
			// "Illegal index base number: " << indexBaseIn);
// #endif
	// indexBase = indexBaseIn;
// }


uint32_t CorrProdMap::getIndexBase() 
{
	return indexBase;
}


// Given the number of antennas, return the number of correlator products.

uint32_t CorrProdMap::getTotal(const uint32_t nAntenna) 
{
	//Permutation perm;
	return perm.getTotal(nAntenna * 2);	// number of correlation products
}


// Given the indices of antennas and polarization product, 
// return correlation product index.

uint32_t CorrProdMap::getIndex(const uint32_t ant1, const uint32_t ant2, 
		const uint32_t polProd) 
{
#ifdef OUTSIDEASKAP
	alertAntennaValue(ant1);
	alertAntennaValue(ant2);
	alertWrongAntennaOrder(ant1, ant2);
	alertPolarisationProductValue(polProd);
#else
	ASKAPCHECK(ant1 >= antBase, "Illegal antenna 1 index: " << ant1);
	ASKAPCHECK(ant2 >= antBase, "Illegal antenna 2 index: " << ant2);
	ASKAPCHECK(ant1 <= ant2, 
            "Antenna are in the wrong order: " << ant1 << ", " << ant2);
	ASKAPCHECK((polProd >= 0) && (polProd <= 3), 
			"Illegal polarisation product: " << polProd);
#endif
	
	// Rearrange the index format of antennas and polarisation product into 
	// composite indices
	std::pair<uint32_t,uint32_t> pols = convertPolarisationToElements(polProd);
	std::pair<uint32_t,uint32_t> comps;
	comps.first = getCompositeIndex(ant1-antBase, pols.first);
	comps.second = getCompositeIndex(ant2-antBase, pols.second);
	
	// Perform permutation on the composite indices.
	//Permutation perm;
	return (perm.getIndex(comps) + indexBase);
}


// Given correlator product, return antennas.

std::pair<uint32_t,uint32_t> CorrProdMap::getAntennas(const uint32_t index) 
{
#ifdef OUTSIDEASKAP
	alertIndexValue(index);
#else
	ASKAPCHECK(index >= indexBase, "Illegal index value: " << index);
#endif

	// Inverse permutation to get composite indices containing antennas and 
	// polarisation product.
	//Permutation perm;
	std::pair<uint32_t,uint32_t> comps = perm.getItems(index-indexBase);

	// Rearrange the composite indices to get the antennas.
	std::pair<uint32_t,uint32_t> ants;
	ants.first = getAntenna(comps.first) + antBase;
	ants.second = getAntenna(comps.second) + antBase;
	return ants;
}


// Given correlator product, return the polarization product: 
// 0:XX, 1:XY, 2:YX, 3:YY

uint32_t CorrProdMap::getPolarisationProduct(const uint32_t index) 
{
#ifdef OUTSIDEASKAP
	alertIndexValue(index);
#else
	ASKAPCHECK(index >= indexBase, "Illegal index value: " << index);
#endif
	
	// Inverse permutation to get composite indices containing antennas and 
	// polarisation product.
	//Permutation perm;
	std::pair<uint32_t,uint32_t> comps = perm.getItems(index-indexBase);
	
	// Rearrange the composite indices to get the polarisation product.
	return (convertPolarisationToProduct(getPolarisation(comps.first), 
			getPolarisation(comps.second)));
}


/// Given correlator product, return the indices of antennas &  
/// polarization product (0:XX, 1:XY, 2:YX, 3:YY).
/// TO BE DEPRECATED

int CorrProdMap::getAntennaAndPolarisationProduct(const uint32_t index, 
		uint32_t& ant1, uint32_t& ant2, uint32_t& polProd) 
{
#ifdef OUTSIDEASKAP
	alertIndexValue (index);
#else
	ASKAPCHECK(index >= indexBase, "Illegal index value: " << index);
#endif

	//Permutation perm;
	std::pair<uint32_t,uint32_t> comps = perm.getItems(index-indexBase);
	
	// convert composite index to antenna
	ant1 = getAntenna(comps.first) + antBase;
	ant2 = getAntenna(comps.second) + antBase;
	polProd = convertPolarisationToProduct(getPolarisation(comps.first), 
			getPolarisation(comps.second));
	return 0;
}


// Internal functions ---------------------------------------------------


// Given antenna and polarity indices, return composite index
// Antenna index is 0-based
// Polarity index: 0 = X, 1 = Y

uint32_t CorrProdMap::getCompositeIndex(const uint32_t ant, const uint32_t pol) 
{
	return (2*ant + pol);
}


// Given composite index, return antenna index.
// Both are 0-based.

uint32_t CorrProdMap::getAntenna(const uint32_t comp) 
{
	return (int(comp/2));
}


// Given composite index, return polarization index.
// Both are 0-based.

uint32_t CorrProdMap::getPolarisation(const uint32_t comp) 
{
	return (comp % 2);
}


// Return the product of 2 polarization.
// Polarization: 0=X, Y=1
// Polarization product: 0=XX, 1=XY, 2=YX, 3=YY

uint32_t CorrProdMap::convertPolarisationToProduct(const uint32_t pol1, 
        const uint32_t pol2) 
{
	return (2*pol1 + pol2);
}


// Return the polarization elements from their product.
 
std::pair<uint32_t,uint32_t> CorrProdMap::convertPolarisationToElements 
        (const uint32_t polProd) 
{
	std::pair<uint32_t,uint32_t> pols;
	pols.first = int(polProd/2);
	pols.second = polProd % 2;
	return pols;
}


#ifdef OUTSIDEASKAP

// Alert when antenna index value goes out of range.
void CorrProdMap::alertAntennaValue(const uint32_t ant) 
{
	//if ((ant < antBase) || (ant > antBase + nAnt - 1)) {
	if (ant < antBase) {
		cerr << "ERROR in CorrProdMap: illegal antenna: " << ant << endl;
	}
}


// Alert when index value goes out of range.
void CorrProdMap::alertIndexValue(const uint32_t index) 
{
	//if ((index < indexBase) || (index > indexBase + nTotal - 1)) {
	if (index < indexBase) {
		cerr << "ERROR in CorrProdMap: illegal index: " << index << endl;
	}
}


// Alert when polarisation product value goes out of range.
void CorrProdMap::alertPolarisationProductValue(const uint32_t polProd) 
{
	if ((polProd < 0) || (polProd > 3)) {
		cerr << "ERROR in CorrProdMap: illegal polarisation product value: " << 
				polProd << endl;
	}
}


// Alert when a value goes out of range.
void CorrProdMap::alertValueOutsideRange(const uint32_t value, 
		const uint32_t minValue, const uint32_t maxValue) 
{
	if ((value < minValue) || (value > maxValue)) {
		cerr << "ERROR in CorrProdMap: value is outside range: " << 
            value << endl;
	}
}


// Alert when antenna order is wrong
void CorrProdMap::alertWrongAntennaOrder(const uint32_t ant1, const uint32_t ant2) 
{
	if (ant1 > ant2) {
		cerr << "ERROR in CorrProdMap: wrong antenna order: " << 
            ant1 << " > " << ant2 << endl;
	}
}

#endif

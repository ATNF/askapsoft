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
//#define OUTSIDEASKAP

// Include own header file first
#include "CorrProdMap.h"

// System includes
#include <stdint.h>
#include <cmath>
#include <iostream>

#include "Permutation.h"


#ifndef OUTSIDEASKAP

// Include package level header file
#include "askap_correlatorsim.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"

using namespace askap;
using namespace askap::utility;
using namespace askap::cp;
#endif

using namespace std;


CorrProdMap::CorrProdMap () 
{
	antBase = 0;	// default base number. Usual value is either 0 or 1
	indexBase = 0;	// default base number. Usual value is either 0 or 1
	//nAnt = nAntIn;	// number of antennas
	//Permutation perm;
	//nTotal = perm.total (nAnt*2);	// number of correlation products
}
	
	
CorrProdMap::~CorrProdMap () 
{
}


/// The namespace "corrProd" contains functions to map correlation products
/// (also known as "baselines"):
///   correlation product index <-> antenna1, antenna2, polarization product
/// The base number of correlation product and antenna indices are changeable
/// (default = 0).
/// Values for polarization product: 0:XX, 1:XY, 2:YX, 3:YY.


/// Set antenna base number (default = 0). Usual value is either 0 or 1

void CorrProdMap::setAntennaBase (uint32_t antBaseIn) 
{
	antBase = antBaseIn;
}

uint32_t CorrProdMap::getAntennaBase () 
{
	return antBase;
}


/// Set the base number of correlation product index (default = 0).
/// Usual value is either 0 or 1

void CorrProdMap::setIndexBase (uint32_t indexBaseIn) 
{
	indexBase = indexBaseIn;
}

uint32_t CorrProdMap::getIndexBase () 
{
	return indexBase;
}


/// Given the number of antennas, return the number of correlator products.

uint32_t CorrProdMap::totalCount (uint32_t nAntenna) 
{
	Permutation perm;
	return perm.total (nAntenna * 2);	// number of correlation products
}


/// Given the indices of antennas and polarization product, 
/// return correlation product index.

uint32_t CorrProdMap::getIndex (uint32_t ant1, uint32_t ant2, 
        uint32_t polProd) 
{
	alertAntennaValue (ant1);
	alertAntennaValue (ant2);
	alertWrongAntennaOrder (ant1, ant2);
	alertPolarisationProductValue (polProd);
	
	// Rearrange the index format of antennas and polarisation product into 
	// composite indices
	std::pair<uint32_t,uint32_t> pols = polarisationElement (polProd);
	std::pair<uint32_t,uint32_t> comps;
	comps.first = compositeIndex (ant1-antBase, pols.first);
	comps.second = compositeIndex (ant2-antBase, pols.second);
	
	// Perform permutation on the composite indices.
	Permutation perm;
	return (perm.indexNoCheck (comps) + indexBase);
}


/// Given correlator product, return antennas.

std::pair<uint32_t,uint32_t> CorrProdMap::getAntennas (uint32_t index) 
{
	alertIndexValue (index);
	
	// Inverse permutation to get composite indices containing antennas and 
	// polarisation product.
	Permutation perm;
	std::pair<uint32_t,uint32_t> comps = perm.itemsNoCheck (index-indexBase);

	// Rearrange the composite indices to get the antennas.
	std::pair<uint32_t,uint32_t> ants;
	ants.first = antenna (comps.first) + antBase;
	ants.second = antenna (comps.second) + antBase;
	return ants;
}


/// Given correlator product, return the polarization product: 
/// 0:XX, 1:XY, 2:YX, 3:YY

uint32_t CorrProdMap::getPolarisationProduct (uint32_t index) 
{
	alertIndexValue (index);
	
	// Inverse permutation to get composite indices containing antennas and 
	// polarisation product.
	Permutation perm;
	std::pair<uint32_t,uint32_t> comps = perm.itemsNoCheck (index-indexBase);
	
	// Rearrange the composite indices to get the polarisation product.
	return (polarisationProduct (polarisation(comps.first), polarisation(comps.second)));
}


/// Given correlator product, return the indices of antennas &  
/// polarization product (0:XX, 1:XY, 2:YX, 3:YY).
/// TO BE DEPRECATED

int CorrProdMap::getAntennaAndPolarisationProduct (uint32_t index, 
		uint32_t& ant1, uint32_t& ant2, uint32_t& polProd) 
{
	alertIndexValue (index);

	Permutation perm;
	std::pair<uint32_t,uint32_t> comps = perm.itemsNoCheck (index-indexBase);
	
	// convert composite index to antenna
	ant1 = antenna (comps.first) + antBase;
	ant2 = antenna (comps.second) + antBase;
	polProd = polarisationProduct (polarisation(comps.first), polarisation(comps.second));
	return 0;
}


// Internal functions ---------------------------------------------------


// Given antenna and polarity indices, return composite index
// Antenna index is 0-based
// Polarity index: 0 = X, 1 = Y

uint32_t CorrProdMap::compositeIndex (uint32_t ant, uint32_t pol) 
{
	return (2*ant + pol);
}


// Given composite index, return antenna index.
// Both are 0-based.

uint32_t CorrProdMap::antenna (uint32_t comp) 
{
	return (int(comp/2));
}


// Given composite index, return polarization index.
// Both are 0-based.

uint32_t CorrProdMap::polarisation (uint32_t comp) 
{
	return (comp % 2);
}


// Return the product of 2 polarization.
// Polarization: 0=X, Y=1
// Polarization product: 0=XX, 1=XY, 2=YX, 3=YY

uint32_t CorrProdMap::polarisationProduct (uint32_t pol1, uint32_t pol2) 
{
	return (2*pol1 + pol2);
}


// Return the polarization elements from their product.
 
std::pair<uint32_t,uint32_t> CorrProdMap::polarisationElement (uint32_t polProd) 
{
	std::pair<uint32_t,uint32_t> pols;
	pols.first = int(polProd/2);
	pols.second = polProd % 2;
	return pols;
}


// Alert when antenna index value goes out of range.
void CorrProdMap::alertAntennaValue (uint32_t ant) 
{
	//if ((ant < antBase) || (ant > antBase + nAnt - 1)) {
	if (ant < antBase) {
		cerr << "ERROR in CorrProdMap: illegal antenna: " << ant << endl;
	}
}


// Alert when index value goes out of range.
void CorrProdMap::alertIndexValue (uint32_t index) 
{
	//if ((index < indexBase) || (index > indexBase + nTotal - 1)) {
	if (index < indexBase) {
		cerr << "ERROR in CorrProdMap: illegal index: " << index << endl;
	}
}


// Alert when polarisation product value goes out of range.
void CorrProdMap::alertPolarisationProductValue (uint32_t polProd) 
{
	if ((polProd < 0) || (polProd > 3)) {
		cerr << "ERROR in CorrProdMap: illegal polarisation product value: " << 
				polProd << endl;
	}
}


// Alert when a value goes out of range.
void CorrProdMap::alertValueOutsideRange (uint32_t value, uint32_t minValue, 
		uint32_t maxValue) 
{
	if ((value < minValue) || (value > maxValue)) {
		cerr << "ERROR in CorrProdMap: value is outside range: " << 
            value << endl;
	}
}


// Alert when antenna order is wrong
void CorrProdMap::alertWrongAntennaOrder (uint32_t ant1, uint32_t ant2) 
{
	if (ant1 > ant2) {
		cerr << "ERROR in CorrProdMap: wrong antenna order: " << 
            ant1 << " > " << ant2 << endl;
	}
}

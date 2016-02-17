/// @file Permutation.cc
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
///

// Allow coding & testing outside ASKAP before deployment
//#define OUTSIDEASKAP

// Include own header file first
#include "Permutation.h"

// System includes
#include <iostream>
#include <stdint.h>
#include <cmath>

#ifndef OUTSIDEASKAP
#include "askap/AskapError.h"

using namespace askap;
using namespace askap::cp;
#endif

using namespace std;

#ifdef OUTSIDEASKAP
// forward declaration of internal functions
void alertValueTooBig(uint32_t value, uint32_t maxValue);
void alertValueOutsideRange(uint32_t value, uint32_t minValue, 
        uint32_t maxValue);
void alertWrongItemOrder(uint32_t item1, uint32_t item2);
void alertWrongItemOrder(std::pair<uint32_t,uint32_t> items);
#endif


Permutation::Permutation() 
{
}


//Permutation::~Permutation() 
//{
//}


// Return the total number of permutation.

uint32_t const Permutation::getTotal(const uint32_t n) const
{
	return ((n*n + n)/2);
}


// Given the items, return the permutation index.

uint32_t const Permutation::getIndex(const uint32_t item1, 
		const uint32_t item2) const
{
#ifdef OUTSIDEASKAP
	alertWrongItemOrder(item1, item2);
#else
	ASKAPCHECK(item1 <= item2, "Illegal item order: " << item1 << 
            ", " << item2);
#endif
	return (item1 + getTotal(item2));
}


// Given the items, return the permutation index.

uint32_t const Permutation::getIndex
		(const std::pair<uint32_t,uint32_t> items) const
{
#ifdef OUTSIDEASKAP
	alertWrongItemOrder(items);
#else
	ASKAPCHECK(items.first <= items.second, 
			"Illegal item order: " << items.first << ", " << items.second);
#endif
	return (items.first + getTotal(items.second));
}


// Given the items, return the permutation index. 

uint32_t const Permutation::getIndex(const uint32_t item1, 
		const uint32_t item2, const uint32_t nItem) const
{	
#ifdef OUTSIDEASKAP
	alertValueTooBig(item1, nItem-1);
	alertValueTooBig(item2, nItem-1);
	alertWrongItemOrder(item1, item2);
#else
	ASKAPCHECK(item1 <= nItem-1, "Illegal item 1 value: " << item1);
	ASKAPCHECK(item2 <= nItem-1, "Illegal item 2 value: " << item2);
	ASKAPCHECK(item1 <= item2, "Illegal item order: " << item1 << ", " << item2);
#endif
    return (item1 + getTotal(item2));
}


// Given the items, return the permutation index. 

uint32_t const Permutation::getIndex(const std::pair<uint32_t,uint32_t> items, 
		const uint32_t nItem) const
{
#ifdef OUTSIDEASKAP
	alertValueTooBig(items.first, nItem-1);
	alertValueTooBig(items.second, nItem-1);
	alertWrongItemOrder(items);
#else
	ASKAPCHECK(items.first <= nItem-1, "Illegal item 1 value: " << items.first);
	ASKAPCHECK(items.second <= nItem-1, "Illegal item 2 value: " << items.second);
	ASKAPCHECK(items.first <= items.second, 
			"Illegal item order: " << items.first << ", " << items.second);
#endif
	return (items.first + getTotal(items.second));
}


// Given the permutation index, return the items. 

std::pair<uint32_t,uint32_t> const 
		Permutation::getItems(const uint32_t index) const
{	
	std::pair<uint32_t,uint32_t> items;
	items.second = int((sqrt(1+8*index) - 1)/2);
	items.first = index - getTotal(items.second);
	return items;
}


// Internal functions

#ifdef OUTSIDEASKAP

// Alert when a value is too big.
void alertValueTooBig(const uint32_t value, const uint32_t maxValue) const
{
	if (value > maxValue) {
		cerr << "ERROR in Permutation: value is too big: " << value << endl;
	}
}


// Alert when a value goes out of range.
void alertValueOutsideRange(const uint32_t value, const uint32_t minValue, 
		const uint32_t maxValue) const
{
	if ((value < minValue) || (value > maxValue)) {
		cerr << "ERROR in Permutation: value is outside range: " << value << 
            endl;
	}
}


// Alert when the order of items is wrong (item1 > item2)
void alertWrongItemOrder(const uint32_t item1, const uint32_t item2) const
{
	if (item1 > item2) {
		cerr << "ERROR in Permutation: wrong item order: " << item1 << 
				" > " << item2 << endl;
	}
}


void alertWrongItemOrder(const std::pair<uint32_t,uint32_t> items) const
{
	if (items.first > items.second) {
		cerr << "ERROR in Permutation: wrong item order: " << 
            items.first << " > " << items.second << endl;
	}
}

#endif

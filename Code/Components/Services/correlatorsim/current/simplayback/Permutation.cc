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

//#define OUTSIDEASKAP

// Include own header file first
#include "Permutation.h"

// System includes
#include <iostream>
#include <stdint.h>
#include <cmath>

using namespace std;

#ifndef OUTSIDEASKAP
using namespace askap;
using namespace askap::cp;
#endif


// forward declaration of internal functions
void alertValueOutsideRange (uint32_t value, uint32_t minValue, 
        uint32_t maxValue);
void alertWrongItemOrder (uint32_t item1, uint32_t item2);
void alertWrongItemOrder (std::pair<uint32_t,uint32_t> items);


Permutation::Permutation () {;}


Permutation::~Permutation() {;}


/// Return the total number of permutation.
uint32_t Permutation::total (const uint32_t n) {
	return ((n*n + n)/2);
}


/// Given the items, return the permutation index. With no input check.
uint32_t Permutation::indexNoCheck (const uint32_t item1, const uint32_t item2) {
	return (item1 + total(item2));
}

uint32_t Permutation::indexNoCheck (const std::pair<uint32_t,uint32_t> items) {
	return (items.first + total(items.second));
}


/// Given the items, return the permutation index. Input is checked.
/// The correct input ordering is enforced.

uint32_t Permutation::index (const uint32_t item1, const uint32_t item2,
		const uint32_t nItem) {
	
	alertValueOutsideRange (item1, 0, nItem-1);
	alertValueOutsideRange (item2, 0, nItem-1);
	alertWrongItemOrder (item1, item2);
    return (item1 + total(item2));
}

uint32_t Permutation::index (const std::pair<uint32_t,uint32_t> items, 
		const uint32_t nItem) {
	
	alertValueOutsideRange (items.first, 0, nItem-1);
	alertValueOutsideRange (items.second, 0, nItem-1);
	alertWrongItemOrder (items);
	return (items.first + total(items.second));
}


/// Given the permutation index, return the items. With no input check.
std::pair<uint32_t,uint32_t> Permutation::itemsNoCheck (const uint32_t index) {
	
	std::pair<uint32_t,uint32_t> items;
	items.second = int((sqrt(1+8*index) - 1)/2);
	items.first = index - total(items.second);
	return items;
}


/// Given the permutation index, return the items.
std::pair<uint32_t,uint32_t> Permutation::items (const uint32_t index, 
		const uint32_t nItem) {
	
	alertValueOutsideRange (index, 0, total(nItem)-1);
	std::pair<uint32_t,uint32_t> items;
	items.second = int((sqrt(1+8*index) - 1)/2);
	items.first = index - total(items.second);
	return items;
}


// Internal functions


// Alert when a value goes out of range.
void alertValueOutsideRange (uint32_t value, uint32_t minValue, 
        uint32_t maxValue) {
	if ((value < minValue) || (value > maxValue)) {
		cerr << "ERROR in Permutation: value is outside range: " << value << 
            endl;
	}
}


// Alert when the order of items is wrong (item1 > item2)
void alertWrongItemOrder (uint32_t item1, uint32_t item2) {
	if (item1 > item2) {
		cerr << "ERROR in Permutation: wrong item order: " << item1 << 
				" > " << item2 << endl;
	}
}

void alertWrongItemOrder (std::pair<uint32_t,uint32_t> items) {
	if (items.first > items.second) {
		cerr << "ERROR in Permutation: wrong item order: " << 
            items.first << " > " << items.second << endl;
	}
}

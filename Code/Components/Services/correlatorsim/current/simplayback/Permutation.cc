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

//#define PERMUTATION_DEBUG

// Include own header file first
#include "Permutation.h"

// System includes
#include <iostream>
#include <stdint.h>
#include <cmath>

using namespace std;

#ifndef PERMUTATION_DEBUG
using namespace askap;
using namespace askap::cp;

//ASKAP_LOGGER(logger, ".Permutation");
#endif

Permutation::Permutation () {;}

Permutation::~Permutation () {;}


/// Return the total number of permutation.

uint32_t Permutation::total (uint32_t n) {
	return ((n*n + n)/2);
}


/// Given the members, return the permutation index.

uint32_t Permutation::index (uint32_t a, uint32_t b) {
	return (a + total(b));
}


/// Given the permutation index, return the members.

void Permutation::getMembers (uint32_t index, uint32_t& a, uint32_t& b) {
	b = int((sqrt(1+8*index) - 1)/2);
	a = index - total(b);
}


#ifdef PERMUTATION_DEBUG

// Test permutation for a given item number.
// Check whether the 2-way mapping between index and members work.

int main () {
	
	#include "Permutation.h"

	const uint32_t n = 7200;
	
	uint32_t a, b;
	uint32_t icheck;
	uint32_t nerror = 0;
	
	Permutation perm;
	
	uint32_t nperm = perm.total(n);
	cout << "Total permutations of " << n << " items is " << nperm << '\n';
	
	for (int i=0; i<nperm; i++) {
		perm.getMembers (i, a, b);
		icheck = perm.index (a,b);
		//cout << i << ": " << a << ", " << b << ": " << icheck << '\n';
		if (icheck != i) {
			cout << "ERROR in Permutation: " << i << " != " << 
                    icheck << '\n';
			nerror++;
		}
	}
	cout << "nerror: " << nerror << '\n';
}

#endif

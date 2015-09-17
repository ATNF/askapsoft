/// @file corrProduct.cc
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
/// The file contains functions to map correlation products
/// (also known as "baselines"):
///   correlation product index <-> antenna1, antenna2, coupled polarization
/// The base number of correlation product and antenna indices are changeable
/// (default = 0).
/// Values for coupled polarization: 0:XX, 1:XY, 2:YX, 3:YY.
/// The index numbering is according to "revtriangle.txt"
//

//#define PERMUTATION_DEBUG
//#define CORRPROD_DEBUG

#include <iostream>
#include <cstdint>
#include <cmath>

#ifdef CORRPROD_DEBUG
#include <time.h>
#endif

using namespace std;


/// The namespace "permutation" contains generic functions to map index
/// for 2-permutation of n items:
///   permutation index <-> component indices
/// The permutation is posed as the lower triangle of a table.
///      0     1     2
///   +-------------------
/// 0 | 00=0   .     .
/// 1 | 01=1  11=2   .
/// 2 | 02=3  12=4  22=5 
///
/// Note that the functions will become more complex for upper triangle.

namespace permutation {
	
/// Return the total number of permutation.
uint32_t total (uint32_t n) {
	return ((n*n + n)/2);
}


/// Given the components, return the permutation index.
uint32_t index (uint32_t a, uint32_t b) {
	return (a + total(b));
}


/// Given the permutation index, return the components.
void getComponents (uint32_t ab, uint32_t& a, uint32_t& b) {
	b = int((sqrt(1+8*ab) - 1)/2);
	a = ab - total(b);
}


#ifdef PERMUTATION_DEBUG

// Test permutation for a given item number.
// Check whether the 2-way mapping between index and components work.
int test (int n) {
	uint32_t nperm = total(n);
	uint32_t a, b;
	uint32_t icheck;
	uint32_t nerror = 0;
	
	cout << "Total permutations of " << n << " items is " << nperm << '\n';
	for (int i=0; i<nperm; i++) {
		getComponents (i, a, b);
		icheck = index (a,b);
		//cout << i << ": " << a << ", " << b << ": " << icheck << '\n';
		if (icheck != i) {
			cout << "ERROR in permutation::test: " << i << " != " << 
                    icheck << '\n';
			nerror++;
		}
	}
	if (nerror > 0) return -1;
	else return 0;
}

#endif

}	// namespace permutation



/// The namespace "corrProd" contains functions to map correlation products
/// (also known as "baselines"):
///   correlation product index <-> antenna1, antenna2, coupled polarization
/// The base number of correlation product and antenna indices are changeable
/// (default = 0).
/// Values for coupled polarization: 0:XX, 1:XY, 2:YX, 3:YY.

namespace corrProd {

// base numbers for antenna and index
uint32_t ANTBASE = 0;
uint32_t INDEXBASE = 0;

// forward declaration
uint32_t compositeIndex (uint32_t ant, uint32_t pol);
uint32_t antenna (uint32_t comp);
uint32_t polar (uint32_t comp);
uint32_t polarCouple (uint32_t pol1, uint32_t pol2);
void polarDecouple (uint32_t couple, uint32_t& pol1, uint32_t& pol2);


/// Set antenna base number (default = 0).

void setAntennaBase (uint32_t antBase) {ANTBASE = antBase;}

uint32_t getAntennaBase () {return (ANTBASE);}


/// Set the base number of correlation product index (default = 0).

void setIndexBase (uint32_t indexBase) {INDEXBASE = indexBase;}

uint32_t getIndexBase () {return (INDEXBASE);}


/// Given the number of antennas, return the number of correlator products.

uint32_t getTotal (uint32_t nantenna) {
	return (permutation::total (nantenna * 2));
}


/// Given the indices of antennas and coupled polarization, 
/// return correlator product index.

int getIndex (uint32_t ant1, uint32_t ant2, uint32_t coupledPol, 
        uint32_t& index) {
			
	if ((ant1 < ANTBASE) || (ant2 < ANTBASE)) {
		cout << "ERROR in corrProd::getIndex: illegal antenna: " <<
				ant1 << ", " << ant2 << '\n';
		return -1;
	}
	if ((coupledPol < 0) || (coupledPol > 3)) {
		cout << "ERROR in corrProd::getIndex: illegal coupled polarization: " 
				<< coupledPol << '\n';
		return -2;
	}
	
	uint32_t pol1, pol2;
	uint32_t comp1, comp2;
	
	// form a composite index from antenna and coupled polarization 
	polarDecouple (coupledPol, pol1, pol2);
	comp1 = compositeIndex (ant1-ANTBASE, pol1);
	comp2 = compositeIndex (ant2-ANTBASE, pol2);
	
	index = permutation::index (comp1, comp2) + INDEXBASE;
	
	return 0;
}


/// Given correlator product, return the indices of antennas & coupled 
/// polarization. Values for coupled polarization: 0:XX, 1:XY, 2:YX, 3:YY

int getAntennaAndCoupledPolar (uint32_t index, uint32_t& ant1, uint32_t& ant2, 
		uint32_t& coupledPol) {
			
	if (index < INDEXBASE) {
		cout << "ERROR in corrProd::getIndex: illegal index: " << index << 
                '\n';
		return -1;
	}
	
	uint32_t comp1, comp2;
	permutation::getComponents (index-INDEXBASE, comp1, comp2);
	
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

uint32_t compositeIndex (uint32_t ant, uint32_t pol) {
	return (2*ant + pol);
}


// Given composite index, return antenna index.
// Both are 0-based.

uint32_t antenna (uint32_t comp) {
	return (int(comp/2));
}


// Given composite index, return polarization index.
// Both are 0-based.

uint32_t polar (uint32_t comp) {
	return (comp % 2);
}


// Return the coupled index of 2 polarization.
// Polarization: 0=X, Y=1
// Coupled polarization: 0=XX, 1=XY, 2=YX, 3=YY

uint32_t polarCouple (uint32_t pol1, uint32_t pol2) {
	return (2*pol1 + pol2);
}


// Return the polarization indices from their coupled index.
 
void polarDecouple (uint32_t couple, uint32_t& pol1, uint32_t& pol2) {
	pol1 = int(couple/2);
	pol2 = couple % 2;
}


#ifdef CORRPROD_DEBUG
// Test functions --------------------------------------------------


int test (uint32_t nant) {
	cout << "corrProd::test: " << '\n';
	
	uint32_t ntotal = getTotal (nant);
	cout << "antenna count: " << nant << '\n';
	cout << "number of correlation products: " << ntotal << '\n';
	
	uint32_t ant1, ant2, coupledPol, i2;
	uint32_t nerror = 0;
	int status;
	for (int i=INDEXBASE; i<ntotal+INDEXBASE; i++) {
		status = getAntennaAndCoupledPolar (i, ant1, ant2, coupledPol);
		if (status != 0) cout << "ERROR\n"; 
		status = getIndex (ant1, ant2, coupledPol, i2);
		if (status != 0) cout << "ERROR\n"; 
		if (i != i2) {
			cout << "ERROR in corrProd::test: failed to map back: " << '\n';
			cout << i << ": " << ant1 << ", " << ant2 << ", " << coupledPol <<
					": " << i2 << '\n';
			nerror++;
		}
		//cout << i << ": " << ant1 << ", " << ant2 << ", " << coupledPol <<
		//		": " << i2 << '\n';
	}
	cout << "error: " << nerror << '\n';
	if (nerror > 0) return -1;
	else return 0;
}

#endif

} // namespace corrProd



#ifdef CORRPROD_DEBUG

// Test program and sample of usage.

int main () {
    clock_t click;
    click = clock();

	uint32_t nantenna = 3600;
	cout << "antenna count: " << nantenna << '\n';
	
	corrProd::setAntennaBase (1);
	//corrProd::setIndexBase (1);	// index base is left at default value (0)
	
	//int status = corrProd::test (nantenna);

	uint32_t ntotal = corrProd::getTotal (nantenna);
	cout << "number of correlation products: " << ntotal << '\n';
	
	uint32_t ant1, ant2, coupledPol, index2;
	uint32_t nerror = 0;
	int status;
	for (uint32_t index=0; index<ntotal; index++) {
		status = corrProd::getAntennaAndCoupledPolar (index, ant1, ant2, 
                coupledPol);
		if (status != 0) cout << "ERROR\n"; 
		
		// convert back
		status = corrProd::getIndex (ant1, ant2, coupledPol, index2);
		if (status != 0) cout << "ERROR\n"; 
		
		if (index != index2) {
			cout << "ERROR in corrProd::test: failed to map back: " << '\n';
			cout << index << ": " << ant1 << ", " << ant2 << ", " << 
					coupledPol << ": " << index2 << '\n';
			nerror++;
		}
		//cout << i << ": " << ant1 << ", " << ant2 << ", " << coupledPol <<
		//		": " << i2 << '\n';
	}
	cout << "error: " << nerror << '\n';
	
    click = clock() - click;
    cout << "time: " << ((float)click) / CLOCKS_PER_SEC << '\n';

    return 0;
}

#endif

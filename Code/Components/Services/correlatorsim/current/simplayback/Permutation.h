/// @file Permutation.h
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
/// Contains generic functions to map index for 2-permutation of n items:
///   permutation index <-> component indices
/// The permutation is posed as the lower triangle of a table.
///
///      0     1     2
///   +-------------------
/// 0 | 00=0   .     .
/// 1 | 01=1  11=2   .
/// 2 | 02=3  12=4  22=5 
///
/// Note:
/// - The numbering for members and permutation index are 0-based.
/// - The functions would become more complex for upper triangle, because
///   it is necessary to take into account the total number of items in advance.

#ifndef ASKAP_CP_PERMUTATION_H
#define ASKAP_CP_PERMUTATION_H

//#define PERMUTATION_DEBUG

#include <cstdint>

#ifndef PERMUTATION_DEBUG
namespace askap {
namespace cp {
#endif

class Permutation {
	
	public:
		Permutation ();
		
		~Permutation();
		
		/// Return the total number of permutation.
		uint32_t total (uint32_t n);

		/// Given the members, return the permutation index.
		uint32_t index (uint32_t a, uint32_t b);

		/// Given the permutation index, return the members.
		void getMembers (uint32_t index, uint32_t& a, uint32_t& b);
};

#ifndef PERMUTATION_DEBUG
};
};
#endif

#endif

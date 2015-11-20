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
/// Contains a collection of permutation functions to map:
///   permutation index <-> component indices
///      0     1     2
///   +-------------------
/// 0 | 00=0   .     .
/// 1 | 01=1  11=2   .
/// 2 | 02=3  12=4  22=5 
///
/// Note:
/// - The permutation is posed as the lower triangle of a table, as shown above.
///   The functions would become more complex for upper triangle, 
///   as it would be necessary to count total number of items in advance.
/// - This class contains no data.
/// - The numbering for items and permutation index are 0-based.
/// - The paired items is always in order where item1 <= item2.

#ifndef ASKAP_CP_PERMUTATION_H
#define ASKAP_CP_PERMUTATION_H

// Allow coding & testing outside ASKAP before deployment
//#define OUTSIDEASKAP

#include <stdint.h>
#include <utility>

#ifndef OUTSIDEASKAP
namespace askap {
namespace cp {
#endif

class Permutation 
{
	public :
	
		/// Constructor
		Permutation ();
		
		/// Destructor
		virtual ~Permutation();
		
		/// @param[in] The number of items to permutate
		/// @return The total number of permutation
		uint32_t const getTotal (const uint32_t nItem);

		/// Given the items, return the permutation index. 
		/// @param[in] item1    Must be less or equal item2
		/// @param[in] item2    Must be greater of equal item1
		/// @return Permutation index
		uint32_t const getIndex (const uint32_t item1, const uint32_t item2);
		
		/// Given the items, return the permutation index. 
		/// @param[in] items    Item pair, where element 1 <= element 2
		/// @return Permutation index		
		uint32_t const getIndex (const std::pair<uint32_t,uint32_t> items);

		/// Given the items, return the permutation index. 
		/// @param[in] item1    Must be less or equal item2
		/// @param[in] item2    Must be greater of equal item1
		/// @param[in] nItem    Total number of items
		/// @return Permutation index		
		uint32_t const getIndex (const uint32_t item1, const uint32_t item2, 
                const uint32_t nItem);

		/// Given the items, return the permutation index. 
		/// @param[in] items    Item pair, where element 1 <= element 2
		/// @param[in] nItem    Total number of items
		/// @return Permutation index		
		uint32_t const getIndex (const std::pair<uint32_t,uint32_t> items, 
                const uint32_t nItem);

		/// Given the permutation index, return the items in pair. 
		/// @param[in] index    Permutation index
		/// @return Items in pair, where element 1 <= element 2
		std::pair<uint32_t,uint32_t> const getItems (const uint32_t index);
};
 
#ifndef OUTSIDEASKAP
};
};
#endif

#endif

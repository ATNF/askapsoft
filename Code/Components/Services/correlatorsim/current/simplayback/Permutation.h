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

//#define OUTSIDEASKAP

#include <stdint.h>
#include <utility>

#ifndef OUTSIDEASKAP
namespace askap {
namespace cp {
#endif

class Permutation {
	
	public :
	
		// Constructor
		Permutation ();
		
		// Destructor
		virtual ~Permutation();
		
		/// Return the total number of permutation.
		uint32_t total (uint32_t nItem);

		/// Given the items, return the permutation index. With no input check.
		uint32_t indexNoCheck (const uint32_t item1, const uint32_t item2);
		uint32_t indexNoCheck (const std::pair<uint32_t,uint32_t> items);

		/// Given the items, return the permutation index. 
		/// The correct input ordering is enforced.
		uint32_t index (const uint32_t item1, const uint32_t item2, 
                const uint32_t nItem);
		uint32_t index (const std::pair<uint32_t,uint32_t> items, 
                const uint32_t nItem);

		/// Given the permutation index, return the items in pair. 
        /// With no input check.
		std::pair<uint32_t,uint32_t> itemsNoCheck (const uint32_t index);
		
		/// Given the permutation index, return the items in pair.
		std::pair<uint32_t,uint32_t> items (const uint32_t index, 
                const uint32_t nItem);
};
 
#ifndef OUTSIDEASKAP
};
};
#endif

#endif

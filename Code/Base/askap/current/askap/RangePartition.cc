/// @file 
///
/// @brief Partition a rangle into the given number of subranges    
/// @details This helper class can be used to distribute work between
/// parallel workers, for example divide a range of spectral channels
/// into subranges. The concept is similar to parallel multi-dimensional
/// iterator we have but with only one iteration required (and 1D).
/// It is handy to encapsulate all operations in one class to simplify
/// testing.
///
/// @copyright (c) 2007 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>


// own include
#include "askap/RangePartition.h"

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"

//#include <iostream>

namespace askap {

namespace utility {

/// @brief setup partition
/// @details The number of groups and the total number of elements
/// to distribute together determine the partition.
/// @param[in] nItems number of items (should be 1 or greater)
/// @param[in] nGroups number of groups desired (should be 1 or greater)
/// @note The constructor encapsulates all the logic of doing the partitioning.
/// The result is cached in the data members. The adopted numbering is such that
/// all groups with non-zero number of items have lower numbers. If equal disribution
/// is not possible, the last group will have less elements.
RangePartition::RangePartition(unsigned int nItems, unsigned int nGroups) : 
       itsNumItems(nItems), itsNumGroups(nGroups), itsNumNonVoidGroups(1u), itsTypicalItemsPerGroup(nItems)
{
   ASKAPCHECK(nItems > 0, "Number of items to distribute should be 1 or more, you have "<<nItems);
   ASKAPCHECK(nGroups > 0, "Number of groups should be 1 or more, you have "<<nGroups);
   if (nGroups > 1) {
       // non-trivial case is dealt with here, the trivial case is set up at initialisation

       if (nItems < nGroups) {
           // more items than groups
           itsTypicalItemsPerGroup = 1;
           itsNumNonVoidGroups = itsNumItems;
       } else {
           itsTypicalItemsPerGroup = itsNumItems / itsNumGroups;
           itsNumNonVoidGroups = itsNumGroups;
           ASKAPDEBUGASSERT(itsTypicalItemsPerGroup > 0);
   
           if (itsNumItems % itsNumGroups != 0) {
               ++itsTypicalItemsPerGroup;
          }    
       }
   }
}
   
/// @brief obtain the number of items in the given group
/// @param[in] group group to work with
/// @return number of items in the given group
unsigned int RangePartition::nItemsThisGroup(unsigned int group) const
{
   ASKAPCHECK(group < itsNumGroups, "Requested group = "<<group<<" exceeds the number of groups defined ("<<itsNumGroups<<")");
   if (group + 1 < itsNumNonVoidGroups) {
       return itsTypicalItemsPerGroup;
   } else if (group + 1 == itsNumNonVoidGroups) {
       if (group == 0) {
           return itsNumItems;
       }
       ASKAPDEBUGASSERT(itsNumNonVoidGroups >= 1);
       const unsigned int itemsInAllGroupsBeforeLast = itsTypicalItemsPerGroup * group;
       //std::cout<<"group = "<<group<<" "<<itsTypicalItemsPerGroup<<" "<<itsNumNonVoidGroups<<" "<<itemsInAllGroupsBeforeLast<<std::endl;
       ASKAPDEBUGASSERT(itemsInAllGroupsBeforeLast < itsNumItems);
       return itsNumItems - itemsInAllGroupsBeforeLast;
   }
   return 0;
}

   
/// @brief get the first item of the given group
/// @param[in] group group to work with
/// @return the sequence number of the first item in the given group
/// @return An exception is thrown if the given group is unused
unsigned int RangePartition::first(unsigned int group) const
{
   ASKAPCHECK(group < itsNumNonVoidGroups, "Requested group = "<<group<<" exceeds the number of groups with non-zero elements ("<<itsNumNonVoidGroups<<")");
  
   return group * itsTypicalItemsPerGroup;
}

/// @brief get the last item of the given group
/// @param[in] group group to work with
/// @return the sequence number of the last item in the given group
/// @return An exception is thrown if the given group is unused
unsigned int RangePartition::last(unsigned int group) const
{
   const unsigned int itemAfterLast = first(group) + nItemsThisGroup(group);
   ASKAPDEBUGASSERT(itemAfterLast > 0);
   return itemAfterLast - 1;
}
   
} // namespace utility

} // namespace askap


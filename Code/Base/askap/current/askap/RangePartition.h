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

#ifndef ASKAP_UTILITY_RANGE_PARTITION
#define ASKAP_UTILITY_RANGE_PARTITION


namespace askap {

namespace utility {

/// @brief Partition a rangle into the given number of subranges    
/// @details This helper class can be used to distribute work between
/// parallel workers, for example divide a range of spectral channels
/// into subranges. The concept is similar to parallel multi-dimensional
/// iterator we have but with only one iteration required (and 1D).
/// It is handy to encapsulate all operations in one class to simplify
/// testing.
class RangePartition  {
public:
   /// @brief setup partition
   /// @details The number of groups and the total number of elements
   /// to distribute together determine the partition.
   /// @param[in] nItems number of items (should be 1 or greater)
   /// @param[in] nGroups number of groups desired (should be 1 or greater)
   /// @note The constructor encapsulates all the logic of doing the partitioning.
   /// The result is cached in the data members. The adopted numbering is such that
   /// all groups with non-zero number of items have lower numbers. If equal disribution
   /// is not possible, the last groups will have less elements.
   RangePartition(unsigned int nItems, unsigned int nGroups);
   
   /// @brief obtain the number of items in the given group
   /// @param[in] group group to work with
   /// @return number of items in the given group
   unsigned int nItemsThisGroup(unsigned int group) const;

   /// @brief check if the given group is unused
   /// @param[in] group group to work with
   /// @return true, if the given group has no items to work with
   inline bool voidGroup(unsigned int group) const { return group >= itsNumNonVoidGroups; }
   
   /// @brief get the first item of the given group
   /// @param[in] group group to work with
   /// @return the sequence number of the first item in the given group
   /// @return An exception is thrown if the given group is unused
   unsigned int first(unsigned int group) const;

   /// @brief get the last item of the given group
   /// @param[in] group group to work with
   /// @return the sequence number of the last item in the given group
   /// @return An exception is thrown if the given group is unused
   unsigned int last(unsigned int group) const;
   
   /// @brief get total number of items
   /// @return return the number of items passed at construction
   inline unsigned int nItems() const { return itsNumItems;}

   /// @brief get number of groups
   /// @return return the number of groups passed at construction
   inline unsigned int nGroups() const { return itsNumGroups;}

   /// @brief get number of non-void groups
   /// @return return the number of groups which have non-zero number of elements.
   inline unsigned int nNonVoidGroups() const { return itsNumNonVoidGroups;}
   
private:
   /// @brief number of items
   unsigned int itsNumItems;

   /// @brief number of groups
   unsigned int itsNumGroups;

   // the following numbers are derived at construction

   /// @brief number of non-void groups
   unsigned int itsNumNonVoidGroups;

   /// @brief typical number of items per group
   /// @details The first several groups may have one extra item.  The number of
   /// such groups is controlled by itsNumGroupsWithExtraItem;
   unsigned int itsTypicalItemsPerGroup;

   /// @brief number of groups with one additional item
   unsigned int itsNumGroupsWithExtraItem;
};

} // namespace utility

} // namespace askap


#endif // #ifndef ASKAP_UTILITY_RANGE_PARTITION



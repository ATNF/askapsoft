/// @file 
///
/// @brief Map antenna and/or beam indices into a continuous range
/// @details During BETA3 experiments we plan to use antennas with
/// non-contiguous indices in the data stream (they correspond to 
/// actual antenna/beam numbers given in the data stream; and possibly
/// also one-based). This class simplifies mapping into a continuous 
/// range of indices. Same functionality is likely to be required for
/// beams. 
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

#include <askap/IndexConverter.h>
#include <askap/AskapError.h>
#include <askap/AskapUtil.h>

namespace askap {

namespace utility {

/// @brief default index converter - no conversion
IndexConverter::IndexConverter() 
{
}
   
/// @brief setup conversion from a string
/// @details Extracts rules like 1:0, 4:1, 5:2
/// @param[in] indexMap string with the rules
IndexConverter::IndexConverter(const std::string &indexMap) 
{
  add(indexMap);
}
   
/// @brief add mapping
/// @param[in] in input index
/// @param[in] target output index
void IndexConverter::add(const int in, const int target)
{
  ASKAPCHECK(in >= 0, "Input indices should be non-negative; you have "<<in);
  const bool created = itsMap.insert(std::pair<int,int>(in,target)).second;
  ASKAPCHECK(created, "Attempting to override map for index "<<in);
}
   
/// @brief add mapping from string
/// @param[in] indexMap string with the rules
void IndexConverter::add(const std::string &indexMap)
{
  for (size_t pos = 0; pos<indexMap.size(); ++pos) {
       const size_t nextDelim = indexMap.find(",",pos);
       const std::string thisElem = nextDelim != std::string::npos ? indexMap.substr(pos) : 
               indexMap.substr(pos, nextDelim - pos);
       if (nextDelim != std::string::npos) {        
           pos = nextDelim;
       } else {
           pos = indexMap.size();
       }
       const size_t pos1 = thisElem.find(":");
       if (pos1 == std::string::npos) {
           continue;
       }
       const int input = utility::fromString<int>(thisElem.substr(0, pos1));
       ASKAPCHECK(pos1 + 1 < thisElem.size(), "Missing target number in element "<<thisElem<<" of "<<indexMap);
       const int target = utility::fromString<int>(thisElem.substr(pos1 + 1));
       add(input, target);
  }
}
   
/// @brief do the translation
/// @param[in] in input index
/// @return output index
/// @note a negative index is returned if the input index is out of range
int IndexConverter::operator()(const int in)  const
{ 
  if (itsMap.size() == 0) {
      return in;
  } else {
      ASKAPCHECK(in >= 0, "Input index in IndexConverter::operator() should be non-negative; you have "<<in);
      std::map<int, int>::const_iterator ci = itsMap.find(in);
      if (ci != itsMap.end()) {
          return ci->second;
      }
  }
  return -1;
}

/// @brief size of the map
/// @details returns the number of conversions set up, i.e. zero if there are none
/// @return number of conversion rules
size_t IndexConverter::nRules() const
{
   return itsMap.size();
}


} // namespace utility

} // namespace askap


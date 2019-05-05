/// @file CasacoreFwdDefines.h
/// @brief Forward definition of Casacore operator<< overloads
///
/// @copyright (c) 2019 CSIRO
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
/// @author Rodrigo Tobar
///
//
#ifndef ASKAP_CASACORE_FWD_DEFINES_H
#define ASKAP_CASACORE_FWD_DEFINES_H

#include <iosfwd>
#include <map>
#include <list>
#include <set>
#include <utility>
#include <vector>

// These forward definitions are here only to solve a problem present in
// casacore up to version 3.0.1, in which the operator<< overloads defined by
// casacore were used by a template function defined *before* the declaration
// of the overloads themselves, triggering errors in more pedantic compilers.
//
// For this forward definitions to actually work, they need to be included
// *before* casacore/casa/BasicSL/STLIO.h, which is where the problem resided.
//
// For details see https://github.com/casacore/casacore/pull/888
namespace casa {

  template <typename T, typename U>
  inline std::ostream& operator<< (std::ostream& os, const std::pair<T,U>& p);

  template<typename T>
  inline std::ostream& operator<<(std::ostream& os, const std::vector<T>& v);

  template<typename T>
  inline std::ostream& operator<<(std::ostream& os, const std::set<T>& v);

  template<typename T>
  inline std::ostream& operator<<(std::ostream& os, const std::list<T>& v);

  template<typename T, typename U>
  inline std::ostream& operator<<(std::ostream& os, const std::map<T,U>& m);

} // namespace casa

#endif // ASKAP_CASACORE_FWD_DEFINES_H

/// @file
///
/// XXX Notes on program XXX
///
/// @copyright (c) 2014 CSIRO
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#ifndef ASKAP_ANALYSIS_CATPREP_H_
#define ASKAP_ANALYSIS_CATPREP_H_

#include <string>
#include <vector>

namespace askap {

namespace analysis {

/// Find the correct component suffix. Returns a string to
/// uniquely identify a fit that is part of an island. The first
/// 26 numbers (zero-based), get a single letter a-z. After that,
/// it becomes aa,ab,ac,...az,ba,bb,bc,...bz,ca,... If there are
/// more than 702 (=26^2+26), we move to three characters:
/// zy,zz,aaa,aab,aac,... And so on.
std::string getSuffix(unsigned int num);

}

}


#endif

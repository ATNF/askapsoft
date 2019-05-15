/// @file VariableThresholdingHelpers.h
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

#ifndef ASKAP_ANALYSIS_VAR_THRESH_HELP_H_
#define ASKAP_ANALYSIS_VAR_THRESH_HELP_H_

#include <casacore/casa/aipstype.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/namespace.h>

namespace askap {

namespace analysis {

void slidingBoxStats(casacore::Array<Float> &input,
                     casacore::Array<Float> &middle,
                     casacore::Array<Float> &spread,
                     casacore::IPosition &box,
                     bool useRobust);

casacore::Array<Float> calcSNR(casacore::Array<Float> &input,
                           casacore::Array<Float> &middle,
                           casacore::Array<Float> &spread);

void slidingBoxMaskedStats(casacore::MaskedArray<Float> &input,
                           casacore::Array<Float> &middle,
                           casacore::Array<Float> &spread,
                           casacore::IPosition &box,
                           bool useRobust);

casacore::Array<Float> calcMaskedSNR(casacore::MaskedArray<Float> &input,
                                 casacore::Array<Float> &middle,
                                 casacore::Array<Float> &spread);


}

}

#endif

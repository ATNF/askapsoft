/// @file
///
/// Class to give primary beam sizes for a given MS & frequency
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
/// published by the Free Software Foundation; either version 3 of the License,
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
#ifndef ASKAP_CP_PIPELINETASKS_PRIMARY_BEAM_H
#define ASKAP_CP_PIPELINETASKS_PRIMARY_BEAM_H

// System includes
#include <string>
#include <vector>

#include "Common/ParameterSet.h"

namespace askap {
namespace cp {
namespace pipelinetasks {

class PrimaryBeamEstimator {
public:
    PrimaryBeamEstimator(const LOFAR::ParameterSet &parset);

    void define(float frequency);

    float major(){return itsMajorAxis;};
    float minor(){return itsMinorAxis;};
    float pa(){return itsPositionAngle;};

protected:
    float itsMajorAxis;
    float itsMinorAxis;
    float itsPositionAngle;
    
};

}
}
}

#endif


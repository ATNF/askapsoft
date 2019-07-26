/// @file
///
/// Implementation of class to give primary beam sizes for a given MS & frequency
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
// Include own header file first
#include "casdaupload/PrimaryBeamEstimator.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>
#include <sstream>
#include <cmath>
#include <stdint.h>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "Common/ParameterSet.h"

// Using
using namespace std;
using namespace askap::cp::pipelinetasks;

ASKAP_LOGGER(logger, ".PrimaryBeamEstimator");

PrimaryBeamEstimator::PrimaryBeamEstimator(const LOFAR::ParameterSet &parset):
    itsMajorAxis(0.),
    itsMinorAxis(0.),
    itsPositionAngle(0.)
{
    
}

void PrimaryBeamEstimator::define(float frequency)
{
    // Initial definition - circular gaussian beam with FWHM scaled by frequency

    // FWHM in radians
    float fwhm = 1.09 * (299792458.0 / frequency) / 12.;

    itsMajorAxis = fwhm * 180.0 / M_PI;
    itsMinorAxis = fwhm * 180.0 / M_PI;

    itsPositionAngle = 0.0;
    
}


/// @file
///
/// Constants needed for CASDA catalogues
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
#ifndef ASKAP_ANALYSIS_CATALOGUES_CASDA_H_
#define ASKAP_ANALYSIS_CATALOGUES_CASDA_H_

#include <string>

namespace askap {

namespace analysis {

namespace casda {

/// Which type of fit to use for the CASDA components.
const std::string componentFitType = "best";

/// Units for reporting frequency.
const std::string freqUnit = "MHz";

/// Units for reporting frequency width.
const std::string freqWidthUnit = "kHz";

/// Units for reporting fluxes from image (peak flux, noise, rms
/// residual).
const std::string fluxUnit = "mJy/beam";

/// Units for reporting integrated flux.
const std::string intFluxUnit = "mJy";

/// Units for reporting velocity
const std::string velocityUnit = "km/s";

/// Units for lambda-squared
const std::string lamsqUnit = "m2";

/// Units for Faraday Depth
const std::string faradayDepthUnit = "rad/m2";

/// Units for angle (such as polarisation position angle)
const std::string angleUnit = "deg";

/// Precision for reporting fluxes
const int precFlux = 3;
/// Precision for reporting frequency
const int precFreq = 1;
/// Precision for reporting sizes (maj/min/pa etc).
const int precSize = 2;
/// Precision for reporting alpha & beta values
const int precSpec = 2;
/// Precision for reporting RA/DEC positions
const int precPos = 6;
/// Precision for reporting pixel locations
const int precPix = 2;
/// Precision for reporting lambda-squared
const int precLamsq = 4;
/// Precision for reporting Faraday Depth values
const int precFD = 4;
/// Precision for reporting angles
const int precAngle = 3;
/// Precision for polarisation fraction
const int precPfrac = 2;
/// Precision for polarisation statistics
const int precStats = 2;

}

}

}

#endif

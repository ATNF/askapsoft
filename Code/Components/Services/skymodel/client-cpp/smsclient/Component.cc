/// @file Component.cc
///
/// @copyright (c) 2011 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "Component.h"

// Include package level header file
#include "askap_smsclient.h"

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "casacore/casa/Quanta/Quantum.h"

// Using
using namespace askap::cp::sms::client;

Component::Component(
	const ComponentId id,
        const casacore::Quantity& rightAscension,
        const casacore::Quantity& declination,
        const casacore::Quantity& positionAngle,
        const casacore::Quantity& majorAxis,
        const casacore::Quantity& minorAxis,
        const casacore::Quantity& i1400,
        const casacore::Double& spectralIndex,
        const casacore::Double& spectralCurvature)
    : 
    itsId(id),
    itsRightAscension(rightAscension),
    itsDeclination(declination),
    itsPositionAngle(positionAngle),
    itsMajorAxis(majorAxis),
    itsMinorAxis(minorAxis),
    itsI1400(i1400),
    itsSpectralIndex(spectralIndex),
    itsSpectralCurvature(spectralCurvature)
{
    ASKAPCHECK(itsRightAscension.isConform("deg"), "ra must conform to degrees");
    ASKAPCHECK(itsDeclination.isConform("deg"), "dec must conform to degrees");
    ASKAPCHECK(itsPositionAngle.isConform("rad"), "position angle must conform to radians");
    ASKAPCHECK(itsMajorAxis.isConform("arcsec"), "major axis must conform to degrees");
    ASKAPCHECK(itsMinorAxis.isConform("arcsec"), "minor axis must conform to degrees");
    ASKAPCHECK(itsI1400.isConform("Jy"), "i1400 must conform to Jy");
}

ComponentId Component::id() const
{
    return itsId;
}

casacore::Quantity Component::rightAscension() const
{
    return itsRightAscension;
}

casacore::Quantity Component::declination() const
{
    return itsDeclination;
}

casacore::Quantity Component::positionAngle() const
{
    return itsPositionAngle;
}

casacore::Quantity Component::majorAxis() const
{
    return itsMajorAxis;
}

casacore::Quantity Component::minorAxis() const
{
    return itsMinorAxis;
}

casacore::Quantity Component::i1400() const
{
    return itsI1400;
}

casacore::Double Component::spectralIndex() const
{
    return itsSpectralIndex;
}

casacore::Double Component::spectralCurvature() const
{
    return itsSpectralCurvature;
}

//// @file Component.h
///
//// @copyright (c) 2011 CSIRO
//// Australia Telescope National Facility (ATNF)
//// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
//// PO Box 76, Epping NSW 1710, Australia
//// atnf-enquiries@csiro.au
///
//// This file is part of the ASKAP software distribution.
///
//// The ASKAP software distribution is free software: you can redistribute it
//// and/or modify it under the terms of the GNU General Public License as
//// published by the Free Software Foundation; either version 2 of the License,
//// or (at your option) any later version.
///
//// This program is distributed in the hope that it will be useful,
//// but WITHOUT ANY WARRANTY; without even the implied warranty of
//// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// GNU General Public License for more details.
///
//// You should have received a copy of the GNU General Public License
//// along with this program; if not, write to the Free Software
//// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
//// @author Ben Humphreys <ben.humphreys@csiro.au>

#ifndef ASKAP_CP_SKYMODELSERVICE_COMPONENT_H
#define ASKAP_CP_SKYMODELSERVICE_COMPONENT_H

/// ASKAPsoft includes
#include "casacore/casa/aipstype.h"
#include "casacore/casa/Quanta/Quantum.h"

namespace askap {
namespace cp {
namespace skymodelservice {

/// Component identifier typedef
typedef casacore::Long ComponentId;

class Component {

    public:

        /// Constructor
        ///
        /// @param[in] id   can be ignored for creation of new components. This
        ///                 is used internally to the package.
        ///   
        /// @throw  AskapError  in the case one ore more of the Quantities does
        /// not conform to the appropriate unit. See the accessor methods for
        /// the specification of units for each attribute.
        Component(const ComponentId id,
                  const casacore::Quantity& rightAscension,
                  const casacore::Quantity& declination,
                  const casacore::Quantity& positionAngle,
                  const casacore::Quantity& majorAxis,
                  const casacore::Quantity& minorAxis,
                  const casacore::Quantity& i1400,
                  const casacore::Double& spectralIndex,
                  const casacore::Double& spectralCurvature);

        /// Unique component index number
        ComponentId id() const;

        /// Right ascension in the J2000 coordinate system
        /// Base units: degrees
        casacore::Quantity rightAscension() const;

        /// Declination in the J2000 coordinate system
        /// Base units: degrees
        casacore::Quantity declination() const;

        /// Position angle. Counted east from north.
        /// Base units: radians
        casacore::Quantity positionAngle() const;

        /// Major axis
        /// Base units: arcsecs
        casacore::Quantity majorAxis() const;

        /// Minor axis
        /// Base units: arcsecs
        casacore::Quantity minorAxis() const;

        /// Flux at 1400 Mhz
        /// Base units: Jy
        casacore::Quantity i1400() const;

        /// Spectral index
        /// Base units: N/A
        casacore::Double spectralIndex() const;

        /// Spectral curvature
        /// Base units: N/A
        casacore::Double spectralCurvature() const;

    private:
        ComponentId itsId;
        casacore::Quantity itsRightAscension;
        casacore::Quantity itsDeclination;
        casacore::Quantity itsPositionAngle;
        casacore::Quantity itsMajorAxis;
        casacore::Quantity itsMinorAxis;
        casacore::Quantity itsI1400;
        casacore::Double itsSpectralIndex;
        casacore::Double itsSpectralCurvature;
};

};
};
};

#endif

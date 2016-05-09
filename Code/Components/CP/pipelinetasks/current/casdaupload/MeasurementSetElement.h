/// @file MeasurementSetElement.h
///
/// @copyright (c) 2015 CSIRO
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

#ifndef ASKAP_CP_PIPELINETASKS_MEASUREMENTSET_ELEMENT_H
#define ASKAP_CP_PIPELINETASKS_MEASUREMENTSET_ELEMENT_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "casacore/measures/Measures/MEpoch.h"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "Common/ParameterSet.h"

// Local package includes
#include "ProjectElementBase.h"
#include "ScanElement.h"

namespace askap {
namespace cp {
namespace pipelinetasks {

/// Encapsulates a measurement set artifct to be uploaded to CASDA. A
/// specialisation of the ProjectElementBase class, with the
/// constructor defining the element name ("measurement_set") and
/// format ("tar"). Additional members include the start/stop time
/// and the set of scan elements.
class MeasurementSetElement : public ProjectElementBase {
    public:
        MeasurementSetElement(const LOFAR::ParameterSet &parset);

        xercesc::DOMElement* toXmlElement(xercesc::DOMDocument& doc) const;

        casa::MEpoch getObsStart(void) const;

        casa::MEpoch getObsEnd(void) const;

    protected:

        void extractData(void);

        casa::MEpoch itsObsStart;
        casa::MEpoch itsObsEnd;
        std::vector<ScanElement> itsScans;
};

}
}
}

#endif

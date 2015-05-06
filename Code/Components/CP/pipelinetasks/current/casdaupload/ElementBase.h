/// @file ElementBase.h
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
///
#ifndef ASKAP_CP_PIPELINETASKS_ELEMENT_BASE_H
#define ASKAP_CP_PIPELINETASKS_ELEMENT_BASE_H

// System includes
#include <string>

// ASKAPsoft includes
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "Common/ParameterSet.h"

namespace askap {
namespace cp {
namespace pipelinetasks {

/// Base class for encapsulating an artifact for upload to
/// CASDA. Holds the filename, the format and the element name (which
/// becomes the name of the entry in the XML file), and provides
/// methods to produce the XML encoding and the filepath.
class ElementBase {
    public:
        ElementBase(const LOFAR::ParameterSet &parset);

        xercesc::DOMElement* toXmlElement(xercesc::DOMDocument& doc) const;

        boost::filesystem::path getFilepath(void) const;

    protected:
        boost::filesystem::path itsFilepath;
        std::string itsFormat;
        std::string itsName;

};

}
}
}

#endif

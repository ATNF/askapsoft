/// @file ProjectElementBase.cc
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
// Include own header file first
#include "casdaupload/ProjectElementBase.h"
#include "casdaupload/ElementBase.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "votable/XercescString.h"
#include "votable/XercescUtils.h"
#include "Common/ParameterSet.h"

// Using
using namespace askap::cp::pipelinetasks;
using xercesc::DOMElement;
using askap::accessors::XercescString;
using askap::accessors::XercescUtils;

ProjectElementBase::ProjectElementBase(const LOFAR::ParameterSet &parset)
    : ElementBase(parset)
{
    if (parset.isDefined("project")) {
        itsProject = parset.getString("project");
    } else {
        ASKAPTHROW(AskapError,
                   "Project is not defined for artifact: " <<
                   parset.getString("artifactparam"));
    }
}

xercesc::DOMElement* ProjectElementBase::toXmlElement(xercesc::DOMDocument& doc) const
{
    DOMElement* e = ElementBase::toXmlElement(doc);

    XercescUtils::addTextElement(*e, "project", itsProject);

    return e;
}


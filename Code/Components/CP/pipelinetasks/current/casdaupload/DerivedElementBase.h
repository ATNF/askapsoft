/// @file DerivedElementBase.h
///
/// Specification of an XML element that is derived from an image element.
///
/// @copyright (c) 2017 CSIRO
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

#ifndef ASKAP_CP_PIPELINETASKS_DERIVED_ELEMENT_BASE_H
#define ASKAP_CP_PIPELINETASKS_DERIVED_ELEMENT_BASE_H

// System includes
#include <string>

// ASKAPsoft includes
#include "casdaupload/ElementBase.h"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "Common/ParameterSet.h"

// Local package includes

namespace askap {
namespace cp {
namespace pipelinetasks {

/// Encapsulates an artifact for upload to CASDA that is derived from
/// an image artifact. This can be either a 1D spectrum or a moment
/// map.
///
/// This class derives from the ElementBase class: it requires a
/// <type> tag, but can not use the ProjectElementBase constructor (it
/// doesn't need the <project> tag, since it inherits that of the
/// ImageElement it derives from, and if it doesn't have one the
/// ProjectElementBase will throw an exception). It therefore
/// duplicates the type functionality of TypeElementBase, but doesn't
/// derive from it. It is intended as a base class, to encapsulate the
/// key functionality of this type of element, with implemented
/// classes deriving from this.
///
/// A key feature of this class is the use of wildcards in the names
/// of the files and the thumbnails, along with code to resolve these
/// and record the number of matching files.

class DerivedElementBase : public ElementBase {
    public:
        DerivedElementBase(const LOFAR::ParameterSet &parset);

        xercesc::DOMElement* toXmlElement(xercesc::DOMDocument& doc) const;

        void copyAndChecksum(const boost::filesystem::path& outdir) const;

        void checkWildcards();

    protected:
        std::string itsType;

        /// The large PNG/JPG thumbnail image
        boost::filesystem::path itsThumbnail;

        /// List of names that match the filename definition
        std::vector<std::string> itsFilenameList;
        /// List of thumbnails that match the itsThumbnail definition
        std::vector<std::string> itsThumbnailList;

        /// Number of files meeting image name definition
        unsigned int itsNumFiles;

};

}
}
}

#endif

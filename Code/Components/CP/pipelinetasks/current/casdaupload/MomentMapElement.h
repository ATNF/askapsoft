/// @file MomentMapElement.h
///
/// Specification of a moment_map element for the casdaupload utility
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

#ifndef ASKAP_CP_PIPELINETASKS_MOMENTMAP_ELEMENT_H
#define ASKAP_CP_PIPELINETASKS_MOMENTMAP_ELEMENT_H

// System includes
#include <string>

// ASKAPsoft includes
#include "casdaupload/TypeElementBase.h"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "Common/ParameterSet.h"

// Local package includes

namespace askap {
namespace cp {
namespace pipelinetasks {

/// Encapsulates an moment map artifact for upload to CASDA. Such an
/// artifact is a 2D image typically extracted and calculated from a
/// larger 3D cube, and will usually be in FITS format. Simply a
/// specialisation of the ProjectElementBase class, with the
/// constructor defining the element name ("image") and format
/// ("fits"), as well as (optionally) the filenames of a thumbnail
/// image. The class allows the element filename and thumbnail name to
/// contain wildcards, and it also records how many spectra there are
/// that meet the wildcard definition. If a thumbnail is given, it
/// must resolve to the same number of files as the filename.
class MomentMapElement : public TypeElementBase {
    public:
        MomentMapElement(const LOFAR::ParameterSet &parset);

        xercesc::DOMElement* toXmlElement(xercesc::DOMDocument& doc) const;

        void copyAndChecksum(const boost::filesystem::path& outdir) const;

    void checkWildcards();

    protected:
        /// The large PNG/JPG thumbnail image
    boost::filesystem::path itsThumbnail;

    /// List of names that match the filename definition
    std::vector<std::string> itsFilenameList;
    /// List of thumbnails that match the itsThumbnail definition
    std::vector<std::string> itsThumbnailList;

    /// Number of spectra meeting image name definition
    unsigned int itsNumMoms;

};

}
}
}

#endif

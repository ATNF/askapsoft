/// @file ImageElement.h
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

#ifndef ASKAP_CP_PIPELINETASKS_IMAGE_ELEMENT_H
#define ASKAP_CP_PIPELINETASKS_IMAGE_ELEMENT_H

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

/// Encapsulates an image artifact (e.g. a FITS image) for upload to
/// CASDA. Simply a specialisation of the ProjectElementBase class,
/// with the constructor defining the element name ("image") and
/// format ("fits"), as well as (optionally) the filenames of large
/// and small thumbnail images,
class ImageElement : public TypeElementBase {
    public:
        ImageElement(const LOFAR::ParameterSet &parset);

        xercesc::DOMElement* toXmlElement(xercesc::DOMDocument& doc) const;

        void copyAndChecksum(const boost::filesystem::path& outdir) const;

    protected:
        /// The large PNG/JPG thumbnail image
        boost::filesystem::path itsThumbnailLarge;
        /// The small PNG/JPG thumbnail image
        boost::filesystem::path itsThumbnailSmall;

};

}
}
}

#endif

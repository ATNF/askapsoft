/// @file ImageElement.cc
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

// Include own header file first
#include "casdaupload/ImageElement.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>

// ASKAPsoft includes
#include "casdaupload/ProjectElementBase.h"
#include "casdaupload/ElementBase.h"
#include "casdaupload/CasdaFileUtils.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "votable/XercescString.h"
#include "votable/XercescUtils.h"

// Using
using namespace askap::cp::pipelinetasks;
using xercesc::DOMElement;
using askap::accessors::XercescString;
using askap::accessors::XercescUtils;

ASKAP_LOGGER(logger, ".ImageElement");

ImageElement::ImageElement(const LOFAR::ParameterSet &parset)
    : TypeElementBase(parset)
{
    itsName = "image";
    itsFormat = "fits";
    if (itsFilepath.extension() != "." + itsFormat) {
        ASKAPTHROW(AskapError,
                   "Unsupported format image - Expect " << itsFormat << " file extension");
    }
    itsThumbnailLarge = parset.getString("thumbnail_large", "");
    itsThumbnailSmall = parset.getString("thumbnail_small", "");
}

xercesc::DOMElement* ImageElement::toXmlElement(xercesc::DOMDocument& doc) const
{
    DOMElement* e = TypeElementBase::toXmlElement(doc);

    if (itsThumbnailLarge != "") {
        XercescUtils::addTextElement(*e, "thumbnail_large", itsThumbnailLarge.filename().string());
    }
    if (itsThumbnailSmall != "") {
        XercescUtils::addTextElement(*e, "thumbnail_small", itsThumbnailSmall.filename().string());
    }

    return e;
}


void ImageElement::copyAndChecksum(const boost::filesystem::path& outdir) const
{

    const boost::filesystem::path in(itsFilepath);
    const boost::filesystem::path out(outdir / in.filename());
    ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << in);
    CasdaFileUtils::copyAndChecksum(in, out);

    const boost::filesystem::path inLarge(itsThumbnailLarge);
    const boost::filesystem::path outLarge(outdir / inLarge.filename());
    ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << inLarge);
    CasdaFileUtils::copyAndChecksum(inLarge, outLarge);

    const boost::filesystem::path inSmall(itsThumbnailSmall);
    const boost::filesystem::path outSmall(outdir / inSmall.filename());
    ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << inSmall);
    CasdaFileUtils::copyAndChecksum(inSmall, outSmall);

}




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
#include "casdaupload/SpectrumElement.h"
#include "casdaupload/MomentMapElement.h"
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

    // Find spectra
    // Expect the parset to have things like
    //    image1.spectralist = [spectra1,spectra2]
    //    image1.spectra1.filename = ...
    //    image1.spectra2.filename = ...
    // and so forth
    if (parset.isDefined("spectra")) {
        std::vector<std::string> spectraList = parset.getStringVector("spectra", "");
        std::vector<std::string>::iterator spec = spectraList.begin();
        for (; spec < spectraList.end(); spec++) {
            LOFAR::ParameterSet subset = parset.makeSubset(*spec + ".");
            subset.replace("artifactparam", *spec);
            itsSpectra.push_back(SpectrumElement(subset));
        }
    }

    if (parset.isDefined("momentmaps")) {
        std::vector<std::string> momentMapList = parset.getStringVector("momentmaps", "");
        std::vector<std::string>::iterator mom = momentMapList.begin();
        for (; mom < momentMapList.end(); mom++) {
            LOFAR::ParameterSet subset = parset.makeSubset(*mom + ".");
            subset.replace("artifactparam", *mom);
            itsMomentmaps.push_back(MomentMapElement(subset));
        }
    }

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

    // Create Spectra elements
    DOMElement* childSpec = doc.createElement(XercescString("spectra"));
    std::vector<SpectrumElement>::const_iterator spec = itsSpectra.begin();
    for (; spec != itsSpectra.end(); ++spec) {
        childSpec->appendChild(spec->toXmlElement(doc));
    }
    e->appendChild(childSpec);

    // Create MomentMap elements
    DOMElement* childMom = doc.createElement(XercescString("moment_maps"));
    std::vector<MomentMapElement>::const_iterator mom = itsMomentmaps.begin();
    for (; mom != itsMomentmaps.end(); ++mom) {
        childMom->appendChild(mom->toXmlElement(doc));
    }
    e->appendChild(childMom);


    return e;
}


void ImageElement::copyAndChecksum(const boost::filesystem::path& outdir) const
{

    const boost::filesystem::path in(itsFilepath);
    const boost::filesystem::path out(outdir / in.filename());
    ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << in);
    CasdaFileUtils::copyAndChecksum(in, out);

    if (itsThumbnailLarge != "") {
        const boost::filesystem::path inLarge(itsThumbnailLarge);
        const boost::filesystem::path outLarge(outdir / inLarge.filename());
        ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << inLarge);
        CasdaFileUtils::copyAndChecksum(inLarge, outLarge);
    }

    if (itsThumbnailSmall != "") {
        const boost::filesystem::path inSmall(itsThumbnailSmall);
        const boost::filesystem::path outSmall(outdir / inSmall.filename());
        ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << inSmall);
        CasdaFileUtils::copyAndChecksum(inSmall, outSmall);
    }

    std::vector<SpectrumElement>::const_iterator spec = itsSpectra.begin();
    for (; spec < itsSpectra.end(); spec++) {
        spec->copyAndChecksum(outdir);
    }

    std::vector<MomentMapElement>::const_iterator mom = itsMomentmaps.begin();
    for (; mom < itsMomentmaps.end(); mom++) {
        mom->copyAndChecksum(outdir);
    }

}




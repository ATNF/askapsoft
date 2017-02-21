/// @file MomentMapElement.cc
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

// Include own header file first
#include "casdaupload/MomentMapElement.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>
#include <glob.h>

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

ASKAP_LOGGER(logger, ".MomentMapElement");


MomentMapElement::MomentMapElement(const LOFAR::ParameterSet &parset)
    : TypeElementBase(parset),
      itsFilenameList(),
      itsThumbnailList(),
      itsNumMoms(0)
{
    itsName = "moment_map";
    itsFormat = "fits";
    if (itsFilepath.extension() != "." + itsFormat) {
        ASKAPTHROW(AskapError,
                   "Unsupported format image - Expect " << itsFormat << " file extension");
    }
    itsThumbnail = parset.getString("thumbnail", "");

    checkWildcards();
}

void MomentMapElement::checkWildcards()
{

    // glob itsFilepath to get a list of possible names
    //     --> fills itsFilenameList
    glob_t momGlob;
    int errMom = glob(itsFilepath.string().c_str(), 0, NULL, &momGlob);
    ASKAPCHECK(errMom==0, "Failure interpreting moment map filepath \""
               << itsFilepath.filename().string() << "\"");
    itsNumMoms = momGlob.gl_pathc;
    for(size_t i=0;i<itsNumMoms; i++) {
        itsFilenameList.push_back(momGlob.gl_pathv[i]);
    }
    globfree(&momGlob);

    // glob itsThumbnail to get a list of possible names
    //     --> fills itsThumbnailList

    if (itsThumbnail != "" ){
        glob_t thumbGlob;
        int errThumb = glob(itsThumbnail.string().c_str(), 0, NULL, &thumbGlob);
        ASKAPCHECK(errThumb==0, "Failure interpreting thumbnail filepath \""
                   << itsThumbnail.filename().string() << "\"");
        ASKAPCHECK(thumbGlob.gl_pathc == itsNumMoms,
                   "Thumbnail wildcard for moment maps produces different number of files than filename");
        for(size_t i=0;i<thumbGlob.gl_pathc; i++) {
            itsThumbnailList.push_back(thumbGlob.gl_pathv[i]);
        }
        globfree(&thumbGlob);
    }
    
}

xercesc::DOMElement* MomentMapElement::toXmlElement(xercesc::DOMDocument& doc) const
{
    DOMElement* e = TypeElementBase::toXmlElement(doc);

    if (itsThumbnail != "") {
        XercescUtils::addTextElement(*e, "thumbnail", itsThumbnail.filename().string());
    }

    std::stringstream ss;
    ss << itsNumMoms;
    XercescUtils::addTextElement(*e, "number", ss.str());

    return e;
}


void MomentMapElement::copyAndChecksum(const boost::filesystem::path& outdir) const
{

    for(size_t i=0;i<itsFilenameList.size();i++){
        const boost::filesystem::path in(itsFilenameList[i]);
        const boost::filesystem::path out(outdir / in.filename());
        ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << in);
        CasdaFileUtils::copyAndChecksum(in, out);
    }
    
    if (itsThumbnail != "" ) {
        for(size_t i=0;i<itsThumbnailList.size();i++){
            const boost::filesystem::path in(itsThumbnailList[i]);
            const boost::filesystem::path out(outdir / in.filename());
            ASKAPLOG_INFO_STR(logger, "Copying and calculating checksum for " << in);
            CasdaFileUtils::copyAndChecksum(in, out);
        }
    }

}


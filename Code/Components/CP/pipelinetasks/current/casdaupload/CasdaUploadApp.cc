/// @file CasdaUploadApp.cc
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
#include "casdaupload/CasdaUploadApp.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "Common/ParameterSet.h"
#include "Common/KVpair.h"
#include "askap/StatReporter.h"
#include "boost/scoped_ptr.hpp"
#include "boost/filesystem.hpp"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "xercesc/framework/LocalFileFormatTarget.hpp"
#include "xercesc/framework/XMLFormatter.hpp"
#include "votable/XercescString.h"
#include "votable/XercescUtils.h"

// Local package includes
#include "casdaupload/IdentityElement.h"
#include "casdaupload/ObservationElement.h"
#include "casdaupload/ImageElement.h"
#include "casdaupload/CatalogueElement.h"
#include "casdaupload/MeasurementSetElement.h"
#include "casdaupload/EvaluationReportElement.h"
#include "casdaupload/CasdaChecksumFile.h"
#include "casdaupload/CasdaFileUtils.h"

// Using
using namespace std;
using namespace askap::cp::pipelinetasks;
using namespace xercesc;
using askap::accessors::XercescString;
namespace fs = boost::filesystem;

ASKAP_LOGGER(logger, ".CasdaUploadApp");

int CasdaUploadApp::run(int argc, char* argv[])
{
    StatReporter stats;

    itsParset = config();
    checkParset();

    const IdentityElement identity(itsParset);

    const vector<ImageElement> images(
        buildArtifactElements<ImageElement>("images.artifactlist"));
    const vector<CatalogueElement> catalogues(
        buildArtifactElements<CatalogueElement>("catalogues.artifactlist"));
    const vector<MeasurementSetElement> ms(
        buildArtifactElements<MeasurementSetElement>("measurementsets.artifactlist"));
    const vector<EvaluationReportElement> reports(
        buildArtifactElements<EvaluationReportElement>("evaluation.artifactlist"));

    if (images.empty() && catalogues.empty() && ms.empty()) {
        ASKAPTHROW(AskapError, "No artifacts declared for upload");
    }

    // If a measurement set is present, we can determine the time range for the
    // observation. Note, only the first measurement set (if there are multiple)
    // is used in this calculation.
    ObservationElement obs;
    if (!ms.empty()) {
        if (ms.size() > 1) {
            ASKAPLOG_WARN_STR(logger, "Multiple measurement set were specified."
                              << " Only the first one will be used to populate the"
                              << " observation metadata");
        }
        const MeasurementSetElement& firstMs = ms[0];
        obs.setObsTimeRange(firstMs.getObsStart(), firstMs.getObsEnd());
    } else {
        casa::MEpoch start, end;

        if (itsParset.isDefined("obsStart")) {
            std::string obsStart = itsParset.getString("obsStart");
            casa::Quantity qStart;
            casa::MVTime::read(qStart, obsStart);
            start = casa::MEpoch(qStart);
        } else {
            ASKAPTHROW(AskapError, "Unknown observation start time - please use \"obsStart\" to specify the start time in the absence of measurement sets.");
        }
        if (itsParset.isDefined("obsEnd")) {
            std::string obsEnd = itsParset.getString("obsEnd");
            casa::Quantity qEnd;
            casa::MVTime::read(qEnd, obsEnd);
            end = casa::MEpoch(qEnd);
        } else {
            ASKAPTHROW(AskapError, "Unknown observation end time - please use \"obsStart\" to specify the end time in the absence of measurement sets.");
        }
        obs.setObsTimeRange(start, end);
    }

    // Create the output directory
    const fs::path outbase(itsParset.getString("outputdir"));
    if (!is_directory(outbase)) {
        ASKAPTHROW(AskapError, "Directory " << outbase
                   << " does not exists or is not a directory");
    }
    const fs::path outdir = outbase / itsParset.getString("sbid");
    ASKAPLOG_INFO_STR(logger, "Using output directory: " << outdir);
    if (!is_directory(outdir)) {
        create_directory(outdir);
    }
    if (!exists(outdir)) {
        ASKAPTHROW(AskapError, "Failed to create directory " << outdir);
    }
    permissions(outdir, boost::filesystem::add_perms | boost::filesystem::group_write);
    const fs::path metadataFile = outdir / "observation.xml";
    generateMetadataFile(metadataFile, identity, obs, images, catalogues, ms, reports);
    CasdaFileUtils::checksumFile(metadataFile);

    // Tar up measurement sets
    for (vector<MeasurementSetElement>::const_iterator it = ms.begin();
            it != ms.end(); ++it) {
        const fs::path in(it->getFilepath());
        fs::path out(outdir / in.filename());
        out += ".tar";
        ASKAPLOG_INFO_STR(logger, "Tarring file " << in << " to " << out);
        CasdaFileUtils::tarAndChecksum(in, out);
    }

    // Copy artifacts and checksum
    copyAndChecksumElements<ImageElement>(images, outdir);
    copyAndChecksumElements<CatalogueElement>(catalogues, outdir);
    copyAndChecksumElements<EvaluationReportElement>(reports, outdir);

    // Finally, and specifically as the last step, write the READY file
    // For now, this is only done if the config file specifically
    // requests it via the writeREADYfile parameter.
    if (itsParset.getBool("writeREADYfile", false)) {
        const fs::path readyFilename = outdir / "READY";
        CasdaFileUtils::writeReadyFile(readyFilename);
    }

    stats.logSummary();
    return 0;
}

void CasdaUploadApp::generateMetadataFile(
    const fs::path& file,
    const IdentityElement& identity,
    const ObservationElement& obs,
    const std::vector<ImageElement>& images,
    const std::vector<CatalogueElement>& catalogues,
    const std::vector<MeasurementSetElement>& ms,
    const std::vector<EvaluationReportElement>& reports)
{
    xercesc::XMLPlatformUtils::Initialize();

    boost::scoped_ptr<LocalFileFormatTarget> target(new LocalFileFormatTarget(
                XercescString(file.string())));

    // Create document
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(XercescString("LS"));
    DOMDocument* doc = impl->createDocument();
    doc->setXmlVersion(XercescString("1.0"));
    doc->setXmlStandalone(true);

    // Create the root element and add it to the document
    DOMElement* root = doc->createElement(XercescString("dataset"));
    root->setAttributeNS(XercescString("http://www.w3.org/2000/xmlns/"),
                         XercescString("xmlns"), XercescString("http://au.csiro/askap/observation"));
    doc->appendChild(root);

    // Add identity element
    root->appendChild(identity.toXmlElement(*doc));

    // Add observation element
    root->appendChild(obs.toXmlElement(*doc));

    // Create artifact elements
    appendElementCollection<ImageElement>(images, "images", root);
    appendElementCollection<CatalogueElement>(catalogues, "catalogues", root);
    appendElementCollection<MeasurementSetElement>(ms, "measurement_sets", root);
    appendElementCollection<EvaluationReportElement>(reports, "evaluations", root);

    // Write
    DOMLSSerializer* writer = ((DOMImplementationLS*)impl)->createLSSerializer();

    if (writer->getDomConfig()->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true)) {
        writer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    }

    DOMLSOutput* output = ((DOMImplementationLS*)impl)->createLSOutput();
    output->setByteStream(target.get());
    writer->write(doc, output);

    // Cleanup
    output->release();
    writer->release();
    doc->release();
    target.reset(0);
    xercesc::XMLPlatformUtils::Terminate();
}

template <typename T>
std::vector<T> CasdaUploadApp::buildArtifactElements(const std::string& key) const
{
    vector<T> elements;
    bool useAbsolutePath = itsParset.getBool("useAbsolutePath","true");
    
    if (itsParset.isDefined(key)) {
        const vector<string> names = itsParset.getStringVector(key);
        for (vector<string>::const_iterator it = names.begin(); it != names.end(); ++it) {
            LOFAR::ParameterSet subset = itsParset.makeSubset(*it + ".");
            subset.replace("artifactparam", *it);
            subset.replace(LOFAR::KVpair("useAbsolutePath", useAbsolutePath));
            elements.push_back(T(subset));
        }
    }

    return elements;
}

template <typename T>
void CasdaUploadApp::appendElementCollection(const std::vector<T>& elements,
        const std::string& tag,
        xercesc::DOMElement* root)
{
    // Create measurement set elements
    if (!elements.empty()) {
        DOMDocument* doc = root->getOwnerDocument();
        DOMElement* child = doc->createElement(XercescString(tag));
        for (typename vector<T>::const_iterator it = elements.begin();
                it != elements.end(); ++it) {
            child->appendChild(it->toXmlElement(*doc));
        }
        root->appendChild(child);
    }
}

template <typename T>
void CasdaUploadApp::copyAndChecksumElements(const std::vector<T>& elements,
        const fs::path& outdir)
{
    for (typename vector<T>::const_iterator it = elements.begin();
            it != elements.end(); ++it) {
        it->copyAndChecksum(outdir);
    }
}

void CasdaUploadApp::checkParset()
{
    std::string listnames[4] = {"images", "catalogues", "measurementsets", "evaluation"};
    for (int i = 0; i < 4; i++) {
        if ((itsParset.isDefined(listnames[i] + ".artefactlist")) &&
                (!itsParset.isDefined(listnames[i] + ".artifactlist"))) {

            ASKAPLOG_WARN_STR(logger, "You have defined " << listnames[i] <<
                              ".artefactlist instead of " << listnames[i] <<
                              ".artifactlist. Replacing for now, but CHANGE YOUR PARSET!");

            itsParset.add(listnames[i] + ".artifactlist",
                          itsParset.get(listnames[i] + ".artefactlist"));

        }
    }

}

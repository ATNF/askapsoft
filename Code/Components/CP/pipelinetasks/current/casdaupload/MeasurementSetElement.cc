/// @file MeasurementSetElement.cc
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
#include "casdaupload/MeasurementSetElement.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "xercesc/dom/DOM.hpp" // Includes all DOM
#include "boost/filesystem.hpp"
#include "askap/votable/XercescString.h"
#include "askap/votable/XercescUtils.h"
#include "casacore/casa/Quanta/MVTime.h"
#include "casacore/casa/Quanta/Quantum.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/measures/Measures/MEpoch.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"

// Local package includes
#include "casdaupload/ScanElement.h"

// Using
using namespace std;
using namespace askap::cp::pipelinetasks;
using namespace casacore;
using xercesc::DOMElement;
using askap::accessors::XercescString;
using askap::accessors::XercescUtils;

ASKAP_LOGGER(logger, ".MeasurementSetElement");

MeasurementSetElement::MeasurementSetElement(const LOFAR::ParameterSet &parset)
    : ProjectElementBase(parset)
{
    itsName = "measurement_set";
    itsFormat = "tar";

    extractData();
}

xercesc::DOMElement* MeasurementSetElement::toXmlElement(xercesc::DOMDocument& doc) const
{
    // Have to break the inheritence pattern, as we need to append
    // '.tar' onto the filename path so that the entry in the
    // observation.xml file matches what is on disk.
    DOMElement* e = doc.createElement(XercescString(itsName));

    if (itsUseAbsolutePaths) {
        std::string path = itsFilepath.string();
        if (path[0] != '/') {
            path = boost::filesystem::current_path().string() + "/" + path;
        }
        XercescUtils::addTextElement(*e, "filename", path + ".tar");
    } else {
        XercescUtils::addTextElement(*e, "filename", itsFilepath.filename().string() + ".tar");
    }
    XercescUtils::addTextElement(*e, "format", itsFormat);
    XercescUtils::addTextElement(*e, "project", itsProject);

    // Confirm that there is at least one scan element
    // Throw an error if not
    ASKAPCHECK(itsScans.size() > 0,
               "No scans are present in the measurement set " << itsFilepath);

    // Create scan elements
    DOMElement* child = doc.createElement(XercescString("scans"));
    for (vector<ScanElement>::const_iterator it = itsScans.begin();
            it != itsScans.end(); ++it) {
        child->appendChild(it->toXmlElement(doc));
    }
    e->appendChild(child);
    return e;
}

casacore::MEpoch MeasurementSetElement::getObsStart(void) const
{
    return itsObsStart;
}

casacore::MEpoch MeasurementSetElement::getObsEnd(void) const
{
    return itsObsEnd;
}

void MeasurementSetElement::extractData()
{
    ASKAPLOG_INFO_STR(logger, "Extracting metadata from measurement set: "
                      << itsFilepath);
    casacore::MeasurementSet ms(itsFilepath.string(), casacore::Table::Old);
    ROMSColumns msc(ms);

    // Extract observation start and stop time
    const casacore::Int obsId = msc.observationId()(0);
    const ROMSObservationColumns& obsc = msc.observation();
    const casacore::Vector<casacore::MEpoch> timeRange = obsc.timeRangeMeas()(obsId);
    itsObsStart = timeRange(0);
    itsObsEnd = timeRange(1);

    const ROMSFieldColumns& fieldc = msc.field();
    const ROMSDataDescColumns& ddc = msc.dataDescription();
    const ROMSPolarizationColumns& polc = msc.polarization();
    const ROMSSpWindowColumns& spwc = msc.spectralWindow();

    // Iterate over all rows, creating a ScanElement for each scan
    casacore::Int lastScanId = -1;
    casacore::uInt row = 0;
    while (row < msc.nrow()) {
        const casacore::Int scanNum = msc.scanNumber()(row);
        if (scanNum > lastScanId) {
            lastScanId = scanNum;
            // 1: Collect scan metadata that is expected to remain constant for the whole scan
            const casacore::MEpoch startTime = msc.timeMeas()(row);

            // Field
            const casacore::Int fieldId = msc.fieldId()(row);
            const casacore::Vector<MDirection> dirVec = fieldc.phaseDirMeasCol()(fieldId);
            const MDirection fieldDirection = dirVec(0);
            const std::string fieldName = fieldc.name()(fieldId);

            // Polarisations
            const casacore::Int dataDescId = msc.dataDescId()(row);
            const casacore::uInt descPolId = ddc.polarizationId()(dataDescId);
            const casacore::Vector<casacore::Int> stokesTypesInt = polc.corrType()(descPolId);

            // Spectral window
            const casacore::uInt descSpwId = ddc.spectralWindowId()(dataDescId);
            const casacore::Vector<casacore::Double> frequencies = spwc.chanFreq()(descSpwId);
            const casacore::Int nChan = frequencies.size();
            casacore::Double centreFreq = 0.0;
            if (nChan % 2 == 0) {
                centreFreq = (frequencies(nChan / 2) + frequencies((nChan / 2) + 1)) / 2.0;
            } else {
                centreFreq = frequencies(nChan / 2);
            }
            const casacore::Vector<double> chanWidth = spwc.chanWidth()(descSpwId);

            // 2: Find the final timestamp for this scan
            while (row < msc.nrow() && msc.scanNumber()(row) == scanNum) {
                ++row;
            }
            const MEpoch endTime = msc.timeMeas()(row - 1);

            // 3: Store the ScanElement
            itsScans.push_back(ScanElement(scanNum,
                                           startTime,
                                           endTime,
                                           fieldDirection,
                                           fieldName,
                                           stokesTypesInt,
                                           nChan,
                                           Quantity(centreFreq, "Hz"),
                                           Quantity(chanWidth(0), "Hz")));
        } else {
            ++row;
        }
    }

}

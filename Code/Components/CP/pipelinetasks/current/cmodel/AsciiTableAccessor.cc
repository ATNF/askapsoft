/// @file AsciiTableAccessor.cc
///
/// @copyright (c) 2011 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "cmodel/AsciiTableAccessor.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <vector>
#include <list>

// ASKAPsoft include
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/lexical_cast.hpp"

// Casacore includes
#include "casacore/casa/aipstype.h"
#include "casacore/casa/Quanta/Quantum.h"
#include "casacore/casa/Quanta/Unit.h"
#include "casacore/casa/Quanta/MVDirection.h"

// Using
using namespace casacore;
using namespace std;
using namespace askap::cp::pipelinetasks;
using namespace askap::cp::sms::client;

ASKAP_LOGGER(logger, ".AsciiTableAccessor");

AsciiTableAccessor::AsciiTableAccessor(const std::string& filename,
                                       const LOFAR::ParameterSet& parset)
        : itsFile(new std::ifstream(filename.c_str())),
          itsFields(makeFieldDesc(parset))
{
    if (!itsFile->good()) {
        ASKAPTHROW(AskapError, "Error opening file: " << filename);
    }
}

AsciiTableAccessor::AsciiTableAccessor(const std::stringstream& sstream,
                                       const LOFAR::ParameterSet& parset)
        : itsFile(new std::stringstream),
          itsFields(makeFieldDesc(parset))
{
    *(dynamic_cast<std::stringstream*>(itsFile.get())) << sstream.str();
}

AsciiTableAccessor::~AsciiTableAccessor()
{
}

ComponentListPtr AsciiTableAccessor::coneSearch(
    const casacore::Quantity& ra,
    const casacore::Quantity& dec,
    const casacore::Quantity& searchRadius,
    const casacore::Quantity& fluxLimit)
{
    ASKAPLOG_INFO_STR(logger, "Cone search - ra: " << ra.getValue("deg")
                           << " deg, dec: " << dec.getValue("deg")
                           << " deg, radius: " << searchRadius.getValue("deg")
                           << " deg, Fluxlimit: " << fluxLimit.getValue("Jy") << " Jy");

    // Seek back to the beginning of the buffer before reading line by line
    itsFile->seekg(0, std::ios::beg);
    std::string line;
    itsBelowFluxLimit = 0;
    itsOutsideSearchCone = 0;
    casacore::uLong total = 0;

    // Initially built as a list to allow efficient growth
    std::list<askap::cp::sms::client::Component> list;

    while (getline(*itsFile, line)) {
        if (line.find_first_of("#") == std::string::npos) {
            processLine(line, ra, dec, searchRadius, fluxLimit, list);
            total++;

            if (total % 100000 == 0) {
                ASKAPLOG_DEBUG_STR(logger, "Read " << total << " component entries");
            }
        }
    }

    ASKAPLOG_INFO_STR(logger, "Sources discarded due to flux threshold: " << itsBelowFluxLimit);
    ASKAPLOG_INFO_STR(logger, "Sourced discarded due to being outside the search cone: " << itsOutsideSearchCone);

    // Returned as a vector to minimise memory usage
    return ComponentListPtr(new ComponentList(list.begin(), list.end()));
}

void AsciiTableAccessor::processLine(
    const std::string& line,
    const casacore::Quantity& searchRA,
    const casacore::Quantity& searchDec,
    const casacore::Quantity& searchRadius,
    const casacore::Quantity& fluxLimit,
    std::list<askap::cp::sms::client::Component>& list)
{
    // Create these once to avoid the performance impact of creating them over and over.
    static casacore::Unit deg("deg");
    static casacore::Unit rad("rad");
    static casacore::Unit arcsec("arcsec");
    static casacore::Unit Jy("Jy");

    // Tokenize the line
    stringstream iss(line);
    vector<string> tokens;
    tokens.reserve(8); // For performance only, will be grown if needed
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string> >(tokens));

    const casacore::Quantity ra(boost::lexical_cast<casacore::Double>(tokens[itsFields[RA].first]), itsFields[RA].second);
    const casacore::Quantity dec(boost::lexical_cast<casacore::Double>(tokens[itsFields[DEC].first]), itsFields[DEC].second);
    // Discard if outside cone
    const MVDirection searchRefDir(searchRA, searchDec);
    const MVDirection componentDir(ra, dec);
    const Quantity separation = searchRefDir.separation(componentDir, deg);
    if (separation.getValue(deg) > searchRadius.getValue(deg)) {
        itsOutsideSearchCone++;
        return;
    }

    const casacore::Quantity flux(boost::lexical_cast<casacore::Double>(tokens[itsFields[FLUX].first]), itsFields[FLUX].second);
    // Discard if below flux limit
    if (flux.getValue(Jy) < fluxLimit.getValue(Jy)) {
        itsBelowFluxLimit++;
        return;
    }

    casacore::Quantity majorAxis(boost::lexical_cast<casacore::Double>(tokens[itsFields[MAJOR_AXIS].first]), itsFields[MAJOR_AXIS].second);
    casacore::Quantity minorAxis(boost::lexical_cast<casacore::Double>(tokens[itsFields[MINOR_AXIS].first]), itsFields[MINOR_AXIS].second);
    const casacore::Quantity positionAngle(boost::lexical_cast<casacore::Double>(tokens[itsFields[POSITION_ANGLE].first]), itsFields[POSITION_ANGLE].second);

    // Ensure major axis is larger than minor axis
    if (majorAxis.getValue() < minorAxis.getValue()) {
        swap(majorAxis, minorAxis);
    }

    // Ensure if major axis is non-zero, so is the minor axis
    if (majorAxis.getValue() > 0.0 && minorAxis.getValue() == 0.0) {
        minorAxis = casacore::Quantity(1.0e-15, arcsec);
    }

    // Spectral Index if present
    double spectralIndex = 0.0;
    if (itsFields.find(SPECTRAL_INDEX) != itsFields.end()) {
        spectralIndex = boost::lexical_cast<casacore::Double>(tokens[itsFields[SPECTRAL_INDEX].first]);
    }

    // Spectral Curvature if present
    double spectralCurvature = 0.0;
    if (itsFields.find(SPECTRAL_CURVATURE) != itsFields.end()) {
        spectralCurvature = boost::lexical_cast<casacore::Double>(tokens[itsFields[SPECTRAL_CURVATURE].first]);
    }

    // Build the Component object and add to the list. This component
    // has a constant spectrum
    // NOTE: The Component ID has no meaning for this accessor
    askap::cp::sms::client::Component c(-1, ra, dec, positionAngle,
            majorAxis, minorAxis, flux, spectralIndex, spectralCurvature);
    list.push_back(c);
}

std::pair< short, casacore::Unit > AsciiTableAccessor::makeFieldDescEntry(
        const LOFAR::ParameterSet& parset,
        const std::string& colkey,
        const std::string& unitskey)
{
    const short pos = parset.getUint(colkey);
    const casacore::Unit units(parset.getString(unitskey));
    return make_pair(pos, units);
}


AsciiTableAccessor::FieldDesc AsciiTableAccessor::makeFieldDesc(const LOFAR::ParameterSet& parset)
{
    FieldDesc f;

    f[RA] = makeFieldDescEntry(parset, "tablespec.ra.col", "tablespec.ra.units");
    f[DEC] = makeFieldDescEntry(parset, "tablespec.dec.col", "tablespec.dec.units");
    f[FLUX] = makeFieldDescEntry(parset, "tablespec.flux.col", "tablespec.flux.units");
    f[MAJOR_AXIS] = makeFieldDescEntry(parset, "tablespec.majoraxis.col", "tablespec.majoraxis.units");
    f[MINOR_AXIS] = makeFieldDescEntry(parset, "tablespec.minoraxis.col", "tablespec.minoraxis.units");
    f[POSITION_ANGLE] = makeFieldDescEntry(parset, "tablespec.posangle.col", "tablespec.posangle.units");

    if (parset.isDefined("tablespec.spectralindex.col")) {
        f[SPECTRAL_INDEX] = makeFieldDescEntry(parset, "tablespec.spectralindex.col", "tablespec.spectralindex.units");
    }
    if (parset.isDefined("tablespec.spectralcurvature.col")) {
        f[SPECTRAL_CURVATURE] = makeFieldDescEntry(parset, "tablespec.spectralcurvature.col", "tablespec.spectralcurvature.units");
    }

    return f;
}

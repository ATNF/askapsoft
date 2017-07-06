/// @file
///
/// XXX Notes on program XXX
///
/// @copyright (c) 2014 CSIRO
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
#include <catalogues/CasdaIsland.h>
#include <askap_analysis.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/Casda.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <wcslib/wcs.h>
#include <vector>

ASKAP_LOGGER(logger, ".casdaisland");

namespace askap {

namespace analysis {

CasdaIsland::CasdaIsland():
    CatalogueEntry()
{
}

CasdaIsland::CasdaIsland(sourcefitting::RadioSource &obj,
                         const LOFAR::ParameterSet &parset):
    CatalogueEntry(parset),
    itsName(obj.getName()),
    itsNumComponents(obj.numFits("best")),
    itsRAs(obj.getRAs()),
    itsDECs(obj.getDecs()),
    itsRA(obj.getRA()),
    itsDEC(obj.getDec()),
    itsFreq(obj.getVel()),
    itsMaj(obj.getMajorAxis()),
    itsMin(obj.getMinorAxis()),
    itsPA(obj.getPositionAngle()),
    itsFluxInt(obj.getIntegFlux()),
    itsFluxPeak(obj.getPeakFlux()),
    itsXmin(obj.getXmin()),
    itsXmax(obj.getXmax()),
    itsYmin(obj.getYmin()),
    itsYmax(obj.getYmax()),
    itsNumPix(obj.getSpatialSize()),
    itsXaverage(obj.getXaverage()),
    itsYaverage(obj.getYaverage()),
    itsXcentroid(obj.getXCentroid()),
    itsYcentroid(obj.getYCentroid()),
    itsXpeak(obj.getXPeak()),
    itsYpeak(obj.getYPeak()),
    itsFlag1(0),
    itsFlag2(0),
    itsFlag3(0),
    itsFlag4(0),
    itsComment("")
{
    std::stringstream id;
    id << itsIDbase << obj.getID();
    itsIslandID = id.str();

    // Convert the header class to use FREQ type and the appropriate unit
    duchamp::FitsHeader newHead_freq = changeSpectralAxis(obj.header(), "FREQ-???", casda::freqUnit);

    casa::Unit wcsFreqUnits(newHead_freq.getSpectralUnits());
    casa::Unit freqUnits(casda::freqUnit);
    double freqScale = casa::Quantity(1., wcsFreqUnits).getValue(freqUnits);

    double peakFluxscale = getPeakFluxConversionScale(newHead_freq, casda::fluxUnit);
    itsFluxPeak *= peakFluxscale;

    double intFluxscale = getIntFluxConversionScale(newHead_freq, casda::intFluxUnitContinuum);
    itsFluxInt *= intFluxscale;

    // Re-calculate WCS parameters
    obj.calcWCSparams(newHead_freq);
    itsFreq = obj.getVel() * freqScale;

}

const float CasdaIsland::ra()
{
    return itsRA;
}

const float CasdaIsland::dec()
{
    return itsDEC;
}

const std::string CasdaIsland::id()
{
    return itsIslandID;
}

void CasdaIsland::printTableRow(std::ostream &stream,
                                duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
}

void CasdaIsland::printTableEntry(std::ostream &stream,
                                  duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.printEntry(stream, itsIslandID);
    } else if (type == "NAME") {
        column.printEntry(stream, itsName);
    } else if (type == "NCOMP") {
        column.printEntry(stream, itsNumComponents);
    } else if (type == "RA") {
        column.printEntry(stream, itsRAs);
    } else if (type == "DEC") {
        column.printEntry(stream, itsDECs);
    } else if (type == "RAJD") {
        column.printEntry(stream, itsRA);
    } else if (type == "DECJD") {
        column.printEntry(stream, itsDEC);
    } else if (type == "FREQ") {
        column.printEntry(stream, itsFreq);
    } else if (type == "MAJ") {
        column.printEntry(stream, itsMaj);
    } else if (type == "MIN") {
        column.printEntry(stream, itsMin);
    } else if (type == "PA") {
        column.printEntry(stream, itsPA);
    } else if (type == "FINT") {
        column.printEntry(stream, itsFluxInt);
    } else if (type == "FPEAK") {
        column.printEntry(stream, itsFluxPeak);
    } else if (type == "XMIN") {
        column.printEntry(stream, itsXmin);
    } else if (type == "XMAX") {
        column.printEntry(stream, itsXmax);
    } else if (type == "YMIN") {
        column.printEntry(stream, itsYmin);
    } else if (type == "YMAX") {
        column.printEntry(stream, itsYmax);
    } else if (type == "NPIX") {
        column.printEntry(stream, itsNumPix);
    } else if (type == "XAV") {
        column.printEntry(stream, itsXaverage);
    } else if (type == "YAV") {
        column.printEntry(stream, itsYaverage);
    } else if (type == "XCENT") {
        column.printEntry(stream, itsXcentroid);
    } else if (type == "YCENT") {
        column.printEntry(stream, itsYcentroid);
    } else if (type == "XPEAK") {
        column.printEntry(stream, itsXpeak);
    } else if (type == "YPEAK") {
        column.printEntry(stream, itsYpeak);
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlag1);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlag2);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlag3);
    } else if (type == "FLAG4") {
        column.printEntry(stream, itsFlag4);
    } else if (type == "COMMENT") {
        column.printEntry(stream, itsComment);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaIsland::checkCol(duchamp::Catalogues::Column &column, bool checkTitle)
{
    std::string type = column.type();
    if (type == "ID") {
        column.check(itsIslandID, checkTitle);
    } else if (type == "NAME") {
        column.check(itsName, checkTitle);
    } else if (type == "NCOMP") {
        column.check(itsNumComponents, checkTitle);
    } else if (type == "RA") {
        column.check(itsRAs, checkTitle);
    } else if (type == "DEC") {
        column.check(itsDECs, checkTitle);
    } else if (type == "RAJD") {
        column.check(itsRA, checkTitle);
    } else if (type == "DECJD") {
        column.check(itsDEC, checkTitle);
    } else if (type == "FREQ") {
        column.check(itsFreq, checkTitle);
    } else if (type == "MAJ") {
        column.check(itsMaj, checkTitle);
    } else if (type == "MIN") {
        column.check(itsMin, checkTitle);
    } else if (type == "PA") {
        column.check(itsPA, checkTitle);
    } else if (type == "FINT") {
        column.check(itsFluxInt, checkTitle);
    } else if (type == "FPEAK") {
        column.check(itsFluxPeak, checkTitle);
    } else if (type == "XMIN") {
        column.check(itsXmin, checkTitle);
    } else if (type == "XMAX") {
        column.check(itsXmax, checkTitle);
    } else if (type == "YMIN") {
        column.check(itsYmin, checkTitle);
    } else if (type == "YMAX") {
        column.check(itsYmax, checkTitle);
    } else if (type == "NPIX") {
        column.check(itsNumPix, checkTitle);
    } else if (type == "XAV") {
        column.check(itsXaverage, checkTitle);
    } else if (type == "YAV") {
        column.check(itsYaverage, checkTitle);
    } else if (type == "XCENT") {
        column.check(itsXcentroid, checkTitle);
    } else if (type == "YCENT") {
        column.check(itsYcentroid, checkTitle);
    } else if (type == "XPEAK") {
        column.check(itsXpeak, checkTitle);
    } else if (type == "YPEAK") {
        column.check(itsYpeak, checkTitle);
    } else if (type == "FLAG1") {
        column.check(itsFlag1, checkTitle);
    } else if (type == "FLAG2") {
        column.check(itsFlag2, checkTitle);
    } else if (type == "FLAG3") {
        column.check(itsFlag3, checkTitle);
    } else if (type == "FLAG4") {
        column.check(itsFlag4, checkTitle);
    } else if (type == "COMMENT") {
        column.check(itsComment, checkTitle);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaIsland::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle)
{
    for (size_t i = 0; i < spec.size(); i++) {
        this->checkCol(spec.column(i), checkTitle);
    }
}

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& blob, CasdaIsland& src)
{
    std::string s;
    double d;
    unsigned int u;
    int i;

    s = src.itsIslandID; blob << s;
    s = src.itsName; blob << s;
    u = src.itsNumComponents; blob << u;
    s = src.itsRAs; blob << s;
    s = src.itsDECs; blob << s;
    d = src.itsRA; blob << d;
    d = src.itsDEC; blob << d;
    d = src.itsFreq; blob << d;
    d = src.itsMaj; blob << d;
    d = src.itsMin; blob << d;
    d = src.itsPA; blob << d;
    d = src.itsFluxInt; blob << d;
    d = src.itsFluxPeak; blob << d;
    i = src.itsXmin; blob << i;
    i = src.itsXmax; blob << i;
    i = src.itsYmin; blob << i;
    i = src.itsYmax; blob << i;
    u = src.itsNumPix; blob << u;
    d = src.itsXaverage; blob << d;
    d = src.itsYaverage; blob << d;
    d = src.itsXcentroid; blob << d;
    d = src.itsYcentroid; blob << d;
    i = src.itsXpeak; blob << i;
    i = src.itsYpeak; blob << i;
    u = src.itsFlag1; blob << u;
    u = src.itsFlag2; blob << u;
    u = src.itsFlag3; blob << u;
    u = src.itsFlag4; blob << u;
    s = src.itsComment; blob << s;

    return blob;
}

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& blob, CasdaIsland& src)
{
    std::string s;
    double d;
    unsigned int u;
    int i;

    blob >> s; src.itsIslandID = s;
    blob >> s; src.itsName = s;
    blob >> u; src.itsNumComponents = u;
    blob >> s; src.itsRAs = s;
    blob >> s; src.itsDECs = s;
    blob >> d; src.itsRA = d;
    blob >> d; src.itsDEC = d;
    blob >> d; src.itsFreq = d;
    blob >> d; src.itsMaj = d;
    blob >> d; src.itsMin = d;
    blob >> d; src.itsPA = d;
    blob >> d; src.itsFluxInt = d;
    blob >> d; src.itsFluxPeak = d;
    blob >> i; src.itsXmin = i;
    blob >> i; src.itsXmax = i;
    blob >> i; src.itsYmin = i;
    blob >> i; src.itsYmax = i;
    blob >> u; src.itsNumPix = u;
    blob >> d; src.itsXaverage = d;
    blob >> d; src.itsYaverage = d;
    blob >> d; src.itsXcentroid = d;
    blob >> d; src.itsYcentroid = d;
    blob >> i; src.itsXpeak = i;
    blob >> i; src.itsYpeak = i;
    blob >> u; src.itsFlag1 = u;
    blob >> u; src.itsFlag2 = u;
    blob >> u; src.itsFlag3 = u;
    blob >> u; src.itsFlag4 = u;
    blob >> s; src.itsComment = s;

    return blob;

}

}

}

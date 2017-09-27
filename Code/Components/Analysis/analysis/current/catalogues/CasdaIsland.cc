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
#include <extraction/IslandData.h>

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
                         const LOFAR::ParameterSet &parset,
                         const std::string fitType):
    CatalogueEntry(parset),
    itsName(obj.getName()),
    itsNumComponents(obj.numFits(casda::componentFitType)),
    itsRAs(obj.getRAs()),
    itsDECs(obj.getDecs()),
    itsRA(obj.getRA()),
    itsDEC(obj.getDec()),
    itsFreq(obj.getVel()),
    itsPA(obj.getPositionAngle()),
    itsFluxPeak(obj.getPeakFlux()),
    itsMeanBackground(0.),
    itsBackgroundNoise(0.),
    itsMaxResidual(0.),
    itsMinResidual(0.),
    itsMeanResidual(0.),
    itsRMSResidual(0.),
    itsStddevResidual(0.),
    itsXmin(obj.getXmin()),
    itsXmax(obj.getXmax()),
    itsYmin(obj.getYmin()),
    itsYmax(obj.getYmax()),
    itsNumPix(obj.getSpatialSize()),
    itsSolidAngle(0.),
    itsBeamArea(0.),
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

    double pixelSize = obj.header().getAvPixScale();
    std::string pixelUnits(obj.header().WCS().cunit[0]);
    pixelUnits += "2";
    casa::Unit solidAngleUnits(casda::solidangleUnit);
    double pixelToSolidAngle = casa::Quantity(pixelSize * pixelSize, pixelUnits).getValue(solidAngleUnits);

    double peakFluxscale = getPeakFluxConversionScale(newHead_freq, casda::fluxUnit);
    itsFluxPeak *= peakFluxscale;

    itsFluxInt.value() = obj.getIntegFlux();
    itsFluxInt.error() = obj.getIntegFluxError();  // this won't work as we don't know the stats
    double intFluxscale = getIntFluxConversionScale(newHead_freq, casda::intFluxUnitContinuum);
    itsFluxInt.value() *= intFluxscale;

    // scale factor for the angular size
    casa::Unit headerShapeUnits(obj.header().getShapeUnits());
    casa::Unit shapeUnits(casda::shapeUnit);
    double shapeScale = casa::Quantity(1., headerShapeUnits).getValue(shapeUnits);
    itsMaj = obj.getMajorAxis() * shapeScale;
    itsMin = obj.getMinorAxis() * shapeScale;

    // Re-calculate WCS parameters
    obj.calcWCSparams(newHead_freq);
    itsFreq = obj.getVel() * freqScale;

    // Average values for the background level & noise
    // Residual pixel statistics
    IslandData islanddata(parset, fitType);
    islanddata.setSource(&obj);
    islanddata.findVoxelStats();

    itsMeanBackground = islanddata.background() * peakFluxscale;
    itsBackgroundNoise = islanddata.noise() * peakFluxscale;
    itsMaxResidual = islanddata.residualMax() * peakFluxscale;
    itsMinResidual = islanddata.residualMin() * peakFluxscale;
    itsMeanResidual = islanddata.residualMean() * peakFluxscale;
    itsStddevResidual = islanddata.residualStddev() * peakFluxscale;
    itsRMSResidual = islanddata.residualRMS() * peakFluxscale;

    // Convert npix to solid angle
    itsSolidAngle = itsNumPix * pixelToSolidAngle;
    // Get beam size in solid angle
    itsBeamArea = obj.header().beam().area() * pixelToSolidAngle;

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
        column.printEntry(stream, itsFluxInt.value());
    } else if (type == "FINTERR") {
        column.printEntry(stream, itsFluxInt.error());
    } else if (type == "FPEAK") {
        column.printEntry(stream, itsFluxPeak);
    } else if (type == "BACKGND") {
        column.printEntry(stream, itsMeanBackground);
    } else if (type == "NOISE") {
        column.printEntry(stream, itsBackgroundNoise);
    } else if (type == "MAXRESID") {
        column.printEntry(stream, itsMaxResidual);
    } else if (type == "MINRESID") {
        column.printEntry(stream, itsMinResidual);
    } else if (type == "MEANRESID") {
        column.printEntry(stream, itsMeanResidual);
    } else if (type == "RMSRESID") {
        column.printEntry(stream, itsRMSResidual);
    } else if (type == "STDDEVRESID") {
        column.printEntry(stream, itsStddevResidual);
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
    } else if (type == "SOLIDANGLE") {
        column.printEntry(stream, itsSolidAngle);
    } else if (type == "BEAMAREA") {
        column.printEntry(stream, itsBeamArea);
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
    bool checkPrec = false;
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
        column.check(itsRA, checkTitle, checkPrec);
    } else if (type == "DECJD") {
        column.check(itsDEC, checkTitle, checkPrec);
    } else if (type == "FREQ") {
        column.check(itsFreq, checkTitle, checkPrec);
    } else if (type == "MAJ") {
        column.check(itsMaj, checkTitle, checkPrec);
    } else if (type == "MIN") {
        column.check(itsMin, checkTitle, checkPrec);
    } else if (type == "PA") {
        column.check(itsPA, checkTitle, checkPrec);
    } else if (type == "FINT") {
        column.check(itsFluxInt.value(), checkTitle, checkPrec);
    } else if (type == "FINTERR") {
        column.check(itsFluxInt.error(), checkTitle, checkPrec);
    } else if (type == "FPEAK") {
        column.check(itsFluxPeak, checkTitle, checkPrec);
    } else if (type == "BACKGND") {
        column.check(itsMeanBackground, checkTitle, checkPrec);
    } else if (type == "NOISE") {
        column.check(itsBackgroundNoise, checkTitle, checkPrec);
    } else if (type == "MAXRESID") {
        column.check(itsMaxResidual, checkTitle, checkPrec);
    } else if (type == "MINRESID") {
        column.check(itsMinResidual, checkTitle, checkPrec);
    } else if (type == "MEANRESID") {
        column.check(itsMeanResidual, checkTitle, checkPrec);
    } else if (type == "RMSRESID") {
        column.check(itsRMSResidual, checkTitle, checkPrec);
    } else if (type == "STDDEVRESID") {
        column.check(itsStddevResidual, checkTitle, checkPrec);
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
    } else if (type == "SOLIDANGLE") {
        column.check(itsSolidAngle, checkTitle, checkPrec);
    } else if (type == "BEAMAREA") {
        column.check(itsBeamArea, checkTitle, checkPrec);
    } else if (type == "XAV") {
        column.check(itsXaverage, checkTitle, checkPrec);
    } else if (type == "YAV") {
        column.check(itsYaverage, checkTitle, checkPrec);
    } else if (type == "XCENT") {
        column.check(itsXcentroid, checkTitle, checkPrec);
    } else if (type == "YCENT") {
        column.check(itsYcentroid, checkTitle, checkPrec);
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
    casda::ValueError v;
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
    v = src.itsFluxInt; blob << v;
    d = src.itsFluxPeak; blob << d;
    d = src.itsMeanBackground; blob << d;
    d = src.itsBackgroundNoise; blob << d;
    d = src.itsMaxResidual; blob << d;
    d = src.itsMinResidual; blob << d;
    d = src.itsMeanResidual; blob << d;
    d = src.itsRMSResidual; blob << d;
    d = src.itsStddevResidual; blob << d;
    i = src.itsXmin; blob << i;
    i = src.itsXmax; blob << i;
    i = src.itsYmin; blob << i;
    i = src.itsYmax; blob << i;
    u = src.itsNumPix; blob << u;
    d = src.itsSolidAngle; blob << d;
    d = src.itsBeamArea; blob << d;
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
    casda::ValueError v;
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
    blob >> v; src.itsFluxInt = v;
    blob >> d; src.itsFluxPeak = d;
    blob >> d; src.itsMeanBackground = d;
    blob >> d; src.itsBackgroundNoise = d;
    blob >> d; src.itsMaxResidual = d;
    blob >> d; src.itsMinResidual = d;
    blob >> d; src.itsMeanResidual = d;
    blob >> d; src.itsRMSResidual = d;
    blob >> d; src.itsStddevResidual = d;
    blob >> i; src.itsXmin = i;
    blob >> i; src.itsXmax = i;
    blob >> i; src.itsYmin = i;
    blob >> i; src.itsYmax = i;
    blob >> u; src.itsNumPix = u;
    blob >> d; src.itsSolidAngle = d;
    blob >> d; src.itsBeamArea = d;
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

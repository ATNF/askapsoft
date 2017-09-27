/// @file
///
/// Implementation of the CASDA Component class
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
#include <catalogues/CasdaComponent.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaIsland.h>
#include <catalogues/Casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <sourcefitting/FitResults.h>
#include <outputs/CataloguePreparation.h>
#include <mathsutils/MathsUtils.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>

ASKAP_LOGGER(logger, ".casdacomponent");

namespace askap {

namespace analysis {

CasdaComponent::CasdaComponent():
    CatalogueEntry()
{
}

CasdaComponent::CasdaComponent(sourcefitting::RadioSource &obj,
                               const LOFAR::ParameterSet &parset,
                               const unsigned int fitNumber,
                               const std::string fitType):
    CatalogueEntry(parset),
    itsFlagSpectralIndexOrigin(0),
    itsFlag4(0),
    itsComment("")
{
    // check that we are requesting a valid fit number
    ASKAPCHECK(fitNumber < obj.numFits(fitType),
               "For fitType=" << fitType << ", fitNumber=" << fitNumber << ", but source " << obj.getID() <<
               "(" << obj.getName() << ") only has " << obj.numFits(fitType));

    sourcefitting::FitResults results = obj.fitResults(fitType);

    // Define local variables that will get printed
    casa::Gaussian2D<Double> gauss = obj.gaussFitSet(fitType)[fitNumber];
    casa::Vector<Double> errors = obj.fitResults(fitType).errors(fitNumber);
    CasdaIsland theIsland(obj, parset);

    itsIslandID = theIsland.id();
    std::stringstream id;
    id << itsIDbase << obj.getID() << getSuffix(fitNumber);
    itsComponentID = id.str();

    duchamp::FitsHeader newHead_freq = changeSpectralAxis(obj.header(), "FREQ", casda::freqUnit);

    double thisRA, thisDec, zworld;
    newHead_freq.pixToWCS(gauss.xCenter(), gauss.yCenter(), obj.getZcentre(),
                          thisRA, thisDec, zworld);
    itsRA.value() = thisRA;
    itsDEC.value() = thisDec;

    double freqScale = 0.;
    if (newHead_freq.WCS().spec >= 0) {
        casa::Unit imageFreqUnits(newHead_freq.WCS().cunit[newHead_freq.WCS().spec]);
        casa::Unit freqUnits(casda::freqUnit);
        freqScale = casa::Quantity(1., imageFreqUnits).getValue(freqUnits);
    }
    itsFreq = zworld * freqScale;

    int lng = newHead_freq.WCS().lng;
    int precision = -int(log10(fabs(newHead_freq.WCS().cdelt[lng] * 3600. / 10.)));
    float pixscale = newHead_freq.getAvPixScale() * 3600.; // convert from pixels to arcsec
    itsRAs  = decToDMS(thisRA, newHead_freq.lngtype(), precision);
    itsDECs = decToDMS(thisDec, newHead_freq.lattype(), precision);
    itsRA.error() = errors[1] * pixscale;
    itsDEC.error() = errors[2] * pixscale;
    itsName = newHead_freq.getIAUName(itsRA.value(), itsDEC.value());

    double peakFluxscale = getPeakFluxConversionScale(newHead_freq, casda::fluxUnit);
    itsFluxPeak.value() = gauss.height() * peakFluxscale;
    itsFluxPeak.error() = errors[0] * peakFluxscale;

    itsMaj.value() = gauss.majorAxis() * pixscale;
    itsMaj.error() = errors[3] * pixscale;
    itsMin.value() = gauss.minorAxis() * pixscale;
    itsMin.error() = errors[4] * pixscale;
    itsPA.value() = gauss.PA() * 180. / M_PI;
    itsPA.error() = errors[5] * 180. / M_PI;

    double intFluxscale = getIntFluxConversionScale(newHead_freq, casda::intFluxUnitContinuum);
    itsFluxInt.value() = gauss.flux() * intFluxscale;
    // To calculate error on integrated flux of Gaussian, use Eq.42 from Condon (1997, PASP 109, 166).
    double beamScaling = newHead_freq.getBeam().maj() * newHead_freq.getBeam().min() * pixscale * pixscale /
                         (itsMaj.value() * itsMin.value());
    itsFluxInt.error() = itsFluxInt.value() *
                         sqrt((itsFluxPeak.error() * itsFluxPeak.error()) / (itsFluxPeak.value() * itsFluxPeak.value())
                              + beamScaling * ((itsMaj.error() * itsMaj.error()) / (itsMaj.value() * itsMaj.value()) +
                                      (itsMaj.error() * itsMaj.error()) / (itsMaj.value() * itsMaj.value())));

    std::vector<Double> deconv = analysisutilities::deconvolveGaussian(gauss, errors,
                                 newHead_freq.getBeam());
    itsMaj_deconv.value() = deconv[0] * pixscale;
    itsMin_deconv.value() = deconv[1] * pixscale;
    itsPA_deconv.value() = deconv[2] * 180. / M_PI;
    itsMaj_deconv.error() = deconv[3] * pixscale;
    itsMin_deconv.error() = deconv[4] * pixscale;
    itsPA_deconv.error() = deconv[5] * 180. / M_PI;

    itsChisq = results.chisq();
    itsRMSfit = results.RMS() * peakFluxscale;

    itsAlpha.value() = obj.alphaValues(fitType)[fitNumber];
    itsBeta.value() = obj.betaValues(fitType)[fitNumber];
    itsAlpha.error() = obj.alphaErrors(fitType)[fitNumber];
    itsBeta.error() = obj.betaErrors(fitType)[fitNumber];

    itsRMSimage = obj.noiseLevel() * peakFluxscale;

    itsFlagGuess = results.fitIsGuess() ? 1 : 0;
    itsFlagSiblings = obj.numFits(fitType) > 1 ? 1 : 0;

    /// @todo - fix this to respond to how alpha/beta came about. Only
    /// one way to calculate them at the moment.
    itsFlagSpectralIndexOrigin = 1;

    // These are the additional parameters not used in the CASDA
    // component catalogue v1.7:
    std::stringstream localid;
    localid << obj.getID() << getSuffix(fitNumber);
    itsLocalID = localid.str();
    itsXpos = gauss.xCenter();
    itsYpos = gauss.yCenter();
    itsFluxInt_island = obj.getIntegFlux() * intFluxscale;
    itsFluxPeak_island = obj.getPeakFlux() * peakFluxscale;
    itsNfree_fit = results.numFreeParam();
    itsNDoF_fit = results.ndof();
    itsNpix_fit = results.numPix();
    itsNpix_island = obj.getSize();

}

const float CasdaComponent::ra()
{
    return itsRA.value();
}

const float CasdaComponent::dec()
{
    return itsDEC.value();
}

const float CasdaComponent::raErr()
{
    return itsRA.error();
}

const float CasdaComponent::decErr()
{
    return itsDEC.error();
}

const std::string CasdaComponent::componentID()
{
    return itsComponentID;
}

const std::string CasdaComponent::name()
{
    return itsName;
}

const double CasdaComponent::intFlux()
{
    return itsFluxInt.value();
}

const double CasdaComponent::intFlux(std::string unit)
{
    return casa::Quantum<float>(itsFluxInt.value(), casda::intFluxUnitContinuum).getValue(unit);
}

const double CasdaComponent::freq()
{
    return itsFreq;
}

const double CasdaComponent::freq(std::string unit)
{
    return casa::Quantum<float>(itsFreq, casda::freqUnit).getValue(unit);
}

const double CasdaComponent::alpha()
{
    return itsAlpha.value();
}

const double CasdaComponent::beta()
{
    return itsBeta.value();
}


void CasdaComponent::printTableRow(std::ostream &stream,
                                   duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
}

void CasdaComponent::printTableEntry(std::ostream &stream,
                                     duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ISLAND") {
        column.printEntry(stream, itsIslandID);
    } else if (type == "ID") {
        column.printEntry(stream, itsComponentID);
    } else if (type == "NAME") {
        column.printEntry(stream, itsName);
    } else if (type == "RA") {
        column.printEntry(stream, itsRAs);
    } else if (type == "DEC") {
        column.printEntry(stream, itsDECs);
    } else if (type == "RAJD") {
        column.printEntry(stream, itsRA.value());
    } else if (type == "DECJD") {
        column.printEntry(stream, itsDEC.value());
    } else if (type == "RAERR") {
        column.printEntry(stream, itsRA.error());
    } else if (type == "DECERR") {
        column.printEntry(stream, itsDEC.error());
    } else if (type == "FREQ") {
        column.printEntry(stream, itsFreq);
    } else if (type == "FPEAK") {
        column.printEntry(stream, itsFluxPeak.value());
    } else if (type == "FPEAKERR") {
        column.printEntry(stream, itsFluxPeak.error());
    } else if (type == "FINT") {
        column.printEntry(stream, itsFluxInt.value());
    } else if (type == "FINTERR") {
        column.printEntry(stream, itsFluxInt.error());
    } else if (type == "MAJ") {
        column.printEntry(stream, itsMaj.value());
    } else if (type == "MIN") {
        column.printEntry(stream, itsMin.value());
    } else if (type == "PA") {
        column.printEntry(stream, itsPA.value());
    } else if (type == "MAJERR") {
        column.printEntry(stream, itsMaj.error());
    } else if (type == "MINERR") {
        column.printEntry(stream, itsMin.error());
    } else if (type == "PAERR") {
        column.printEntry(stream, itsPA.error());
    } else if (type == "MAJDECONV") {
        column.printEntry(stream, itsMaj_deconv.value());
    } else if (type == "MINDECONV") {
        column.printEntry(stream, itsMin_deconv.value());
    } else if (type == "PADECONV") {
        column.printEntry(stream, itsPA_deconv.value());
    } else if (type == "MAJDECONVERR") {
        column.printEntry(stream, itsMaj_deconv.error());
    } else if (type == "MINDECONVERR") {
        column.printEntry(stream, itsMin_deconv.error());
    } else if (type == "PADECONVERR") {
        column.printEntry(stream, itsPA_deconv.error());
    } else if (type == "CHISQ") {
        column.printEntry(stream, itsChisq);
    } else if (type == "RMSFIT") {
        column.printEntry(stream, itsRMSfit);
    } else if (type == "ALPHA") {
        column.printEntry(stream, itsAlpha.value());
    } else if (type == "BETA") {
        column.printEntry(stream, itsBeta.value());
    } else if (type == "ALPHAERR") {
        column.printEntry(stream, itsAlpha.error());
    } else if (type == "BETAERR") {
        column.printEntry(stream, itsBeta.error());
    } else if (type == "RMSIMAGE") {
        column.printEntry(stream, itsRMSimage);
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlagSiblings);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlagGuess);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlagSpectralIndexOrigin);
    } else if (type == "FLAG4") {
        column.printEntry(stream, itsFlag4);
    } else if (type == "COMMENT") {
        column.printEntry(stream, itsComment);
    } else if (type == "LOCALID") {
        column.printEntry(stream, itsLocalID);
    } else if (type == "XPOS") {
        column.printEntry(stream, itsXpos);
    } else if (type == "YPOS") {
        column.printEntry(stream, itsYpos);
    } else if (type == "FINTISLAND") {
        column.printEntry(stream, itsFluxInt_island);
    } else if (type == "FPEAKISLAND") {
        column.printEntry(stream, itsFluxPeak_island);
    } else if (type == "NFREEFIT") {
        column.printEntry(stream, itsNfree_fit);
    } else if (type == "NDOFFIT") {
        column.printEntry(stream, itsNDoF_fit);
    } else if (type == "NPIXFIT") {
        column.printEntry(stream, itsNpix_fit);
    } else if (type == "NPIXISLAND") {
        column.printEntry(stream, itsNpix_island);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaComponent::checkCol(duchamp::Catalogues::Column &column, bool checkTitle)
{
    bool checkPrec = false;
    std::string type = column.type();
    if (type == "ISLAND") {
        column.check(itsIslandID, checkTitle);
    } else if (type == "ID") {
        column.check(itsComponentID, checkTitle);
    } else if (type == "NAME") {
        column.check(itsName, checkTitle);
    } else if (type == "RA") {
        column.check(itsRAs, checkTitle);
    } else if (type == "DEC") {
        column.check(itsDECs, checkTitle);
    } else if (type == "RAJD") {
        column.check(itsRA.value(), checkTitle, checkPrec);
    } else if (type == "DECJD") {
        column.check(itsDEC.value(), checkTitle, checkPrec);
    } else if (type == "RAERR") {
        column.check(itsRA.error(), checkTitle, checkPrec);
    } else if (type == "DECERR") {
        column.check(itsDEC.error(), checkTitle, checkPrec);
    } else if (type == "FREQ") {
        column.check(itsFreq, checkTitle, checkPrec);
    } else if (type == "FPEAK") {
        column.check(itsFluxPeak.value(), checkTitle, checkPrec);
    } else if (type == "FPEAKERR") {
        column.check(itsFluxPeak.error(), checkTitle, checkPrec);
    } else if (type == "FINT") {
        column.check(itsFluxInt.value(), checkTitle, checkPrec);
    } else if (type == "FINTERR") {
        column.check(itsFluxInt.error(), checkTitle, checkPrec);
    } else if (type == "MAJ") {
        column.check(itsMaj.value(), checkTitle, checkPrec);
    } else if (type == "MIN") {
        column.check(itsMin.value(), checkTitle, checkPrec);
    } else if (type == "PA") {
        column.check(itsPA.value(), checkTitle, checkPrec);
    } else if (type == "MAJERR") {
        column.check(itsMaj.error(), checkTitle, checkPrec);
    } else if (type == "MINERR") {
        column.check(itsMin.error(), checkTitle, checkPrec);
    } else if (type == "PAERR") {
        column.check(itsPA.error(), checkTitle, checkPrec);
    } else if (type == "MAJDECONV") {
        column.check(itsMaj_deconv.value(), checkTitle, checkPrec);
    } else if (type == "MINDECONV") {
        column.check(itsMin_deconv.value(), checkTitle, checkPrec);
    } else if (type == "PADECONV") {
        column.check(itsPA_deconv.value(), checkTitle, checkPrec);
    } else if (type == "MAJDECONVERR") {
        column.check(itsMaj_deconv.error(), checkTitle, checkPrec);
    } else if (type == "MINDECONVERR") {
        column.check(itsMin_deconv.error(), checkTitle, checkPrec);
    } else if (type == "PADECONVERR") {
        column.check(itsPA_deconv.error(), checkTitle, checkPrec);
    } else if (type == "CHISQ") {
        column.check(itsChisq, checkTitle, checkPrec);
    } else if (type == "RMSFIT") {
        column.check(itsRMSfit, checkTitle, checkPrec);
    } else if (type == "ALPHA") {
        column.check(itsAlpha.value(), checkTitle, checkPrec);
    } else if (type == "BETA") {
        column.check(itsBeta.value(), checkTitle, checkPrec);
    } else if (type == "ALPHAERR") {
        column.check(itsAlpha.error(), checkTitle, checkPrec);
    } else if (type == "BETAERR") {
        column.check(itsBeta.error(), checkTitle, checkPrec);
    } else if (type == "RMSIMAGE") {
        column.check(itsRMSimage, checkTitle, checkPrec);
    } else if (type == "FLAG1") {
        column.check(itsFlagSiblings, checkTitle);
    } else if (type == "FLAG2") {
        column.check(itsFlagGuess, checkTitle);
    } else if (type == "FLAG3") {
        column.check(itsFlagSpectralIndexOrigin, checkTitle);
    } else if (type == "FLAG4") {
        column.check(itsFlag4, checkTitle);
    } else if (type == "COMMENT") {
        column.check(itsComment, checkTitle);
    } else if (type == "LOCALID") {
        column.check(itsLocalID, checkTitle);
    } else if (type == "XPOS") {
        column.check(itsXpos, checkTitle);
    } else if (type == "YPOS") {
        column.check(itsYpos, checkTitle);
    } else if (type == "FINTISLAND") {
        column.check(itsFluxInt_island, checkTitle, checkPrec);
    } else if (type == "FPEAKISLAND") {
        column.check(itsFluxPeak_island, checkTitle, checkPrec);
    } else if (type == "NFREEFIT") {
        column.check(itsNfree_fit, checkTitle);
    } else if (type == "NDOFFIT") {
        column.check(itsNDoF_fit, checkTitle);
    } else if (type == "NPIXFIT") {
        column.check(itsNpix_fit, checkTitle);
    } else if (type == "NPIXISLAND") {
        column.check(itsNpix_island, checkTitle);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaComponent::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle)
{
    for (size_t i = 0; i < spec.size(); i++) {
        this->checkCol(spec.column(i), checkTitle);
    }
}

void CasdaComponent::writeAnnotation(boost::shared_ptr<duchamp::AnnotationWriter> &writer)
{

    std::stringstream ss;
    ss << "Component " << itsLocalID << ":";
    writer->writeCommentString(ss.str());
    // Have maj/min in arcsec, so need to convert to deg, and then
    // halve so that we give the semi-major axis
    writer->ellipse(itsRA.value(), itsDEC.value(),
                    itsMaj.value() / 3600. / 2., itsMin.value() / 3600. / 2., itsPA.value());

}

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& blob, CasdaComponent& src)
{
    std::string s;
    double d;
    unsigned int u;
    casda::ValueError v;

    s = src.itsIslandID; blob << s;
    s = src.itsComponentID; blob << s;
    s = src.itsName; blob << s;
    s = src.itsRAs; blob << s;
    s = src.itsDECs; blob << s;
    v = src.itsRA; blob << v;
    v = src.itsDEC; blob << v;
    d = src.itsFreq; blob << d;
    v = src.itsFluxPeak; blob << v;
    v = src.itsFluxInt; blob << v;
    v = src.itsMaj; blob << v;
    v = src.itsMin; blob << v;
    v = src.itsPA; blob << v;
    v = src.itsMaj_deconv; blob << v;
    v = src.itsMin_deconv; blob << v;
    v = src.itsPA_deconv; blob << v;
    d = src.itsChisq; blob << d;
    d = src.itsRMSfit; blob << d;
    v = src.itsAlpha; blob << v;
    v = src.itsBeta; blob << v;
    d = src.itsRMSimage; blob << d;
    u = src.itsFlagSiblings; blob << u;
    u = src.itsFlagGuess; blob << u;
    u = src.itsFlagSpectralIndexOrigin; blob << u;
    u = src.itsFlag4; blob << u;
    s = src.itsComment; blob << s;
    s = src.itsLocalID; blob << s;
    d = src.itsXpos; blob << d;
    d = src.itsYpos; blob << d;
    d = src.itsFluxInt_island; blob << d;
    d = src.itsFluxPeak_island; blob << d;
    u = src.itsNfree_fit; blob << u;
    u = src.itsNDoF_fit; blob << u;
    u = src.itsNpix_fit; blob << u;
    u = src.itsNpix_island; blob << u;

    return blob;
}

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& blob, CasdaComponent& src)
{
    std::string s;
    double d;
    casda::ValueError v;
    unsigned int u;

    blob >> s; src.itsIslandID = s;
    blob >> s; src.itsComponentID = s;
    blob >> s; src.itsName = s;
    blob >> s; src.itsRAs = s;
    blob >> s; src.itsDECs = s;
    blob >> v; src.itsRA = v;
    blob >> v; src.itsDEC = v;
    blob >> d; src.itsFreq = d;
    blob >> v; src.itsFluxPeak = v;
    blob >> v; src.itsFluxInt = v;
    blob >> v; src.itsMaj = v;
    blob >> v; src.itsMin = v;
    blob >> v; src.itsPA = v;
    blob >> v; src.itsMaj_deconv = v;
    blob >> v; src.itsMin_deconv = v;
    blob >> v; src.itsPA_deconv = v;
    blob >> d; src.itsChisq = d;
    blob >> d; src.itsRMSfit = d;
    blob >> v; src.itsAlpha = v;
    blob >> v; src.itsBeta = v;
    blob >> d; src.itsRMSimage = d;
    blob >> u; src.itsFlagSiblings = u;
    blob >> u; src.itsFlagGuess = u;
    blob >> u; src.itsFlagSpectralIndexOrigin = u;
    blob >> u; src.itsFlag4 = u;
    blob >> s; src.itsComment = s;
    blob >> s; src.itsLocalID = s;
    blob >> d; src.itsXpos = d;
    blob >> d; src.itsYpos = d;
    blob >> d; src.itsFluxInt_island = d;
    blob >> d; src.itsFluxPeak_island = d;
    blob >> u; src.itsNfree_fit = u;
    blob >> u; src.itsNDoF_fit = u;
    blob >> u; src.itsNpix_fit = u;
    blob >> u; src.itsNpix_island = u;

    return blob;
}

}

}

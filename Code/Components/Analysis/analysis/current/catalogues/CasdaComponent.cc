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
#include <catalogues/casda.h>
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

CasdaComponent::CasdaComponent(sourcefitting::RadioSource &obj,
                               const LOFAR::ParameterSet &parset,
                               const unsigned int fitNumber,
                               const std::string fitType):
    CatalogueEntry(parset),
    itsFlag3(0),
    itsFlag4(0),
    itsComment("")
{
    // check that we are requesting a valid fit number
    ASKAPCHECK(fitNumber < obj.numFits(fitType),
               "fitNumber=" << fitNumber << ", but source " << obj.getID() <<
               " only has " << obj.numFits(fitType));

    sourcefitting::FitResults results = obj.fitResults(fitType);

    // Define local variables that will get printed
    casa::Gaussian2D<Double> gauss = obj.gaussFitSet(fitType)[fitNumber];
    casa::Vector<Double> errors = obj.fitResults(fitType).errors(fitNumber);
    CasdaIsland theIsland(obj, parset);
    
    itsIslandID = theIsland.id();
    std::stringstream id;
    id << itsIDbase << obj.getID() << getSuffix(fitNumber);
    itsComponentID = id.str();

    duchamp::FitsHeader newHead_freq = changeSpectralAxis(obj.header(),"FREQ",casda::freqUnit);

    double thisRA, thisDec, zworld;
    newHead_freq.pixToWCS(gauss.xCenter(), gauss.yCenter(), obj.getZcentre(),
                          thisRA, thisDec, zworld);
    itsRA.value() = thisRA;
    itsDEC.value() = thisDec;

    double freqScale=0.;
    if (newHead_freq.WCS().spec >= 0 ){
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
    double beamScaling = newHead_freq.getBeam().maj() * newHead_freq.getBeam().min() *pixscale * pixscale /
        (itsMaj.value()*itsMin.value());
    itsFluxInt.error() = itsFluxInt.value() *
        sqrt( (itsFluxPeak.error()*itsFluxPeak.error())/(itsFluxPeak.value()*itsFluxPeak.value())
              + beamScaling * ( (itsMaj.error()*itsMaj.error())/(itsMaj.value()*itsMaj.value()) +
                                (itsMaj.error()*itsMaj.error())/(itsMaj.value()*itsMaj.value())  ) );
              
    std::vector<Double> deconv = analysisutilities::deconvolveGaussian(gauss,
                                 newHead_freq.getBeam());
    itsMaj_deconv = deconv[0] * pixscale;
    itsMin_deconv = deconv[1] * pixscale;
    itsPA_deconv = deconv[2] * 180. / M_PI;

    itsChisq = results.chisq();
    itsRMSfit = results.RMS() * peakFluxscale;

    itsAlpha = obj.alphaValues(fitType)[fitNumber];
    itsBeta = obj.betaValues(fitType)[fitNumber];

    itsRMSimage = obj.noiseLevel() * peakFluxscale;

    itsFlagGuess = results.fitIsGuess() ? 1 : 0;
    itsFlagSiblings = obj.numFits(fitType) > 1 ? 1 : 0;

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

const std::string CasdaComponent::componentID()
{
    return itsComponentID;
}

const double CasdaComponent::intFlux()
{
    return itsFluxInt.value();
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
        column.printEntry(stream, itsMaj_deconv);
    } else if (type == "MINDECONV") {
        column.printEntry(stream, itsMin_deconv);
    } else if (type == "PADECONV") {
        column.printEntry(stream, itsPA_deconv);
    } else if (type == "CHISQ") {
        column.printEntry(stream, itsChisq);
    } else if (type == "RMSFIT") {
        column.printEntry(stream, itsRMSfit);
    } else if (type == "ALPHA") {
        column.printEntry(stream, itsAlpha);
    } else if (type == "BETA") {
        column.printEntry(stream, itsBeta);
    } else if (type == "RMSIMAGE") {
        column.printEntry(stream, itsRMSimage);
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlagSiblings);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlagGuess);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlag3);
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

void CasdaComponent::checkCol(duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ISLAND") {
        column.check(itsIslandID);
    } else if (type == "ID") {
        column.check(itsComponentID);
    } else if (type == "NAME") {
        column.check(itsName);
    } else if (type == "RA") {
        column.check(itsRAs);
    } else if (type == "DEC") {
        column.check(itsDECs);
    } else if (type == "RAJD") {
        column.check(itsRA.value());
    } else if (type == "DECJD") {
        column.check(itsDEC.value());
    } else if (type == "RAERR") {
        column.check(itsRA.error());
    } else if (type == "DECERR") {
        column.check(itsDEC.error());
    } else if (type == "FREQ") {
        column.check(itsFreq);
    } else if (type == "FPEAK") {
        column.check(itsFluxPeak.value());
    } else if (type == "FPEAKERR") {
        column.check(itsFluxPeak.error());
    } else if (type == "FINT") {
        column.check(itsFluxInt.value());
    } else if (type == "FINTERR") {
        column.check(itsFluxInt.error());
    } else if (type == "MAJ") {
        column.check(itsMaj.value());
    } else if (type == "MIN") {
        column.check(itsMin.value());
    } else if (type == "PA") {
        column.check(itsPA.value());
    } else if (type == "MAJERR") {
        column.check(itsMaj.error());
    } else if (type == "MINERR") {
        column.check(itsMin.error());
    } else if (type == "PAERR") {
        column.check(itsPA.error());
    } else if (type == "MAJDECONV") {
        column.check(itsMaj_deconv);
    } else if (type == "MINDECONV") {
        column.check(itsMin_deconv);
    } else if (type == "PADECONV") {
        column.check(itsPA_deconv);
    } else if (type == "CHISQ") {
        column.check(itsChisq);
    } else if (type == "RMSFIT") {
        column.check(itsRMSfit);
    } else if (type == "ALPHA") {
        column.check(itsAlpha);
    } else if (type == "BETA") {
        column.check(itsBeta);
    } else if (type == "RMSIMAGE") {
        column.check(itsRMSimage);
    } else if (type == "FLAG1") {
        column.check(itsFlagSiblings);
    } else if (type == "FLAG2") {
        column.check(itsFlagGuess);
    } else if (type == "FLAG3") {
        column.check(itsFlag3);
    } else if (type == "FLAG4") {
        column.check(itsFlag4);
    } else if (type == "COMMENT") {
        column.check(itsComment);
    } else if (type == "LOCALID") {
        column.check(itsLocalID);
    } else if (type == "XPOS") {
        column.check(itsXpos);
    } else if (type == "YPOS") {
        column.check(itsYpos);
    } else if (type == "FINTISLAND") {
        column.check(itsFluxInt_island);
    } else if (type == "FPEAKISLAND") {
        column.check(itsFluxPeak_island);
    } else if (type == "NFREEFIT") {
        column.check(itsNfree_fit);
    } else if (type == "NDOFFIT") {
        column.check(itsNDoF_fit);
    } else if (type == "NPIXFIT") {
        column.check(itsNpix_fit);
    } else if (type == "NPIXISLAND") {
        column.check(itsNpix_island);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaComponent::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool allColumns)
{
    for (size_t i = 0; i < spec.size(); i++) {
        if ((spec.column(i).getDatatype() == "char") || allColumns) {
            this->checkCol(spec.column(i));
        }
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



}

}

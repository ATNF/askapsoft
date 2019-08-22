/// @file
///
/// Implementation of the CASDA Absorption object class
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
#include <catalogues/CasdaAbsorptionObject.h>
#include <catalogues/CasdaObject.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/Casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <sourcefitting/FitResults.h>
#include <outputs/CataloguePreparation.h>
#include <mathsutils/MathsUtils.h>
#include <coordutils/SpectralUtilities.h>
#include <imageaccess/CasaImageAccess.h>
#include <casainterface/CasaInterface.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/casa/Quanta/MVTime.h>
#include <casacore/images/Images/ImageInterface.h>
#include <boost/shared_ptr.hpp>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>

ASKAP_LOGGER(logger, ".casdaabsorptionobject");

namespace askap {

namespace analysis {

CasdaAbsorptionObject::CasdaAbsorptionObject():
    CasdaObject()
{
}

CasdaAbsorptionObject::CasdaAbsorptionObject(CasdaComponent &component,
        sourcefitting::RadioSource &obj,
        const LOFAR::ParameterSet &parset):
    CasdaObject(parset),
    itsFlag2(0),
    itsFlag3(0),
    itsComment("")
{

    itsImageID = parset.getString("image");

    boost::shared_ptr<casa::ImageInterface<Float> > imagePtr = analysisutilities::openImage(itsImageID);
    Quantity mjd = imagePtr->coordinates().obsInfo().obsDate().get("d");
    itsDate = casa::MVTime(mjd.getValue()).string(MVTime::FITS);
    itsComponentID = component.componentID();
    itsContinuumFlux = component.intFlux();

    // Define local variables that will get printed
    std::stringstream id;
    id << itsComponentID << "_" << obj.getID();
    itsObjectID = id.str();

    itsRA.value() = component.ra();
    itsDEC.value() = component.dec();

//    casa::Unit imageFreqUnits(obj.header().getSpectralUnits());
    casa::Unit imageFreqUnits(obj.header().WCS().cunit[obj.header().WCS().spec]);
    casa::Unit freqUnits(casda::freqUnit);
    double freqScale = casa::Quantity(1., imageFreqUnits).getValue(freqUnits);
    // itsFreq = zworld * freqScale;

    int lng = obj.header().WCS().lng;
    int precision = -int(log10(fabs(obj.header().WCS().cdelt[lng] * 3600. / 10.)));
    float pixscale = obj.header().getAvPixScale() * 3600.; // convert from pixels to arcsec
    itsRAs  = decToDMS(itsRA.value(), obj.header().lngtype(), precision);
    itsDECs = decToDMS(itsDEC.value(), obj.header().lattype(), precision);
    itsName = obj.header().getIAUName(itsRA.value(), itsDEC.value());

    double peakFluxscale = getPeakFluxConversionScale(obj.header(), casda::fluxUnit);
    double intFluxscale = getIntFluxConversionScale(obj.header(), casda::intFluxUnitSpectral);

    itsFreqUW.value() = obj.getVel() * freqScale;
    itsFreqW.value() = itsFreqUW.value() + (random() / (RAND_MAX + 1.0) - 0.5) * 0.1 * obj.getW50() * freqScale;

    // need to transform from zpeak with header/WCS
    float nuPeak = itsFreqUW.value() +
                   (random() / (RAND_MAX + 1.0) - 0.5) * 0.1 * obj.getW50() * freqScale;

    // Rest-frame HI frequency in our CASDA units
    const float HI = analysisutilities::nu0_HI / casda::freqScale;
    itsZHI_UW.value() = HI / itsFreqUW.value() - 1.;
    itsZHI_W.value() = HI / itsFreqW.value() - 1.;
    itsZHI_peak.value() = HI / nuPeak - 1.;

    itsW50.value() = obj.getW50();
    itsW20.value() = obj.getW20();

    itsRMSimagecube = obj.noiseLevel() * peakFluxscale;

    // @todo Optical depth calculations - rough & ready at present - assume constant component flux
//    itsOpticalDepth_peak = -1. * log(obj.getPeakFlux() / itsContinuumFlux);
    itsOpticalDepth_peak.value() = -1. * log(obj.getPeakFlux() / itsContinuumFlux);
    itsOpticalDepth_peak.error() = 0.;
    itsOpticalDepth_int.value() = -1. * log(obj.getIntegFlux() / itsContinuumFlux);

    // @todo - Need to add logic to measure resolvedness.
    itsFlagResolved = 1;

}

const float CasdaAbsorptionObject::ra()
{
    return itsRA.value();
}

const float CasdaAbsorptionObject::dec()
{
    return itsDEC.value();
}

const std::string CasdaAbsorptionObject::id()
{
    return itsObjectID;
}

void CasdaAbsorptionObject::printTableEntry(std::ostream &stream,
        duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "IMAGEID") {
        column.printEntry(stream, itsImageID);
    } else if (type == "DATEOBS") {
        column.printEntry(stream, itsDate);
    } else if (type == "COMP_ID") {
        column.printEntry(stream, itsComponentID);
    } else if (type == "CONTFLUX") {
        column.printEntry(stream, itsContinuumFlux);
    } else if (type == "ID") {
        column.printEntry(stream, itsObjectID);
    } else if (type == "NAME") {
        column.printEntry(stream, itsName);
    } else if (type == "RA") {
        column.printEntry(stream, itsRAs);
    } else if (type == "DEC") {
        column.printEntry(stream, itsDECs);
    } else if (type == "RAJD") {
        column.printEntry(stream, itsRA.value());
    } else if (type == "RAERR") {
        column.printEntry(stream, itsRA.error());
    } else if (type == "DECJD") {
        column.printEntry(stream, itsDEC.value());
    } else if (type == "DECERR") {
        column.printEntry(stream, itsDEC.error());
    } else if (type == "FREQ_UW") {
        column.printEntry(stream, itsFreqUW.value());
    } else if (type == "FREQ_UW_ERR") {
        column.printEntry(stream, itsFreqUW.error());
    } else if (type == "FREQ_W") {
        column.printEntry(stream, itsFreqW.value());
    } else if (type == "FREQ_W_ERR") {
        column.printEntry(stream, itsFreqW.error());
    } else if (type == "Z_HI_UW") {
        column.printEntry(stream, itsZHI_UW.value());
    } else if (type == "Z_HI_UW_ERR") {
        column.printEntry(stream, itsZHI_UW.error());
    } else if (type == "Z_HI_W") {
        column.printEntry(stream, itsZHI_W.value());
    } else if (type == "Z_HI_W_ERR") {
        column.printEntry(stream, itsZHI_W.error());
    } else if (type == "Z_HI_PEAK") {
        column.printEntry(stream, itsZHI_peak.value());
    } else if (type == "Z_HI_PEAK_ERR") {
        column.printEntry(stream, itsZHI_peak.error());
    } else if (type == "W50") {
        column.printEntry(stream, itsW50.value());
    } else if (type == "W50_ERR") {
        column.printEntry(stream, itsW50.error());
    } else if (type == "W20") {
        column.printEntry(stream, itsW20.value());
    } else if (type == "W20_ERR") {
        column.printEntry(stream, itsW20.error());
    } else if (type == "RMS_IMAGECUBE") {
        column.printEntry(stream, itsRMSimagecube);
    } else if (type == "OPT_DEPTH_PEAK") {
        column.printEntry(stream, itsOpticalDepth_peak.value());
    } else if (type == "OPT_DEPTH_PEAK_ERR") {
        column.printEntry(stream, itsOpticalDepth_peak.error());
    } else if (type == "OPT_DEPTH_INT") {
        column.printEntry(stream, itsOpticalDepth_int.value());
    } else if (type == "OPT_DEPTH_INT_ERR") {
        column.printEntry(stream, itsOpticalDepth_int.error());
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlagResolved);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlag2);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlag3);
    } else if (type == "COMMENT") {
        column.printEntry(stream, itsComment);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaAbsorptionObject::checkCol(duchamp::Catalogues::Column &column, bool checkTitle)
{
    bool checkPrec = false;
    std::string type = column.type();
    if (type == "IMAGEID") {
        column.check(itsImageID, checkTitle);
    } else if (type == "DATEOBS") {
        column.check(itsDate, checkTitle);
    } else if (type == "COMP_ID") {
        column.check(itsComponentID, checkTitle);
    } else if (type == "CONTFLUX") {
        column.check(itsContinuumFlux, checkTitle, checkPrec);
    } else if (type == "ID") {
        column.check(itsObjectID, checkTitle);
    } else if (type == "NAME") {
        column.check(itsName, checkTitle);
    } else if (type == "RA") {
        column.check(itsRAs, checkTitle);
    } else if (type == "DEC") {
        column.check(itsDECs, checkTitle);
    } else if (type == "RAJD") {
        column.check(itsRA.value(), checkTitle, checkPrec);
    } else if (type == "RAERR") {
        column.check(itsRA.error(), checkTitle, checkPrec);
    } else if (type == "DECJD") {
        column.check(itsDEC.value(), checkTitle, checkPrec);
    } else if (type == "DECERR") {
        column.check(itsDEC.error(), checkTitle, checkPrec);
    } else if (type == "FREQ_UW") {
        column.check(itsFreqUW.value(), checkTitle, checkPrec);
    } else if (type == "FREQ_UW_ERR") {
        column.check(itsFreqUW.error(), checkTitle, checkPrec);
    } else if (type == "FREQ_W") {
        column.check(itsFreqW.value(), checkTitle, checkPrec);
    } else if (type == "FREQ_W_ERR") {
        column.check(itsFreqW.error(), checkTitle, checkPrec);
    } else if (type == "Z_HI_UW") {
        column.check(itsZHI_UW.value(), checkTitle, checkPrec);
    } else if (type == "Z_HI_UW_ERR") {
        column.check(itsZHI_UW.error(), checkTitle, checkPrec);
    } else if (type == "Z_HI_W") {
        column.check(itsZHI_W.value(), checkTitle, checkPrec);
    } else if (type == "Z_HI_W_ERR") {
        column.check(itsZHI_W.error(), checkTitle, checkPrec);
    } else if (type == "Z_HI_PEAK") {
        column.check(itsZHI_peak.value(), checkTitle, checkPrec);
    } else if (type == "Z_HI_PEAK_ERR") {
        column.check(itsZHI_peak.error(), checkTitle, checkPrec);
    } else if (type == "W50") {
        column.check(itsW50.value(), checkTitle, checkPrec);
    } else if (type == "W50_ERR") {
        column.check(itsW50.error(), checkTitle, checkPrec);
    } else if (type == "W20") {
        column.check(itsW20.value(), checkTitle, checkPrec);
    } else if (type == "W20_ERR") {
        column.check(itsW20.error(), checkTitle, checkPrec);
    } else if (type == "RMS_IMAGECUBE") {
        column.check(itsRMSimagecube, checkTitle, checkPrec);
    } else if (type == "OPT_DEPTH_PEAK") {
        column.check(itsOpticalDepth_peak.value(), checkTitle, checkPrec);
    } else if (type == "OPT_DEPTH_PEAK_ERR") {
        column.check(itsOpticalDepth_peak.error(), checkTitle, checkPrec);
    } else if (type == "OPT_DEPTH_INT") {
        column.check(itsOpticalDepth_int.value(), checkTitle, checkPrec);
    } else if (type == "OPT_DEPTH_INT_ERR") {
        column.check(itsOpticalDepth_int.error(), checkTitle, checkPrec);
    } else if (type == "FLAG1") {
        column.check(itsFlagResolved, checkTitle);
    } else if (type == "FLAG2") {
        column.check(itsFlag2, checkTitle);
    } else if (type == "FLAG3") {
        column.check(itsFlag3, checkTitle);
    } else if (type == "COMMENT") {
        column.check(itsComment, checkTitle);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &blob,
                               CasdaAbsorptionObject& src)
{
    std::string s;
    double d;
    casda::ValueError v;

    s = src.itsImageID; blob << s;
    s = src.itsDate; blob << s;
    s = src.itsComponentID; blob << s;
    d = src.itsContinuumFlux; blob << d;
    s = src.itsObjectID; blob << s;
    s = src.itsName; blob << s;
    s = src.itsRAs; blob << s;
    s = src.itsDECs; blob << s;
    v = src.itsRA; blob << v;
    v = src.itsDEC; blob << v;
    v = src.itsFreqUW; blob << v;
    v = src.itsFreqW; blob << v;
    v = src.itsZHI_UW; blob << v;
    v = src.itsZHI_W; blob << v;
    v = src.itsZHI_peak; blob << v;
    v = src.itsW50; blob << v;
    v = src.itsW20; blob << v;
    d = src.itsRMSimagecube; blob << d;
    v = src.itsOpticalDepth_peak; blob << v;
    v = src.itsOpticalDepth_int; blob << v;
    s = src.itsComment; blob << s;

    return blob;
}

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &blob,
                               CasdaAbsorptionObject& src)
{
    std::string s;
    double d;
    casda::ValueError v;

    blob >> s; src.itsImageID = s;
    blob >> s; src.itsDate = s;
    blob >> s; src.itsComponentID = s;
    blob >> d; src.itsContinuumFlux = d;
    blob >> s; src.itsObjectID = s;
    blob >> s; src.itsName = s;
    blob >> s; src.itsRAs = s;
    blob >> s; src.itsDECs = s;
    blob >> v; src.itsRA = v;
    blob >> v; src.itsDEC = v;
    blob >> v; src.itsFreqUW = v;
    blob >> v; src.itsFreqW = v;
    blob >> v; src.itsZHI_UW = v;
    blob >> v; src.itsZHI_W = v;
    blob >> v; src.itsZHI_peak = v;
    blob >> v; src.itsW50 = v;
    blob >> v; src.itsW20 = v;
    blob >> d; src.itsRMSimagecube = d;
    blob >> v; src.itsOpticalDepth_peak = v;
    blob >> v; src.itsOpticalDepth_int = v;
    blob >> s; src.itsComment = s;

    return blob;
}

}
}

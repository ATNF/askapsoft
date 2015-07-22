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
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <sourcefitting/FitResults.h>
#include <outputs/CataloguePreparation.h>
#include <mathsutils/MathsUtils.h>
#include <coordutils/SpectralUtilities.h>

#include <Common/ParameterSet.h>
#include <casa/Quanta/Quantum.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>

ASKAP_LOGGER(logger, ".casdaabsorptionobject");

namespace askap {

namespace analysis {

CasdaAbsorptionObject::CasdaAbsorptionObject(CasdaComponent &component,
        sourcefitting::RadioSource &obj,
        const LOFAR::ParameterSet &parset):
    CatalogueEntry(parset),
    itsRA_err(0.),
    itsDEC_err(0.),
    itsFreqUW_err(0.),
    itsFreqW_err(0.),
    itsZHI_UW_err(0.),
    itsZHI_W_err(0.),
    itsZHI_peak_err(0.),
    itsW50_err(0.),
    itsW20_err(0.),
    itsOpticalDepth_peak_err(0.),
    itsOpticalDepth_int_err(0.),
    itsFlag2(0),
    itsFlag3(0),
    itsComment("")
{

    itsImageID = parset.getString("image");
    itsDate = "DATE-GOES-HERE";
    itsComponentID = component.componentID();
    itsContinuumFlux = component.intFlux();

    // Define local variables that will get printed
    std::stringstream id;
    id << itsComponentID << "_" << obj.getID();
    itsObjectID = id.str();

    itsRA = component.ra();
    itsDEC = component.dec();

//    casa::Unit imageFreqUnits(obj.header().getSpectralUnits());
    casa::Unit imageFreqUnits(obj.header().WCS().cunit[obj.header().WCS().spec]);
    casa::Unit freqUnits(casda::freqUnit);
    double freqScale = casa::Quantity(1., imageFreqUnits).getValue(freqUnits);
    // itsFreq = zworld * freqScale;

    int lng = obj.header().WCS().lng;
    int precision = -int(log10(fabs(obj.header().WCS().cdelt[lng] * 3600. / 10.)));
    float pixscale = obj.header().getAvPixScale() * 3600.; // convert from pixels to arcsec
    itsRAs  = decToDMS(itsRA, obj.header().lngtype(), precision);
    itsDECs = decToDMS(itsDEC, obj.header().lattype(), precision);
    itsName = obj.header().getIAUName(itsRA, itsDEC);

    casa::Unit imageFluxUnits(obj.header().getFluxUnits());
    casa::Unit fluxUnits(casda::fluxUnit);
    double peakFluxscale = casa::Quantity(1., imageFluxUnits).getValue(fluxUnits);
//    itsFluxPeak = gauss.height() * peakFluxscale;

    casa::Unit imageIntFluxUnits(obj.header().getIntFluxUnits());
    casa::Unit intFluxUnits(casda::intFluxUnit);
    double intFluxscale = casa::Quantity(1., imageIntFluxUnits).getValue(intFluxUnits);
    //  itsFluxInt = gauss.flux() * intFluxscale;
    // if (obj.header().needBeamSize()) {
    //     itsFluxInt /= obj.header().beam().area(); // Convert from mJy/beam to mJy
    // }

    float nuPeak = 1.; // need to transform from zpeak with header/WCS

    itsFreqUW = obj.getVel();
    itsFreqW = 1.;
    itsZHI_UW = analysisutilities::nu0_HI / itsFreqUW - 1.;
    itsZHI_W = analysisutilities::nu0_HI / itsFreqW - 1.;
    itsZHI_peak = analysisutilities::nu0_HI / nuPeak - 1.;

    itsW50 = obj.getW50();
    itsW20 = obj.getW20();

    itsRMSimagecube = obj.noiseLevel() * peakFluxscale;

    // Optical depth calculations - rough & ready at present - assume constant component flux
    itsOpticalDepth_peak = -1. * log(obj.getPeakFlux() / itsContinuumFlux);
    itsOpticalDepth_int = -1. * log(obj.getIntegFlux() / itsContinuumFlux);

    // Need to add logic to measure resolvedness.
    itsFlagResolved = 1;

}

const float CasdaAbsorptionObject::ra()
{
    return itsRA;
}

const float CasdaAbsorptionObject::dec()
{
    return itsDEC;
}

void CasdaAbsorptionObject::printTableRow(std::ostream &stream,
        duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
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
        column.printEntry(stream, itsRA);
    } else if (type == "DECJD") {
        column.printEntry(stream, itsDEC);
    } else if (type == "RAERR") {
        column.printEntry(stream, itsRA_err);
    } else if (type == "DECERR") {
        column.printEntry(stream, itsDEC_err);
    } else if (type == "FREQ_UW") {
        column.printEntry(stream, itsFreqUW);
    } else if (type == "FREQ_UW_ERR") {
        column.printEntry(stream, itsFreqUW_err);
    } else if (type == "FREQ_W") {
        column.printEntry(stream, itsFreqW);
    } else if (type == "FREQ_W_ERR") {
        column.printEntry(stream, itsFreqW_err);
    } else if (type == "Z_HI_UW") {
        column.printEntry(stream, itsZHI_UW);
    } else if (type == "Z_HI_UW_ERR") {
        column.printEntry(stream, itsZHI_UW_err);
    } else if (type == "Z_HI_W") {
        column.printEntry(stream, itsZHI_W);
    } else if (type == "Z_HI_W_ERR") {
        column.printEntry(stream, itsZHI_W_err);
    } else if (type == "Z_HI_PEAK") {
        column.printEntry(stream, itsZHI_peak);
    } else if (type == "Z_HI_PEAK_ERR") {
        column.printEntry(stream, itsZHI_peak_err);
    } else if (type == "W50") {
        column.printEntry(stream, itsW50);
    } else if (type == "W50_ERR") {
        column.printEntry(stream, itsW50_err);
    } else if (type == "W20") {
        column.printEntry(stream, itsW20);
    } else if (type == "W20_ERR") {
        column.printEntry(stream, itsW20_err);
    } else if (type == "RMS_IMAGECUBE") {
        column.printEntry(stream, itsRMSimagecube);
    } else if (type == "OPT_DEPTH_PEAK") {
        column.printEntry(stream, itsOpticalDepth_peak);
    } else if (type == "OPT_DEPTH_PEAK_ERR") {
        column.printEntry(stream, itsOpticalDepth_peak_err);
    } else if (type == "OPT_DEPTH_INT") {
        column.printEntry(stream, itsOpticalDepth_int);
    } else if (type == "OPT_DEPTH_INT_ERR") {
        column.printEntry(stream, itsOpticalDepth_int_err);
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

void CasdaAbsorptionObject::checkCol(duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "IMAGEID") {
        column.check(itsImageID);
    } else if (type == "DATEOBS") {
        column.check(itsDate);
    } else if (type == "COMP_ID") {
        column.check(itsComponentID);
    } else if (type == "CONTFLUX") {
        column.check(itsContinuumFlux);
    } else if (type == "ID") {
        column.check(itsObjectID);
    } else if (type == "NAME") {
        column.check(itsName);
    } else if (type == "RA") {
        column.check(itsRAs);
    } else if (type == "DEC") {
        column.check(itsDECs);
    } else if (type == "RAJD") {
        column.check(itsRA);
    } else if (type == "DECJD") {
        column.check(itsDEC);
    } else if (type == "RAERR") {
        column.check(itsRA_err);
    } else if (type == "DECERR") {
        column.check(itsDEC_err);
    } else if (type == "FREQ_UW") {
        column.check(itsFreqUW);
    } else if (type == "FREQ_UW_ERR") {
        column.check(itsFreqUW_err);
    } else if (type == "FREQ_W") {
        column.check(itsFreqW);
    } else if (type == "FREQ_W_ERR") {
        column.check(itsFreqW_err);
    } else if (type == "Z_HI_UW") {
        column.check(itsZHI_UW);
    } else if (type == "Z_HI_UW_ERR") {
        column.check(itsZHI_UW_err);
    } else if (type == "Z_HI_W") {
        column.check(itsZHI_W);
    } else if (type == "Z_HI_W_ERR") {
        column.check(itsZHI_W_err);
    } else if (type == "Z_HI_PEAK") {
        column.check(itsZHI_peak);
    } else if (type == "Z_HI_PEAK_ERR") {
        column.check(itsZHI_peak_err);
    } else if (type == "W50") {
        column.check(itsW50);
    } else if (type == "W50_ERR") {
        column.check(itsW50_err);
    } else if (type == "W20") {
        column.check(itsW20);
    } else if (type == "W20_ERR") {
        column.check(itsW20_err);
    } else if (type == "RMS_IMAGECUBE") {
        column.check(itsRMSimagecube);
    } else if (type == "OPT_DEPTH_PEAK") {
        column.check(itsOpticalDepth_peak);
    } else if (type == "OPT_DEPTH_PEAK_ERR") {
        column.check(itsOpticalDepth_peak_err);
    } else if (type == "OPT_DEPTH_INT") {
        column.check(itsOpticalDepth_int);
    } else if (type == "OPT_DEPTH_INT_ERR") {
        column.check(itsOpticalDepth_int_err);
    } else if (type == "FLAG1") {
        column.check(itsFlagResolved);
    } else if (type == "FLAG2") {
        column.check(itsFlag2);
    } else if (type == "FLAG3") {
        column.check(itsFlag3);
    } else if (type == "COMMENT") {
        column.check(itsComment);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaAbsorptionObject::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool allColumns)
{
    for (size_t i = 0; i < spec.size(); i++) {
        if ((spec.column(i).getDatatype() == "char") || allColumns) {
            this->checkCol(spec.column(i));
        }
    }
}




}

}

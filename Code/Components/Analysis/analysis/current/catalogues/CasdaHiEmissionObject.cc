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
#include <catalogues/CasdaHiEmissionObject.h>
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
#include <imageaccess/CasaImageAccess.h>
#include <casainterface/CasaInterface.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
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

CasdaHiEmissionObject::CasdaHiEmissionObject(sourcefitting::RadioSource &obj,
                                             const LOFAR::ParameterSet &parset):
    CatalogueEntry(parset),
    itsName(""),
    itsRAs_w(""),
    itsDECs_w(""),
    itsMajorAxis(0.),
    itsMinorAxis(0.),
    itsPositionAngle(0.),
    itsSizeX(0),
    itsSizeY(0),
    itsSizeZ(0),
    itsNumVoxels(0),
    itsFreq_peak(0.),
    itsVelHI_peak(0.),
    itsFluxMax(0.),
    itsFluxMin(0.),
    itsFluxMean(0.),
    itsFluxStddev(0.),
    itsFluxRMS(0.),
    itsRMSimagecube(0.),
    itsFlagResolved(0),
    itsFlag2(0),
    itsFlag3(0),
    itsComment("")
{

//    itsImageID = parset.getString("image");

    std::stringstream id;
    id << itsIDbase << obj.getID();
    itsObjectID = id.str();

    itsRA_w.value() = obj.getRA();
    itsDEC_w.value() = obj.getDec();

//    casa::Unit imageFreqUnits(obj.header().getSpectralUnits());
    casa::Unit imageFreqUnits(obj.header().WCS().cunit[obj.header().WCS().spec]);
    casa::Unit freqUnits(casda::freqUnit);
    double freqScale = casa::Quantity(1., imageFreqUnits).getValue(freqUnits);
    // itsFreq = zworld * freqScale;

    int lng = obj.header().WCS().lng;
    int precision = -int(log10(fabs(obj.header().WCS().cdelt[lng] * 3600. / 10.)));
    float pixscale = obj.header().getAvPixScale() * 3600.; // convert from pixels to arcsec
    itsRAs_w  = decToDMS(itsRA_w.value(), obj.header().lngtype(), precision);
    itsDECs_w = decToDMS(itsDEC_w.value(), obj.header().lattype(), precision);
    itsName = obj.header().getIAUName(itsRA_w.value(), itsDEC_w.value());

    double peakFluxscale = getPeakFluxConversionScale(obj.header(), casda::fluxUnit);
    double intFluxscale = getIntFluxConversionScale(obj.header(), casda::intFluxUnitSpectral);

    // Initial version of getting parameters - assuming we are in velocity units
    itsVelHI_uw.value() = obj.getVel();
    itsMajorAxis = obj.getMajorAxis() * 3600.;
    itsMinorAxis = obj.getMinorAxis() * 3600.;
    itsPositionAngle = obj.getPositionAngle();
    itsSizeX = obj.getXmax() - obj.getXmin() + 1;
    itsSizeY = obj.getYmax() - obj.getYmin() + 1;
    itsSizeZ = obj.getZmax() - obj.getZmin() + 1;
    itsNumVoxels = obj.getSize();
    itsW50_vel.value() = obj.getW50();
    itsW20_vel.value() = obj.getW20();
    itsIntegFlux.value() = obj.getIntegFlux();
    itsIntegFlux.error() = obj.getIntegFluxError();
    itsFluxMax = obj.getPeakFlux() * peakFluxscale;
    

    // itsFreqUW = obj.getVel() * freqScale;
    // itsFreqW = itsFreqUW + (random() / (RAND_MAX + 1.0) - 0.5) * 0.1 * obj.getW50() * freqScale;

    // need to transform from zpeak with header/WCS
    float nuPeak = itsFreq_uw.value() +
        (random() / (RAND_MAX + 1.0) - 0.5) * 0.1 * obj.getW50() * freqScale;

    // Rest-frame HI frequency in our CASDA units
    const float HI=analysisutilities::nu0_HI / casda::freqScale;

    // itsW50 = obj.getW50();
    // itsW20 = obj.getW20();

    itsRMSimagecube = obj.noiseLevel() * peakFluxscale;


    // @todo - Need to add logic to measure resolvedness.
    itsFlagResolved = 1;

}

const float CasdaHiEmissionObject::ra()
{
    return itsRA_w.value();
}

const float CasdaHiEmissionObject::dec()
{
    return itsDEC_w.value();
}

void CasdaHiEmissionObject::printTableRow(std::ostream &stream,
                                          duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
}

void CasdaHiEmissionObject::printTableEntry(std::ostream &stream,
                                            duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.printEntry(stream, itsObjectID);
    } else if (type == "NAME") {
        column.printEntry(stream, itsName);
    } else if (type == "RA") {
        column.printEntry(stream, itsRAs_w);
    } else if (type == "DEC") {
        column.printEntry(stream, itsDECs_w);
    } else if (type == "RA_W") {
        column.printEntry(stream, itsRA_w.value());
    } else if (type == "RA_W_ERR") {
        column.printEntry(stream, itsRA_w.error());
    } else if (type == "DEC_W") {
        column.printEntry(stream, itsDEC_w.value());
    } else if (type == "DEC_W_ERR") {
        column.printEntry(stream, itsDEC_w.error());
    } else if (type == "RA_UW") {
        column.printEntry(stream, itsRA_uw.value());
    } else if (type == "RA_UW_ERR") {
        column.printEntry(stream, itsRA_uw.error());
    } else if (type == "DEC_UW") {
        column.printEntry(stream, itsDEC_uw.value());
    } else if (type == "DEC_UW_ERR") {
        column.printEntry(stream, itsDEC_uw.error());
    } else if (type == "GLONG_W") {
        column.printEntry(stream, itsGlong_w.value());
    } else if (type == "GLONG_W_ERR") {
        column.printEntry(stream, itsGlong_w.error());
    } else if (type == "GLAT_W") {
        column.printEntry(stream, itsGlat_w.value());
    } else if (type == "GLAT_W_ERR") {
        column.printEntry(stream, itsGlat_w.error());
    } else if (type == "GLONG_UW") {
        column.printEntry(stream, itsGlong_uw.value());
    } else if (type == "GLONG_UW_ERR") {
        column.printEntry(stream, itsGlong_uw.error());
    } else if (type == "GLAT_UW") {
        column.printEntry(stream, itsGlat_uw.value());
    } else if (type == "GLAT_UW_ERR") {
        column.printEntry(stream, itsGlat_uw.error());
    } else if (type == "MAJ") {
        column.printEntry(stream, itsMajorAxis);
    } else if (type == "MIN") {
        column.printEntry(stream, itsMinorAxis);
    } else if (type == "PA") {
        column.printEntry(stream, itsPositionAngle);
    } else if (type == "MAJFIT") {
        column.printEntry(stream, itsMajorAxis_fit.value());
    } else if (type == "MAJFIT_ERR") {
        column.printEntry(stream, itsMajorAxis_fit.error());
    } else if (type == "MINFIT") {
        column.printEntry(stream, itsMinorAxis_fit.value());
    } else if (type == "MINFIT_ERR") {
        column.printEntry(stream, itsMinorAxis_fit.error());
    } else if (type == "PAFIT") {
        column.printEntry(stream, itsPositionAngle_fit.value());
    } else if (type == "PAFIT_ERR") {
        column.printEntry(stream, itsPositionAngle_fit.error());
    } else if (type == "SIZEX") {
        column.printEntry(stream, itsSizeX);
    } else if (type == "SIZEY") {
        column.printEntry(stream, itsSizeY);
    } else if (type == "SIZEZ") {
        column.printEntry(stream, itsSizeZ);
    } else if (type == "NVOX") {
        column.printEntry(stream, itsNumVoxels);
    } else if (type == "ASYMM2D") {
        column.printEntry(stream, itsAsymmetry2d.value());
    } else if (type == "ASYMM2D_ERR") {
        column.printEntry(stream, itsAsymmetry2d.error());
    } else if (type == "ASYMM3D") {
        column.printEntry(stream, itsAsymmetry3d.value());
    } else if (type == "ASYMM3D_ERR") {
        column.printEntry(stream, itsAsymmetry3d.error());
    } else if (type == "FREQ_UW") {
        column.printEntry(stream, itsFreq_uw.value());
    } else if (type == "FREQ_UW_ERR") {
        column.printEntry(stream, itsFreq_uw.error());
    } else if (type == "FREQ_W") {
        column.printEntry(stream, itsFreq_w.value());
    } else if (type == "FREQ_W_ERR") {
        column.printEntry(stream, itsFreq_w.error());
    } else if (type == "FREQ_PEAK") {
        column.printEntry(stream, itsFreq_peak);
    } else if (type == "VEL_UW") {
        column.printEntry(stream, itsVelHI_uw.value());
    } else if (type == "VEL_UW_ERR") {
        column.printEntry(stream, itsVelHI_uw.error());
    } else if (type == "VEL_W") {
        column.printEntry(stream, itsVelHI_w.value());
    } else if (type == "VEL_W_ERR") {
        column.printEntry(stream, itsVelHI_w.error());
    } else if (type == "VEL_PEAK") {
        column.printEntry(stream, itsVelHI_peak);
    } else if (type == "FINT") {
        column.printEntry(stream, itsIntegFlux.value());
    } else if (type == "FINT_ERR") {
        column.printEntry(stream, itsIntegFlux.error());
    } else if (type == "FLUXMAX") {
        column.printEntry(stream, itsFluxMax);
    } else if (type == "FLUXMIN") {
        column.printEntry(stream, itsFluxMin);
    } else if (type == "FLUXMEAN") {
        column.printEntry(stream, itsFluxMean);
    } else if (type == "FLUXSTDDEV") {
        column.printEntry(stream, itsFluxStddev);
    } else if (type == "FLUXRMS") {
        column.printEntry(stream, itsFluxRMS);
    } else if (type == "RMS_IMAGECUBE") {
        column.printEntry(stream, itsRMSimagecube);
    } else if (type == "W50_FREQ") {
        column.printEntry(stream, itsW50_freq.value());
    } else if (type == "W50_FREQ_ERR") {
        column.printEntry(stream, itsW50_freq.error());
    } else if (type == "CW50_FREQ") {
        column.printEntry(stream, itsCW50_freq.value());
    } else if (type == "CW50_FREQ_ERR") {
        column.printEntry(stream, itsCW50_freq.error());
    } else if (type == "W20_FREQ") {
        column.printEntry(stream, itsW20_freq.value());
    } else if (type == "W20_FREQ_ERR") {
        column.printEntry(stream, itsW20_freq.error());
    } else if (type == "CW20_FREQ") {
        column.printEntry(stream, itsCW20_freq.value());
    } else if (type == "CW20_FREQ_ERR") {
        column.printEntry(stream, itsCW20_freq.error());
    } else if (type == "W50_VEL") {
        column.printEntry(stream, itsW50_vel.value());
    } else if (type == "W50_VEL_ERR") {
        column.printEntry(stream, itsW50_vel.error());
    } else if (type == "CW50_VEL") {
        column.printEntry(stream, itsCW50_vel.value());
    } else if (type == "CW50_VEL_ERR") {
        column.printEntry(stream, itsCW50_vel.error());
    } else if (type == "W20_VEL") {
        column.printEntry(stream, itsW20_vel.value());
    } else if (type == "W20_VEL_ERR") {
        column.printEntry(stream, itsW20_vel.error());
    } else if (type == "CW20_VEL") {
        column.printEntry(stream, itsCW20_vel.value());
    } else if (type == "CW20_VEL_ERR") {
        column.printEntry(stream, itsCW20_vel.error());
    } else if (type == "FREQ_W50_UW") {
        column.printEntry(stream, itsFreq_W50clip_uw.value());
    } else if (type == "FREQ_W50_UW_ERR") {
        column.printEntry(stream, itsFreq_W50clip_uw.error());
    } else if (type == "FREQ_CW50_UW") {
        column.printEntry(stream, itsFreq_CW50clip_uw.value());
    } else if (type == "FREQ_CW50_UW_ERR") {
        column.printEntry(stream, itsFreq_CW50clip_uw.error());
    } else if (type == "FREQ_W20_UW") {
        column.printEntry(stream, itsFreq_W20clip_uw.value());
    } else if (type == "FREQ_W20_UW_ERR") {
        column.printEntry(stream, itsFreq_W20clip_uw.error());
    } else if (type == "FREQ_CW20_UW") {
        column.printEntry(stream, itsFreq_CW20clip_uw.value());
    } else if (type == "FREQ_CW20_UW_ERR") {
        column.printEntry(stream, itsFreq_CW20clip_uw.error());
    } else if (type == "VEL_W50_UW") {
        column.printEntry(stream, itsVelHI_W50clip_uw.value());
    } else if (type == "VEL_W50_UW_ERR") {
        column.printEntry(stream, itsVelHI_W50clip_uw.error());
    } else if (type == "VEL_CW50_UW") {
        column.printEntry(stream, itsVelHI_CW50clip_uw.value());
    } else if (type == "VEL_CW50_UW_ERR") {
        column.printEntry(stream, itsVelHI_CW50clip_uw.error());
    } else if (type == "VEL_W20_UW") {
        column.printEntry(stream, itsVelHI_W20clip_uw.value());
    } else if (type == "VEL_W20_UW_ERR") {
        column.printEntry(stream, itsVelHI_W20clip_uw.error());
    } else if (type == "VEL_CW20_UW") {
        column.printEntry(stream, itsVelHI_CW20clip_uw.value());
    } else if (type == "VEL_CW20_UW_ERR") {
        column.printEntry(stream, itsVelHI_CW20clip_uw.error());
    } else if (type == "FREQ_W50_W") {
        column.printEntry(stream, itsFreq_W50clip_w.value());
    } else if (type == "FREQ_W50_W_ERR") {
        column.printEntry(stream, itsFreq_W50clip_w.error());
    } else if (type == "FREQ_CW50_W") {
        column.printEntry(stream, itsFreq_CW50clip_w.value());
    } else if (type == "FREQ_CW50_W_ERR") {
        column.printEntry(stream, itsFreq_CW50clip_w.error());
    } else if (type == "FREQ_W20_W") {
        column.printEntry(stream, itsFreq_W20clip_w.value());
    } else if (type == "FREQ_W20_W_ERR") {
        column.printEntry(stream, itsFreq_W20clip_w.error());
    } else if (type == "FREQ_CW20_W") {
        column.printEntry(stream, itsFreq_CW20clip_w.value());
    } else if (type == "FREQ_CW20_W_ERR") {
        column.printEntry(stream, itsFreq_CW20clip_w.error());
    } else if (type == "VEL_W50_W") {
        column.printEntry(stream, itsVelHI_W50clip_w.value());
    } else if (type == "VEL_W50_W_ERR") {
        column.printEntry(stream, itsVelHI_W50clip_w.error());
    } else if (type == "VEL_CW50_W") {
        column.printEntry(stream, itsVelHI_CW50clip_w.value());
    } else if (type == "VEL_CW50_W_ERR") {
        column.printEntry(stream, itsVelHI_CW50clip_w.error());
    } else if (type == "VEL_W20_W") {
        column.printEntry(stream, itsVelHI_W20clip_w.value());
    } else if (type == "VEL_W20_W_ERR") {
        column.printEntry(stream, itsVelHI_W20clip_w.error());
    } else if (type == "VEL_CW20_W") {
        column.printEntry(stream, itsVelHI_CW20clip_w.value());
    } else if (type == "VEL_CW20_W_ERR") {
        column.printEntry(stream, itsVelHI_CW20clip_w.error());
    } else if (type == "FINT_W50") {
        column.printEntry(stream, itsIntegFlux_W50clip.value());
    } else if (type == "FINT_W50_ERR") {
        column.printEntry(stream, itsIntegFlux_W50clip.error());
    } else if (type == "FINT_CW50") {
        column.printEntry(stream, itsIntegFlux_CW50clip.value());
    } else if (type == "FINT_CW50_ERR") {
        column.printEntry(stream, itsIntegFlux_CW50clip.error());
    } else if (type == "FINT_W20") {
        column.printEntry(stream, itsIntegFlux_W20clip.value());
    } else if (type == "FINT_W20_ERR") {
        column.printEntry(stream, itsIntegFlux_W20clip.error());
    } else if (type == "FINT_CW20") {
        column.printEntry(stream, itsIntegFlux_CW20clip.value());
    } else if (type == "FINT_CW20_ERR") {
        column.printEntry(stream, itsIntegFlux_CW20clip.error());
    } else if (type == "BF_A") {
        column.printEntry(stream, itsBFfit_a.value());
    } else if (type == "BF_A_ERR") {
        column.printEntry(stream, itsBFfit_a.error());
    } else if (type == "BF_W") {
        column.printEntry(stream, itsBFfit_w.value());
    } else if (type == "BF_W_ERR") {
        column.printEntry(stream, itsBFfit_w.error());
    } else if (type == "BF_B1") {
        column.printEntry(stream, itsBFfit_b1.value());
    } else if (type == "BF_B1_ERR") {
        column.printEntry(stream, itsBFfit_b1.error());
    } else if (type == "BF_B2") {
        column.printEntry(stream, itsBFfit_b2.value());
    } else if (type == "BF_B2_ERR") {
        column.printEntry(stream, itsBFfit_b2.value());
    } else if (type == "BF_XE") {
        column.printEntry(stream, itsBFfit_xe.value());
    } else if (type == "BF_XE_ERR") {
        column.printEntry(stream, itsBFfit_xe.error());
    } else if (type == "BF_XP") {
        column.printEntry(stream, itsBFfit_xp.value());
    } else if (type == "BF_XP_ERR") {
        column.printEntry(stream, itsBFfit_xp.error());
    } else if (type == "BF_C") {
        column.printEntry(stream, itsBFfit_c.value());
    } else if (type == "BF_C_ERR") {
        column.printEntry(stream, itsBFfit_c.error());
    } else if (type == "BF_N") {
        column.printEntry(stream, itsBFfit_n.value());
    } else if (type == "BF_N_ERR") {
        column.printEntry(stream, itsBFfit_n.error());
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlagResolved);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlag2);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlag3);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaHiEmissionObject::checkCol(duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.check(itsObjectID);
    } else if (type == "NAME") {
        column.check(itsName);
    } else if (type == "RA") {
        column.check(itsRAs_w);
    } else if (type == "DEC") {
        column.check(itsDECs_w);
    } else if (type == "RA_W") {
        column.check(itsRA_w.value());
    } else if (type == "RA_W_ERR") {
        column.check(itsRA_w.error());
    } else if (type == "DEC_W") {
        column.check(itsDEC_w.value());
    } else if (type == "DEC_W_ERR") {
        column.check(itsDEC_w.error());
    } else if (type == "RA_UW") {
        column.check(itsRA_uw.value());
    } else if (type == "RA_UW_ERR") {
        column.check(itsRA_uw.error());
    } else if (type == "DEC_UW") {
        column.check(itsDEC_uw.value());
    } else if (type == "DEC_UW_ERR") {
        column.check(itsDEC_uw.error());
    } else if (type == "GLONG_W") {
        column.check(itsGlong_w.value());
    } else if (type == "GLONG_W_ERR") {
        column.check(itsGlong_w.error());
    } else if (type == "GLAT_W") {
        column.check(itsGlat_w.value());
    } else if (type == "GLAT_W_ERR") {
        column.check(itsGlat_w.error());
    } else if (type == "GLONG_UW") {
        column.check(itsGlong_uw.value());
    } else if (type == "GLONG_UW_ERR") {
        column.check(itsGlong_uw.error());
    } else if (type == "GLAT_UW") {
        column.check(itsGlat_uw.value());
    } else if (type == "GLAT_UW_ERR") {
        column.check(itsGlat_uw.error());
    } else if (type == "MAJ") {
        column.check(itsMajorAxis);
    } else if (type == "MIN") {
        column.check(itsMinorAxis);
    } else if (type == "PA") {
        column.check(itsPositionAngle);
    } else if (type == "MAJFIT") {
        column.check(itsMajorAxis_fit.value());
    } else if (type == "MAJFIT_ERR") {
        column.check(itsMajorAxis_fit.error());
    } else if (type == "MINFIT") {
        column.check(itsMinorAxis_fit.value());
    } else if (type == "MINFIT_ERR") {
        column.check(itsMinorAxis_fit.error());
    } else if (type == "PAFIT") {
        column.check(itsPositionAngle_fit.value());
    } else if (type == "PAFIT_ERR") {
        column.check(itsPositionAngle_fit.error());
    } else if (type == "SIZEX") {
        column.check(itsSizeX);
    } else if (type == "SIZEY") {
        column.check(itsSizeY);
    } else if (type == "SIZEZ") {
        column.check(itsSizeZ);
    } else if (type == "NVOX") {
        column.check(itsNumVoxels);
    } else if (type == "ASYMM2D") {
        column.check(itsAsymmetry2d.value());
    } else if (type == "ASYMM2D_ERR") {
        column.check(itsAsymmetry2d.error());
    } else if (type == "ASYMM3D") {
        column.check(itsAsymmetry3d.value());
    } else if (type == "ASYMM3D_ERR") {
        column.check(itsAsymmetry3d.error());
    } else if (type == "FREQ_UW") {
        column.check(itsFreq_uw.value());
    } else if (type == "FREQ_UW_ERR") {
        column.check(itsFreq_uw.error());
    } else if (type == "FREQ_W") {
        column.check(itsFreq_w.value());
    } else if (type == "FREQ_W_ERR") {
        column.check(itsFreq_w.error());
    } else if (type == "FREQ_PEAK") {
        column.check(itsFreq_peak);
    } else if (type == "VEL_UW") {
        column.check(itsVelHI_uw.value());
    } else if (type == "VEL_UW_ERR") {
        column.check(itsVelHI_uw.error());
    } else if (type == "VEL_W") {
        column.check(itsVelHI_w.value());
    } else if (type == "VEL_W_ERR") {
        column.check(itsVelHI_w.error());
    } else if (type == "VEL_PEAK") {
        column.check(itsVelHI_peak);
    } else if (type == "FINT") {
        column.check(itsIntegFlux.value());
    } else if (type == "FINT_ERR") {
        column.check(itsIntegFlux.error());
    } else if (type == "FLUXMAX") {
        column.check(itsFluxMax);
    } else if (type == "FLUXMIN") {
        column.check(itsFluxMin);
    } else if (type == "FLUXMEAN") {
        column.check(itsFluxMean);
    } else if (type == "FLUXSTDDEV") {
        column.check(itsFluxStddev);
    } else if (type == "FLUXRMS") {
        column.check(itsFluxRMS);
    } else if (type == "RMS_IMAGECUBE") {
        column.check(itsRMSimagecube);
    } else if (type == "W50_FREQ") {
        column.check(itsW50_freq.value());
    } else if (type == "W50_FREQ_ERR") {
        column.check(itsW50_freq.error());
    } else if (type == "CW50_FREQ") {
        column.check(itsCW50_freq.value());
    } else if (type == "CW50_FREQ_ERR") {
        column.check(itsCW50_freq.error());
    } else if (type == "W20_FREQ") {
        column.check(itsW20_freq.value());
    } else if (type == "W20_FREQ_ERR") {
        column.check(itsW20_freq.error());
    } else if (type == "CW20_FREQ") {
        column.check(itsCW20_freq.value());
    } else if (type == "CW20_FREQ_ERR") {
        column.check(itsCW20_freq.error());
    } else if (type == "W50_VEL") {
        column.check(itsW50_vel.value());
    } else if (type == "W50_VEL_ERR") {
        column.check(itsW50_vel.error());
    } else if (type == "CW50_VEL") {
        column.check(itsCW50_vel.value());
    } else if (type == "CW50_VEL_ERR") {
        column.check(itsCW50_vel.error());
    } else if (type == "W20_VEL") {
        column.check(itsW20_vel.value());
    } else if (type == "W20_VEL_ERR") {
        column.check(itsW20_vel.error());
    } else if (type == "CW20_VEL") {
        column.check(itsCW20_vel.value());
    } else if (type == "CW20_VEL_ERR") {
        column.check(itsCW20_vel.error());
    } else if (type == "FREQ_W50_UW") {
        column.check(itsFreq_W50clip_uw.value());
    } else if (type == "FREQ_W50_UW_ERR") {
        column.check(itsFreq_W50clip_uw.error());
    } else if (type == "FREQ_CW50_UW") {
        column.check(itsFreq_CW50clip_uw.value());
    } else if (type == "FREQ_CW50_UW_ERR") {
        column.check(itsFreq_CW50clip_uw.error());
    } else if (type == "FREQ_W20_UW") {
        column.check(itsFreq_W20clip_uw.value());
    } else if (type == "FREQ_W20_UW_ERR") {
        column.check(itsFreq_W20clip_uw.error());
    } else if (type == "FREQ_CW20_UW") {
        column.check(itsFreq_CW20clip_uw.value());
    } else if (type == "FREQ_CW20_UW_ERR") {
        column.check(itsFreq_CW20clip_uw.error());
    } else if (type == "VEL_W50_UW") {
        column.check(itsVelHI_W50clip_uw.value());
    } else if (type == "VEL_W50_UW_ERR") {
        column.check(itsVelHI_W50clip_uw.error());
    } else if (type == "VEL_CW50_UW") {
        column.check(itsVelHI_CW50clip_uw.value());
    } else if (type == "VEL_CW50_UW_ERR") {
        column.check(itsVelHI_CW50clip_uw.error());
    } else if (type == "VEL_W20_UW") {
        column.check(itsVelHI_W20clip_uw.value());
    } else if (type == "VEL_W20_UW_ERR") {
        column.check(itsVelHI_W20clip_uw.error());
    } else if (type == "VEL_CW20_UW") {
        column.check(itsVelHI_CW20clip_uw.value());
    } else if (type == "VEL_CW20_UW_ERR") {
        column.check(itsVelHI_CW20clip_uw.error());
    } else if (type == "FREQ_W50_W") {
        column.check(itsFreq_W50clip_w.value());
    } else if (type == "FREQ_W50_W_ERR") {
        column.check(itsFreq_W50clip_w.error());
    } else if (type == "FREQ_CW50_W") {
        column.check(itsFreq_CW50clip_w.value());
    } else if (type == "FREQ_CW50_W_ERR") {
        column.check(itsFreq_CW50clip_w.error());
    } else if (type == "FREQ_W20_W") {
        column.check(itsFreq_W20clip_w.value());
    } else if (type == "FREQ_W20_W_ERR") {
        column.check(itsFreq_W20clip_w.error());
    } else if (type == "FREQ_CW20_W") {
        column.check(itsFreq_CW20clip_w.value());
    } else if (type == "FREQ_CW20_W_ERR") {
        column.check(itsFreq_CW20clip_w.error());
    } else if (type == "VEL_W50_W") {
        column.check(itsVelHI_W50clip_w.value());
    } else if (type == "VEL_W50_W_ERR") {
        column.check(itsVelHI_W50clip_w.error());
    } else if (type == "VEL_CW50_W") {
        column.check(itsVelHI_CW50clip_w.value());
    } else if (type == "VEL_CW50_W_ERR") {
        column.check(itsVelHI_CW50clip_w.error());
    } else if (type == "VEL_W20_W") {
        column.check(itsVelHI_W20clip_w.value());
    } else if (type == "VEL_W20_W_ERR") {
        column.check(itsVelHI_W20clip_w.error());
    } else if (type == "VEL_CW20_W") {
        column.check(itsVelHI_CW20clip_w.value());
    } else if (type == "VEL_CW20_W_ERR") {
        column.check(itsVelHI_CW20clip_w.error());
    } else if (type == "FINT_W50") {
        column.check(itsIntegFlux_W50clip.value());
    } else if (type == "FINT_W50_ERR") {
        column.check(itsIntegFlux_W50clip.error());
    } else if (type == "FINT_CW50") {
        column.check(itsIntegFlux_CW50clip.value());
    } else if (type == "FINT_CW50_ERR") {
        column.check(itsIntegFlux_CW50clip.error());
    } else if (type == "FINT_W20") {
        column.check(itsIntegFlux_W20clip.value());
    } else if (type == "FINT_W20_ERR") {
        column.check(itsIntegFlux_W20clip.error());
    } else if (type == "FINT_CW20") {
        column.check(itsIntegFlux_CW20clip.value());
    } else if (type == "FINT_CW20_ERR") {
        column.check(itsIntegFlux_CW20clip.error());
    } else if (type == "BF_A") {
        column.check(itsBFfit_a.value());
    } else if (type == "BF_A_ERR") {
        column.check(itsBFfit_a.error());
    } else if (type == "BF_W") {
        column.check(itsBFfit_w.value());
    } else if (type == "BF_W_ERR") {
        column.check(itsBFfit_w.error());
    } else if (type == "BF_B1") {
        column.check(itsBFfit_b1.value());
    } else if (type == "BF_B1_ERR") {
        column.check(itsBFfit_b1.error());
    } else if (type == "BF_B2") {
        column.check(itsBFfit_b2.value());
    } else if (type == "BF_B2_ERR") {
        column.check(itsBFfit_b2.value());
    } else if (type == "BF_XE") {
        column.check(itsBFfit_xe.value());
    } else if (type == "BF_XE_ERR") {
        column.check(itsBFfit_xe.error());
    } else if (type == "BF_XP") {
        column.check(itsBFfit_xp.value());
    } else if (type == "BF_XP_ERR") {
        column.check(itsBFfit_xp.error());
    } else if (type == "BF_C") {
        column.check(itsBFfit_c.value());
    } else if (type == "BF_C_ERR") {
        column.check(itsBFfit_c.error());
    } else if (type == "BF_N") {
        column.check(itsBFfit_n.value());
    } else if (type == "BF_N_ERR") {
        column.check(itsBFfit_n.error());
    } else if (type == "FLAG1") {
        column.check(itsFlagResolved);
    } else if (type == "FLAG2") {
        column.check(itsFlag2);
    } else if (type == "FLAG3") {
        column.check(itsFlag3);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaHiEmissionObject::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool allColumns)
{
    for (size_t i = 0; i < spec.size(); i++) {
        if ((spec.column(i).getDatatype() == "char") || allColumns) {
            this->checkCol(spec.column(i));
        }
    }
}




}

}

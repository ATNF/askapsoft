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
#include <catalogues/Casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <extraction/HIdata.h>
#include <sourcefitting/RadioSource.h>
#include <sourcefitting/FitResults.h>
#include <outputs/CataloguePreparation.h>
#include <mathsutils/MathsUtils.h>
#include <coordutils/SpectralUtilities.h>
#include <imageaccess/CasaImageAccess.h>
#include <casainterface/CasaInterface.h>
#include <coordutils/PositionUtilities.h>
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
#include <duchamp/Utils/utils.hh>
#include <vector>

ASKAP_LOGGER(logger, ".casdaabsorptionobject");

namespace askap {

namespace analysis {

CasdaHiEmissionObject::CasdaHiEmissionObject():
    CatalogueEntry()
{
}

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

    LOFAR::ParameterSet hiParset = parset.makeSubset("HiEmissionCatalogue.");
    if (! hiParset.isDefined("imagetype")) {
        hiParset.add("imagetype", "fits");
    }

    HIdata hidata(parset);
    hidata.setSource(&obj);
    hidata.extract();
    if (hiParset.getBool("writeSpectra", "true")) {
        hidata.write();
    }

    double peakFluxscale = getPeakFluxConversionScale(obj.header(), casda::fluxUnit);

    int lng = obj.header().WCS().lng;
    int precision = -int(log10(fabs(obj.header().WCS().cdelt[lng] * 3600. / 10.)));
    float pixscale = obj.header().getAvPixScale() * 3600.; // convert from pixels to arcsec

    duchamp::FitsHeader newHead_freq = changeSpectralAxis(obj.header(), "FREQ-???", casda::freqUnit);
    ASKAPLOG_DEBUG_STR(logger, newHead_freq.WCS().ctype[newHead_freq.WCS().spec] << " " <<
                       strncmp(newHead_freq.WCS().ctype[newHead_freq.WCS().spec], "FREQ", 4));
    bool doFreq = (strncmp(newHead_freq.WCS().ctype[newHead_freq.WCS().spec], "FREQ", 4) == 0);
    if (! doFreq) {
        ASKAPLOG_ERROR_STR(logger,
                           "Conversion to Frequency-based WCS failed - cannot compute frequency-based quantities.");
    }
    duchamp::FitsHeader newHead_vel = changeSpectralAxis(obj.header(), "VOPT-???", casda::velocityUnit);
    ASKAPLOG_DEBUG_STR(logger, newHead_vel.WCS().ctype[newHead_vel.WCS().spec] << " " <<
                       strncmp(newHead_vel.WCS().ctype[newHead_vel.WCS().spec], "VOPT", 4));
    bool doVel = (strncmp(newHead_vel.WCS().ctype[newHead_vel.WCS().spec], "VOPT", 4) == 0);
    if (! doVel) {
        ASKAPLOG_ERROR_STR(logger,
                           "Conversion to Velocity-based WCS failed - cannot compute velocity-based quantities.");
    }
    double intFluxscale = getIntFluxConversionScale(newHead_vel, casda::intFluxUnitSpectral);

    casa::Unit imageFreqUnits(newHead_freq.WCS().cunit[obj.header().WCS().spec]);
    casa::Unit freqUnits(casda::freqUnit);
    double freqScale = casa::Quantity(1., imageFreqUnits).getValue(freqUnits);
    casa::Unit freqWidthUnits(casda::freqWidthUnit);
    double freqWidthScale = casa::Quantity(1., imageFreqUnits).getValue(freqWidthUnits);
    casa::Unit imageVelUnits(newHead_vel.WCS().cunit[obj.header().WCS().spec]);
    casa::Unit velUnits(casda::velocityUnit);
    double velScale = casa::Quantity(1., imageVelUnits).getValue(velUnits);

    double xpeak = obj.getXPeak(), ypeak = obj.getYPeak(), zpeak = obj.getZPeak();
    double xave = obj.getXaverage(), yave = obj.getYaverage(), zave = obj.getZaverage();
    double xcent = obj.getXCentroid(), ycent = obj.getYCentroid(), zcent = obj.getZCentroid();
    double ra, dec, spec;

    int flag;
    // Peak location - don't care about RA/DEC
    if (doVel) {
        // peak location
        flag = newHead_vel.pixToWCS(xpeak, ypeak, zpeak, ra, dec, spec);
        if (flag == 0) {
            itsVelHI_peak = spec * velScale;
        } else {
            ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                              << ", peak location, with code " << flag);
        }

        // Average (unweighted) location
        flag = newHead_vel.pixToWCS(xave, yave, zave, ra, dec, spec);
        if (flag == 0) {
            itsVelHI_uw.value() = spec * velScale;
            itsRA_uw.value() = ra;
            itsDEC_uw.value() = dec;
            analysisutilities::equatorialToGalactic(ra, dec, itsGlong_uw.value(), itsGlat_uw.value());
        } else {
            ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                              << ", unweighted location, with code " << flag);
        }

        // Centroid (flux-weighted) location
        flag = newHead_vel.pixToWCS(xcent, ycent, zcent, ra, dec, spec);
        if (flag == 0) {
            itsVelHI_w.value() = spec * velScale;
            itsRA_w.value() = ra;
            itsDEC_w.value() = dec;
            analysisutilities::equatorialToGalactic(ra, dec, itsGlong_w.value(), itsGlat_w.value());
            itsRAs_w  = decToDMS(itsRA_w.value(), obj.header().lngtype(), precision);
            itsDECs_w = decToDMS(itsDEC_w.value(), obj.header().lattype(), precision);
            itsName = obj.header().getIAUName(itsRA_w.value(), itsDEC_w.value());
        } else {
            ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                              << ", weighted location, with code " << flag);
        }
    }

    if (doFreq) {
        // frequency values for spectral location

        // Peak location
        flag = newHead_freq.pixToWCS(xpeak, ypeak, zpeak, ra, dec, spec);
        if (flag == 0) {
            itsFreq_peak = spec * freqScale;
        } else {
            ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                              << ", peak location, with code " << flag);
        }

        // Average (unweighted) location
        flag = newHead_freq.pixToWCS(xave, yave, zave, ra, dec, spec);
        if (flag == 0) {
            itsFreq_uw.value() = spec * freqScale;
        } else {
            ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                              << ", unweighted location, with code " << flag);
        }

        // Centroid (flux-weighted) location
        flag = newHead_freq.pixToWCS(xcent, ycent, zcent, ra, dec, spec);
        if (flag == 0) {
            itsFreq_w.value() = spec * freqScale;
        } else {
            ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                              << ", weighted location, with code " << flag);
        }

    }

    itsRMSimagecube = obj.noiseLevel() * peakFluxscale;

    itsMajorAxis = obj.getMajorAxis() * 60.; // major axis from Object is in arcmin
    itsMinorAxis = obj.getMinorAxis() * 60.;
    itsPositionAngle = obj.getPositionAngle();

    // From 2D Gaussian fit to the moment-zero image
    hidata.fitToMom0();
    itsMajorAxis_fit.value() = hidata.mom0Fit()[0] * 3600.;
    itsMajorAxis_fit.error() = hidata.mom0FitError()[0] * 3600.;
    itsMinorAxis_fit.value() = hidata.mom0Fit()[1] * 3600.;
    itsMinorAxis_fit.error() = hidata.mom0FitError()[1] * 3600.;
    itsPositionAngle_fit.value() = hidata.mom0Fit()[2] * 180. / M_PI;
    itsPositionAngle_fit.error() = hidata.mom0FitError()[2] * 180. / M_PI;
    itsSizeX = obj.getXmax() - obj.getXmin() + 1;
    itsSizeY = obj.getYmax() - obj.getYmin() + 1;
    itsSizeZ = obj.getZmax() - obj.getZmin() + 1;
    itsNumVoxels = obj.getSize();

    double spec1, spec2;
    int flag1, flag2;
    // @todo - how do we calculate errors on these quantities
    // W50 & W20 for frequency values:
    if (doFreq) {
        flag1 = newHead_freq.pixToWCS(xcent, ycent, obj.getZ50min(), ra, dec, spec1);
        flag2 = newHead_freq.pixToWCS(xcent, ycent, obj.getZ50max(), ra, dec, spec2);
        if ((flag1 == 0) && (flag2 == 0)) {
            itsW50_freq.value() = fabs(spec1 - spec2) * freqWidthScale;
            ASKAPLOG_DEBUG_STR(logger, "W50_FREQ: " << obj.getZ50min() << " " << obj.getZ50max() << " " <<
                               spec1 << " " << spec2 << " " << itsW50_freq.value());
        } else {
            if (flag1 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                                  << ", 50% flux width, with code " << flag1);
            }
            if (flag2 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                                  << ", 50% flux width, with code " << flag2);
            }
        }

        flag1 = newHead_freq.pixToWCS(xcent, ycent, obj.getZ20min(), ra, dec, spec1);
        flag2 = newHead_freq.pixToWCS(xcent, ycent, obj.getZ20max(), ra, dec, spec2);
        if ((flag1 == 0) && (flag2 == 0)) {
            itsW20_freq.value() = fabs(spec1 - spec2) * freqWidthScale;
        } else {
            if (flag1 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                                  << ", 20% flux width, with code " << flag1);
            }
            if (flag2 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for frequency units"
                                  << ", 20% flux width, with code " << flag2);
            }
        }
    }

    if (doVel) {
        // W50 & W20 for velocity values:
        flag1 = newHead_vel.pixToWCS(xcent, ycent, obj.getZ20min(), ra, dec, spec1);
        flag2 = newHead_vel.pixToWCS(xcent, ycent, obj.getZ20max(), ra, dec, spec2);
        if ((flag1 == 0) && (flag2 == 0)) {
            itsW20_vel.value() = fabs(spec1 - spec2) * velScale;
        } else {
            if (flag1 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                                  << ", 20% flux width, with code " << flag1);
            }
            if (flag2 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                                  << ", 20% flux width, with code " << flag2);
            }
        }
        flag1 = newHead_vel.pixToWCS(xcent, ycent, obj.getZ50min(), ra, dec, spec1);
        flag2 = newHead_vel.pixToWCS(xcent, ycent, obj.getZ50max(), ra, dec, spec2);
        if ((flag1 == 0) && (flag2 == 0)) {
            itsW50_vel.value() = fabs(spec1 - spec2) * velScale;
        } else {
            if (flag1 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                                  << ", 50% flux width, with code " << flag1);
            }
            if (flag2 != 0) {
                ASKAPLOG_WARN_STR(logger, "pix to world conversion failed for velocity units"
                                  << ", 50% flux width, with code " << flag2);
            }
        }
    }

    itsIntegFlux.value() = obj.getIntegFlux() * intFluxscale;
    //itsIntegFlux.error() = obj.getIntegFluxError();
    itsIntegFlux.error() = sqrt(itsNumVoxels) * itsRMSimagecube * (intFluxscale / peakFluxscale) *
                           newHead_vel.WCS().cdelt[newHead_vel.WCS().spec] * velScale /
                           newHead_vel.beam().area();

    // Voxel statistics
    hidata.findVoxelStats();
    itsFluxMax = hidata.fluxMax() * peakFluxscale;
    itsFluxMin = hidata.fluxMin() * peakFluxscale;
    itsFluxMean = hidata.fluxMean() * peakFluxscale;
    itsFluxStddev = hidata.fluxStddev() * peakFluxscale;
    itsFluxRMS = hidata.fluxRMS() * peakFluxscale;



    // @todo - Make moment-0 array, then fit single Gaussian to it.
    // Q - what if fit fails?

    // Busy Function fitting
    ASKAPLOG_INFO_STR(logger, "Fitting Busy function to spectrum of object " << itsObjectID);
    int status = hidata.busyFunctionFit();
    if (status == 0) {
        casa::Vector<double> BFparams = hidata.BFparams();
        ASKAPLOG_INFO_STR(logger, "BF results: " << BFparams);
        casa::Vector<double> BFerrors = hidata.BFerrors();
        const float channelFreqWidth = newHead_freq.WCS().cdelt[obj.header().WCS().spec] * freqScale;
        itsBFfit_a.value() = BFparams[0];
        itsBFfit_a.error() = BFerrors[0];
        itsBFfit_b1.value() = BFparams[1];
        itsBFfit_b1.error() = BFerrors[1];
        itsBFfit_b2.value() = BFparams[2];
        itsBFfit_b2.error() = BFerrors[2];
        itsBFfit_c.value() = BFparams[3];
        itsBFfit_c.error() = BFerrors[3];
        itsBFfit_xe.value() = BFparams[4] * channelFreqWidth;
        itsBFfit_xe.error() = BFerrors[4] * channelFreqWidth;
        itsBFfit_xp.value() = BFparams[5] * channelFreqWidth;
        itsBFfit_xp.error() = BFerrors[5] * channelFreqWidth;
        itsBFfit_w.value() = BFparams[6] * channelFreqWidth;
        itsBFfit_w.error() = BFerrors[6] * channelFreqWidth;
        itsBFfit_n.value() = BFparams[7];
        itsBFfit_n.error() = BFerrors[7];
    } else {
        ASKAPLOG_WARN_STR(logger, "Could not fit busy function to object " << itsObjectID);
    }

    // @todo - Need to add logic to measure resolvedness. Currently it
    // is "Is the mom0 map adequately fitted by a PSF-shaped Gaussian?
    // If so, it is not resolved."
    itsFlagResolved = hidata.mom0Resolved() ? 1 : 0;

}

const float CasdaHiEmissionObject::ra()
{
    return itsRA_w.value();
}

const float CasdaHiEmissionObject::dec()
{
    return itsDEC_w.value();
}

const std::string CasdaHiEmissionObject::id()
{
    return itsObjectID;
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

//**************************************************************//

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& blob, CasdaHiEmissionObject& src)
{
    std::string s;
    double d;
    unsigned int u;
    casda::ValueError v;
    int i;

    s = src.itsObjectID; blob << s;
    s = src.itsName; blob << s;
    s = src.itsRAs_w; blob << s;
    s = src.itsDECs_w; blob << s;
    v = src.itsRA_w; blob << v;
    v = src.itsDEC_w; blob << v;
    v = src.itsRA_uw; blob << v;
    v = src.itsDEC_uw; blob << v;
    v = src.itsGlong_w; blob << v;
    v = src.itsGlat_w; blob << v;
    v = src.itsGlong_uw; blob << v;
    v = src.itsGlat_uw; blob << v;
    d = src.itsMajorAxis; blob << d;
    d = src.itsMinorAxis; blob << d;
    d = src.itsPositionAngle; blob << d;
    v = src.itsMajorAxis_fit; blob << v;
    v = src.itsMinorAxis_fit; blob << v;
    v = src.itsPositionAngle_fit; blob << v;
    i = src.itsSizeX; blob << i;
    i = src.itsSizeY; blob << i;
    i = src.itsSizeZ; blob << i;
    i = src.itsNumVoxels; blob << i;
    v = src.itsAsymmetry2d; blob << v;
    v = src.itsAsymmetry3d; blob << v;
    v = src.itsFreq_uw; blob << v;
    v = src.itsFreq_w; blob << v;
    d = src.itsFreq_peak; blob << d;
    v = src.itsVelHI_uw; blob << v;
    v = src.itsVelHI_w; blob << v;
    d = src.itsVelHI_peak; blob << d;
    v = src.itsIntegFlux; blob << v;
    d = src.itsFluxMax; blob << d;
    d = src.itsFluxMin; blob << d;
    d = src.itsFluxMean; blob << d;
    d = src.itsFluxStddev; blob << d;
    d = src.itsFluxRMS; blob << d;
    d = src.itsRMSimagecube; blob << d;
    v = src.itsW50_freq; blob << v;
    v = src.itsW20_freq; blob << v;
    v = src.itsCW50_freq; blob << v;
    v = src.itsCW20_freq; blob << v;
    v = src.itsW50_vel; blob << v;
    v = src.itsW20_vel; blob << v;
    v = src.itsCW50_vel; blob << v;
    v = src.itsCW20_vel; blob << v;
    v = src.itsFreq_W50clip_uw; blob << v;
    v = src.itsFreq_W20clip_uw; blob << v;
    v = src.itsFreq_CW50clip_uw; blob << v;
    v = src.itsFreq_CW20clip_uw; blob << v;
    v = src.itsFreq_W50clip_w; blob << v;
    v = src.itsFreq_W20clip_w; blob << v;
    v = src.itsFreq_CW50clip_w; blob << v;
    v = src.itsFreq_CW20clip_w; blob << v;
    v = src.itsVelHI_W50clip_uw; blob << v;
    v = src.itsVelHI_W20clip_uw; blob << v;
    v = src.itsVelHI_CW50clip_uw; blob << v;
    v = src.itsVelHI_CW20clip_uw; blob << v;
    v = src.itsVelHI_W50clip_w; blob << v;
    v = src.itsVelHI_W20clip_w; blob << v;
    v = src.itsVelHI_CW50clip_w; blob << v;
    v = src.itsVelHI_CW20clip_w; blob << v;
    v = src.itsIntegFlux_W50clip; blob << v;
    v = src.itsIntegFlux_W20clip; blob << v;
    v = src.itsIntegFlux_CW50clip; blob << v;
    v = src.itsIntegFlux_CW20clip; blob << v;
    v = src.itsBFfit_a; blob << v;
    v = src.itsBFfit_w; blob << v;
    v = src.itsBFfit_b1; blob << v;
    v = src.itsBFfit_b2; blob << v;
    v = src.itsBFfit_xe; blob << v;
    v = src.itsBFfit_xp; blob << v;
    v = src.itsBFfit_c; blob << v;
    v = src.itsBFfit_n; blob << v;
    u = src.itsFlagResolved; blob << u;
    u = src.itsFlag2; blob << u;
    u = src.itsFlag3; blob << u;
    s = src.itsComment; blob << s;

    return blob;

}

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& blob, CasdaHiEmissionObject& src)
{
    std::string s;
    double d;
    casda::ValueError v;
    unsigned int u;
    int i;

    blob >> s; src.itsObjectID = s;
    blob >> s; src.itsName = s;
    blob >> s; src.itsRAs_w = s;
    blob >> s; src.itsDECs_w = s;
    blob >> v; src.itsRA_w = v;
    blob >> v; src.itsDEC_w = v;
    blob >> v; src.itsRA_uw = v;
    blob >> v; src.itsDEC_uw = v;
    blob >> v; src.itsGlong_w = v;
    blob >> v; src.itsGlat_w = v;
    blob >> v; src.itsGlong_uw = v;
    blob >> v; src.itsGlat_uw = v;
    blob >> d; src.itsMajorAxis = d;
    blob >> d; src.itsMinorAxis = d;
    blob >> d; src.itsPositionAngle = d;
    blob >> v; src.itsMajorAxis_fit = v;
    blob >> v; src.itsMinorAxis_fit = v;
    blob >> v; src.itsPositionAngle_fit = v;
    blob >> i; src.itsSizeX = i;
    blob >> i; src.itsSizeY = i;
    blob >> i; src.itsSizeZ = i;
    blob >> i; src.itsNumVoxels = i;
    blob >> v; src.itsAsymmetry2d = v;
    blob >> v; src.itsAsymmetry3d = v;
    blob >> v; src.itsFreq_uw = v;
    blob >> v; src.itsFreq_w = v;
    blob >> d; src.itsFreq_peak = d;
    blob >> v; src.itsVelHI_uw = v;
    blob >> v; src.itsVelHI_w = v;
    blob >> d; src.itsVelHI_peak = d;
    blob >> v; src.itsIntegFlux = v;
    blob >> d; src.itsFluxMax = d;
    blob >> d; src.itsFluxMin = d;
    blob >> d; src.itsFluxMean = d;
    blob >> d; src.itsFluxStddev = d;
    blob >> d; src.itsFluxRMS = d;
    blob >> d; src.itsRMSimagecube = d;
    blob >> v; src.itsW50_freq = v;
    blob >> v; src.itsW20_freq = v;
    blob >> v; src.itsCW50_freq = v;
    blob >> v; src.itsCW20_freq = v;
    blob >> v; src.itsW50_vel = v;
    blob >> v; src.itsW20_vel = v;
    blob >> v; src.itsCW50_vel = v;
    blob >> v; src.itsCW20_vel = v;
    blob >> v; src.itsFreq_W50clip_uw = v;
    blob >> v; src.itsFreq_W20clip_uw = v;
    blob >> v; src.itsFreq_CW50clip_uw = v;
    blob >> v; src.itsFreq_CW20clip_uw = v;
    blob >> v; src.itsFreq_W50clip_w = v;
    blob >> v; src.itsFreq_W20clip_w = v;
    blob >> v; src.itsFreq_CW50clip_w = v;
    blob >> v; src.itsFreq_CW20clip_w = v;
    blob >> v; src.itsVelHI_W50clip_uw = v;
    blob >> v; src.itsVelHI_W20clip_uw = v;
    blob >> v; src.itsVelHI_CW50clip_uw = v;
    blob >> v; src.itsVelHI_CW20clip_uw = v;
    blob >> v; src.itsVelHI_W50clip_w = v;
    blob >> v; src.itsVelHI_W20clip_w = v;
    blob >> v; src.itsVelHI_CW50clip_w = v;
    blob >> v; src.itsVelHI_CW20clip_w = v;
    blob >> v; src.itsIntegFlux_W50clip = v;
    blob >> v; src.itsIntegFlux_W20clip = v;
    blob >> v; src.itsIntegFlux_CW50clip = v;
    blob >> v; src.itsIntegFlux_CW20clip = v;
    blob >> v; src.itsBFfit_a = v;
    blob >> v; src.itsBFfit_w = v;
    blob >> v; src.itsBFfit_b1 = v;
    blob >> v; src.itsBFfit_b2 = v;
    blob >> v; src.itsBFfit_xe = v;
    blob >> v; src.itsBFfit_xp = v;
    blob >> v; src.itsBFfit_c = v;
    blob >> v; src.itsBFfit_n = v;
    blob >> u; src.itsFlagResolved = u;
    blob >> u; src.itsFlag2 = u;
    blob >> u; src.itsFlag3 = u;
    blob >> s; src.itsComment = s;

    return blob;
}

}

}

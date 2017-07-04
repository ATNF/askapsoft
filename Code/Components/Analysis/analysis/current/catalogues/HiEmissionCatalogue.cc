/// @file
///
/// Defining an HI emission-line object Catalogue
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
#include <catalogues/HiEmissionCatalogue.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <parallelanalysis/DistributedHIemission.h>
#include <catalogues/CasdaHiEmissionObject.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/Casda.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <vector>
#include <map>

ASKAP_LOGGER(logger, ".hiemissioncatalogue");

namespace askap {

namespace analysis {

HiEmissionCatalogue::HiEmissionCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                                         const LOFAR::ParameterSet &parset,
                                         duchamp::Cube *cube,
                                         askap::askapparallel::AskapParallel &comms):
    itsObjects(),
    itsSpec(),
    itsCube(cube),
    itsVersion("casda.sl_hi_emission_object_v0.11")
{
    this->defineSpec();

    DistributedHIemission distribHI(comms, parset, srclist);
    distribHI.distribute();
    distribHI.parameterise();
    distribHI.gather();
    itsObjects = distribHI.finalList();

    duchamp::Param par = parseParset(parset);
    std::string filenameBase = par.getOutFile();
    filenameBase.replace(filenameBase.rfind(".txt"),
                         std::string::npos, ".hiobjects");
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";

}

void HiEmissionCatalogue::defineObjects(std::vector<sourcefitting::RadioSource> &srclist,
                                        const LOFAR::ParameterSet &parset)
{
    std::vector<sourcefitting::RadioSource>::iterator obj;
    for (obj = srclist.begin(); obj != srclist.end(); obj++) {
        CasdaHiEmissionObject object(*obj, parset);
        itsObjects.push_back(object);
    }
}

void HiEmissionCatalogue::defineSpec()
{
    itsSpec.addColumn("ID", "object_id", "", 6, 0,
                      "meta.id;meta.main", "char", "col_object_id", "");
    itsSpec.addColumn("NAME", "object_name", "", 8, 0,
                      "meta.id", "char", "col_object_name", "");
    itsSpec.addColumn("RA", "ra_hms_w", "[" + casda::stringRAUnit + "]", 11, 0,
                      "pos.eq.ra", "char", "col_ra_hms_w", "J2000");
    itsSpec.addColumn("DEC", "dec_dms_w", "[" + casda::stringDECUnit + "]", 11, 0,
                      "pos.eq.dec", "char", "col_dec_dms_w", "J2000");
    itsSpec.addColumn("RA_W", "ra_deg_w", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.eq.ra;meta.main", "double", "col_ra_deg_w", "J2000");
    itsSpec.addColumn("RA_W_ERR", "ra_deg_w_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.ra;meta.main", "float", "col_ra_deg_w_err", "J2000");
    itsSpec.addColumn("DEC_W", "dec_deg_w", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.eq.dec;meta.main", "double", "col_dec_deg_w", "J2000");
    itsSpec.addColumn("DEC_W_ERR", "dec_deg_w_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.dec;meta.main", "float", "col_dec_deg_w_err", "J2000");
    itsSpec.addColumn("RA_UW", "ra_deg_uw", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.eq.ra", "double", "col_ra_deg_uw", "J2000");
    itsSpec.addColumn("RA_UW_ERR", "ra_deg_uw_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.ra", "float", "col_ra_deg_uw_err", "J2000");
    itsSpec.addColumn("DEC_UW", "dec_deg_uw", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.eq.dec", "double", "col_dec_deg_uw", "J2000");
    itsSpec.addColumn("DEC_UW_ERR", "dec_deg_uw_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.dec", "float", "col_dec_deg_uw_err", "J2000");
    itsSpec.addColumn("GLONG_W", "glong_w", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.galactic.lon;meta.main", "double", "col_glong_w", "J2000");
    itsSpec.addColumn("GLONG_W_ERR", "glong_w_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.galactic.lon;meta.main", "float", "col_glong_w_err", "J2000");
    itsSpec.addColumn("GLAT_W", "glat_w", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.galactic.lat;meta.main", "double", "col_glat_w", "J2000");
    itsSpec.addColumn("GLAT_W_ERR", "glat_w_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.galactic.lat;meta.main", "float", "col_glat_w_err", "J2000");
    itsSpec.addColumn("GLONG_UW", "glong_uw", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.galactic.lon;meta.main", "double", "col_glong_uw", "J2000");
    itsSpec.addColumn("GLONG_UW_ERR", "glong_uw_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.galactic.lon;meta.main", "float", "col_glong_uw_err", "J2000");
    itsSpec.addColumn("GLAT_UW", "glat_uw", "[" + casda::positionUnit + "]", 11, casda::precPos,
                      "pos.galactic.lat;meta.main", "double", "col_glat_uw", "J2000");
    itsSpec.addColumn("GLAT_UW_ERR", "glat_uw_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.galactic.lat;meta.main", "float", "col_glat_uw_err", "J2000");
    itsSpec.addColumn("MAJ", "maj_axis", "[arcsec]", 6, casda::precSize,
                      "askap:src.smajAxis;em.radio", "float", "col_maj_axis", "");
    itsSpec.addColumn("MIN", "min_axis", "[arcsec]", 6, casda::precSize,
                      "askap:src.sminAxis;em.radio", "float", "col_min_axis", "");
    itsSpec.addColumn("PA", "pos_ang", "[deg]", 7, casda::precSize,
                      "askap:src.posAng;em.radio", "float", "col_pos_ang", "");
    itsSpec.addColumn("MAJFIT", "maj_axis_fit", "[arcsec]", 6, casda::precSize,
                      "askap:src.smajAxis;em.radio;stat.fit", "float", "col_maj_axis_fit", "");
    itsSpec.addColumn("MAJFIT_ERR", "maj_axis_fit_err", "[arcsec]", 6, casda::precSize,
                      "stat.error;askap:src.smajAxis;em.radio;stat.fit",
                      "float", "col_maj_axis_fit_err", "");
    itsSpec.addColumn("MINFIT", "min_axis_fit", "[arcsec]", 6, casda::precSize,
                      "askap:src.sminAxis;em.radio;stat.fit", "float", "col_min_axis_fit", "");
    itsSpec.addColumn("MINFIT_ERR", "min_axis_fit_err", "[arcsec]", 6, casda::precSize,
                      "stat.error;askap:src.sminAxis;em.radio;stat.fit",
                      "float", "col_min_axis_fit_err", "");
    itsSpec.addColumn("PAFIT", "pos_ang_fit", "[deg]", 7, casda::precSize,
                      "askap:src.posAng;em.radio;stat.fit", "float", "col_pos_ang_fit", "");
    itsSpec.addColumn("PAFIT_ERR", "pos_ang_fit_err", "[deg]", 7, casda::precSize,
                      "stat.error;askap:src.posAng;em.radio;stat.fit",
                      "float", "col_pos_ang_fit_err", "");
    itsSpec.addColumn("SIZEX", "size_x", "", 6, 0,
                      "askap:src.size;instr.pixel", "int", "col_size_x", "");
    itsSpec.addColumn("SIZEY", "size_y", "", 6, 0,
                      "askap:src.size;instr.pixel", "int", "col_size_y", "");
    itsSpec.addColumn("SIZEZ", "size_z", "", 6, 0,
                      "askap:src.size;spect.binSize", "int", "col_size_z", "");
    itsSpec.addColumn("NVOX", "n_vox", "", 9, 0,
                      "askap:src.size;askap:instr.voxel", "int", "col_n_vox", "");
    itsSpec.addColumn("ASYMM2D", "asymmetry_2d", "", 6, 3,
                      "askap:src.asymmetry.2d", "float", "col_asymmetry_2d", "");
    itsSpec.addColumn("ASYMM2D_ERR", "asymmetry_2d_err", "", 6, 3,
                      "stat.error;askap:src.asymmetry.2d", "float", "col_asymmetry_2d_err", "");
    itsSpec.addColumn("ASYMM3D", "asymmetry_3d", "", 6, 3,
                      "askap:src.asymmetry.3d", "float", "col_asymmetry_3d", "");
    itsSpec.addColumn("ASYMM3D_ERR", "asymmetry_3d_err", "", 6, 3,
                      "stat.error;askap:src.asymmetry.3d", "float", "col_asymmetry_3d_err", "");
    itsSpec.addColumn("FREQ_UW", "freq_uw", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_uw", "");
    itsSpec.addColumn("FREQ_UW_ERR", "freq_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_uw_err", "");
    itsSpec.addColumn("FREQ_W", "freq_w", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq;meta.main",
                      "double", "col_freq_w", "");
    itsSpec.addColumn("FREQ_W_ERR", "freq_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq;meta.main",
                      "double", "col_freq_w_err", "");
    itsSpec.addColumn("FREQ_PEAK", "freq_peak", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq;phot.flux.density;stat.max",
                      "double", "col_freq_peak", "");
    itsSpec.addColumn("VEL_UW", "vel_uw", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral,
                      "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_uw", "");
    itsSpec.addColumn("VEL_UW_ERR", "vel_uw_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral,
                      "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_uw_err", "");
    itsSpec.addColumn("VEL_W", "vel_w", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral,
                      "spect.dopplerVeloc.opt;em.line.HI;meta.main",
                      "float", "col_vel_w", "");
    itsSpec.addColumn("VEL_W_ERR", "vel_w_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral,
                      "stat.error;spect.dopplerVeloc.opt;em.line.HI;meta.main",
                      "float", "col_vel_w_err", "");
    itsSpec.addColumn("VEL_PEAK", "vel_peak", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral,
                      "spect.dopplerVeloc.opt;em.line.HI;phot.flux.density;stat.max",
                      "float", "col_vel_peak", "");
    itsSpec.addColumn("FINT", "integ_flux", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux", "");
    itsSpec.addColumn("FINT_ERR", "integ_flux_err", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "stat.error;phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_err", "");
    itsSpec.addColumn("FLUXMAX", "flux_voxel_max", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "askap:phot.flux.density.voxel;stat.max;em.radio",
                      "float", "col_flux_voxel_max", "");
    itsSpec.addColumn("FLUXMIN", "flux_voxel_min", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "askap:phot.flux.density.voxel;stat.min;em.radio",
                      "float", "col_flux_voxel_min", "");
    itsSpec.addColumn("FLUXMEAN", "flux_voxel_mean", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "askap:phot.flux.density.voxel;stat.mean;em.radio",
                      "float", "col_flux_voxel_mean", "");
    itsSpec.addColumn("FLUXSTDDEV", "flux_voxel_stddev", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "askap:phot.flux.density.voxel;stat.stdev;em.radio",
                      "float", "col_flux_voxel_stddev", "");
    itsSpec.addColumn("FLUXRMS", "flux_voxel_rms", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "askap:phot.flux.density.voxel;askap:stat.rms;em.radio",
                      "float", "col_flux_voxel_rms", "");
    itsSpec.addColumn("RMS_IMAGECUBE", "rms_imagecube", "[" + casda::fluxUnit + "]", 10,
                      casda::precFlux, "stat.stdev;phot.flux.density",
                      "float", "col_rms_imagecube", "");
    itsSpec.addColumn("W50_FREQ", "w50_freq", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "askap:em.freq.width",
                      "float", "col_w50_freq", "");
    itsSpec.addColumn("W50_FREQ_ERR", "w50_freq_err", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:em.freq.width",
                      "float", "col_w50_freq_err", "");
    itsSpec.addColumn("CW50_FREQ", "cw50_freq", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "askap:em.freq.width",
                      "float", "col_cw50_freq", "");
    itsSpec.addColumn("CW50_FREQ_ERR", "cw50_freq_err", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:em.freq.width",
                      "float", "col_cw50_freq_err", "");
    itsSpec.addColumn("W20_FREQ", "w20_freq", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "askap:em.freq.width",
                      "float", "col_w20_freq", "");
    itsSpec.addColumn("W20_FREQ_ERR", "w20_freq_err", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:em.freq.width",
                      "float", "col_w20_freq_err", "");
    itsSpec.addColumn("CW20_FREQ", "cw20_freq", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "askap:em.freq.width",
                      "float", "col_cw20_freq", "");
    itsSpec.addColumn("CW20_FREQ_ERR", "cw20_freq_err", "[" + casda::freqWidthUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:em.freq.width",
                      "float", "col_cw20_freq_err", "");
    itsSpec.addColumn("W50_VEL", "w50_vel", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "askap:spect.dopplerVeloc.width",
                      "float", "col_w50_vel", "");
    itsSpec.addColumn("W50_VEL_ERR", "w50_vel_err", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:spect.dopplerVeloc.width",
                      "float", "col_w50_vel_err", "");
    itsSpec.addColumn("CW50_VEL", "cw50_vel", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "askap:spect.dopplerVeloc.width",
                      "float", "col_cw50_vel", "");
    itsSpec.addColumn("CW50_VEL_ERR", "cw50_vel_err", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:spect.dopplerVeloc.width",
                      "float", "col_cw50_vel_err", "");
    itsSpec.addColumn("W20_VEL", "w20_vel", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "askap:askap:spect.dopplerVeloc.width",
                      "float", "col_w20_vel", "");
    itsSpec.addColumn("W20_VEL_ERR", "w20_vel_err", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:spect.dopplerVeloc.width",
                      "float", "col_w20_vel_err", "");
    itsSpec.addColumn("CW20_VEL", "cw20_vel", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "askap:askap:spect.dopplerVeloc.width",
                      "float", "col_cw20_vel", "");
    itsSpec.addColumn("CW20_VEL_ERR", "cw20_vel_err", "[" + casda::velocityUnit + "]", 11,
                      casda::precSpecWidth, "stat.error;askap:spect.dopplerVeloc.width",
                      "float", "col_cw20_vel_err", "");
    itsSpec.addColumn("FREQ_W50_UW", "freq_w50_clip_uw", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_w50_clip_uw", "");
    itsSpec.addColumn("FREQ_W50_UW_ERR", "freq_w50_clip_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_w50_clip_uw_err", "");
    itsSpec.addColumn("FREQ_CW50_UW", "freq_cw50_clip_uw", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_cw50_clip_uw", "");
    itsSpec.addColumn("FREQ_CW50_UW_ERR", "freq_cw50_clip_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_cw50_clip_uw_err", "");
    itsSpec.addColumn("FREQ_W20_UW", "freq_w20_clip_uw", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_w20_clip_uw", "");
    itsSpec.addColumn("FREQ_W20_UW_ERR", "freq_w20_clip_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_w20_clip_uw_err", "");
    itsSpec.addColumn("FREQ_CW20_UW", "freq_cw20_clip_uw", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_cw20_clip_uw", "");
    itsSpec.addColumn("FREQ_CW20_UW_ERR", "freq_cw20_clip_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_cw20_clip_uw_err", "");
    itsSpec.addColumn("VEL_W50_UW", "vel_w50_clip_uw", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w50_clip_uw", "");
    itsSpec.addColumn("VEL_W50_UW_ERR", "vel_w50_clip_uw_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w50_clip_uw_err", "");
    itsSpec.addColumn("VEL_CW50_UW", "vel_cw50_clip_uw", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw50_clip_uw", "");
    itsSpec.addColumn("VEL_CW50_UW_ERR", "vel_cw50_clip_uw_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw50_clip_uw_err", "");
    itsSpec.addColumn("VEL_W20_UW", "vel_w20_clip_uw", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w20_clip_uw", "");
    itsSpec.addColumn("VEL_W20_UW_ERR", "vel_w20_clip_uw_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w20_clip_uw_err", "");
    itsSpec.addColumn("VEL_CW20_UW", "vel_cw20_clip_uw", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw20_clip_uw", "");
    itsSpec.addColumn("VEL_CW20_UW_ERR", "vel_cw20_clip_uw_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw20_clip_uw_err", "");
    itsSpec.addColumn("FREQ_W50_W", "freq_w50_clip_w", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_w50_clip_w", "");
    itsSpec.addColumn("FREQ_W50_W_ERR", "freq_w50_clip_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_w50_clip_w_err", "");
    itsSpec.addColumn("FREQ_CW50_W", "freq_cw50_clip_w", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_cw50_clip_w", "");
    itsSpec.addColumn("FREQ_CW50_W_ERR", "freq_cw50_clip_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_cw50_clip_w_err", "");
    itsSpec.addColumn("FREQ_W20_W", "freq_w20_clip_w", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_w20_clip_w", "");
    itsSpec.addColumn("FREQ_W20_W_ERR", "freq_w20_clip_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_w20_clip_w_err", "");
    itsSpec.addColumn("FREQ_CW20_W", "freq_cw20_clip_w", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "double", "col_freq_cw20_clip_w", "");
    itsSpec.addColumn("FREQ_CW20_W_ERR", "freq_cw20_clip_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "double", "col_freq_cw20_clip_w_err", "");
    itsSpec.addColumn("VEL_W50_W", "vel_w50_clip_w", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w50_clip_w", "");
    itsSpec.addColumn("VEL_W50_W_ERR", "vel_w50_clip_w_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w50_clip_w_err", "");
    itsSpec.addColumn("VEL_CW50_W", "vel_cw50_clip_w", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw50_clip_w", "");
    itsSpec.addColumn("VEL_CW50_W_ERR", "vel_cw50_clip_w_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw50_clip_w_err", "");
    itsSpec.addColumn("VEL_W20_W", "vel_w20_clip_w", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w20_clip_w", "");
    itsSpec.addColumn("VEL_W20_W_ERR", "vel_w20_clip_w_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_w20_clip_w_err", "");
    itsSpec.addColumn("VEL_CW20_W", "vel_cw20_clip_w", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw20_clip_w", "");
    itsSpec.addColumn("VEL_CW20_W_ERR", "vel_cw20_clip_w_err", "[" + casda::velocityUnit + "]",
                      11, casda::precVelSpectral, "stat.error;spect.dopplerVeloc.opt;em.line.HI",
                      "float", "col_vel_cw20_clip_w_err", "");
    itsSpec.addColumn("FINT_W50", "integ_flux_w50_clip", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_w50_clip", "");
    itsSpec.addColumn("FINT_W50_ERR", "integ_flux_w50_clip_err", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "stat.error;phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_w50_clip_err", "");
    itsSpec.addColumn("FINT_CW50", "integ_flux_cw50_clip", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_cw50_clip", "");
    itsSpec.addColumn("FINT_CW50_ERR", "integ_flux_cw50_clip_err", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "stat.error;phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_cw50_clip_err", "");
    itsSpec.addColumn("FINT_W20", "integ_flux_w20_clip", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_w20_clip", "");
    itsSpec.addColumn("FINT_W20_ERR", "integ_flux_w20_clip_err", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "stat.error;phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_w20_clip_err", "");
    itsSpec.addColumn("FINT_CW20", "integ_flux_cw20_clip", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_cw20_clip", "");
    itsSpec.addColumn("FINT_CW20_ERR", "integ_flux_cw20_clip_err", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux,
                      "stat.error;phot.flux.density;askap:arith.integrated;em.radio",
                      "float", "col_integ_flux_cw20_clip_err", "");
    itsSpec.addColumn("BF_A", "bf_a", "[" + casda::intFluxUnitSpectral + "]", 10, casda::precFlux,
                      "stat.fit.param", "float", "col_bf_a", "");
    itsSpec.addColumn("BF_A_ERR", "bf_a_err", "[" + casda::intFluxUnitSpectral + "]",
                      10, casda::precFlux, "stat.error;stat.fit.param",
                      "float", "col_bf_a_err", "");
    itsSpec.addColumn("BF_W", "bf_w", "[" + casda::freqUnit + "]", 10, casda::precFreqSpectral,
                      "stat.fit.param", "double", "col_bf_w", "");
    itsSpec.addColumn("BF_W_ERR", "bf_w_err", "[" + casda::freqUnit + "]",
                      10, casda::precFreqSpectral, "stat.error;stat.fit.param",
                      "double", "col_bf_w_err", "");
    itsSpec.addColumn("BF_B1", "bf_b1", "", 10, casda::precFlux,
                      "stat.fit.param", "float", "col_bf_b1", "");
    itsSpec.addColumn("BF_B1_ERR", "bf_b1_err", "", 10, casda::precFlux,
                      "stat.error;stat.fit.param", "float", "col_bf_b1_err", "");
    itsSpec.addColumn("BF_B2", "bf_b2", "", 10, casda::precFlux,
                      "stat.fit.param", "float", "col_bf_b2", "");
    itsSpec.addColumn("BF_B2_ERR", "bf_b2_err", "", 10, casda::precFlux,
                      "stat.error;stat.fit.param", "float", "col_bf_b2_err", "");
    itsSpec.addColumn("BF_XE", "bf_xe", "[" + casda::freqUnit + "]", 10, casda::precFreqSpectral,
                      "stat.fit.param", "double", "col_bf_xe", "");
    itsSpec.addColumn("BF_XE_ERR", "bf_xe_err", "[" + casda::freqUnit + "]",
                      10, casda::precFreqSpectral, "stat.error;stat.fit.param",
                      "double", "col_bf_xe_err", "");
    itsSpec.addColumn("BF_XP", "bf_xp", "[" + casda::freqUnit + "]", 10, casda::precFreqSpectral,
                      "stat.fit.param", "double", "col_bf_xp", "");
    itsSpec.addColumn("BF_XP_ERR", "bf_xp_err", "[" + casda::freqUnit + "]",
                      10, casda::precFreqSpectral, "stat.error;stat.fit.param",
                      "double", "col_bf_xp_err", "");
    itsSpec.addColumn("BF_C", "bf_c", "", 10, casda::precFlux,
                      "stat.fit.param", "float", "col_bf_c", "");
    itsSpec.addColumn("BF_C_ERR", "bf_c_err", "", 10, casda::precFlux,
                      "stat.error;stat.fit.param", "float", "col_bf_c_err", "");
    itsSpec.addColumn("BF_N", "bf_n", "", 10, casda::precFlux,
                      "stat.fit.param", "float", "col_bf_n", "");
    itsSpec.addColumn("BF_N_ERR", "bf_n_err", "", 10, casda::precFlux,
                      "stat.error;stat.fit.param", "float", "col_bf_n_err", "");
    itsSpec.addColumn("FLAG1", "flag_resolved", "", 5, 0,
                      "meta.code", "int", "col_flag_resolved", "");
    itsSpec.addColumn("FLAG2", "flag_s2", "", 5, 0,
                      "meta.code", "int", "col_flag_s2", "");
    itsSpec.addColumn("FLAG3", "flag_s3", "", 5, 0,
                      "meta.code", "int", "col_flag_s3", "");
//    itsSpec.addColumn("COMMENT", "comment", "", 100, 0,
//                      "meta.note", "char", "col_comment", "");

}

void HiEmissionCatalogue::check()
{
    std::vector<CasdaHiEmissionObject>::iterator obj;
    for (obj = itsObjects.begin(); obj != itsObjects.end(); obj++) {
        obj->checkSpec(itsSpec);
    }

}

void HiEmissionCatalogue::write()
{
    this->check();
    this->writeVOT();
    this->writeASCII();
}

void HiEmissionCatalogue::writeVOT()
{
    AskapVOTableCatalogueWriter vowriter(itsVotableFilename);
    vowriter.setup(itsCube);
    ASKAPLOG_DEBUG_STR(logger, "Writing object table to the VOTable " <<
                       itsVotableFilename);
    vowriter.setColumnSpec(&itsSpec);
    vowriter.openCatalogue();
    vowriter.setResourceName("HI Emission-line object catalogue from Selavy source-finding");
    vowriter.setTableName("HI Emission-line object catalogue");
    vowriter.writeHeader();
    duchamp::VOParam version("table_version", "meta.version", "char", itsVersion, itsVersion.size() + 1, "");
    vowriter.writeParameter(version);
    vowriter.writeParameters();
    vowriter.writeFrequencyParam();
    vowriter.writeStats();
    vowriter.writeTableHeader();
    vowriter.writeEntries<CasdaHiEmissionObject>(itsObjects);
    vowriter.writeFooter();
    vowriter.closeCatalogue();
}

void HiEmissionCatalogue::writeASCII()
{

    AskapAsciiCatalogueWriter writer(itsAsciiFilename);
    ASKAPLOG_DEBUG_STR(logger, "Writing Fit results to " << itsAsciiFilename);
    writer.setup(itsCube);
    writer.setColumnSpec(&itsSpec);
    writer.openCatalogue();
    writer.writeTableHeader();
    writer.writeEntries<CasdaHiEmissionObject>(itsObjects);
    writer.writeFooter();
    writer.closeCatalogue();

}



}

}

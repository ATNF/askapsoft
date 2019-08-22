/// @file
///
/// Defining an Absorption object Catalogue
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
#include <catalogues/AbsorptionCatalogue.h>
#include <catalogues/CasdaCatalogue.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <catalogues/CasdaAbsorptionObject.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/Casda.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/CatalogueWriter.hh>
#include <vector>
#include <map>

ASKAP_LOGGER(logger, ".absorptioncatalogue");

namespace askap {

namespace analysis {

AbsorptionCatalogue::AbsorptionCatalogue(std::vector< std::pair<CasdaComponent,
        sourcefitting::RadioSource> > &srclist,
        const LOFAR::ParameterSet &parset,
        duchamp::Cube *cube):
    CasdaCatalogue(parset, cube),
    itsObjects()
{
    itsVersion = "casda.sl_absorption_object_v0.7";
    itsFilenameStub = "absorption";
    itsObjectType = "Absorption object";
    setup();
    this->defineObjects(srclist, parset);
    this->defineSpec();

}

void AbsorptionCatalogue::defineObjects(std::vector< std::pair<CasdaComponent, sourcefitting::RadioSource> > &srclist,
                                        const LOFAR::ParameterSet &parset)
{
    std::vector< std::pair<CasdaComponent, sourcefitting::RadioSource> >::iterator src;
    for (src = srclist.begin(); src != srclist.end(); src++) {
        CasdaComponent comp = src->first;
        sourcefitting::RadioSource obj = src->second;
        CasdaAbsorptionObject object(comp, obj, parset);
        itsObjects.push_back(object);
    }
}

void AbsorptionCatalogue::defineSpec()
{
    // -------------------------------------------
    // DO NOT CHANGE UNLESS COORDINATED WITH CASDA
    // -------------------------------------------

    itsSpec.addColumn("IMAGEID", "image_id", "--", 50, 0,
                      "meta.id", "char", "col_image_id", "");
    itsSpec.addColumn("DATEOBS", "date_time_ut", "--", 50, 0,
                      "time.start", "char", "col_date_time_ut", "");
    itsSpec.addColumn("COMP_ID", "cont_component_id", "--", 6, 0,
                      "meta.id.parent", "char", "col_cont_component_id", "");
    itsSpec.addColumn("CONTFLUX", "cont_flux", "[" + casda::intFluxUnitContinuum + "]",
                      9, casda::precFlux, "phot.flux.density;em.radio;spect.continuum",
                      "float", "col_cont_flux", "");
    itsSpec.addColumn("ID", "object_id", "--", 6, 0,
                      "meta.id;meta.main", "char", "col_object_id", "");
    itsSpec.addColumn("NAME", "object_name", "", 8, 0,
                      "meta.id", "char", "col_object_name", "");
    itsSpec.addColumn("RA", "ra_hms_cont", "", 11, 0,
                      "pos.eq.ra", "char", "col_ra_hms_cont", "J2000");
    itsSpec.addColumn("DEC", "dec_dms_cont", "", 11, 0,
                      "pos.eq.dec", "char", "col_dec_dms_cont", "J2000");
    itsSpec.addColumn("RAJD", "ra_deg_cont", "[deg]", 11, casda::precPos,
                      "pos.eq.ra;meta.main", "double", "col_ra_deg_cont", "J2000");
    itsSpec.addColumn("DECJD", "dec_deg_cont", "[deg]", 11, casda::precPos,
                      "pos.eq.dec;meta.main", "double", "col_dec_deg_cont", "J2000");
    itsSpec.addColumn("RAERR", "ra_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.ra", "float", "col_ra_err", "J2000");
    itsSpec.addColumn("DECERR", "dec_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.dec", "float", "col_dec_err", "J2000");
    itsSpec.addColumn("FREQ_UW", "freq_uw", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq;meta.main",
                      "float", "col_freq_uw", "");
    itsSpec.addColumn("FREQ_UW_ERR", "freq_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq;meta.main",
                      "float", "col_freq_uw_err", "");
    itsSpec.addColumn("FREQ_W", "freq_w", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "em.freq",
                      "float", "col_freq_w", "");
    itsSpec.addColumn("FREQ_W_ERR", "freq_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreqSpectral, "stat.error;em.freq",
                      "float", "col_freq_w_err", "");
    itsSpec.addColumn("Z_HI_UW", "z_hi_uw", "", 11, casda::precZ,
                      "src.redshift;em.line.HI;meta.main", "float", "col_z_hi_uw", "");
    itsSpec.addColumn("Z_HI_UW_ERR", "z_hi_uw_err", "", 11, casda::precZ,
                      "stat.error;src.redshift;em.line.HI;meta.main",
                      "float", "col_z_hi_uw_err", "");
    itsSpec.addColumn("Z_HI_W", "z_hi_w", "", 11, casda::precZ,
                      "src.redshift;em.line.HI", "float", "col_z_hi_w", "");
    itsSpec.addColumn("Z_HI_W_ERR", "z_hi_w_err", "", 11, casda::precZ,
                      "stat.error;src.redshift;em.line.HI",
                      "float", "col_z_hi_w_err", "");
    itsSpec.addColumn("Z_HI_PEAK", "z_hi_peak", "", 11, casda::precZ,
                      "src.redshift;em.line.HI;phys.absorption.opticalDepth;stat.max",
                      "float", "col_z_hi_peak", "");
    itsSpec.addColumn("Z_HI_PEAK_ERR", "z_hi_peak_err", "", 11, casda::precZ,
                      "stat.error;src.redshift;em.line.HI;phys.absorption.opticalDepth;stat.max",
                      "float", "col_z_hi_peak_err", "");
    itsSpec.addColumn("W50", "w50", "[" + casda::freqWidthUnit + "]",
                      11, casda::precSpecWidth,
                      "phys.absorption;spect.line.width;em.freq", "float", "col_w50", "");
    itsSpec.addColumn("W50_ERR", "w50_err", "[" + casda::freqWidthUnit + "]",
                      11, casda::precSpecWidth,
                      "stat.error;phys.absorption;spect.line.width;em.freq",
                      "float", "col_w50_err", "");
    itsSpec.addColumn("W20", "w20", "[" + casda::freqWidthUnit + "]",
                      11, casda::precSpecWidth,
                      "phys.absorption;askap:spect.line.width20;em.freq",
                      "float", "col_w20", "");
    itsSpec.addColumn("W20_ERR", "w20_err", "[" + casda::freqWidthUnit + "]",
                      11, casda::precSpecWidth,
                      "stat.error;phys.absorption;askap:spect.line.width20;em.freq",
                      "float", "col_w20_err", "");
    itsSpec.addColumn("RMS_IMAGECUBE", "rms_imagecube", "[" + casda::fluxUnit + "]", 10,
                      casda::precFlux, "stat.stdev;phot.flux.density",
                      "float", "col_rms_imagecube", "");
    itsSpec.addColumn("OPT_DEPTH_PEAK", "opt_depth_peak", "", 10, casda::precFlux,
                      "phys.absorption.opticalDepth;stat.max",
                      "float", "col_opt_depth_peak");
    itsSpec.addColumn("OPT_DEPTH_PEAK_ERR", "opt_depth_peak_err", "", 10, casda::precFlux,
                      "stat.error;phys.absorption.opticalDepth;stat.max",
                      "float", "col_opt_depth_peak_err");
    itsSpec.addColumn("OPT_DEPTH_INT", "opt_depth_int", "[" + casda::velocityUnit + "]", 10,
                      casda::precFlux, "phys.absorption.opticalDepth;askap:arith.integrated",
                      "float", "col_opt_depth_int");
    itsSpec.addColumn("OPT_DEPTH_INT_ERR", "opt_depth_int_err",
                      "[" + casda::velocityUnit + "]", 10, casda::precFlux,
                      "stat.error;phys.absorption.opticalDepth;askap:arith.integrated",
                      "float", "col_opt_depth_int_err");
    itsSpec.addColumn("FLAG1", "flag_a1", "", 5, 0,
                      "meta.code", "int", "col_flag_a1", "");
    itsSpec.addColumn("FLAG2", "flag_a2", "", 5, 0,
                      "meta.code", "int", "col_flag_a2", "");
    itsSpec.addColumn("FLAG3", "flag_a3", "", 5, 0,
                      "meta.code", "int", "col_flag_a3", "");
//    itsSpec.addColumn("COMMENT", "comment", "", 100, 0,
//                      "meta.note", "char", "col_comment", "");

}

void AbsorptionCatalogue::fixWidths()
{
    // -------------------------------------------
    // DO NOT CHANGE UNLESS COORDINATED WITH CASDA
    // -------------------------------------------

//    fixColWidth(itsSpec.column("IMAGEID"),         50);
    fixColWidth(itsSpec.column("DATEOBS"),           50);
    fixColWidth(itsSpec.column("COMP_ID"),           22);
    fixColWidth(itsSpec.column("CONTFLUX"),           9);
    fixColWidth(itsSpec.column("ID"),                24);
    fixColWidth(itsSpec.column("NAME"),              15);
    fixColWidth(itsSpec.column("RA"),                12);
    fixColWidth(itsSpec.column("DEC"),               13);
    fixColWidth(itsSpec.column("RAJD"),              11);
    fixColWidth(itsSpec.column("DECJD"),             11);
    fixColWidth(itsSpec.column("RAERR"),             11);
    fixColWidth(itsSpec.column("DECERR"),            11);
    fixColWidth(itsSpec.column("FREQ_UW"),           11);
    fixColWidth(itsSpec.column("FREQ_UW_ERR"),       11);
    fixColWidth(itsSpec.column("FREQ_W"),            11);
    fixColWidth(itsSpec.column("FREQ_W_ERR"),        11);
    fixColWidth(itsSpec.column("Z_HI_UW"),           11);
    fixColWidth(itsSpec.column("Z_HI_UW_ERR"),       11);
    fixColWidth(itsSpec.column("Z_HI_W"),            11);
    fixColWidth(itsSpec.column("Z_HI_W_ERR"),        11);
    fixColWidth(itsSpec.column("Z_HI_PEAK"),         11);
    fixColWidth(itsSpec.column("Z_HI_PEAK_ERR"),     11);
    fixColWidth(itsSpec.column("W50"),               11);
    fixColWidth(itsSpec.column("W50_ERR"),           11);
    fixColWidth(itsSpec.column("W20"),               11);
    fixColWidth(itsSpec.column("W20_ERR"),           11);
    fixColWidth(itsSpec.column("RMS_IMAGECUBE"),     10);
    fixColWidth(itsSpec.column("OPT_DEPTH_PEAK"),    10);
    fixColWidth(itsSpec.column("OPT_DEPTH_INT"),     10);
    fixColWidth(itsSpec.column("OPT_DEPTH_INT_ERR"), 10);
    fixColWidth(itsSpec.column("FLAG1"),              5);
    fixColWidth(itsSpec.column("FLAG2"),              5);
    fixColWidth(itsSpec.column("FLAG3"),              5);

}

void AbsorptionCatalogue::check(bool checkTitle)
{
    std::vector<CasdaAbsorptionObject>::iterator obj;
    for (obj = itsObjects.begin(); obj != itsObjects.end(); obj++) {
        obj->checkSpec(itsSpec, checkTitle);
    }

}

void AbsorptionCatalogue::writeAsciiEntries(AskapAsciiCatalogueWriter *writer)
{
    writer->writeEntries<CasdaAbsorptionObject>(itsObjects);
}

void AbsorptionCatalogue::writeVOTableEntries(AskapVOTableCatalogueWriter *writer)
{
    writer->writeEntries<CasdaAbsorptionObject>(itsObjects);
}



}

}

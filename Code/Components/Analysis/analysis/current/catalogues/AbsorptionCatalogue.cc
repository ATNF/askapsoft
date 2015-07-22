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
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <catalogues/CasdaAbsorptionObject.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/casda.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <vector>
#include <map>

ASKAP_LOGGER(logger, ".absorptioncatalogue");

namespace askap {

namespace analysis {

AbsorptionCatalogue::AbsorptionCatalogue(std::multimap<CasdaComponent, sourcefitting::RadioSource> &srclist,
        const LOFAR::ParameterSet &parset,
        duchamp::Cube &cube):
    itsObjects(),
    itsSpec(),
    itsCube(cube),
    itsVersion("casda.sl_absorption_object_v0.7")
{
    this->defineObjects(srclist, parset);
    this->defineSpec();

    duchamp::Param par = parseParset(parset);
    std::string filenameBase = par.getOutFile();
    filenameBase.replace(filenameBase.rfind(".txt"),
                         std::string::npos, ".absobjects");
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";

}

void AbsorptionCatalogue::defineObjects(std::multimap<CasdaComponent, sourcefitting::RadioSource> &srclist,
                                        const LOFAR::ParameterSet &parset)
{
    std::multimap<CasdaComponent, sourcefitting::RadioSource>::iterator src;
    for (src = srclist.begin(); src != srclist.end(); src++) {
        CasdaComponent comp = src->first;
        sourcefitting::RadioSource obj = src->second;
        CasdaAbsorptionObject object(comp, obj, parset);
        itsObjects.push_back(object);
    }
}

void AbsorptionCatalogue::defineSpec()
{
    itsSpec.addColumn("IMAGEID", "image_id", "--", 50, 0,
                      "meta.id", "char", "col_image_id", "");
    itsSpec.addColumn("DATEOBS", "date_time_ut", "--", 50, 0,
                      "time.start", "char", "col_date_time_ut", "");
    itsSpec.addColumn("COMP_ID", "cont_component_id", "--", 6, 0,
                      "meta.id.parent", "char", "col_cont_component_id", "");
    itsSpec.addColumn("CONTFLUX", "cont_flux", "[" + casda::intFluxUnit + "]",
                      9, casda::precFlux, "phot.flux.density;em.radio;spect.continuum",
                      "float", "col_fint", "");
    itsSpec.addColumn("ID", "object_id", "--", 6, 0,
                      "meta.id;meta.main", "char", "col_object_id", "");
    itsSpec.addColumn("NAME", "object_name", "", 8, 0,
                      "meta.id", "char", "col_object_name", "");
    itsSpec.addColumn("RA", "ra_hms_cont", "", 11, 0,
                      "pos.eq.ra", "char", "col_ra", "J2000");
    itsSpec.addColumn("DEC", "dec_dms_cont", "", 11, 0,
                      "pos.eq.dec", "char", "col_dec", "J2000");
    itsSpec.addColumn("RAJD", "ra_deg_cont", "[deg]", 11, casda::precPos,
                      "pos.eq.ra;meta.main", "float", "col_rajd", "J2000");
    itsSpec.addColumn("DECJD", "dec_deg_cont", "[deg]", 11, casda::precPos,
                      "pos.eq.dec;meta.main", "float", "col_decjd", "J2000");
    itsSpec.addColumn("RAERR", "ra_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.ra", "float", "col_raerr", "J2000");
    itsSpec.addColumn("DECERR", "dec_err", "[arcsec]", 11, casda::precSize,
                      "stat.error;pos.eq.dec", "float", "col_decerr", "J2000");
    itsSpec.addColumn("FREQ_UW", "freq_uw", "[" + casda::freqUnit + "]", 11, casda::precFreq,
                      "em.freq;meta.main", "float", "col_freq_uw", "");
    itsSpec.addColumn("FREQ_UW_ERR", "freq_uw_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreq, "stat.error;em.freq;meta.main",
                      "float", "col_freq_uw_err", "");
    itsSpec.addColumn("FREQ_W", "freq_w", "[" + casda::freqUnit + "]", 11, casda::precFreq,
                      "em.freq", "float", "col_freq_w", "");
    itsSpec.addColumn("FREQ_W_ERR", "freq_w_err", "[" + casda::freqUnit + "]",
                      11, casda::precFreq, "stat.error;em.freq",
                      "float", "col_freq_w_err", "");
    itsSpec.addColumn("Z_HI_UW", "z_hi_uw", "", 11, casda::precFreq,
                      "src.redshift;em.line.HI;meta.main", "float", "col_z_hi_uw", "");
    itsSpec.addColumn("Z_HI_UW_ERR", "z_hi_uw_err", "", 11, casda::precFreq,
                      "stat.error;src.redshift;em.line.HI;meta.main",
                      "float", "col_z_hi_uw_err", "");
    itsSpec.addColumn("Z_HI_W", "z_hi_w", "", 11, casda::precFreq,
                      "src.redshift;em.line.HI", "float", "col_z_hi_w", "");
    itsSpec.addColumn("Z_HI_W_ERR", "z_hi_uw_err", "", 11, casda::precFreq,
                      "stat.error;src.redshift;em.line.HI",
                      "float", "col_z_hi_w_err", "");
    itsSpec.addColumn("Z_HI_PEAK", "z_hi_peak", "", 11, casda::precFreq,
                      "src.redshift;em.line.HI;phys.absorption.opticalDepth;stat.max",
                      "float", "col_z_hi_peak", "");
    itsSpec.addColumn("Z_HI_PEAK_ERR", "z_hi_peak_err", "", 11, casda::precFreq,
                      "stat.error;src.redshift;em.line.HI;phys.absorption.opticalDepth;stat.max",
                      "float", "col_z_hi_peak_err", "");
    itsSpec.addColumn("W50", "w50", "[" + casda::freqWidthUnit + "]", 11, casda::precFreq,
                      "phys.absorption;spect.line.width;em.freq", "float", "col_w50", "");
    itsSpec.addColumn("W50_ERR", "w50_err", "[" + casda::freqWidthUnit + "]",
                      11, casda::precFreq,
                      "stat.error;phys.absorption;spect.line.width;em.freq",
                      "float", "col_w50_err", "");
    itsSpec.addColumn("W20", "w20", "[" + casda::freqWidthUnit + "]", 11, casda::precFreq,
                      "phys.absorption;askap:spect.line.width20;em.freq",
                      "float", "col_w20", "");
    itsSpec.addColumn("W20_ERR", "w20_err", "[" + casda::freqWidthUnit + "]",
                      11, casda::precFreq,
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
                      "float", "col_opt_depth_int_err");
    itsSpec.addColumn("OPT_DEPTH_INT", "opt_depth_int", "", 10, casda::precFlux,
                      "phys.absorption.opticalDepth;askap:arith.integrated",
                      "float", "col_opt_depth_peak");
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

void AbsorptionCatalogue::check(bool allColumns)
{
    std::vector<CasdaAbsorptionObject>::iterator obj;
    for (obj = itsObjects.begin(); obj != itsObjects.end(); obj++) {
        obj->checkSpec(itsSpec, allColumns);
    }

}

void AbsorptionCatalogue::write()
{
    this->check(false);
    this->writeVOT();
    this->check(true);
    this->writeASCII();
}

void AbsorptionCatalogue::writeVOT()
{
    AskapVOTableCatalogueWriter vowriter(itsVotableFilename);
    vowriter.setup(&itsCube);
    ASKAPLOG_DEBUG_STR(logger, "Writing object table to the VOTable " <<
                       itsVotableFilename);
    vowriter.setColumnSpec(&itsSpec);
    vowriter.openCatalogue();
    vowriter.setResourceName("Absorption object catalogue from Selavy source-finding");
    vowriter.setTableName("Absorption object catalogue");
    vowriter.writeHeader();
    duchamp::VOParam version("table_version", "meta.version", "char", itsVersion, 39, "");
    vowriter.writeParameter(version);
    vowriter.writeParameters();
    vowriter.writeFrequencyParam();
    vowriter.writeStats();
    vowriter.writeTableHeader();
    vowriter.writeEntries<CasdaAbsorptionObject>(itsObjects);
    vowriter.writeFooter();
    vowriter.closeCatalogue();
}

void AbsorptionCatalogue::writeASCII()
{

    AskapAsciiCatalogueWriter writer(itsAsciiFilename);
    ASKAPLOG_DEBUG_STR(logger, "Writing Fit results to " << itsAsciiFilename);
    writer.setup(&itsCube);
    writer.setColumnSpec(&itsSpec);
    writer.openCatalogue();
    writer.writeTableHeader();
    writer.writeEntries<CasdaAbsorptionObject>(itsObjects);
    writer.writeFooter();
    writer.closeCatalogue();

}



}

}

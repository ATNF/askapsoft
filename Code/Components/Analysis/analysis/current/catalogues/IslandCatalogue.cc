/// @file
///
/// Defining an Island Catalogue
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
#include <catalogues/IslandCatalogue.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <catalogues/Casda.h>
#include <catalogues/CasdaIsland.h>
#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <vector>

ASKAP_LOGGER(logger, ".islandcatalogue");

namespace askap {

namespace analysis {

IslandCatalogue::IslandCatalogue(std::vector<CasdaIsland> &islandList,
                                 const LOFAR::ParameterSet &parset,
                                 duchamp::Cube *cube):
    itsIslands(islandList),
    itsSpec(),
    itsCube(cube),
    itsVersion("casda.continuum_island_description_v0.7")
{
    this->setup(parset);
}

IslandCatalogue::IslandCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                                 const LOFAR::ParameterSet &parset,
                                 duchamp::Cube *cube):
    itsIslands(),
    itsSpec(),
    itsCube(cube),
    itsVersion("casda.continuum_island_description_v0.7")
{
    this->defineIslands(srclist, parset);
    this->setup(parset);
}

void IslandCatalogue::setup(const LOFAR::ParameterSet &parset)
{

    this->defineSpec();

    duchamp::Param par = parseParset(parset);
    std::string filenameBase = par.getOutFile();
    filenameBase.replace(filenameBase.rfind(".txt"),
                         std::string::npos, ".islands");
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";

}


void IslandCatalogue::defineIslands(std::vector<sourcefitting::RadioSource> &srclist,
                                    const LOFAR::ParameterSet &parset)
{
    std::vector<sourcefitting::RadioSource>::iterator src;
    for (src = srclist.begin(); src != srclist.end(); src++) {
        CasdaIsland island(*src, parset);
        itsIslands.push_back(island);
    }
}

void IslandCatalogue::defineSpec()
{
    // -------------------------------------------
    // DO NOT CHANGE UNLESS COORDINATED WITH CASDA
    // -------------------------------------------

    itsSpec.addColumn("ID", "island_id", "--", 6, 0,
                      "meta.id;meta.main", "char", "col_island_id", "");
    itsSpec.addColumn("NAME", "island_name", "", 8, 0,
                      "meta.id", "char", "col_island_name", "");
    itsSpec.addColumn("NCOMP", "n_components", "", 5, 0,
                      "meta.number", "int", "col_n_components", "");
    itsSpec.addColumn("RA", "ra_hms_cont", "", 10, 0,
                      "pos.eq.ra", "char", "col_ra_hms_cont", "J2000");
    itsSpec.addColumn("DEC", "dec_dms_cont", "", 9, 0,
                      "pos.eq.dec", "char", "col_dec_dms_cont", "J2000");
    itsSpec.addColumn("RAJD", "ra_deg_cont", "[deg]", casda::precPos + 2, casda::precPos,
                      "pos.eq.ra;meta.main", "double", "col_ra_deg_cont", "J2000");
    itsSpec.addColumn("DECJD", "dec_deg_cont", "[deg]", casda::precPos + 2, casda::precPos,
                      "pos.eq.dec;meta.main", "double", "col_dec_deg_cont", "J2000");
    itsSpec.addColumn("FREQ", "freq", "[MHz]", casda::precPos + 2, casda::precFreqContinuum,
                      "em.freq", "float", "col_freq", "");
    itsSpec.addColumn("MAJ", "maj_axis", "["+casda::shapeUnit+"]", casda::precSize+2, casda::precSize,
                      "phys.angSize.smajAxis;em.radio", "float", "col_maj_axis", "");
    itsSpec.addColumn("MIN", "min_axis", "["+casda::shapeUnit+"]", casda::precSize+2, casda::precSize,
                      "phys.angSize.sminAxis;em.radio", "float", "col_min_axis", "");
    itsSpec.addColumn("PA", "pos_ang", "[deg]", casda::precSize + 2, casda::precSize,
                      "phys.angSize;pos.posAng;em.radio", "float", "col_pos_ang", "");
    itsSpec.addColumn("FINT", "flux_int", "[" + casda::intFluxUnitContinuum + "]", casda::precFlux + 2, casda::precFlux,
                      "phot.flux.density.integrated;em.radio", "float", "col_flux_int", "");
    itsSpec.addColumn("FINTERR", "flux_int_err", "[" + casda::intFluxUnitContinuum + "]", casda::precFlux + 2, casda::precFlux,
                      "stat.error;phot.flux.density.integrated;em.radio", "float", "col_flux_int_err", "");
    itsSpec.addColumn("FPEAK", "flux_peak", "[" + casda::fluxUnit + "]", casda::precFlux + 2,
                      casda::precFlux, "phot.flux.density;stat.max;em.radio", "float",
                      "col_flux_peak", "");
    itsSpec.addColumn("BACKGND", "mean_background", "[" + casda::fluxUnit + "]",
                      casda::precFlux + 2, casda::precFlux,
                      "askap:phot.flux.density.voxel;instr.skyLevel;stat.mean;em.radio",
                      "float", "col_mean_background", "");
    itsSpec.addColumn("NOISE", "background_noise", "[" + casda::fluxUnit + "]",
                      casda::precFlux + 2, casda::precFlux,
                      "askap:phot.flux.density.voxel;instr.skyLevel;askap:stat.rms;em.radio",
                      "float", "col_background_noise", "");
    itsSpec.addColumn("MAXRESID", "max_residual", "[" + casda::fluxUnit + "]", casda::precFlux + 2,
                      casda::precFlux, "askap:phot.flux.density.voxel;stat.max;src.net;em.radio",
                      "float", "col_mean_background", "");
    itsSpec.addColumn("MINRESID", "min_residual", "[" + casda::fluxUnit + "]", casda::precFlux + 2,
                      casda::precFlux, "askap:phot.flux.density.voxel;stat.min;src.net;em.radio",
                      "float", "col_mean_background", "");
    itsSpec.addColumn("MEANRESID", "mean_residual", "[" + casda::fluxUnit + "]", casda::precFlux + 2,
                      casda::precFlux, "askap:phot.flux.density.voxel;stat.mean;src.net;em.radio",
                      "float", "col_mean_background", "");
    itsSpec.addColumn("RMSRESID", "max_residual", "[" + casda::fluxUnit + "]", casda::precFlux + 2,
                      casda::precFlux, "askap:phot.flux.density.voxel;askap:stat.rms;src.net;em.radio",
                      "float", "col_mean_background", "");
    itsSpec.addColumn("STDDEVRESID", "max_residual", "[" + casda::fluxUnit + "]", casda::precFlux + 2,
                      casda::precFlux, "askap:phot.flux.density.voxel;stat.stdev;src.net;em.radio",
                      "float", "col_mean_background", "");
    itsSpec.addColumn("XMIN", "x_min", "", 5, 0,
                      "pos.cartesian.x;stat.min", "int", "col_x_min", "");
    itsSpec.addColumn("XMAX", "x_max", "", 5, 0,
                      "pos.cartesian.x;stat.max", "int", "col_x_max", "");
    itsSpec.addColumn("YMIN", "y_min", "", 5, 0,
                      "pos.cartesian.y;stat.min", "int", "col_y_min", "");
    itsSpec.addColumn("YMAX", "y_max", "", 5, 0,
                      "pos.cartesian.y;stat.max", "int", "col_y_max", "");
    itsSpec.addColumn("NPIX", "n_pix", "", 9, 0,
                      "phys.angArea;instr.pixel;meta.number", "int", "col_n_pix", "");
    itsSpec.addColumn("SOLIDANGLE", "solid_angle", "[" + casda::solidangleUnit + "]", 9, casda::precSolidangle,
                      "phys.angArea", "int", "col_solid_angle", "");
    itsSpec.addColumn("BEAMAREA", "beam_area", "[" + casda::solidangleUnit + "]", 9, casda::precSolidangle,
                      "phys.angArea;instr.beam", "int", "col_beam_area", "");
    itsSpec.addColumn("XAV", "x_ave", "", casda::precPix + 2, casda::precPix,
                      "pos.cartesian.x;stat.mean", "float", "col_x_ave", "");
    itsSpec.addColumn("YAV", "y_ave", "", casda::precPix + 2, casda::precPix,
                      "pos.cartesian.y;stat.mean", "float", "col_y_ave", "");
    itsSpec.addColumn("XCENT", "x_cen", "", casda::precPix + 2, casda::precPix,
                      "pos.cartesian.x;askap:stat.centroid", "float", "col_x_cen", "");
    itsSpec.addColumn("YCENT", "y_cen", "", casda::precPix + 2, casda::precPix,
                      "pos.cartesian.y;askap:stat.centroid", "float", "col_y_cen", "");
    itsSpec.addColumn("XPEAK", "x_peak", "", casda::precPix + 2, casda::precPix,
                      "pos.cartesian.x;phot.flux;stat.max", "int", "col_x_peak", "");
    itsSpec.addColumn("YPEAK", "y_peak", "", casda::precPix + 2, casda::precPix,
                      "pos.cartesian.y;phot.flux;stat.max", "int", "col_y_peak", "");
    itsSpec.addColumn("FLAG1", "flag_i1", "", 5, 0,
                      "meta.code", "int", "col_flag_i1", "");
    itsSpec.addColumn("FLAG2", "flag_i2", "", 5, 0,
                      "meta.code", "int", "col_flag_i2", "");
    itsSpec.addColumn("FLAG3", "flag_i3", "", 5, 0,
                      "meta.code", "int", "col_flag_i3", "");
    itsSpec.addColumn("FLAG4", "flag_i4", "", 5, 0,
                      "meta.code", "int", "col_flag_i4", "");
    itsSpec.addColumn("COMMENT", "comment", "", 100, 0,
                      "meta.note", "char", "col_comment", "");

}

void IslandCatalogue::check(bool checkTitle)
{
    std::vector<CasdaIsland>::iterator isle;
    for (isle = itsIslands.begin(); isle != itsIslands.end(); isle++) {
        isle->checkSpec(itsSpec, checkTitle);
    }

}

void IslandCatalogue::write()
{
    this->check(false);
    this->writeVOT();
    this->check(true);
    this->writeASCII();
}

void IslandCatalogue::writeVOT()
{
    AskapVOTableCatalogueWriter vowriter(itsVotableFilename);
    vowriter.setup(itsCube);
    ASKAPLOG_DEBUG_STR(logger, "Writing island table to the VOTable " <<
                       itsVotableFilename);
    vowriter.setColumnSpec(&itsSpec);
    vowriter.openCatalogue();
    vowriter.setResourceName("Island catalogue from Selavy source finding");
    vowriter.setTableName("Island catalogue");
    vowriter.writeHeader();
    duchamp::VOParam version("table_version", "meta.version", "char", itsVersion, itsVersion.size() + 1, "");
    vowriter.writeParameter(version);
    vowriter.writeParameters();
    vowriter.writeStats();
    vowriter.writeTableHeader();
    vowriter.writeEntries<CasdaIsland>(itsIslands);
    vowriter.writeFooter();
    vowriter.closeCatalogue();
}

void IslandCatalogue::writeASCII()
{

    AskapAsciiCatalogueWriter writer(itsAsciiFilename);
    ASKAPLOG_DEBUG_STR(logger, "Writing islands results to " << itsAsciiFilename);
    writer.setup(itsCube);
    writer.setColumnSpec(&itsSpec);
    writer.openCatalogue();
    writer.writeTableHeader();
    writer.writeEntries<CasdaIsland>(itsIslands);
    writer.writeFooter();
    writer.closeCatalogue();

}



}

}

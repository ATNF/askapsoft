/// @file
///
/// Defining an Component Catalogue
///
/// @copyright (c) 2016 CSIRO
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
#include <catalogues/RMCatalogue.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <catalogues/CasdaPolarisationEntry.h>
#include <catalogues/ComponentCatalogue.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/casda.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <vector>

ASKAP_LOGGER(logger, ".rmcatalogue");

namespace askap {

namespace analysis {

RMCatalogue::RMCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                         const LOFAR::ParameterSet &parset,
                         duchamp::Cube &cube):
    itsComponents(),
    itsSpec(),
    itsCube(cube),
    itsVersion("casda.polarisation_v0.7")
{
    this->defineComponents(srclist, parset);
    this->defineSpec();

    duchamp::Param par = parseParset(parset);
    std::string filenameBase = par.getOutFile();
    filenameBase.replace(filenameBase.rfind(".txt"),
                         std::string::npos, ".polarisation");
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";

}

void RMCatalogue::defineComponents(std::vector<sourcefitting::RadioSource> &srclist,
                                   const LOFAR::ParameterSet &parset)
{
    ComponentCatalogue compCat(srclist,parset,itsCube,"best");
    std::vector<CasdaComponent> comps=compCat.components();
    for(unsigned int i=0;i<comps.size();i++){
        CasdaPolarisationEntry pol(&comps[i],parset);
        itsComponents.push_back(pol);
    }

}

void RMCatalogue::defineSpec()
{
    itsSpec.addColumn("ID", "component_id", "", 6, 0,
                      "meta.id;meta.main", "char", "col_component_id", "");
    itsSpec.addColumn("NAME", "component_name", "", 8, 0,
                      "meta.id", "char", "col_component_name", "");
    itsSpec.addColumn("RAJD", "ra_deg_cont", "[deg]", 11, casda::precPos,
                      "pos.eq.ra;meta.main", "double", "col_ra_deg_cont", "J2000");
    itsSpec.addColumn("DECJD", "dec_deg_cont", "[deg]", 11, casda::precPos,
                      "pos.eq.dec;meta.main", "double", "col_dec_deg_cont", "J2000");
    itsSpec.addColumn("IFLUX", "flux_I_median", "[" + casda::fluxUnit + "]",
                      9, casda::precFlux,
                      "phot.flux.density;em.radio", "double", "col_flux_I_median", "");
    itsSpec.addColumn("QFLUX", "flux_Q_median", "[" + casda::fluxUnit + "]",
                      9, casda::precFlux,
                      "phot.flux.density;em.radio;askap:phys.polarization.stokes.Q",
                      "double", "col_flux_Q_median", "");
    itsSpec.addColumn("UFLUX", "flux_U_median", "[" + casda::fluxUnit + "]",
                      9, casda::precFlux,
                      "phot.flux.density;em.radio;askap:phys.polarization.stokes.U",
                      "double", "col_flux_U_median", "");
    itsSpec.addColumn("VFLUX", "flux_V_median", "[" + casda::fluxUnit + "]",
                      9, casda::precFlux,
                      "phot.flux.density;em.radio;askap:phys.polarization.stokes.V",
                      "double", "col_flux_V_median", "");
    itsSpec.addColumn("RMS_I", "rms_I", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "stat.stdev;phot.flux.density", "double", "col_rms_I", "");
    itsSpec.addColumn("RMS_Q", "rms_Q", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "stat.stdev;phot.flux.density;askap:phys.polarization.stokes.Q",
                      "double", "col_rms_Q", "");
    itsSpec.addColumn("RMS_U", "rms_U", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "stat.stdev;phot.flux.density;askap:phys.polarization.stokes.U",
                      "double", "col_rms_U", "");
    itsSpec.addColumn("RMS_V", "rms_V", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "stat.stdev;phot.flux.density;askap:phys.polarization.stokes.V",
                      "double", "col_rms_V", "");
    itsSpec.addColumn("CO1", "co_1", "", 8, casda::precFlux,
                      "stat.fit.param;spect.continuum", "double", "col_co_1", "");
    itsSpec.addColumn("CO2", "co_2", "", 8, casda::precFlux,
                      "stat.fit.param;spect.continuum", "double", "col_co_2", "");
    itsSpec.addColumn("CO3", "co_3", "", 8, casda::precFlux,
                      "stat.fit.param;spect.continuum", "double", "col_co_3", "");
    itsSpec.addColumn("CO4", "co_4", "", 8, casda::precFlux,
                      "stat.fit.param;spect.continuum", "double", "col_co_4", "");
    itsSpec.addColumn("CO5", "co_5", "", 8, casda::precFlux,
                      "stat.fit.param;spect.continuum", "double", "col_co_5", "");
    itsSpec.addColumn("LAMSQ", "lambda_ref_sq", "[" + casda::lamsqUnit + "]", 9, casda::precLamsq,
                      "askap:em.wl.squared", "double", "col_lambda_ref_sq", "");
    itsSpec.addColumn("RMSF", "rmsf_fwhm", "[" + casda::faradayDepthUnit + "]", 8, casda::precFD,
                      "phys.polarization.rotMeasure;askap:phys.polarization.rmsfWidth",
                      "double", "col_rmsf_fwhm", "");
    itsSpec.addColumn("POLPEAK", "pol_peak", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "phot.flux.density;phys.polarization.rotMeasure;stat.max",
                      "double", "col_pol_peak", "");
    itsSpec.addColumn("POLPEAKDB", "pol_peak_debias",
                      "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      std::string("phot.flux.density;phys.polarization.rotMeasure") +
                      std::string(";stat.max;askap:meta.corrected"),
                      "double", "col_pol_peak_debias", "");
    itsSpec.addColumn("POLPEAKERR", "pol_peak_err", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "stat.error;phot.flux.density;phys.polarization.rotMeasure;stat.max",
                      "double", "col_pol_peak_err", "");
    itsSpec.addColumn("POLPEAKFIT", "pol_peak_fit", "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      "phot.flux.density;phys.polarization.rotMeasure;stat.max;stat.fit",
                      "double", "col_pol_peak_fit", "");
    itsSpec.addColumn("POLPEAKFITDB", "pol_peak_fit_debias",
                      "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      std::string("phot.flux.density;phys.polarization.rotMeasure") +
                      std::string(";stat.max;stat.fit;askap:meta.corrected"),
                      "double", "col_pol_peak_fit_debias", "");
    itsSpec.addColumn("POLPEAKFITERR", "pol_peak_fit_err",
                      "[" + casda::fluxUnit + "]", 9, casda::precFlux,
                      std::string("stat.error;phot.flux.density;phys.polarization.rotMeasure") +
                      std::string(";stat.fit;stat.max"),
                      "double", "col_pol_peak_fit_err", "");
    itsSpec.addColumn("POLPEAKFITSNR", "pol_peak_fit_snr",
                      "", 9, casda::precFlux,
                      std::string("stat.snr;phot.flux.density;") +
                      std::string("phys.polarization.rotMeasure;stat.max;stat.fit"),
                      "double", "col_pol_peak_fit_snr", "");
    itsSpec.addColumn("POLPEAKFITSNRERR", "pol_peak_fit_snr_err",
                      "", 9, casda::precFlux,
                      std::string("stat.error;stat.snr;phot.flux.density;") +
                      std::string("phys.polarization.rotMeasure;stat.fit;stat.max"),
                      "double", "col_pol_peak_fit_snr_err", "");
    itsSpec.addColumn("FDPEAK", "fd_peak", "[" + casda::faradayDepthUnit + "]", 9, casda::precFD,
                      "phys.polarization.rotMeasure", "double", "col_fd_peak", "");
    itsSpec.addColumn("FDPEAKERR", "fd_peak_err", "[" + casda::faradayDepthUnit + "]", 9, casda::precFD,
                      "stat.error;phys.polarization.rotMeasure", "double", "col_fd_peak_err", "");
    itsSpec.addColumn("FDPEAKFIT", "fd_peak_fit", "[" + casda::faradayDepthUnit + "]", 9, casda::precFD,
                      "phys.polarization.rotMeasure;stat.fit", "double", "col_fd_peak_fit", "");
    itsSpec.addColumn("FDPEAKFITERR", "fd_peak_fit_err", "[" + casda::faradayDepthUnit + "]",
                      9, casda::precFD, "stat.error;phys.polarization.rotMeasure;stat.fit",
                      "double", "col_fd_peak_fit_err", "");
    itsSpec.addColumn("POLANG", "pol_ang_ref", "[" + casda::angleUnit + "]", 7, casda::precAngle,
                      "askap:phys.polarization.angle", "double", "col_pol_angle_ref", "");
    itsSpec.addColumn("POLANGERR", "pol_ang_ref_err", "[" + casda::angleUnit + "]", 7, casda::precAngle,
                      "stat.error;askap:phys.polarization.angle",
                      "double", "col_pol_angle_ref_err", "");
    itsSpec.addColumn("POLANG0", "pol_ang_zero", "[" + casda::angleUnit + "]", 7, casda::precAngle,
                      "askap:phys.polarization.angle;meta.corrected",
                      "double", "col_pol_ang_zero", "");
    itsSpec.addColumn("POLANG0ERR", "pol_ang_zero_err", "[" + casda::angleUnit + "]",
                      7, casda::precAngle,
                      "stat.error;askap:phys.polarization.angle;meta.corrected",
                      "double", "col_pol_ang_zero_err", "");
    itsSpec.addColumn("POLFRAC", "pol_frac", "", 6, casda::precPfrac,
                      "phys.polarization", "double", "col_pol_frac", "");
    itsSpec.addColumn("POLFRACERR", "pol_frac_err", "", 6, casda::precPfrac,
                      "stat.error;phys.polarization", "double", "col_pol_frac_err", "");
    itsSpec.addColumn("COMPLEX1", "complex_1", "", 5, casda::precStats,
                      "stat.value;phys.polarization", "double", "col_complex_1", "");
    itsSpec.addColumn("COMPLEX2", "complex_2", "", 5, casda::precStats,
                      "stat.value;phys.polarization", "double", "col_complex_2", "");
    itsSpec.addColumn("FLAG1", "flag_is_detection", "", 1, 0, "meta.code", "boolean", "col_flag_is_detection", "");
    itsSpec.addColumn("FLAG2", "flag_edge", "", 1, 0, "meta.code", "boolean", "col_flag_edge", "");
    itsSpec.addColumn("FLAG3", "flag_p3", "", 1, 0, "meta.code", "char", "col_flag_p3", "");
    itsSpec.addColumn("FLAG4", "flag_p4", "", 1, 0, "meta.code", "char", "col_flag_p4", "");

}

void RMCatalogue::check()
{
    std::vector<CasdaPolarisationEntry>::iterator comp;
    for (comp = itsComponents.begin(); comp != itsComponents.end(); comp++) {
        comp->checkSpec(itsSpec);
    }

}

void RMCatalogue::write()
{
    this->check();
    this->writeVOT();
    this->check();
    this->writeASCII();
}

void RMCatalogue::writeVOT()
{
    AskapVOTableCatalogueWriter vowriter(itsVotableFilename);
    vowriter.setup(&itsCube);
    ASKAPLOG_DEBUG_STR(logger, "Writing component table to the VOTable " <<
                       itsVotableFilename);
    vowriter.setColumnSpec(&itsSpec);
    vowriter.openCatalogue();
    vowriter.writeHeader();
    duchamp::VOParam version("table_version", "meta.version", "char", itsVersion, itsVersion.size()+1, "");
    vowriter.writeParameter(version);
    vowriter.writeParameters();
    vowriter.writeFrequencyParam();
    vowriter.writeStats();
    vowriter.writeTableHeader();
    vowriter.writeEntries<CasdaPolarisationEntry>(itsComponents);
    vowriter.writeFooter();
    vowriter.closeCatalogue();
}

void RMCatalogue::writeASCII()
{

    AskapAsciiCatalogueWriter writer(itsAsciiFilename);
    ASKAPLOG_DEBUG_STR(logger, "Writing Fit results to " << itsAsciiFilename);
    writer.setup(&itsCube);
    writer.setColumnSpec(&itsSpec);
    writer.openCatalogue();
    writer.writeTableHeader();
    writer.writeEntries<CasdaPolarisationEntry>(itsComponents);
    writer.writeFooter();
    writer.closeCatalogue();

}



}

}

/// @file
///
/// Defining an Component Catalogue
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
#include <catalogues/ComponentCatalogue.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <catalogues/CasdaComponent.h>
#include <catalogues/Casda.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/KarmaAnnotationWriter.hh>
#include <duchamp/Outputs/CasaAnnotationWriter.hh>
#include <duchamp/Outputs/DS9AnnotationWriter.hh>
#include <vector>

ASKAP_LOGGER(logger, ".componentcatalogue");

namespace askap {

namespace analysis {

ComponentCatalogue::ComponentCatalogue(std::vector<CasdaComponent> &componentList,
                                       const LOFAR::ParameterSet &parset,
                                       duchamp::Cube *cube,
                                       const std::string fitType):
    itsFitType(casda::componentFitType),
    itsComponents(componentList),
    itsSpec(),
    itsCube(cube),
    itsKarmaFilename(""),
    itsCASAFilename(""),
    itsDS9Filename(""),
    itsVersion("casda.continuum_component_description_v1.7")
{
    ASKAPLOG_DEBUG_STR(logger, "Defining component catalogue, version " << itsVersion);

    this->setup(parset);
}

ComponentCatalogue::ComponentCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                                       const LOFAR::ParameterSet &parset,
                                       duchamp::Cube *cube,
                                       const std::string fitType):
    itsFitType(casda::componentFitType),
    itsComponents(),
    itsSpec(),
    itsCube(cube),
    itsKarmaFilename(""),
    itsCASAFilename(""),
    itsDS9Filename(""),
    itsVersion("casda.continuum_component_description_v1.7")
{
    ASKAPLOG_DEBUG_STR(logger, "Defining component catalogue, version " << itsVersion);

    this->defineComponents(srclist, parset);
    this->setup(parset);
}

void ComponentCatalogue::setup(const LOFAR::ParameterSet &parset)
{

    this->defineSpec();

    duchamp::Param par = parseParset(parset);
    std::string filenameBase = par.getOutFile();
    filenameBase.replace(filenameBase.rfind(".txt"),
                         std::string::npos, ".components");
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";
    if (parset.getBool("flagKarma", false)) {
        itsKarmaFilename = filenameBase + ".ann";
    }
    if (parset.getBool("flagCasa", false)) {
        itsCASAFilename = filenameBase + ".crf";
    }
    if (parset.getBool("flagDS9", false)) {
        itsDS9Filename = filenameBase + ".reg";
    }

}

void ComponentCatalogue::defineComponents(std::vector<sourcefitting::RadioSource> &srclist,
        const LOFAR::ParameterSet &parset)
{
    std::vector<sourcefitting::RadioSource>::iterator src;
    for (src = srclist.begin(); src != srclist.end(); src++) {
        for (size_t i = 0; i < src->numFits(); i++) {
            CasdaComponent component(*src, parset, i, itsFitType);
            itsComponents.push_back(component);
        }
    }
}

void ComponentCatalogue::defineSpec()
{
    itsSpec.addColumn("ISLAND", "island_id", "--", 6, 0,
                      "meta.id.parent", "char", "col_island_id", "");
    itsSpec.addColumn("ID", "component_id", "--", 6, 0,
                      "meta.id;meta.main", "char", "col_component_id", "");
    itsSpec.addColumn("NAME", "component_name", "", 8, 0,
                      "meta.id", "char", "col_component_name", "");
    itsSpec.addColumn("RA", "ra_hms_cont", "", 10, 0,
                      "pos.eq.ra", "char", "col_ra_hms_cont", "J2000");
    itsSpec.addColumn("DEC", "dec_dms_cont", "", 9, 0,
                      "pos.eq.dec", "char", "col_dec_dms_cont", "J2000");
    itsSpec.addColumn("RAJD", "ra_deg_cont", "[deg]", casda::precPos+2, casda::precPos,
                      "pos.eq.ra;meta.main", "double", "col_ra_deg_cont", "J2000");
    itsSpec.addColumn("DECJD", "dec_deg_cont", "[deg]", casda::precPos+2, casda::precPos,
                      "pos.eq.dec;meta.main", "double", "col_dec_deg_cont", "J2000");
    itsSpec.addColumn("RAERR", "ra_err", "[arcsec]", casda::precSize+2, casda::precSize,
                      "stat.error;pos.eq.ra", "float", "col_ra_err", "J2000");
    itsSpec.addColumn("DECERR", "dec_err", "[arcsec]", casda::precSize+2, casda::precSize,
                      "stat.error;pos.eq.dec", "float", "col_dec_err", "J2000");
    itsSpec.addColumn("FREQ", "freq", "[" + casda::freqUnit + "]",
                      casda::precFreqContinuum+2, casda::precFreqContinuum,
                      "em.freq", "float", "col_freq", "");
    itsSpec.addColumn("FPEAK", "flux_peak", "[" + casda::fluxUnit + "]",
                      casda::precFlux+2, casda::precFlux,
                      "phot.flux.density;stat.max;em.radio;stat.fit",
                      "float", "col_flux_peak", "");
    itsSpec.addColumn("FPEAKERR", "flux_peak_err", "[" + casda::fluxUnit + "]",
                      casda::precFlux+2, casda::precFlux,
                      "stat.error;phot.flux.density;stat.max;em.radio;stat.fit",
                      "float", "col_flux_peak_err", "");
    itsSpec.addColumn("FINT", "flux_int", "[" + casda::intFluxUnitContinuum + "]",
                      casda::precFlux+2, casda::precFlux,
                      "phot.flux.density;em.radio;stat.fit",
                      "float", "col_flux_int", "");
    itsSpec.addColumn("FINTERR", "flux_int_err", "[" + casda::intFluxUnitContinuum + "]",
                      casda::precFlux+2, casda::precFlux,
                      "stat.error;phot.flux.density;em.radio;stat.fit",
                      "float", "col_flux_int_err", "");
    itsSpec.addColumn("MAJ", "maj_axis", "[arcsec]", casda::precSize+2, casda::precSize,
                      "phys.angSize.smajAxis;em.radio;stat.fit",
                      "float", "col_maj_axis", "");
    itsSpec.addColumn("MIN", "min_axis", "[arcsec]", casda::precSize+2, casda::precSize,
                      "phys.angSize.sminAxis;em.radio;stat.fit",
                      "float", "col_min_axis", "");
    itsSpec.addColumn("PA", "pos_ang", "[deg]", casda::precSize+2, casda::precSize,
                      "phys.angSize;pos.posAng;em.radio;stat.fit",
                      "float", "col_pos_ang", "");
    itsSpec.addColumn("MAJERR", "maj_axis_err", "[arcsec]", casda::precSize+2, casda::precSize,
                      "stat.error;phys.angSize.smajAxis;em.radio",
                      "float", "col_maj_axis_err", "");
    itsSpec.addColumn("MINERR", "min_axis_err", "[arcsec]", casda::precSize+2, casda::precSize,
                      "stat.error;phys.angSize.sminAxis;em.radio",
                      "float", "col_min_axis_err", "");
    itsSpec.addColumn("PAERR", "pos_ang_err", "[deg]", casda::precSize+2, casda::precSize,
                      "stat.error;phys.angSize;pos.posAng;em.radio",
                      "float", "col_pos_ang_err", "");
    itsSpec.addColumn("MAJDECONV", "maj_axis_deconv", "[arcsec]", casda::precSize+2, casda::precSize,
                      "phys.angSize.smajAxis;em.radio;askap:meta.deconvolved",
                      "float", "col_maj_axis_deconv", "");
    itsSpec.addColumn("MINDECONV", "min_axis_deconv", "[arcsec]", casda::precSize+2, casda::precSize,
                      "phys.angSize.sminAxis;em.radio;askap:meta.deconvolved",
                      "float", "col_min_axis_deconv", "");
    itsSpec.addColumn("PADECONV", "pos_ang_deconv", "[deg]", casda::precSize+2, casda::precSize,
                      "phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved",
                      "float", "col_pos_ang_deconv", "");
    itsSpec.addColumn("CHISQ", "chi_squared_fit", "--", casda::precFlux+2, casda::precFlux,
                      "stat.fit.chi2", "float", "col_chi_squared_fit", "");
    itsSpec.addColumn("RMSFIT", "rms_fit_gauss", "[" + casda::fluxUnit + "]", casda::precFlux+2, casda::precFlux,
                      "stat.stdev;stat.fit", "double", "col_rms_fit_gauss", "");
    itsSpec.addColumn("ALPHA", "spectral_index", "--", casda::precSpecShape+2, casda::precSpecShape,
                      "spect.index;em.radio", "double", "col_spectral_index", "");
    itsSpec.addColumn("BETA", "spectral_curvature", "--", casda::precSpecShape+2, casda::precSpecShape,
                      "askap:spect.curvature;em.radio", "float", "col_spectral_curvature", "");
    itsSpec.addColumn("RMSIMAGE", "rms_image", "[" + casda::fluxUnit + "]", casda::precFlux+2, casda::precFlux,
                      "stat.stdev;phot.flux.density", "float", "col_rms_image", "");
    itsSpec.addColumn("FLAG1", "has_siblings", "", 5, 0,
                      "meta.code", "int", "col_has_siblings", "");
    itsSpec.addColumn("FLAG2", "fit_is_estimate", "", 5, 0,
                      "meta.code", "int", "col_fit_is_estimate", "");
    itsSpec.addColumn("FLAG3", "flag_c3", "", 5, 0,
                      "meta.code", "int", "col_flag_c3", "");
    itsSpec.addColumn("FLAG4", "flag_c4", "", 5, 0,
                      "meta.code", "int", "col_flag_c4", "");
    itsSpec.addColumn("COMMENT", "comment", "", 100, 0,
                      "meta.note", "char", "col_comment", "");

}

void ComponentCatalogue::check(bool checkTitle)
{
    std::vector<CasdaComponent>::iterator comp;
    for (comp = itsComponents.begin(); comp != itsComponents.end(); comp++) {
        comp->checkSpec(itsSpec, checkTitle);
    }

}

std::vector<CasdaComponent> &ComponentCatalogue::components()
{
    std::vector<CasdaComponent> &comp = itsComponents;
    return comp;
}


void ComponentCatalogue::write()
{
    this->check(false);
    this->writeVOT();
    this->check(true);
    this->writeASCII();
    this->writeAnnotations();
}

void ComponentCatalogue::writeVOT()
{
    AskapVOTableCatalogueWriter vowriter(itsVotableFilename);
    vowriter.setup(itsCube);
    ASKAPLOG_DEBUG_STR(logger, "Writing component table to the VOTable " <<
                       itsVotableFilename);
    vowriter.setColumnSpec(&itsSpec);
    vowriter.openCatalogue();
    writeVOTinformation(vowriter);
    vowriter.writeHeader();
    duchamp::VOParam version("table_version", "meta.version", "char", itsVersion, itsVersion.size() + 1, "");
    vowriter.writeParameter(version);
    vowriter.writeParameters();
    vowriter.writeFrequencyParam();
    vowriter.writeStats();
    vowriter.writeTableHeader();
    vowriter.writeEntries<CasdaComponent>(itsComponents);
    vowriter.writeFooter();
    vowriter.closeCatalogue();
}

void ComponentCatalogue::writeVOTinformation(AskapVOTableCatalogueWriter &vowriter)
{
    vowriter.setResourceName("Component catalogue from Selavy source-finding");
    vowriter.setTableName("Component catalogue");
}

void ComponentCatalogue::writeASCII()
{

    AskapAsciiCatalogueWriter writer(itsAsciiFilename);
    ASKAPLOG_DEBUG_STR(logger, "Writing Fitted components to " << itsAsciiFilename);
    writer.setup(itsCube);
    writer.setColumnSpec(&itsSpec);
    writer.openCatalogue();
    writer.writeTableHeader();
    writer.writeEntries<CasdaComponent>(itsComponents);
    writer.writeFooter();
    writer.closeCatalogue();

}

void ComponentCatalogue::writeAnnotations()
{

    // still to draw boxes

    for (int loop = 0; loop < 3; loop++) {
        boost::shared_ptr<duchamp::AnnotationWriter> writer;

        if (loop == 0) { // Karma
            if (itsKarmaFilename != "") {
                writer = boost::shared_ptr<duchamp::KarmaAnnotationWriter>(
                             new duchamp::KarmaAnnotationWriter(itsKarmaFilename));
                ASKAPLOG_INFO_STR(logger, "Writing fit results to karma annotation file: " <<
                                  itsKarmaFilename);
            }
        } else if (loop == 1) { // DS9
            if (itsDS9Filename != "") {
                writer = boost::shared_ptr<duchamp::DS9AnnotationWriter>(
                             new duchamp::DS9AnnotationWriter(itsDS9Filename));
                ASKAPLOG_INFO_STR(logger, "Writing fit results to DS9 region file: " <<
                                  itsDS9Filename);
            }
        } else { // CASA
            if (itsCASAFilename != "") {
                writer = boost::shared_ptr<duchamp::CasaAnnotationWriter>(
                             new duchamp::CasaAnnotationWriter(itsCASAFilename));
                ASKAPLOG_INFO_STR(logger, "Writing fit results to CASA region file: " <<
                                  itsCASAFilename);
            }
        }

        if (writer.get() != 0) {
            writer->setup(itsCube);
            writer->openCatalogue();
            writer->setColourString("BLUE");
            writer->writeHeader();
            writer->writeParameters();
            writer->writeStats();
            writer->writeTableHeader();

            std::vector<CasdaComponent>::iterator comp;
            for (comp = itsComponents.begin(); comp != itsComponents.end(); comp++) {
                comp->writeAnnotation(writer);
            }

            writer->writeFooter();
            writer->closeCatalogue();

            writer.reset();
        }

    }

}


}

}

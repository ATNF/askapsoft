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
#include <catalogues/FitCatalogue.h>
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
#include <vector>

ASKAP_LOGGER(logger, ".fitcatalogue");

namespace askap {

namespace analysis {

FitCatalogue::FitCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                           const LOFAR::ParameterSet &parset,
                           duchamp::Cube *cube,
                           const std::string fitType):
    ComponentCatalogue(srclist, parset, cube, fitType)
{
    itsVersion = ASKAP_PACKAGE_VERSION;
    itsFitType = fitType;
    itsSpec = duchamp::Catalogues::CatalogueSpecification();
    this->defineSpec();

    duchamp::Param par = parseParset(parset);
    std::string filenameBase = par.getOutFile();
    if (itsFitType == "best") {
        filenameBase.replace(filenameBase.rfind(".txt"),
                             std::string::npos, ".fitResults");
    } else {
        filenameBase.replace(filenameBase.rfind(".txt"),
                             std::string::npos, ".fitResults." + itsFitType);
    }
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";

}

void FitCatalogue::defineSpec()
{
    itsSpec.addColumn("LOCALID", "ID", "--", 6, 0,
                      "meta.id;meta.main", "char", "col_component_id", "");
    itsSpec.addColumn("NAME", "Name", "--", 8, 0,
                      "meta.id", "char", "col_component_name", "");
    itsSpec.addColumn("RAJD", "RA", "[deg]", 11, casda::precPos,
                      "pos.eq.ra;meta.main", "double", "col_rajd", "J2000");
    itsSpec.addColumn("DECJD", "DEC", "[deg]", 11, casda::precPos,
                      "pos.eq.dec;meta.main", "double", "col_decjd", "J2000");
    itsSpec.addColumn("XPOS", "X", "[pix]", 6, casda::precPix,
                      "pos.cartesian.x", "float", "col_xpos", "");
    itsSpec.addColumn("YPOS", "Y", "[pix]", 6, casda::precPix,
                      "pos.cartesian.y", "float", "col_ypos", "");
    itsSpec.addColumn("FINTISLAND", "F_int", "[" + casda::intFluxUnitContinuum + "]",
                      9, casda::precFlux,
                      "phot.flux.density;em.radio;meta.id.parent",
                      "float", "col_fint_island", "");
    itsSpec.addColumn("FPEAKISLAND", "F_peak", "[" + casda::fluxUnit + "]",
                      9, casda::precFlux,
                      "phot.flux.density;stat.max;em.radio;meta.id.parent",
                      "float", "col_fpeak_island", "");
    itsSpec.addColumn("FINT", "F_int(fit)", "[" + casda::intFluxUnitContinuum + "]",
                      9, casda::precFlux,
                      "phot.flux.density;em.radio;stat.fit",
                      "float", "col_fint", "");
    itsSpec.addColumn("FPEAK", "F_pk(fit)", "[" + casda::fluxUnit + "]",
                      9, casda::precFlux,
                      "phot.flux.density;stat.max;em.radio;stat.fit",
                      "float", "col_fpeak", "");
    itsSpec.addColumn("MAJ", "Maj(fit)", "[arcsec]", 6, casda::precSize,
                      "phys.angSize.smajAxis;em.radio;stat.fit",
                      "float", "col_maj", "");
    itsSpec.addColumn("MIN", "Min(fit)", "[arcsec]", 6, casda::precSize,
                      "phys.angSize.sminAxis;em.radio;stat.fit",
                      "float", "col_min", "");
    itsSpec.addColumn("PA", "PA(fit)", "[deg]", 7, casda::precSize,
                      "phys.angSize;pos.posAng;em.radio;stat.fit",
                      "float", "col_pa", "");
    itsSpec.addColumn("MAJERR", "maj_axis_err", "["+casda::shapeUnit+"]", casda::precSize+2, casda::precSize,
                      "stat.error;phys.angSize.smajAxis;em.radio",
                      "float", "col_maj_axis_err", "");
    itsSpec.addColumn("MINERR", "min_axis_err", "["+casda::shapeUnit+"]", casda::precSize+2, casda::precSize,
                      "stat.error;phys.angSize.sminAxis;em.radio",
                      "float", "col_min_axis_err", "");
    itsSpec.addColumn("PAERR", "pos_ang_err", "[deg]", casda::precSize + 2, casda::precSize,
                      "stat.error;phys.angSize;pos.posAng;em.radio",
                      "float", "col_pos_ang_err", "");
    itsSpec.addColumn("MAJDECONV", "Maj(fit_deconv)", "[arcsec]", 6, casda::precSize,
                      "phys.angSize.smajAxis;em.radio;askap:meta.deconvolved",
                      "float", "col_maj_deconv", "");
    itsSpec.addColumn("MINDECONV", "Min(fit_deconv)", "[arcsec]", 6, casda::precSize,
                      "phys.angSize.sminAxis;em.radio;askap:meta.deconvolved",
                      "float", "col_min_deconv", "");
    itsSpec.addColumn("PADECONV", "PA(fit_deconv)", "[deg]", 7, casda::precSize,
                      "phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved",
                      "float", "col_pa_deconv", "");
    itsSpec.addColumn("MAJDECONVERR", "maj_axis_deconv_err", "[arcsec]", casda::precSize + 2, casda::precSize,
                      "stat.error;phys.angSize.smajAxis;em.radio;askap:meta.deconvolved",
                      "float", "col_maj_axis_deconv_err", "");
    itsSpec.addColumn("MINDECONVERR", "min_axis_deconv_err", "[arcsec]", casda::precSize + 2, casda::precSize,
                      "stat.error;phys.angSize.sminAxis;em.radio;askap:meta.deconvolved",
                      "float", "col_min_axis_deconv_err", "");
    itsSpec.addColumn("PADECONVERR", "pos_ang_deconv_err", "[deg]", casda::precSize + 2, casda::precSize,
                      "stat.error;phys.angSize;pos.posAng;em.radio;askap:meta.deconvolved",
                      "float", "col_pos_ang_deconv_err", "");
    itsSpec.addColumn("ALPHA", "Alpha", "--", 8, casda::precSpecShape,
                      "spect.index;em.radio", "float", "col_alpha", "");
    itsSpec.addColumn("BETA", "Beta", "--", 8, casda::precSpecShape,
                      "askap:spect.curvature;em.radio", "float", "col_beta", "");
    itsSpec.addColumn("ALPHAERR", "spectral_index_err", "--", casda::precSpecShape + 2, casda::precSpecShape,
                      "stat.error;spect.index;em.radio", "float", "col_spectral_index_err", "");
    itsSpec.addColumn("BETAERR", "spectral_curvature_err", "--", casda::precSpecShape + 2, casda::precSpecShape,
                      "stat.error;askap:spect.curvature;em.radio", "float", "col_spectral_curvature_err", "");
    itsSpec.addColumn("CHISQ", "Chisq(fit)", "--", 10, casda::precFlux,
                      "stat.fit.chi2", "float", "col_chisqfit", "");
    itsSpec.addColumn("RMSIMAGE", "RMS(image)", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "stat.stdev;phot.flux.density", "float", "col_rmsimage", "");
    itsSpec.addColumn("RMSFIT", "RMS(fit)", "[" + casda::fluxUnit + "]", 10, casda::precFlux,
                      "stat.stdev;stat.fit", "float", "col_rmsfit", "");
    itsSpec.addColumn("NFREEFIT", "Nfree(fit)", "--", 11, 0,
                      "meta.number;stat.fit.param;stat.fit", "int", "col_nfreefit", "");
    itsSpec.addColumn("NDOFFIT", "NDoF(fit)", "--", 10, 0,
                      "stat.fit.dof", "int", "col_ndoffit", "");
    itsSpec.addColumn("NPIXFIT", "NPix(fit)", "--", 10, 0,
                      "meta.number;instr.pixel", "int", "col_npixfit", "");
    itsSpec.addColumn("NPIXISLAND", "NPix(obj)", "--", 10, 0,
                      "meta.number;instr.pixel;stat.fit", "int", "col_npixobj", "");
    itsSpec.addColumn("FLAG2", "fit_is_estimate", "", 5, 0,
                      "meta.flag", "int", "col_fit_is_estimate", "");

}

void FitCatalogue::writeVOTinformation(AskapVOTableCatalogueWriter &vowriter)
{
    vowriter.setResourceName("Catalogue of component fitting results from Selavy source-finding");
    vowriter.setTableName("Fitted component catalogue");

}


}

}


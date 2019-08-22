/// @file
///
/// Implementation of the base CASDA catalogue class
///
/// @copyright (c) 2019 CSIRO
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
#include <catalogues/CasdaCatalogue.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <duchampinterface/DuchampInterface.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/CatalogueWriter.hh>
#include <vector>
#include <map>

ASKAP_LOGGER(logger, ".casdacatalogue");

namespace askap {

namespace analysis {

CasdaCatalogue::CasdaCatalogue(const LOFAR::ParameterSet &parset, duchamp::Cube *cube):
    itsParset(parset),
    itsSpec(),
    itsCube(cube),
    itsFilenameStub("casdaBase"),
    itsObjectType("base"),
    itsVotableFilename(""),
    itsAsciiFilename(""),
    itsKarmaFilename(""),
    itsCASAFilename(""),
    itsDS9Filename(""),
    itsVersion("")
{
}

void CasdaCatalogue::setup()
{

    duchamp::Param par = parseParset(itsParset);
    std::string filenameBase = par.getOutFile();
    filenameBase.replace(filenameBase.rfind("txt"),
                         std::string::npos, itsFilenameStub);
    itsVotableFilename = filenameBase + ".xml";
    itsAsciiFilename = filenameBase + ".txt";
    if (itsParset.getBool("flagKarma", false)) {
        itsKarmaFilename = filenameBase + ".ann";
    }
    if (itsParset.getBool("flagCasa", false)) {
        itsCASAFilename = filenameBase + ".crf";
    }
    if (itsParset.getBool("flagDS9", false)) {
        itsDS9Filename = filenameBase + ".reg";
    }


}

void CasdaCatalogue::fixColWidth(duchamp::Catalogues::Column &col, unsigned int newWidth)
{
    if (col.getWidth() > newWidth) {
        ASKAPLOG_WARN_STR(logger, "Reducing width of column " << col.getName() << " from " << col.getWidth() << " to " << newWidth);
    }
    col.setWidth(newWidth);
}

void CasdaCatalogue::write()
{
    this->check(true);
    this->writeASCII();
    this->fixWidths();
    this->writeVOT();
    this->writeAnnotations();
}



void CasdaCatalogue::writeVOT()
{
    AskapVOTableCatalogueWriter vowriter(itsVotableFilename);
    vowriter.setup(itsCube);
    ASKAPLOG_DEBUG_STR(logger, "Writing " << itsObjectType <<
                       " table to the VOTable " << itsVotableFilename);
    vowriter.setColumnSpec(&itsSpec);
    vowriter.openCatalogue();
    vowriter.setResourceName(itsObjectType + " catalogue from Selavy source finding");
    vowriter.setTableName(itsObjectType + " catalogue");
    vowriter.writeHeader();
    duchamp::VOParam version("table_version", "meta.version",
                             "char", itsVersion, itsVersion.size() + 1, "");
    vowriter.writeParameter(version);
    vowriter.writeParameters();
    vowriter.writeStats();
    vowriter.writeTableHeader();
    writeVOTableEntries(&vowriter);
    vowriter.writeFooter();
    vowriter.closeCatalogue();
}

void CasdaCatalogue::writeASCII()
{

    AskapAsciiCatalogueWriter writer(itsAsciiFilename);
    ASKAPLOG_DEBUG_STR(logger, "Writing " << itsObjectType <<
                       " results to " << itsAsciiFilename);
    writer.setup(itsCube);
    writer.setColumnSpec(&itsSpec);
    writer.openCatalogue();
    writer.writeTableHeader();
    writeAsciiEntries(&writer);
    writer.writeFooter();
    writer.closeCatalogue();

}

}
}

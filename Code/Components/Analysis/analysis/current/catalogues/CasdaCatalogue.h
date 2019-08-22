/// @file
///
/// A base class for catalogues destined for CASDA
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
#ifndef ASKAP_ANALYSIS_CASDA_CAT_H_
#define ASKAP_ANALYSIS_CASDA_CAT_H_

#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/CatalogueWriter.hh>
#include <duchamp/Cubes/cubes.hh>
#include <vector>

namespace askap {

namespace analysis {

/// @brief A class holding all necessary information describing a
/// catalogue of Components, as per the CASDA specifications.
/// @details This class holds both the set of components for a given
/// image as well as the specification detailing how the information
/// should be written to a catalogue. It provides methods to write the
/// information to VOTable and ASCII format files.
class CasdaCatalogue {
    public:

        /// Default Constructor, that defines a few of the member filenames and sets the pointer to the Cube class
        CasdaCatalogue(const LOFAR::ParameterSet &parset, duchamp::Cube *cube);

        /// Default destructor
        virtual ~CasdaCatalogue() {};

        /// Check the widths of the columns based on the values within
        /// the catalogue.
        /// @param checkTitle - whether to include the title widths in the checking
        virtual void check(bool checkTitle) = 0;

        /// Write the catalogue to the ASCII & VOTable files (acts as
        /// a front-end to the writeVOT() and writeASCII() functions)
        virtual void write();


    protected:
        /// Complete the initialisation of the catalogue - defining the
        /// catalogue spec and setting up filenames. The filenames are set
        /// based on the output file given in the parset.
        virtual void setup();

        /// Define the catalogue specification. This function
        /// individually defines the columns used in describing the
        /// catalogue, using the Duchamp interface.
        virtual void defineSpec() = 0;

        void fixColWidth(duchamp::Catalogues::Column &col, unsigned int newWidth);
        virtual void fixWidths() = 0;

        virtual void writeAsciiEntries(AskapAsciiCatalogueWriter *writer) = 0;
        virtual void writeVOTableEntries(AskapVOTableCatalogueWriter *writer) = 0;

/// Writes the catalogue to a VOTable that conforms to the
        /// CASDA requirements. It has the necessary header
        /// information, the catalogue version number, and a table
        /// entry for each Component in the catalogue.
        virtual void writeVOT();

        /// Writes the catalogue to an ASCII text file that is
        /// human-readable (with space-separated and aligned
        /// columns). It has a commented line (ie. starting with '#')
        /// with the column titles, another with the units, then one
        /// line for each Component.
        virtual void writeASCII();

        /// Write annotation files for use with Karma, DS9 and CASA
        /// viewers. The annotations show the location and size of the
        /// components, drawing them as ellipses where appropriate. The
        /// filenames have the same form as the votable and ascii files,
        /// but with .ann/.reg/.crf suffixes.
        virtual void writeAnnotations() {};

        /// @brief Parset - can add things here
        LOFAR::ParameterSet            itsParset;

        /// The specification for the individual columns
        duchamp::Catalogues::CatalogueSpecification itsSpec;

        /// The duchamp::Cube, used to help instantiate the classes to
        /// write out the ASCII and VOTable files.
        duchamp::Cube *itsCube;

        /// The "stub" for the filename, that identifies the type of catalogue
        std::string itsFilenameStub;

        /// For purposes of logging and metadata in the VOTable, what sort of objects are we cataloguing?
        std::string itsObjectType;

        /// The filename of the VOTable output file
        std::string itsVotableFilename;

        /// The filename of the ASCII text output file.
        std::string itsAsciiFilename;

        /// The filename of the Karma annotation file
        std::string itsKarmaFilename;

        /// The filename of the CASA region file
        std::string itsCASAFilename;

        /// The filename of the DS9 region file
        std::string itsDS9Filename;

        /// The version of the catalogue specification, from CASDA.
        std::string itsVersion;

};

}

}

#endif

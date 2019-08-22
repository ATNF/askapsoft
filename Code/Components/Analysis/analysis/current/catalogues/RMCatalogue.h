/// @file
///
/// All that's needed to define a catalogue of Components for CASDA
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
#ifndef ASKAP_ANALYSIS_RM_CAT_H_
#define ASKAP_ANALYSIS_RM_CAT_H_

#include <catalogues/CasdaCatalogue.h>
#include <catalogues/CasdaPolarisationEntry.h>
#include <outputs/AskapVOTableCatalogueWriter.h>
#include <outputs/AskapAsciiCatalogueWriter.h>
#include <sourcefitting/RadioSource.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/CatalogueWriter.hh>
#include <vector>

namespace askap {

namespace analysis {

/// @brief A class holding all necessary information describing a
/// catalogue of RM measurements made on components, as per the CASDA
/// specifications.  @details This class holds both the set of
/// components for a given image as well as the specification
/// detailing how the information should be written to a catalogue. It
/// provides methods to write the information to VOTable and ASCII
/// format files.
class RMCatalogue : public CasdaCatalogue {
    public:
        /// Constructor, that calls defineComponents to define the
        /// catalogue from a set of RadioSource object, and defineSpec
        /// to set the column specification. The filenames are set
        /// based on the output file given in the parset.
        RMCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                    const LOFAR::ParameterSet &parset,
                    duchamp::Cube *cube,
                    askap::askapparallel::AskapParallel &itsComms);

        /// Default destructor
        virtual ~RMCatalogue() {};

        /// Check the widths of the columns based on the values within
        /// the catalogue.
        void check(bool checkTitle);


    protected:
        /// Define the vector list of Components using the input list
        /// of RadioSource objects and the parset. One component is
        /// created for each fitted Gaussian component from each
        /// RadioSource, then added to its Components.
        void defineComponents(std::vector<sourcefitting::RadioSource> &srclist,
                              const LOFAR::ParameterSet &parset);

        /// Define the catalogue specification. This function
        /// individually defines the columns used in describing the
        /// catalogue, using the Duchamp interface.
        void defineSpec();

        void fixWidths();

        void writeAsciiEntries(AskapAsciiCatalogueWriter *writer);
        void writeVOTableEntries(AskapVOTableCatalogueWriter *writer);

        /// The list of catalogued Components.
        std::vector<CasdaPolarisationEntry> itsComponents;

};

}

}

#endif

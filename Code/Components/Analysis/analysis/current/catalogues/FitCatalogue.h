/// @file
///
/// All that's needed to define a catalogue of Fitted Components
/// (slightly different in form to the CASDA component catalogue).
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
#ifndef ASKAP_ANALYSIS_FIT_CAT_H_
#define ASKAP_ANALYSIS_FIT_CAT_H_

#include <catalogues/CasdaComponent.h>
#include <catalogues/ComponentCatalogue.h>
#include <sourcefitting/RadioSource.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <vector>

namespace askap {

namespace analysis {

/// @brief A class holding all necessary information describing a
/// catalogue of fitted Components, with an emphasis on the fit
/// results.
/// @details This class holds both the set of fitted components for a
/// given image as well as the specification detailing how the
/// information should be written to a catalogue. It provides methods
/// to write the information to VOTable and ASCII format files. It
/// differs from the ComponentCatalogue class by focusing on the
/// fitted results and including items like the number of degrees of
/// freedom in the fit. The outputs are what Selavy would
/// traditionally produce in the "fit results" file. This class also
/// provides methods to produce annotation files showing the location
/// of fitted components.
class FitCatalogue : public ComponentCatalogue {
    public:
        /// Constructor, that calls defineComponents to define the
        /// catalogue from a set of RadioSource object, and defineSpec
        /// to set the column specification. The filenames are set
        /// based on the output file given in the parset.
        FitCatalogue(std::vector<sourcefitting::RadioSource> &srclist,
                     const LOFAR::ParameterSet &parset,
                     duchamp::Cube *cube,
                     const std::string fitType);

        /// Default destructor
        virtual ~FitCatalogue() {};

    protected:

        /// Define the catalogue specification. This function
        /// individually defines the columns used in describing the
        /// catalogue, using the Duchamp interface. This is
        /// reimplemented from that in ComponentCatalogue.
        void defineSpec();

        /// Writes the table-specific resource and table name fields to
        /// the VOTable. This will change for each derived class.
        void writeVOTinformation(AskapVOTableCatalogueWriter &vowriter);


};

}

}

#endif

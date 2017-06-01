/// @file
///
/// Class for specifying an entry in the Absorption Object catalogue
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
#ifndef ASKAP_ANALYSIS_CASDA_ABSORPTION_H_
#define ASKAP_ANALYSIS_CASDA_ABSORPTION_H_

#include <catalogues/Casda.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaComponent.h>
#include <sourcefitting/RadioSource.h>
#include <Common/ParameterSet.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>

namespace askap {

namespace analysis {

/// @brief A class defining an entry in the CASDA Component catalogue.
/// @details This class holds all information that will be written to
/// the CASDA component catalogue for a single fitted component. It
/// allows extraction from a RadioSource object and provides methods
/// to write out the Component to a VOTable or other type of catalogue
/// file.
class CasdaAbsorptionObject : public CatalogueEntry {
    public:
        /// Default constructor that does nothing.
        CasdaAbsorptionObject();
    
        /// Constructor that builds the Absorption object from a
        /// RadioSource.
        /// **THE INTERFACE IS STILL TO BE WORKED OUT FULLY**
        /// It takes a single fitted component, indicated
        /// by the parameter fitNumber, from the fit results given by
        /// the fitType parameter. The parset is used to make the
        /// corresponding Island, to get the Island ID, and is passed
        /// to the CatalogueEntry constructor to get the SB and base
        /// ID.
        CasdaAbsorptionObject(CasdaComponent &component,
                              sourcefitting::RadioSource &obj,
                              const LOFAR::ParameterSet &parset);

        /// Default destructor
        virtual ~CasdaAbsorptionObject() {};

        /// Return the RA (in decimal degrees)
        const float ra();
        /// Return the Declination (in decimal degrees)
        const float dec();
        // Return the ID string
        const std::string id();

        ///  Print a row of values for the objet into an
        ///  output table. Each column from the catalogue
        ///  specification is sent to printTableEntry for output.
        ///  \param stream Where the output is written
        ///  \param columns The vector list of Column objects
        void printTableRow(std::ostream &stream,
                           duchamp::Catalogues::CatalogueSpecification &columns);

        ///  Print a single value (a column) into an output table. The
        ///  column's correct value is extracted according to the
        ///  Catalogues::COLNAME key in the column given.
        ///  \param stream Where the output is written
        ///  \param column The Column object defining the formatting.
        void printTableEntry(std::ostream &stream,
                             duchamp::Catalogues::Column &column);

        /// Allow the Column provided to check its width against that
        /// required by the value for this object, and increase its
        /// width if need be. The correct value is chose according to
        /// the COLNAME key. If a key is given that was not expected,
        /// an Askap Error is thrown. Column must be non-const as it
        /// could change.
        void checkCol(duchamp::Catalogues::Column &column);

        /// Perform the column check for all colums in
        /// specification. If allColumns is false, only the columns
        /// with type=char are checked, otherwise all are.
        void checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool allColumns = true);

    /// @brief Functions allowing CasdaPolarisationEntry objects to be passed
        /// over LOFAR Blobs
        /// @name
        /// @{
        /// @brief Pass a CasdaPolarisationEntry object into a Blob
        /// @details This function provides a mechanism for passing the
        /// entire contents of a CasdaPolarisationEntry object into a
        /// LOFAR::BlobOStream stream
        friend LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &blob,
                                              CasdaAbsorptionObject& src);
        /// @brief Receive a CasdaPolarisationEntry object from a Blob
        /// @details This function provides a mechanism for receiving the
        /// entire contents of a CasdaPolarisationEntry object from a
        /// LOFAR::BlobIStream stream
        friend LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &blob,
                                              CasdaAbsorptionObject& src);

        /// @}

        /// @brief Comparison operator, using the component ID
        friend bool operator< (CasdaAbsorptionObject lhs, CasdaAbsorptionObject rhs)
        {
            return (lhs.id() < rhs.id());
        }


    protected:
        /// The ID of the image cube in which this object was found
        std::string itsImageID;
        /// The date/time of the observation
        std::string itsDate;
        /// The ID of the component that this object comes from
        std::string itsComponentID;
        /// The flux of the continuum at this object
        double itsContinuumFlux;
        /// The unique ID for this object
        std::string itsObjectID;
        /// The J2000 IAU-format name
        std::string itsName;
        /// The RA in string format: 12:34:56.7
        std::string itsRAs;
        /// The Declination in string format: 12:34:56.7
        std::string itsDECs;
        /// The RA in decimal degrees
        casda::ValueError itsRA;
        /// The Declination in decimal degrees
        casda::ValueError itsDEC;
        /// The frequency of the object, unweighted average
        casda::ValueError itsFreqUW;
        /// The frequency of the object, weighted average
        casda::ValueError itsFreqW;
        /// The HI redshift for the unweighted average frequency of the object
        casda::ValueError itsZHI_UW;
        /// The HI redshift for the weighted average frequency of the object
        casda::ValueError itsZHI_W;
        /// The HI redshift for the frequency of the peak optical depth
        casda::ValueError itsZHI_peak;
        /// The velocity width of the object at 50% of the peak optical depth
        casda::ValueError itsW50;
        /// The velocity width of the object at 20% of the peak optical depth
        casda::ValueError itsW20;
        /// The local RMS noise of the image cube surrounding the object
        double itsRMSimagecube;
        // The peak optical depth of the object
        casda::ValueError itsOpticalDepth_peak;
        /// The integrated optical depth of the object
        casda::ValueError itsOpticalDepth_int;

        /// A flag indicating whether the object's continuum component is resolved spatially
        unsigned int itsFlagResolved;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag2;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag3;
        /// A comment string, not used as yet.
        std::string itsComment;

};

}

}

#endif

/// @file
///
/// Class for specifying an entry in the Component catalogue
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
#ifndef ASKAP_ANALYSIS_CASDA_COMPONENT_H_
#define ASKAP_ANALYSIS_CASDA_COMPONENT_H_

#include <catalogues/Casda.h>
#include <catalogues/CatalogueEntry.h>
#include <sourcefitting/RadioSource.h>
#include <Common/ParameterSet.h>
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
class CasdaComponent : public CatalogueEntry {
    public:
        /// Default constructor that does nothing.
        CasdaComponent();

        /// Constructor that builds the Component object from a
        /// RadioSource. It takes a single fitted component, indicated
        /// by the parameter fitNumber, from the fit results given by
        /// the fitType parameter. The parset is used to make the
        /// corresponding Island, to get the Island ID, and is passed
        /// to the CatalogueEntry constructor to get the SB and base
        /// ID.
        CasdaComponent(sourcefitting::RadioSource &obj,
                       const LOFAR::ParameterSet &parset,
                       const unsigned int fitNumber,
                       const std::string fitType = casda::componentFitType);

        /// Default destructor
        virtual ~CasdaComponent() {};

        /// Return the RA (in decimal degrees)
        const float ra();
        /// Return the Declination (in decimal degrees)
        const float dec();
        /// Return the RA error (in decimal degrees)
        const float raErr();
        /// Return the Declination error (in decimal degrees)
        const float decErr();
        /// Return the component ID
        const std::string componentID();
        // Return the component name
        const std::string name();
        /// Return the integrated flux
        const double intFlux();
        /// Return the integrated flux converted to the requested unit
        const double intFlux(std::string unit);
        /// Return the error on the integrated flux
        const double intFluxErr();
        /// Return the error on the integrated flux converted to the requested unit
        const double intFluxErr(std::string unit);
        /// Return the frequency of observation
        const double freq();
        /// Return the frequency of observation converted to the requested unit
        const double freq(std::string unit);
        /// Return the spectral index
        const double alpha();
        /// Return the spectral index error
        const double alphaErr();
        /// Return the spectral curvature
        const double beta();
        /// Return the spectral curvature error
        const double betaErr();

        ///  Print a row of values for the Component into an
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
        /// required by the value for this Component, and increase its
        /// width if need be. The correct value is chose according to
        /// the COLNAME key. If a key is given that was not expected,
        /// an Askap Error is thrown. Column must be non-const as it
        /// could change.
        void checkCol(duchamp::Catalogues::Column &column, bool checkTitle);

        /// Perform the column check for all colums in
        /// specification.
        void checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle);

        /// Write the ellipse showing the component shape to the given
        /// Annotation file. This allows writing to Karma, DS9 or CASA
        /// annotation/region file.
        void writeAnnotation(boost::shared_ptr<duchamp::AnnotationWriter> &writer);

        /// @brief Functions allowing CasdaComponent objects to be passed
        /// over LOFAR Blobs
        /// @name
        /// @{
        /// @brief Pass a CasdaComponent object into a Blob
        /// @details This function provides a mechanism for passing the
        /// entire contents of a CasdaComponent object into a
        /// LOFAR::BlobOStream stream
        friend LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &stream,
                                              CasdaComponent& src);
        /// @brief Receive a CasdaComponent object from a Blob
        /// @details This function provides a mechanism for receiving the
        /// entire contents of a CasdaComponent object from a
        /// LOFAR::BlobIStream stream
        friend LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &stream,
                                              CasdaComponent& src);

        /// @}

        /// @brief Comparison operator, using the component ID
        friend bool operator< (CasdaComponent lhs, CasdaComponent rhs)
        {
            return (lhs.componentID() < rhs.componentID());
        }

    protected:
        /// The ID of the island that this component came from.
        std::string itsIslandID;
        /// The unique ID for this component
        std::string itsComponentID;
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
        /// The frequency of the image
        double itsFreq;
        /// The fitted peak flux of the component
        casda::ValueError itsFluxPeak;
        /// The integrated flux (fitted) of the component
        casda::ValueError itsFluxInt;
        /// The fitted major axis (FWHM)
        casda::ValueError itsMaj;
        /// The fitted minor axis (FWHM)
        casda::ValueError itsMin;
        /// The position angle of the fitted major axis
        casda::ValueError itsPA;
        /// The major axis after deconvolution
        casda::ValueError itsMaj_deconv;
        /// The minor axis after deconvolution
        casda::ValueError itsMin_deconv;
        /// The position angle of the major axis after deconvolution
        casda::ValueError itsPA_deconv;
        /// The chi-squared value from the fit
        double itsChisq;
        /// The RMS of the residual from the fit
        double itsRMSfit;
        /// The fitted spectral index of the component
        casda::ValueError itsAlpha;
        /// The fitted spectral curvature of the component
        casda::ValueError itsBeta;
        /// The local RMS noise of the image surrounding the component
        double itsRMSimage;
        /// A flag indicating whether more than one component was
        /// fitted to the island
        unsigned int itsFlagSiblings;
        /// A flag indicating the parameters of the component are from
        /// the initial estimate, and not the result of the fit
        unsigned int itsFlagGuess;
        /// A flag indicating origin of spectral indices: true=from Taylor terms, false=from cube
        unsigned int itsFlagSpectralIndexOrigin;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag4;
        /// A comment string, not used as yet.
        std::string itsComment;

        /// The following are not in the CASDA component catalogue at
        /// v1.7, but are reported in the fit catalogues of Selavy
        /// {
        /// The ID of the component, without the SB and image
        /// identifiers.
        std::string itsLocalID;
        /// The x-pixel location of the centre of the component
        double itsXpos;
        /// The y-pixel location of the centre of the component
        double itsYpos;
        /// The integrated flux of the island from which this
        /// component was derived
        double itsFluxInt_island;
        /// The peak flux of the island from which this component was
        /// derived
        double itsFluxPeak_island;
        /// The number of free parameters in the fit
        unsigned int itsNfree_fit;
        /// The number of degrees of freedom in the fit
        unsigned int itsNDoF_fit;
        /// The number of pixels used in the fit
        unsigned int itsNpix_fit;
        /// The number of pixels in the parent island.
        unsigned int itsNpix_island;
        /// }
};

}

}

#endif

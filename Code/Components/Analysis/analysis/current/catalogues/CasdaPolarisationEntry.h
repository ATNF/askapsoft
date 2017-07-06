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
#ifndef ASKAP_ANALYSIS_CASDA_POLARISATION_ENTRY_H_
#define ASKAP_ANALYSIS_CASDA_POLARISATION_ENTRY_H_

#include <catalogues/Casda.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaComponent.h>
#include <polarisation/RMSynthesis.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>

namespace askap {

namespace analysis {

/// @brief A class defining an entry in the CASDA Polarisation catalogue.
/// @details This class holds all information that will be written to
/// the CASDA polarisation catalogue for a single fitted component
/// that has had RM synthesis performed on it. It allows extraction
/// from a Component and RMSynthesis object and provides methods to
/// write out the information to a VOTable or other type of catalogue
/// file.
class CasdaPolarisationEntry : public CatalogueEntry {
    public:
        /// Default constructor that does nothing.
        CasdaPolarisationEntry();

/// Constructor that builds the Polarisation object from a
        /// RadioSource. It takes a single fitted component and runs
        /// the RM Synthesis on it. The parset defines the detection
        /// thresholds, as well as scheduling block information, and
        /// is passed to the RM Synthesis class to determine input
        /// images etc.
        CasdaPolarisationEntry(CasdaComponent *comp,
                               const LOFAR::ParameterSet &parset);

        /// Default destructor
        virtual ~CasdaPolarisationEntry() {};

        /// Return the RA (in decimal degrees)
        const float ra();
        /// Return the Declination (in decimal degrees)
        const float dec();
        // Return the component ID
        const std::string id();

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

        /// Perform the column check for all colums in specification.
    void checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle);

        /// @brief Functions allowing CasdaPolarisationEntry objects to be passed
        /// over LOFAR Blobs
        /// @name
        /// @{
        /// @brief Pass a CasdaPolarisationEntry object into a Blob
        /// @details This function provides a mechanism for passing the
        /// entire contents of a CasdaPolarisationEntry object into a
        /// LOFAR::BlobOStream stream
        friend LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &stream,
                                              CasdaPolarisationEntry& src);
        /// @brief Receive a CasdaPolarisationEntry object from a Blob
        /// @details This function provides a mechanism for receiving the
        /// entire contents of a CasdaPolarisationEntry object from a
        /// LOFAR::BlobIStream stream
        friend LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &stream,
                                              CasdaPolarisationEntry& src);

        /// @}

        /// @brief Comparison operator, using the component ID
        friend bool operator< (CasdaPolarisationEntry lhs, CasdaPolarisationEntry rhs)
        {
            return (lhs.id() < rhs.id());
        }


    protected:

        /// The unique ID for the component
        std::string itsComponentID;
        /// The J2000 IAU-format name for the component
        std::string itsName;
        /// The RA in decimal degrees
        double itsRA;
        /// The Declination in decimal degrees
        double itsDec;

        /// The band-median flux for the Stokes I spectrum
        double itsFluxImedian;
        /// The band-median flux for the Stokes Q spectrum
        double itsFluxQmedian;
        /// The band-median flux for the Stokes U spectrum
        double itsFluxUmedian;
        /// The band-median flux for the Stokes V spectrum
        double itsFluxVmedian;

        /// The band-median value for the Stokes I noise (RMS) spectrum
        double itsRmsI;
        /// The band-median value for the Stokes Q noise (RMS) spectrum
        double itsRmsQ;
        /// The band-median value for the Stokes U noise (RMS) spectrum
        double itsRmsU;
        /// The band-median value for the Stokes V noise (RMS) spectrum
        double itsRmsV;

        /// First-order coefficient for the polynomial fit to the Stokes I
        /// spectrum
        double itsPolyCoeff0;
        /// Second-order coefficient for the polynomial fit to the Stokes
        /// I spectrum
        double itsPolyCoeff1;
        /// Third-order coefficient for the polynomial fit to the Stokes I
        /// spectrum
        double itsPolyCoeff2;
        /// Fourth-order coefficient for the polynomial fit to the Stokes
        /// I spectrum
        double itsPolyCoeff3;
        /// Fifth-order coefficient for the polynomial fit to the Stokes I
        /// spectrum
        double itsPolyCoeff4;

        /// The square of the reference wavelength
        double itsLambdaSqRef;

        /// The FWHM of the RM spread function
        double itsRmsfFwhm;

        /// The signal-to-noise threshold for a valid detection
        float itsDetectionThreshold;
        /// The signal-to-noise threshold above which to perform debiasing
        float itsDebiasThreshold;

        /// The peak polarised intensity in the FDF
        casda::ValueError  itsPintPeak;
        /// The peak polarised intensity in the FDF, corrected for
        /// polarisation bias
        double itsPintPeakDebias;
        /// The fitted peak polarised intensity in the FDF
        casda::ValueError  itsPintPeakFit;
        /// The fitted peak polarised intensity in the FDF, corrected for
        /// polarisation bias
        double itsPintPeakFitDebias;

        /// The signal-to-noise ratio of the fitted peak polarised
        /// intensity
        casda::ValueError  itsPintFitSNR;

        /// The Faraday Depth at the peak of the FDF
        casda::ValueError  itsPhiPeak;
        /// The Faraday Depth from a fit to the peak of the FDF
        casda::ValueError  itsPhiPeakFit;

        /// The polarisation angle at the reference wavelength
        casda::ValueError  itsPolAngleRef;
        /// The polarisation angle at zero wavelength
        casda::ValueError  itsPolAngleZero;

        /// The fractional polarisation
        casda::ValueError  itsFracPol;

        /// The first Faraday Complexity metric - deviation from constant
        /// P(nu)
        double itsComplexity;
        /// The second Faraday Complexity metric - residual structure
        /// beyond a single Faraday-thin component
        double itsComplexity_screen;

        /// If the fitted peak polarised intensity is above the SNR threshold
        unsigned int itsFlagDetection;
        /// If the measured Faraday depth is close to the edge of the FDF
        /// spectrum.
        unsigned int itsFlagEdge;
        ///  A yet-to-be-identified quality flag
        unsigned int itsFlag3;
        ///  A yet-to-be-identified quality flag
        unsigned int itsFlag4;



};


}

}

#endif

/// @file
///
/// A class to encapsulate the modelling of a Stokes I spectrum, for
/// use with the Rotation Measure Synthesis.
///
/// @copyright (c) 2017 CSIRO
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

#ifndef ASKAP_ANALYSIS_POL_STOKESIMODEL_H_
#define ASKAP_ANALYSIS_POL_STOKESIMODEL_H_

#include <polarisation/StokesSpectrum.h>
#include <catalogues/CasdaComponent.h>

#include <casacore/casa/Arrays/Array.h>

#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

/// @brief A class to encapsulate the calculations (if necessary) and
/// the storage of coefficients related to a model of a Stokes I
/// spectrum.
/// @details This class is designed to hold information specifying a
/// model fit to a Stokes I spectrum. This model can be one of two
/// types, given by the itsType member: "taylor" means the
/// coefficients are Taylor-term parameters from the imaging; "poly"
/// means the coefficients are from a polynomial fit to the Stokes I
/// spectrum, of order itsOrder. The type is obtained from the
/// parameter set provided upon initialisation.  Methods are provided
/// to access the coefficients, either individually or as a group, and
/// to calculate the flux at an arbitrary frequency.
class StokesImodel {
    public:
        StokesImodel(const LOFAR::ParameterSet &parset);
        virtual ~StokesImodel() {};

        /// @brief Initialise the model coefficients
        /// @details Initialise the model spectrum according to the method
        /// requested. For "taylor" method, the Taylor-term parameters are
        /// extracted from the CasdaComponent, stored in the coefficients
        /// vector, and used to generate a model spectrum sampled at the
        /// same frequencies as the spectrum in the StokesSpectrum
        /// object. For the "poly" method, the Stokes I spectrum in the
        /// StokesSpectrum object is fitted with a polynomial, and the
        /// model spectrum created from the polynomial coefficients.
        /// @param I StokesSpectrum object that holds the extracted Stokes
        /// I spectrum
        /// @param comp CasdaComponent that holds the information about
        /// the component, including the Taylor-term decomposition.
        void initialise(StokesSpectrum &I, CasdaComponent *comp);

        /// @brief Return all fitted coefficients
        casa::Vector<float> coeffs() {return itsCoeffs;};

        /// @brief Rerun a single fitted coefficient.
        /// @details Return a single coefficient, where i is the index to the
        /// itsCoeffs vector. If i does not fall in the valid range, zero
        /// is returned.
        float coeff(unsigned int i);

        /// @brief Return the type of model fit
        std::string type() {return itsType;};

        /// @brief Return the flux of the model at the given frequency.
        float flux(float frequency);

        /// @brief Return the model spectrum for the same frequency values as the input Stokes I spectrum
        casa::Vector<float> modelSpectrum() {return itsModelSpectrum;};

        /// @brief Set the model spectrum directly
        void setModel(casa::Vector<float> model) {itsModelSpectrum = model; };

        /// @brief Set the fitted coefficients directly
        void setCoeffs(casa::Vector<float> coeffs) {itsCoeffs = coeffs;};

        /// @brief Set the type of model fit
        void setType(std::string type) {itsType = type;};

    protected:

        /// @brief Fit a polynomial to the spectrum
        void fit();

        /// @brief The coefficients describing the model fit - either
        /// polynomial or Taylor-expansion coefficients
        casa::Vector<float> itsCoeffs;

        /// @brief The reference frequency used in the Taylor expansion
        float               itsRefFreq;
        /// @brief The type of model fit: "poly" or "taylor"
        std::string         itsType;
        /// @brief The order of the polynomial fit
        unsigned int        itsOrder;

        /// @brief The list of channel frequency values
        casa::Vector<float> itsFreqs;
        /// @brief The input Stokes I spectrum
        casa::Vector<float> itsIspectrum;
        /// @brief The model spectrum with the same channel sampling
        casa::Vector<float> itsModelSpectrum;


};

}
}
#endif

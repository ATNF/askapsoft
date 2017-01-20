/// @file
///
/// Perform Rotation Measure Synthesis and parameterise
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
#ifndef ASKAP_ANALYSIS_RM_SYNTHESIS_H_
#define ASKAP_ANALYSIS_RM_SYNTHESIS_H_

#include <polarisation/PolarisationData.h>
#include <polarisation/StokesImodel.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/BasicSL/Complex.h>

#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

class RMSynthesis {
    public:
        /// @details Initialises the Farady Depth arrays (phi,
        /// phiForRMSF) according to the parset specification (which
        /// gives the number of phi channels, their spacing and the
        /// centre RM), and sets the FDF and RMSF arrays to zero.
        explicit RMSynthesis(const LOFAR::ParameterSet &parset);
        virtual ~RMSynthesis() {};

        /// @details Takes the PolarisationData object, which
        /// contains the I,Q,U spectra and the QU noise spectrum,
        /// and the lambda-squared array, and calls the main
        /// calculate function on those arrays to perform RM
        /// synthesis.
        void calculate(PolarisationData &poldata);

        /// @details Takes the lambda-squared array and corresponding
        /// Q &U spectra and QU noise spectrum, and defines the
        /// weights, the normalisation and the reference
        /// lambda-squared value. It then performs RM Synthesis,
        /// creating the FDF and RMSF arrays. Also calls the fitRMSF
        /// function to obtain the FWHM of the main RMSF lobe.
        void calculate(const casa::Vector<float> &lsq,
                       const casa::Vector<float> &q,
                       const casa::Vector<float> &u,
                       const casa::Vector<float> &noise);

        /// Fit to the RM Spread Function. Find extent of peak of RMSF
        /// by starting at peak and finding where slope changes -
        /// ie. go left, find where slope become negative. go right,
        /// find where slope become positive
        ///
        /// To that range alone, fit a Gaussian - fitGaussian should be
        /// fine.
        ///
        /// Records the FWHM of the fitted Gaussian
        void fitRMSF();

        /// @brief Type of weighting
        const std::string weightType() {return itsWeightType;};
        /// @brief Number of faraday depth channels
        const unsigned int numPhiChan() {return itsNumPhiChan;};
        /// @brief Spacing between faraday depth channels
        const float deltaPhi() {return itsDeltaPhi;};

        /// @brief Returns the Faraday Dispersion Function vector
        const casa::Vector<casa::Complex> &fdf() {return itsFaradayDF;};
        /// @brief Returns the Faraday Depth vector
        const casa::Vector<float> &phi() {return itsPhi;};
        /// @brief Returns the Rotation Measure Spread function
        const casa::Vector<casa::Complex> &rmsf() {return itsRMSF;};
        /// @brief Returns the Faraday Depth vector that goes with the RMSF
        const casa::Vector<float> &phi_rmsf() {return itsPhiForRMSF;};
        /// @brief Return the (fitted) width of the RMSF
        const float rmsf_width() {return itsRMSFwidth;};
        /// @brief Return the reference lambda-squared value (obtained
        /// from the weighted mean of the lambda-squared values)
        const float refLambdaSq() {return itsRefLambdaSquared;};

        /// @brief Returns the lambda-squared array used in the RM Synthesis
        casa::Vector<float> lambdaSquared() {return itsLamSq;};
        /// @brief Returns the input fractional polarisation spectrum (a complex vector p = q + i u)
        casa::Vector<casa::Complex> fracPolSpectrum() {return itsFracPolSpectrum;};
        /// @brief Define the Stokes I model spectrum by providing a vector
        void setImodel(casa::Vector<float> model) {itsImodel.setModel(model);};
        /// @brief Reference to the StokesImodel object defining the Stokes I model spectrum and fitted coefficients
        StokesImodel &imodel() {return itsImodel;};

        /// @brief Normalisation factor for the FDF
        const float normalisation() {return itsNormalisation;};
        /// @brief The average of the noise spectrum
        const float fdf_noise() {return itsFDFnoise;};

        /// @brief Number of frequency channels used
        const unsigned int numFreqChan() {return itsWeights.size();};
        /// @brief Return the variance of the lambda-squared values
        const float lsqVariance() {return itsLambdaSquaredVariance;};

    private:

        /// @brief Initialise phi and weights based on parset
        void defineVectors();

        /// @brief Vector of weights assigned to each channel
        casa::Vector<float>         itsWeights;
        /// @brief Type of weighting used: either "variance" (default) or "uniform"
        std::string                 itsWeightType;

        /// @brief The input complex fractional polarisation spectrum p=q+iu
        casa::Vector<casa::Complex> itsFracPolSpectrum;

        /// @brief Normalisation constant that depends on the weights
        float                       itsNormalisation;

        /// @brief Vector of lambda-squared values for each channel [m2]
        casa::Vector<float>         itsLamSq;
        /// @brief Variance of the lambda-square values
        float                       itsLambdaSquaredVariance;

        /// @brief Number of channels in the FDF
        unsigned int                itsNumPhiChan;
        /// @brief Spacing between the Faraday Depth channels [rad/m2]
        float                       itsDeltaPhi;
        /// @brief Centre RM of the Faraday depth vector [rad/m2]
        float                       itsPhiZero;
        /// @brief Faraday depth vector [rad/m2]
        casa::Vector<float>         itsPhi;

        /// @brief Faraday Dispersion Function
        casa::Vector<casa::Complex> itsFaradayDF;

        /// @brief The average of the provided noise spectrum, scaled by sqrt(num_freq_chan)
        float                       itsFDFnoise;

        /// @brief The specification of the Stokes I model spectrum
        StokesImodel                itsImodel;

        /// @brief Double-length Faraday depth vector, used to calculate the RMSF [rad/m2]
        casa::Vector<float>         itsPhiForRMSF;
        /// @brief Rotation Measure Spread Function (RMSF)
        casa::Vector<casa::Complex> itsRMSF;

        /// @brief Fitted width of the RMSF [rad/m2]
        float                       itsRMSFwidth;

        /// @brief Reference value of lambda-squared, based on weighted mean of lambda-squared channels [m2]
        float                       itsRefLambdaSquared;


};

}

}


#endif

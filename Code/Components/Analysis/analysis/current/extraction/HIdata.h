/// @file
///
/// Class to hold the extracted data for a single HI source
///
/// @copyright (c) 2016 CSIRO
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
#ifndef ASKAP_ANALYSIS_HI_DATA_H_
#define ASKAP_ANALYSIS_HI_DATA_H_

#include <boost/shared_ptr.hpp>
#include <extraction/SourceSpectrumExtractor.h>
#include <extraction/NoiseSpectrumExtractor.h>
#include <extraction/MomentMapExtractor.h>
#include <extraction/CubeletExtractor.h>
#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

/// @brief Class to hold extracted data used for HI analysis

/// @details This class relates to a specific HI source, and holds
/// extracted source & noise spectra, moment maps, and a cubelet. It
/// provides methods to obtain the extracted arrays for external
/// use. It will provide mechanisms to fit to the moment-0 map and to
/// the integrated spectrum, to support the HI catalogue.
class HIdata {
    public:
        HIdata(const LOFAR::ParameterSet &parset);
        virtual ~HIdata() {};

    /// @brief Set the source to be used.
    void setSource(RadioSource *src) {itsSource=src;};

    /// @brief Calculate the range of voxel statistics needed by the HI catalogue.
    void findVoxelStats();
    
    /// @brief Front-end for the extract functions
    void extract();
    /// @brief Extract the source spectrum using itsSpecExtractor
    void extractSpectrum();
    /// @brief Extract the noise spectrum using itsNoiseExtractor
    void extractNoise();
    /// @brief Extract the moment maps using itsMomentExtractor
    void extractMoments();
    /// @brief Extract the surrounding cubelet using itsCubeletExtractor
    void extractCubelet();

    /// @brief Call the writeImage() function for each extractor
    void write();

    /// @brief Fit a Gaussian to the moment-0 map
    void fitToMom0();

    /// @brief Fit a "busy-function" to the integrated spectrum
    /// @return Returns the return value from the BusyFit::fit() function - anything non-zero is an error.
    int busyFunctionFit();

    const float fluxMin(){return itsFluxMin;};
    const float fluxMax(){return itsFluxMax;};
    const float fluxMean(){return itsFluxMean;};
    const float fluxStddev(){return itsFluxStddev;};
    const float fluxRMS(){return itsFluxRMS;};

    const casa::Vector<double> BFparams(){return itsBFparams;};
    const casa::Vector<double> BFerrors(){return itsBFerrors;};
    const double BFchisq(){return itsBFchisq;};
    const double BFredChisq(){return itsBFredChisq;};
    const size_t BFndof(){return itsBFndof;};

    const casa::Vector<double> mom0Fit(){return itsMom0Fit;};
    const casa::Vector<double> mom0FitError(){return itsMom0FitError;};
    const bool mom0Resolved(){return itsMom0Resolved;};
    
protected:

    /// @brief Parset relating to HI parameters
    LOFAR::ParameterSet            itsParset;
    /// @brief Pointer to defining radio source
    RadioSource                   *itsSource;
    /// @brief Name of the input cube
    std::string                    itsCubeName;
    /// @brief Beam Log recording restoring beam per channel
    std::string                    itsBeamLog;

    /// @brief Extractor to obtain the source spectrum
    boost::shared_ptr<SourceSpectrumExtractor> itsSpecExtractor;
    /// @brief Extractor to obtain the noise spectrum
    boost::shared_ptr<NoiseSpectrumExtractor>  itsNoiseExtractor;
    /// @brief Extractor to obtain the moment maps (contains mom-0,1,2)
    boost::shared_ptr<MomentMapExtractor>      itsMomentExtractor;
    /// @brief Extractor to obtain the cubelets
    boost::shared_ptr<CubeletExtractor>        itsCubeletExtractor;

    /// @{
    /// Flux statistics
    /// @brief Maximum flux of object voxels
    float itsFluxMax;
    /// @brief Minimum flux of object voxels
    float itsFluxMin;
    /// @brief Mean flux over object voxels
    float itsFluxMean;
    /// @brief Standard deviation of object voxel fluxes
    float itsFluxStddev;
    /// @brief Root-mean-squared of the object voxel fluxes
    float itsFluxRMS;
    /// @}

    /// @{
    /// Busy Function fit results
    /// @brief Vector of BF fit parameters.
    casa::Vector<double> itsBFparams;
    /// @brief Vector of BF fit uncertainties on the parameters.
    casa::Vector<double> itsBFerrors;
    /// @brief chi-squared value from busy function fit
    double itsBFchisq;
    /// @brief Reduced chi-squared value (chisq/ndof)
    double itsBFredChisq;
    /// @brief Number of degrees of freedom of the fit.
    size_t itsBFndof;
    /// @}

    /// @{
    /// Gaussian fitting to moment-0 map
    /// @brief Vector of 2D Gaussian shape parameters - major/minor/pa (in degrees)
    casa::Vector<double> itsMom0Fit;
    /// @brief Vector of errors in fitted 2D Gaussian shape parameters - major/minor/pa (in degrees)
    casa::Vector<double> itsMom0FitError;
    /// @brief Is the moment-0 map resolved (does the PSF fit give an acceptable result?)
    bool itsMom0Resolved;
    /// @}
    
    
};

}

}

#endif


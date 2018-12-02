/// @file
///
/// Holds spectral information for a given source for a given Stokes parameter
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
#ifndef ASKAP_ANALYSIS_STOKES_SPECTRUM_H_
#define ASKAP_ANALYSIS_STOKES_SPECTRUM_H_

#include <sourcefitting/RadioSource.h>
#include <catalogues/CasdaComponent.h>
#include <extraction/SourceSpectrumExtractor.h>
#include <extraction/NoiseSpectrumExtractor.h>

#include <Common/ParameterSet.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/measures/Measures/Stokes.h>

namespace askap {

namespace analysis {

/// @brief Class to handle extraction of spectrum & noise
/// spectrum for a single Stokes parameter. This class enables
/// the straightforward extraction from a cube on disk
/// (usually corresponding to a Stokes parameter) of the
/// spectrum for a component, as well as its noise
/// spectrum. It will also measure the band-median value of
/// the spectrum and the noise. This class is designed to be
/// used in conjunction with the PolarisationData class for
/// input into the RMSynthesis pipeline.

class StokesSpectrum {
    public:
        /// @brief Constructor
        /// @details This defines the local parset along with the cube and
        /// output base names. It then constructs parsets for the two
        /// extractors, hard-coding some parameters that the user need not
        /// enter via the RMSynthesis interface. The extractors are then
        /// constructed with these parsets.
        /// @param parset Input parameter set.
        /// @param pol Capital-letter Stokes parameter (passed to
        /// extractor parsets).
        StokesSpectrum(const LOFAR::ParameterSet &parset, std::string pol);
        virtual ~StokesSpectrum() {};

        /// @brief Set the component to be used.
        void setComponent(CasdaComponent *src) {itsComponent = src;};

        /// @brief Front-end for two extract functions
        void extract();
        /// @brief Extract the source spectrum using itsSpecExtractor
        void extractSpectrum();
        /// @brief Extract the noise spectrum using itsNoiseExtractor
        void extractNoise();

        /// @brief Call the writeImage() function for each extractor
        void write();

        /// @brief Number of channels in the spectrum
        unsigned int size() {return itsSpectrum.size();};

        /// @brief Return the source spectrum
        casa::Vector<float> spectrum() {return itsSpectrum;};
        /// @brief Return the noise spectrum
        casa::Vector<float> noiseSpectrum() {return itsNoiseSpectrum;};

        /// @brief Return the median value of source spectrum
        float median() {return itsMedianValue;};
        /// @brief Return the median value of noise spectrum
        float medianNoise() {return itsMedianNoise;};
        /// @brief Return the list of channel frequency values
        casa::Vector<float> frequencies() {return itsFrequencies;};
        /// @brief Return the frequency units as a string
        std::string freqUnit() {return itsSpecExtractor->freqUnit();};
        /// @brief Return the brightness unit for the source spectrum as a
        /// casa::Unit object
        casa::Unit bunit() {return itsSpecExtractor->bunit();};

        /// @brief Name of the cube the spectra are extracted from
        std::string cubeName() {return itsSpecExtractor->inputCube();};

        /// @brief Return pointer to the extractor used for the spectrum
        SourceSpectrumExtractor *specExtractor() {return itsSpecExtractor;};
        /// @brief Return pointer to the extractor used for the noise spectrum
        SourceSpectrumExtractor *noiseExtractor() {return itsSpecExtractor;};

    private:

        /// @brief Parameter set for definition
        LOFAR::ParameterSet     itsParset;
        /// @brief Pointer to defining continuum component
        CasdaComponent          *itsComponent;
        /// @brief The Stokes parameter (I,Q,U,V etc)
        std::string             itsPol;
        /// @brief Name of the input cube
        std::string             itsCubeName;
        /// @brief Base name for the output spectra
        std::string             itsOutputBase;
        /// @brief Beam Log recording restoring beam per channel
        std::string             itsBeamLog;

        /// @brief Extractor to obtain the source spectrum
        SourceSpectrumExtractor *itsSpecExtractor;
        /// @brief Extractor to obtain the noise spectrum
        NoiseSpectrumExtractor  *itsNoiseExtractor;

        /// @brief The extracted spectrum
        casa::Vector<float>     itsSpectrum;
        /// @brief The median value of the extracted spectrum
        float                   itsMedianValue;
        /// @brief The extracted noise spectrum
        casa::Vector<float>     itsNoiseSpectrum;
        /// @brief The median value of the noise spectrum
        float                   itsMedianNoise;

        /// @brief The set of frequency values for the spectra
        casa::Vector<float>     itsFrequencies;

};

}

}

#endif

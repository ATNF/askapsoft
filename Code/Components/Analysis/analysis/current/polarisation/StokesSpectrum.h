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
    StokesSpectrum(const LOFAR::ParameterSet &parset, std::string pol);
    virtual ~StokesSpectrum() {};

    void setComponent(CasdaComponent *src){itsComponent=src;};
    void extract();
    void extractSpectrum();
    void extractNoise();
    void write();

    unsigned int size(){return itsSpectrum.size();};

    casa::Vector<float> spectrum() {return itsSpectrum;};
    casa::Vector<float> noiseSpectrum() {return itsNoiseSpectrum;};
    float median() {return itsMedianValue;};
    float medianNoise() {return itsMedianNoise;};
    casa::Vector<float> frequencies() {return itsFrequencies;};
    std::string freqUnit(){return itsSpecExtractor->freqUnit();};
    casa::Unit bunit(){return itsSpecExtractor->bunit();};

private:

    LOFAR::ParameterSet itsParset;
    CasdaComponent *itsComponent;
    std::string itsPol;
    casa::Vector<casa::Stokes::StokesTypes> itsStokes;
    std::string itsCubeName;
    std::string itsOutputBase;

    SourceSpectrumExtractor *itsSpecExtractor;
    NoiseSpectrumExtractor *itsNoiseExtractor;
    
    casa::Vector<float> itsSpectrum;
    float itsMedianValue;
    casa::Vector<float> itsNoiseSpectrum;
    float itsMedianNoise;

    casa::Vector<float> itsFrequencies;

};

}

}

#endif

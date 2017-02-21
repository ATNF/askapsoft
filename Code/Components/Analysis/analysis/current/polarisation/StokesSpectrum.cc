/// @file
///
/// XXX Notes on program XXX
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
#include <polarisation/StokesSpectrum.h>
#include <askap_analysis.h>

#include <sourcefitting/RadioSource.h>
#include <catalogues/CasdaComponent.h>
#include <extraction/SourceSpectrumExtractor.h>
#include <extraction/NoiseSpectrumExtractor.h>

#include <utils/PolConverter.h>

#include <Common/ParameterSet.h>
#include <Common/KVpair.h>
using namespace LOFAR::TYPES;
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/measures/Measures/Stokes.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".stokesspectrum");

namespace askap {

namespace analysis {

StokesSpectrum::StokesSpectrum(const LOFAR::ParameterSet &parset,
                               std::string pol):
    itsParset(parset), itsPol(pol)
{
    itsCubeName = parset.getString("cube", "");
    ASKAPCHECK(itsCubeName != "", "No cube name given");

    itsOutputBase = parset.getString("outputBase", "");
    ASKAPCHECK(itsOutputBase != "", "No output name given");

    itsBeamLog = parset.getString("beamLog", "");

    std::stringstream outputbase;

    // Define the parset used to set up the source extractor
    LOFAR::ParameterSet specParset;
    specParset.add(LOFAR::KVpair("spectralCube", itsCubeName));
    outputbase << itsOutputBase << "_spec_" << itsPol;
    specParset.add(LOFAR::KVpair("spectralOutputBase", outputbase.str()));
    specParset.add(LOFAR::KVpair("spectralBoxWidth",
                                 itsParset.getInt("boxwidth", 5)));
    specParset.add(LOFAR::KVpair("polarisation", itsPol));
    specParset.add(LOFAR::KVpair("useDetectedPixels", false));
    specParset.add(LOFAR::KVpair("scaleSpectraByBeam", true));
    specParset.add(LOFAR::KVpair("beamLog", itsBeamLog));
    itsSpecExtractor = new SourceSpectrumExtractor(specParset);

    // Define the parset used to set up the noise extractor
    LOFAR::ParameterSet noiseParset;
    noiseParset.add(LOFAR::KVpair("spectralCube", itsCubeName));
    outputbase.str("");
    outputbase << itsOutputBase << "_noise_" << itsPol;
    noiseParset.add(LOFAR::KVpair("spectralOutputBase", outputbase.str()));
    noiseParset.add(LOFAR::KVpair("noiseArea",
                                  itsParset.getFloat("noiseArea", 50.)));
    noiseParset.add(LOFAR::KVpair("robust",
                                  itsParset.getBool("robust", true)));
    noiseParset.add(LOFAR::KVpair("useDetectedPixels", false));
    noiseParset.add(LOFAR::KVpair("scaleSpectraByBeam", true));
    itsNoiseExtractor = new NoiseSpectrumExtractor(noiseParset);


}

void StokesSpectrum::extract()
{

    extractSpectrum();

    extractNoise();

}

void StokesSpectrum::extractSpectrum()
{

    itsSpecExtractor->setSource(itsComponent);
    itsSpecExtractor->extract();
    itsSpectrum = casa::Vector<float>(itsSpecExtractor->array());
    itsMedianValue = casa::median(itsSpectrum);

    itsFrequencies = itsSpecExtractor->frequencies();

}

void StokesSpectrum::extractNoise()
{

    itsNoiseExtractor->setSource(itsComponent);
    itsNoiseExtractor->extract();
    itsNoiseSpectrum = casa::Vector<float>(itsNoiseExtractor->array());
    itsMedianNoise = casa::median(itsNoiseSpectrum);

}

void StokesSpectrum::write()
{

    itsSpecExtractor->writeImage();
    itsNoiseExtractor->writeImage();

}


}

}

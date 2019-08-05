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
#include <casacore/casa/Arrays/ArrayLogical.h>
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
//    ASKAPCHECK(itsOutputBase != "", "No output name given");

    itsBeamLog = parset.getString("beamLog", "");

    std::stringstream outputbase;
    std::string objid = itsParset.getString("objid","");
    std::string objectname = itsParset.getString("objectname","");

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
    specParset.add("imagetype",itsParset.getString("imagetype","fits"));
    if (itsParset.isDefined("imageHistory")){
        specParset.add("imageHistory", itsParset.getString("imageHistory"));
    }
    itsSpecExtractor = boost::shared_ptr<SourceSpectrumExtractor>(new SourceSpectrumExtractor(specParset));
    itsSpecExtractor->setObjectIDs(objid,objectname);

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
    noiseParset.add(LOFAR::KVpair("polarisation", itsPol));
    noiseParset.add(LOFAR::KVpair("useDetectedPixels", false));
    noiseParset.add(LOFAR::KVpair("scaleSpectraByBeam", true));
    noiseParset.add("imagetype",itsParset.getString("imagetype","fits"));
    if (itsParset.isDefined("imageHistory")){
        noiseParset.add("imageHistory", itsParset.getString("imageHistory"));
    }
    itsNoiseExtractor = boost::shared_ptr<NoiseSpectrumExtractor>(new NoiseSpectrumExtractor(noiseParset));
    itsNoiseExtractor->setObjectIDs(objid,objectname);


}

StokesSpectrum::~StokesSpectrum()
{
    itsSpecExtractor.reset();
    itsNoiseExtractor.reset();
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
//    itsMedianValue = casa::median(itsSpectrum(!isNaN(itsSpectrum)).getArray());
    std::vector<float> vec;
    for(size_t i=0;i<itsSpectrum.size();i++){
        if (!isNaN(itsSpectrum[i]) && !isInf(itsSpectrum[i])){
            vec.push_back(i);
        }
    }
    casa::Vector<float> newvec(vec);
    itsMedianValue = casa::median(newvec);

//    itsMedianValue = casa::median(itsSpectrum(isNaN(itsSpectrum)));

    itsFrequencies = itsSpecExtractor->frequencies();

}

void StokesSpectrum::extractNoise()
{

    itsNoiseExtractor->setSource(itsComponent);
    itsNoiseExtractor->extract();
    itsNoiseSpectrum = casa::Vector<float>(itsNoiseExtractor->array());
    //itsMedianNoise = casa::median(itsNoiseSpectrum(!isNaN(itsNoiseSpectrum)).getArray());
    std::vector<float> vec = itsNoiseSpectrum.tovector();
    for(std::vector<float>::iterator i=vec.begin();i<vec.end();i++){
        if (isNaN(*i)){
            vec.erase(i);
        }
    }
    casa::Vector<float> newvec(vec);
    itsMedianNoise = casa::median(newvec);

}

void StokesSpectrum::write()
{

    itsSpecExtractor->writeImage();
    itsNoiseExtractor->writeImage();

}


}

}

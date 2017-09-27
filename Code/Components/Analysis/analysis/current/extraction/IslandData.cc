/// @file
///
/// Class to hold the extracted data for a single continuum island
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
/// published by the Free Software Foundation; either version 3 of the License,
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
#include <extraction/IslandData.h>
#include <askap_analysis.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <boost/shared_ptr.hpp>
#include <extraction/CubeletExtractor.h>
#include <Common/ParameterSet.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".islanddata");

namespace askap {

namespace analysis {

IslandData::IslandData(const LOFAR::ParameterSet &parset, const std::string fitType):
    itsParset(parset),
    itsFitType(fitType)
{

    ASKAPLOG_DEBUG_STR(logger, "Initialising IslandData object");
    itsImageName = parset.getString("image", "");
    ASKAPLOG_DEBUG_STR(logger, "Image name =\"" << itsImageName << "\"");
    ASKAPCHECK(itsImageName != "", "No image name given");
    itsMeanImageName = parset.getString("VariableThreshold.AverageImageName", "");
    itsNoiseImageName = parset.getString("VariableThreshold.NoiseImageName", "");


    // Define the parset used to set up the cubelet extractor for the image data - only used if we don't have the mean or noise maps
    ASKAPLOG_DEBUG_STR(logger, "Setting up image extractor");
    LOFAR::ParameterSet imageExtractParset;
    imageExtractParset.add(LOFAR::KVpair("spectralCube", itsImageName));
    // imageExtractParset.add(LOFAR::KVpair("padSize","[50,50]"));
    itsImageExtractor = boost::shared_ptr<CubeletExtractor>(new CubeletExtractor(imageExtractParset));

    if (itsMeanImageName != "") {
        ASKAPLOG_DEBUG_STR(logger, "Setting up mean image extractor");
        // Define the parset used to set up the cubelet extractor for the noise data
        LOFAR::ParameterSet meanExtractParset;
        meanExtractParset.add(LOFAR::KVpair("spectralCube", itsMeanImageName));
        itsMeanExtractor = boost::shared_ptr<CubeletExtractor>(new CubeletExtractor(meanExtractParset));
    }

    if (itsNoiseImageName != "") {
        ASKAPLOG_DEBUG_STR(logger, "Setting up noise image extractor");
        // Define the parset used to set up the cubelet extractor for the noise data
        LOFAR::ParameterSet noiseExtractParset;
        noiseExtractParset.add(LOFAR::KVpair("spectralCube", itsNoiseImageName));
        itsNoiseExtractor = boost::shared_ptr<CubeletExtractor>(new CubeletExtractor(noiseExtractParset));
    }

}

void IslandData::findVoxelStats()
{
    findBackground();
    findNoise();
    findResidualStats();
}

void IslandData::findBackground()
{
    if (itsMeanImageName != "") {
        // mean map is defined, so extract array
        itsMeanExtractor->setSource(itsSource);
        itsMeanExtractor->extract();
        casa::IPosition start = itsMeanExtractor->slicer().start();
        casa::Array<float> meanArray = itsMeanExtractor->array();
        std::vector<PixelInfo::Voxel> voxelList = itsSource->getPixelSet();
        itsBackground = 0.;
        std::vector<PixelInfo::Voxel>::iterator vox = voxelList.begin();
        for (; vox < voxelList.end(); vox++) {
            //float flux = vox->getF();
            if (itsSource->isInObject(*vox)) {
                // The Stokes axis, if present, will be of length 1, and will be either location 2 or 3 in the IPosition
                casa::IPosition loc;
                if (start.size() == 2) {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY());
                } else if (start.size() == 3) {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ());
                } else {
                    if (itsMeanExtractor->slicer().length()[2] == 1) {
                        loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), 0, vox->getZ());
                    } else {
                        loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ(), 0);
                    }
                }
                itsBackground += meanArray(loc - start);
            }
        }
        float size = float(voxelList.size());
        itsBackground /= size;

    } else {
        itsBackground = 0.;
    }


}
void IslandData::findNoise()
{
    if (itsNoiseImageName != "") {
        // noise map is defined, so extract array
        itsNoiseExtractor->setSource(itsSource);
        itsNoiseExtractor->extract();
        casa::IPosition start = itsNoiseExtractor->slicer().start();
        casa::Array<float> noiseArray = itsNoiseExtractor->array();
        std::vector<PixelInfo::Voxel> voxelList = itsSource->getPixelSet();
        itsNoise = 0.;
        std::vector<PixelInfo::Voxel>::iterator vox = voxelList.begin();
        for (; vox < voxelList.end(); vox++) {
            //float flux = vox->getF();
            if (itsSource->isInObject(*vox)) {
                // The Stokes axis, if present, will be of length 1, and will be either location 2 or 3 in the IPosition
                casa::IPosition loc;
                if (start.size() == 2) {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY());
                } else if (start.size() == 3) {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ());
                } else {
                    if (itsNoiseExtractor->slicer().length()[2] == 1) {
                        loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), 0, vox->getZ());
                    } else {
                        loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ(), 0);
                    }
                }
                itsNoise += noiseArray(loc - start);
            }
        }
        float size = float(voxelList.size());
        itsNoise /= size;

    } else {
        itsNoise = 0.;
    }


}


void IslandData::findResidualStats()
{

    ASKAPLOG_DEBUG_STR(logger, "Setting the source for the image extractor");
    itsImageExtractor->setSource(itsSource);
    ASKAPLOG_DEBUG_STR(logger, "Extracting");
    itsImageExtractor->extract();
    ASKAPLOG_DEBUG_STR(logger, "Starting to find stats");
    casa::IPosition start = itsImageExtractor->slicer().start();
    casa::Array<float> array = itsImageExtractor->array();
    std::vector<PixelInfo::Voxel> voxelList = itsSource->getPixelSet();
    float max, min, sumf = 0., sumff = 0.;
    casa::Vector<double> pos(2);
    std::vector<PixelInfo::Voxel>::iterator vox = voxelList.begin();
    for (; vox < voxelList.end(); vox++) {
        //float flux = vox->getF();
        if (itsSource->isInObject(*vox)) {
            // The Stokes axis, if present, will be of length 1, and will be either location 2 or 3 in the IPosition
            casa::IPosition loc;
            if (start.size() == 2) {
                loc = casa::IPosition(start.size(), vox->getX(), vox->getY());
            } else if (start.size() == 3) {
                loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ());
            } else {
                if (itsImageExtractor->slicer().length()[2] == 1) {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), 0, vox->getZ());
                } else {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ(), 0);
                }
            }
            float flux = array(loc - start);
            std::vector<casa::Gaussian2D<Double> > gaussians = itsSource->gaussFitSet(itsFitType);
            for (int g = 0; g < gaussians.size(); g++) {
                pos(0) = vox->getX() * 1.;
                pos(1) = vox->getY() * 1.;
                flux -= gaussians[g](pos);
            }
            if (vox == voxelList.begin()) {
                min = max = flux;
            } else {
                min = std::min(min, flux);
                max = std::max(max, flux);
            }
            sumf += flux;
            sumff += flux * flux;
        }
    }
    float size = float(voxelList.size());
    itsResidualMax = max;
    itsResidualMin = min;
    itsResidualMean = sumf / size;
    itsResidualStddev = sqrt(sumff / size - sumf * sumf / size / size);
    itsResidualRMS = sqrt(sumff / size);


}

}
}


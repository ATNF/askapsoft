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
#ifndef ASKAP_ANALYSIS_ISLAND_DATA_H_
#define ASKAP_ANALYSIS_ISLAND_DATA_H_

#include <boost/shared_ptr.hpp>
#include <extraction/CubeletExtractor.h>
#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

/// @brief Class to hold extracted data used for analysis of continuum
/// islands.
/// @details This class relates to a specific continuum island, and
/// holds extracted image data from the image itself, the noise map
/// and the continuum-component-residual. It provides methods to
/// obtain the extracted arrays for external use, as well as
/// statistics relevant for cataloguing.
class IslandData {
    public:
        IslandData(const LOFAR::ParameterSet &parset,
                   const std::string fitType);
        virtual ~IslandData() {};

        /// @brief Set the source to be used.
        void setSource(RadioSource *src) {itsSource = src;};

        /// @brief Calculate the range of statistics needed for the Island catalogue
        void findVoxelStats();
        void findBackground();
        void findNoise();
        void findResidualStats();

        const float background() {return itsBackground;};
        const float noise() {return itsNoise;};
        const float residualMin() {return itsResidualMin;};
        const float residualMax() {return itsResidualMax;};
        const float residualMean() {return itsResidualMean;};
        const float residualStddev() {return itsResidualStddev;};
        const float residualRMS() {return itsResidualRMS;};

    protected:

        /// @brief Input parset
        LOFAR::ParameterSet            itsParset;
        /// @brief Pointer to defining radio source
        RadioSource                   *itsSource;
        /// @brief Type of fit to use for calculating fit residuals
        const std::string              itsFitType;
        /// @brief Name of the input image
        std::string                    itsImageName;
        /// @brief Name of the mean image
        std::string                    itsMeanImageName;
        /// @brief Name of the noise image
        std::string                    itsNoiseImageName;
        /// @brief Name of the fit residual image
        std::string                    itsResidualImageName;

        /// @brief Extractor to obtain the image array values
        boost::shared_ptr<CubeletExtractor>        itsImageExtractor;
        /// @brief Extractor to obtain the mean array values
        boost::shared_ptr<CubeletExtractor>        itsMeanExtractor;
        /// @brief Extractor to obtain the noise array values
        boost::shared_ptr<CubeletExtractor>        itsNoiseExtractor;
        /// @brief Extractor to obtain the residual array values (after subtraction of continuum components)
        boost::shared_ptr<CubeletExtractor>        itsResidualExtractor;

        /// @brief Average background level across object voxels
        float itsBackground;

        /// @brief Average background noise across object voxels
        float itsNoise;

        /// @brief Maximum flux of object voxels in fit residual image
        float itsResidualMax;
        /// @brief Minimum flux of object voxels in fit residual image
        float itsResidualMin;
        /// @brief Mean flux over object voxels in fit residual image
        float itsResidualMean;
        /// @brief Standard deviation of object voxel fluxes in fit residual image
        float itsResidualStddev;
        /// @brief Root-mean-squared of the object voxel fluxes in fit residual image
        float itsResidualRMS;
        /// @}

};

}

}

#endif


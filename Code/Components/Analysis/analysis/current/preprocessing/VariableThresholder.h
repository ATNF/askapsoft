/// @file
///
/// Control class to run the calculation of the variable (box) threshold
///
/// @copyright (c) 2011 CSIRO
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
#ifndef ASKAP_ANALYSIS_VAR_THRESH_H_
#define ASKAP_ANALYSIS_VAR_THRESH_H_
#include <askap/askapparallel/AskapParallel.h>
#include <analysisparallel/SubimageDef.h>
#include <outputs/ImageWriter.h>
#include <Common/ParameterSet.h>
#include <duchamp/Cubes/cubes.hh>
#include <string>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/casa/aipstype.h>
#include <parallelanalysis/Weighter.h>
#include <boost/shared_ptr.hpp>

namespace askap {

namespace analysis {

/// @brief Handle the calculation and application of a
/// threshold that varies with location in the image.

/// @details This class handles all operations related to the
/// calculation and application of a variable detection
/// threshold, as well as the output of maps of the threshold,
/// noise, S/N ratio. The threshold is calculated based on the
/// statistics within a sliding box, so that the noise
/// properties for a given pixel depend only on the pixels
/// within a box (2D or 1D) of a specified size centred on
/// that pixel. The statistics can be calculated based on
/// robust measures (median and MADFM), or traditional
/// mean/standard deviation. The threshold applied is a
/// constant signal-to-noise ratio.
///
/// The maps of various quantities can also be written to CASA images
/// on disk. These quantities include the noise level, the threshold
/// (in flux units), the signal-to-noise ratio

class VariableThresholder {
    public:
        VariableThresholder() {};

        /// @details Initialise from a LOFAR parset. Define all
        /// parameters save for the input image, the search type
        /// and the robust stats flag - all of which are set
        /// according to the duchamp::Cube parameters. If an
        /// output image name is not provided, it will not be
        /// written.
        VariableThresholder(askap::askapparallel::AskapParallel& comms,
                            const LOFAR::ParameterSet &parset);
        virtual ~VariableThresholder() {};

        /// @details Initialise the class with information from
        /// the duchamp::Cube. This is done to avoid replicating
        /// parameters and preserving the parameter
        /// hierarchy. Once the input image is known, the output
        /// image names can be set with fixName() (if they have
        /// not been defined via the parset).
        void initialise(duchamp::Cube &cube, analysisutilities::SubimageDef &subdef);

        void setWeighter(boost::shared_ptr<Weighter> &weighter) {itsWeighter = weighter;};

        /// @details Calculate the signal-to-noise at each
        /// pixel. The cube (if it is a cube) is broken up into a
        /// series of lower dimensional data sets - the search
        /// type parameter defines whether this is done as a
        /// series of 2D images or 1D spectra. For each subset,
        /// the "middle" (mean or median) and "spread" (standard
        /// deviation or median absolute deviation from the
        /// median) for each pixel are calculated, and the
        /// signal-to-noise map is formed. At each stage, any
        /// outputs are made, with the subset being written to the
        /// appropriate image at the appropriate location. At the
        /// end, the signal-to-noise map is written to the Cube's
        /// reconstructed array, from where the detections can be
        /// made.
        void calculate();

        /// @details Once the signal-to-noise array is defined, we
        /// extract objects from it based on the signal-to-noise
        /// threshold. The resulting object list is put directly
        /// into the duchamp::Cube object, where it can be
        /// accessed from elsewhere. The detection map is updated
        /// and the Duchamp log file can be written to (if
        /// required).
        void search();

        /// @{

        /// @details The filenames for the different images created by this class. If imagetype=fits, then a ".fits" suffix is added to the filename as provided by the parset.
        std::string snrImage();
        std::string thresholdImage();
        std::string noiseImage();
        std::string averageImage();
        std::string boxSumImage();
        /// @}

        unsigned int boxSize() {return itsBoxSize;};

    protected:

        /// @details Create the output images as requested. Where the
        /// appropriate image name is defined, the image is created. This
        /// is done by the master node only (within the
        /// DistributedImageWriter::create task) when run in parallel.
        void createImages();

        /// @details Writes an array as requested to an image on
        /// disk. Where the image name is defined , the array (one of
        /// mean,noise,boxsum,snr or threshold) is written in
        /// distributed fashion to a CASA or FITS image on disk. The
        /// 'accumulate' method for DistributedImageWriter::write is
        /// used, taking into account any overlapping border regions.
        void writeImage(casacore::Array<Float> &arr,
                        casacore::Array<bool> &mask,
                        std::string imageName,
                        casacore::IPosition &loc);

        void defineChunk(casacore::Array<Float> &inputChunkArr,
                         casacore::MaskedArray<Float> &outputChunk, size_t ctr);

        void saveSNRtoCube(casacore::Array<casacore::Float> &snr, size_t ctr);

        /// @brief The MPI communication information
        askap::askapparallel::AskapParallel *itsComms;

        /// @brief The defining parset
        LOFAR::ParameterSet itsParset;

        /// Should we use robust (ie. median-based) statistics
        bool itsFlagRobustStats;
        float itsSNRthreshold;
        std::string itsSearchType;
        /// The half-box-width used for the sliding-box calculations
        unsigned int itsBoxSize;

        std::string itsInputImage;

        /// Image type of output images
        std::string itsImagetype;
        /// Suffix string to be added to the filenames of output images
        std::string itsImageSuffix;
        /// Name of S/N image to be written
        std::string itsSNRimageName;
        /// Name of Threshold image to be written
        std::string itsThresholdImageName;
        /// Name of Noise image to be written
        std::string itsNoiseImageName;
        /// Name of Mean image to be written
        std::string itsAverageImageName;
        /// Name of box sum image to be written
        std::string itsBoxSumImageName;
        /// Do we need to write any images?
        bool itsFlagWriteImages;
        /// Are we re-using exising images?
        bool itsFlagReuse;

        /// @brief The subimage definition
        analysisutilities::SubimageDef *itsSubimageDef;
        duchamp::Cube *itsCube;
        boost::shared_ptr<Weighter> itsWeighter;
        casacore::Slicer itsSlicer;
        casacore::IPosition itsInputShape;
        casacore::IPosition itsLocation;
        casacore::CoordinateSystem itsInputCoordSys;
        casacore::Array<bool> itsMask;

};

}

}




#endif

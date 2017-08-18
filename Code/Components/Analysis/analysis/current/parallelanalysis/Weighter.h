/// @file
///
/// XXX Notes on program XXX
///
/// @copyright (c) 2010 CSIRO
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
#ifndef ASKAP_ANALYSIS_WEIGHTER_H_
#define ASKAP_ANALYSIS_WEIGHTER_H_

#include <askapparallel/AskapParallel.h>
#include <casainterface/CasaInterface.h>
#include <duchamp/Utils/Section.hh>
#include <casacore/casa/aipstype.h>
#include <casacore/casa/Arrays/Vector.h>

#include <vector>
#include <string>

namespace askap {
namespace analysis {

/// @brief A class to get the relative weight of a given pixel.
/// @details The weights normalisation is found via distributed
/// analysis (image is split into subsections, workers find their
/// local maximum, and the overall maximum is determined by the
/// master). The image pixels can be scaled by the relative weight and
/// searched, and a weight cutoff can be applied to ignore pixels
/// outside some weights contour.

class Weighter {

    public:
        /// @brief Set up the weighter, defining parameters from the
        /// parset and initialising the normalisation to zero.
        Weighter(askap::askapparallel::AskapParallel& comms,
                 const LOFAR::ParameterSet &parset);
        virtual ~Weighter() {};

        /// @brief Store the cube pointer, read the weights image, and
        /// find the normalisation if we require it.
        void initialise(duchamp::Cube &cube, bool doAllocation = true);

        /// @brief Return the weight for a given array index, with pixels
        /// outside the weights cutoff returning zero.
        float weight(size_t i);

        /// @brief Perform a weighted search.
        /// @details Scale the pixel values from the image by the weight
        /// (if itsFlagDoScaling=true), then run the search
        /// algorithm. Results are stored in itsCube.
        void search();

        /// @brief Change the image pixel values that lie outside the
        /// weights cutoff to either zero or the assigned BLANK value
        /// (depending on the value of itsCutoffType).
        void applyCutoff();

        /// @brief The value of the weights cutoff.
        float cutoff() {return itsWeightCutoff;};

        /// @brief Is the weights image defined?
        bool fileOK() {return (itsImage != "");};

        /// @brief Can we apply a cutoff (ie. is the file OK and is the weight cutoff defined)?
        bool doApplyCutoff() {return fileOK() && (itsWeightCutoff > 0.);};

        /// @brief Is a nominated pixel above the weight cutoff?
        bool isValid(size_t i);

        /// @brief Shall we do the scaling of the image pixels?
        bool doScaling() {return fileOK() && itsFlagDoScaling;};

        /// @brief Is the Weighter set up to perform the scaling and/or apply a cutoff?
        bool isValid() {return fileOK() && (doScaling() || doApplyCutoff());};

    protected:
        /// @brief Find the overall weights normalisation (the maximum value across the weights image).
        void findNorm();

        /// @brief Read the weights values for the given subsection
        void readWeights();

        /// @brief ASKAP communicator
        askap::askapparallel::AskapParallel *itsComms;

        /// @brief The weights image
        std::string itsImage;

        /// @brief The Cube under examination
        duchamp::Cube *itsCube;

        /// @brief The normalisation of the weights image
        float itsNorm;

        /// @brief The value of weights below which we reject pixels
        float itsWeightCutoff;

        /// @brief How the cutoff is applied: either "zero" (rejected
        /// pixels are set to zero) or "blank" (set to the BLANK value).
        std::string itsCutoffType;

        /// @brief Whether to scale the pixel values by the weights
        bool itsFlagDoScaling;

        /// @brief Array of weights values - able to have a mask.
        casa::MaskedArray<casa::Float> itsWeights;

};

}
}


#endif

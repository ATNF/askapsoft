/// @file
///
/// Handle the parameterisation of objects that require reading from a file on disk
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
#ifndef ASKAP_DISTRIB_RMSYNTH_H_
#define ASKAP_DISTRIB_RMSYNTH_H_

#include <parallelanalysis/DistributedParameteriserBase.h>
#include <askapparallel/AskapParallel.h>
#include <catalogues/CasdaIsland.h>
#include <catalogues/CasdaComponent.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <vector>

namespace askap {

namespace analysis {

/// @brief Distributed handling of the RM Synthesis.  @details This
/// distributes a list of RadioSource objects - that have had the
/// Gaussian fitting done to them - from the master to the workers, in
/// a round-robin fashion. The workers then do the RM Synthesis and
/// related processing on their local list of objects, and then return
/// the list of Polarisation catalogue entries to the master. The
/// master can then access this for writing out.
class DistributedContinuumParameterisation : public DistributedParameteriserBase {
    public:
        DistributedContinuumParameterisation(askap::askapparallel::AskapParallel& comms,
                                             const LOFAR::ParameterSet &parset,
                                             std::vector<sourcefitting::RadioSource> sourcelist);
        virtual ~DistributedContinuumParameterisation();

        /// @brief Each object on a worker is parameterised, and
        /// fitted (if requested).
        void parameterise();

    /// @brief Add the given Gaussian component to the component image, pixel-by-pixel.
    void addToComponentImage(casa::Gaussian2D<Double> &gauss);

        /// @brief The workers' objects are returned to the master
        void gather();

        /// @brief The final list of islands is returned
        const std::vector<CasdaIsland> finalIslandList() {return itsIslandList;};
        /// @brief The final list of components is returned
        const std::vector<CasdaComponent> finalComponentList() {return itsComponentList;};

    /// @brief Return the array of imaged components
    const casa::Array<float> componentImage() {return itsComponentImage;};

    /// @brief Return the slicer applied to the input image
    const casa::Slicer inputSlicer() {return itsInputSlicer;};

    protected:

        /// @brief The list of polarisation catalogue entries
        std::vector<CasdaIsland> itsIslandList;

        /// @brief The list of continuum components. Only used by the master as a check that we have the correct number at the end
        std::vector<CasdaComponent> itsComponentList;

    /// @brief The shape of the input image as used
    casa::Slicer itsInputSlicer;
    
    /// @brief The array showing imaged components
    casa::Array<float> itsComponentImage;

    
};

}

}


#endif

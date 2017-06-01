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
#ifndef ASKAP_DISTRIB_FITTER_H_
#define ASKAP_DISTRIB_FITTER_H_

#include <parallelanalysis/DistributedParameteriserBase.h>
#include <askapparallel/AskapParallel.h>
#include <parallelanalysis/DuchampParallel.h>
#include <sourcefitting/RadioSource.h>
#include <vector>

namespace askap {

namespace analysis {

/// @brief Distributed handling of the Gaussian fitting.  @details
/// This distributes a list of RadioSource objects from the master to
/// the workers, in a round-robin fashion. The workers then do the
/// Gaussian fitting on their local list of objects, and then return
/// them to the master. The master then has the full list with fitted
/// Gaussians added.
class DistributedFitter : public DistributedParameteriserBase {
    public:
        DistributedFitter(askap::askapparallel::AskapParallel& comms,
                          const LOFAR::ParameterSet &parset,
                          std::vector<sourcefitting::RadioSource> sourcelist);
        ~DistributedFitter();

        /// @brief Each object on a worker is parameterised, and
        /// fitted (if requested).
        void parameterise();

        /// @brief The workers' objects are returned to the master
        void gather();

        /// @brief The final list of objects is returned
        const std::vector<sourcefitting::RadioSource> finalList() {return itsOutputList;};

    protected:

        /// The list of parameterised objects.
        std::vector<sourcefitting::RadioSource> itsOutputList;

        /// The image header information. The WCS is the key element
        /// used in this.
        duchamp::FitsHeader itsHeader;

        /// The set of Duchamp parameters. The subsection and offsets
        /// are the key elements here.
        duchamp::Param itsReferenceParams;

};

}

}


#endif

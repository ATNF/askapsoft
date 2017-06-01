/// @file
///
/// Handle the parameterisation of objects that require reading from a file on disk
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
#ifndef ASKAP_DISTRIB_HIEMISSION_H_
#define ASKAP_DISTRIB_HIEMISSION_H_

#include <parallelanalysis/DistributedParameteriserBase.h>
#include <askapparallel/AskapParallel.h>
#include <catalogues/CasdaHiEmissionObject.h>
#include <vector>

namespace askap {

namespace analysis {

/// @brief Distributed handling of HI emission-line catalogues.
/// @details This distributes a list of RadioSource objects from the
/// master to the workers, in a round-robin fashion. The workers then
/// create the HI emission catalogue and related processing on their
/// local list of objects, and then return the list of HI catalogue
/// entries to the master. The master can then access this for writing
/// out.
class DistributedHIemission : public DistributedParameteriserBase {
    public:
        DistributedHIemission(askap::askapparallel::AskapParallel& comms,
                              const LOFAR::ParameterSet &parset,
                              std::vector<sourcefitting::RadioSource> sourcelist);
        virtual ~DistributedHIemission();

        /// @brief Each object on a worker is parameterised, and
        /// fitted (if requested).
        void parameterise();

        /// @brief The workers' objects are returned to the master
        void gather();

        /// @brief The final list of objects is returned
        const std::vector<CasdaHiEmissionObject> finalList() {return itsOutputList;};

    protected:

        /// The list of polarisation catalogue entries
        std::vector<CasdaHiEmissionObject> itsOutputList;

};

}

}


#endif

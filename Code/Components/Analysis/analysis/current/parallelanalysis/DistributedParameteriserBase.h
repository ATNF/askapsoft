/// @file
///
/// Base class for distributed parameterisation
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
#ifndef ASKAP_DISTRIB_PARAMER_BASE_H_
#define ASKAP_DISTRIB_PARAMER_BASE_H_

#include <askapparallel/AskapParallel.h>
#include <parallelanalysis/DuchampParallel.h>
#include <vector>
#include <boost/shared_ptr.hpp>

namespace askap {

namespace analysis {

/// @brief Base class to handle distributed parameterisation of a list
/// of RadioSource objects.  @details This provides the basic
/// mechanisms for distributing a list of RadioSource objects from the
/// master to a set of workers. The objects are distributed in a
/// round-robin fashion until the list is exhausted. The class also
/// provides for parameterise() and gather() functions that are not
/// defined in the base class. The output of these are left up to the
/// derived classes, as it will be different types of objects that get
/// returned.
class DistributedParameteriserBase {
    public:
        DistributedParameteriserBase(askap::askapparallel::AskapParallel& comms,
                                     const LOFAR::ParameterSet &parset,
                                     std::vector<sourcefitting::RadioSource> sourcelist);
        virtual ~DistributedParameteriserBase();

        /// @brief Master sends list to workers, who fill out
        /// itsInputList
        virtual void distribute();

        /// @brief Each object on a worker is parameterised, and
        /// fitted (if requested).
        virtual void parameterise() = 0;

        /// @brief The workers' objects are returned to the master
        virtual void gather() = 0;

    protected:

        /// The communication class
        askap::askapparallel::AskapParallel *itsComms;

        /// The input parset. Used for fitting purposes.
        LOFAR::ParameterSet itsReferenceParset;

        /// The initial set of objects, before parameterisation
        std::vector<sourcefitting::RadioSource> itsInputList;

        /// The total number of objects that are to be parameterised.
        unsigned int itsTotalListSize;

        boost::shared_ptr<DuchampParallel> itsDP;

/// The reference Duchamp cube
        duchamp::Cube *itsCube;

};

}

}


#endif


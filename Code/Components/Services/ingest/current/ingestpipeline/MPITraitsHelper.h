/// @file MPITraitsHelper.h
///
/// This templated class simplifies writing MPI communication routines for
/// complex templates such as casa arrays.
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#ifndef ASKAP_CP_INGEST_MPITRAITSHELPER_H
#define ASKAP_CP_INGEST_MPITRAITSHELPER_H

// this is an mpi-specific code
#include <mpi.h>

// boost includes
#include "boost/static_assert.hpp"

// casacore includes
#include "casacore/scimath/Mathematics/RigidVector.h"
#include "casacore/casa/BasicSL/Complex.h"


namespace askap {
namespace cp {
namespace ingest {

// helper class to encapsulate mpi-related stuff 
template<typename T>
struct MPITraitsHelper {
   BOOST_STATIC_ASSERT_MSG(sizeof(T) == 0, 
          "Attempted a build for type without traits defined");
};

// specialisation for casacore::uInt
template<>
struct MPITraitsHelper<casacore::uInt> {
   static MPI_Datatype datatype() { return MPI_UNSIGNED; };
   static const int size = 1;
   
   static bool equal(casacore::uInt val1, casacore::uInt val2) { return val1 == val2;}
};


// specialisation for casacore::Float
template<>
struct MPITraitsHelper<casacore::Float> {
   static MPI_Datatype datatype() { return MPI_FLOAT; };
   static const int size = 1;

   static bool equal(casacore::Float val1, casacore::Float val2) { return fabsf(val1 - val2) <= 1e-7 * fabsf(val1 + val2);}
};

// specialisation for casacore::Complex
template<>
struct MPITraitsHelper<casacore::Complex> {
   static MPI_Datatype datatype() { return MPI_FLOAT; };
   static const int size = 2;
};

// specialisation for casacore::Bool
template<>
struct MPITraitsHelper<casacore::Bool> {
   static MPI_Datatype datatype() { return MPI_CHAR; };
   static const int size = 1;

   static bool equal(casacore::Bool val1, casacore::Bool val2) { return val1 == val2;}
};

// specialisation for casacore::Double
template<>
struct MPITraitsHelper<casacore::Double> {
   static MPI_Datatype datatype() { return MPI_DOUBLE; };
   static const int size = 1;

   static bool equal(casacore::Double val1, casacore::Double val2) { return fabs(val1 - val2) <= 1e-13 * fabs(val1 + val2);}
};

// specialisation for casacore::RidigVector<casacore::Double, 3>
template<>
struct MPITraitsHelper<casacore::RigidVector<casacore::Double, 3> > {
   static MPI_Datatype datatype() { return MPI_DOUBLE; };
   static const int size = 3;

   // skip comparison - we don't really need it in the cross-check code
   static bool equal(casacore::RigidVector<casacore::Double, 3>, casacore::RigidVector<casacore::Double, 3>) { return true;}
};

// specialisation for unsigned long
template<>
struct MPITraitsHelper<unsigned long> {
   static MPI_Datatype datatype() { return MPI_UNSIGNED_LONG; };
   static const int size = 1;
};

// specialisation for int8_t
template<>
struct MPITraitsHelper<int8_t> {
   static MPI_Datatype datatype() { return MPI_BYTE; };
   static const int size = 1;
};

}
}
}

#endif

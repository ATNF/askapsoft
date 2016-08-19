/// @file CorrBufferUnit.h
///
/// @copyright (c) 2015 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>
///
/// Buffer unit that provides convenient intermediate data format during
/// conversion from measurement set to datagram

#ifndef ASKAP_CP_CORRBUFFERUNIT_H
#define ASKAP_CP_CORRBUFFERUNIT_H 

#include <stdint.h>
#include "cpcommon/FloatComplex.h"

namespace askap {
namespace cp {

#define VERBOSE
//#define NEW_BUFFER


#ifdef NEW_BUFFER

// Enumerated constants for data status.
// UNKNOWN: the data status is unknown
// EMPTY  : no data inside
// FULL   : data inside
enum CorrBufferUnitStatus { UNKNOWN = 0, EMPTY = 1, FULL = 2 };

#endif



class CorrBufferUnit
{
    public :

        /// Constructor
        CorrBufferUnit();

        /// Destructor
        virtual ~CorrBufferUnit();

		/// Initialize the data (also functions as delete)
		void init();

		/// Insert the data into the slot (works only if the slot is empty)
		void insert(const FloatComplex& visIn);
		void insert(const float& realIn, const float& imagIn);
		void insert(const CorrBufferUnit& bufferUnitIn);

		/// Return the data
		FloatComplex query() const;

#ifdef NEW_BUFFER

		/// Return the data status
		uint32_t queryStatus() const;

        /// Return true if it contains data
        bool isFull() const;

        /// Return true if it contains no data
        bool isEmpty() const;

#endif

		/// Remove the data from the slot, the data is returned
		//FloatComplex remove();

        /// Print out data
        void print() const;

		/// Return true if the data is the same
		bool isSame(const CorrBufferUnit& unit2, const float small) const;

		// TO DO: make the data structure private 

        /// Visibility data
        FloatComplex vis;

#ifdef NEW_BUFFER

		// Data status, as in CorrBufferUnitStatus
		uint32_t status;

#endif

};

};
};

#endif

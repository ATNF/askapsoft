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

class CorrBufferUnit
{
    public :

        /// Constructor
        CorrBufferUnit ();

        /// Destructor
        virtual ~CorrBufferUnit ();

        /// Print out data
        void print ();


        // data
        
        FloatComplex vis;

        // True if the data is ready to use
        bool ready;
};

};
};

#endif

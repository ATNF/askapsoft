/// @file CubeComms.cc
///
/// Class to provide extra MPI communicator functionality to manage the writing
/// of distributed spectral cubes.
///
/// @copyright (c) 2016 CSIRO
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
/// @author Stephen Ord <Stephen.Ord@csiro.au>
///

/// out own header first
#include "distributedimager/CubeComms.h"

///ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>


ASKAP_LOGGER(logger, ".CubeComms");

using namespace askap;
using namespace askap::cp;

CubeComms::CubeComms(int argc, const char** argv) : AskapParallel(argc, const_cast<const char **>(argv))
    {
    ASKAPLOG_DEBUG_STR(logger,"Constructor");
}
CubeComms::~CubeComms() {
    ASKAPLOG_DEBUG_STR(logger,"Destructor");
}
bool CubeComms::isWriter() {
    ASKAPLOG_DEBUG_STR(logger,"Providing writer status");
    return true;
}

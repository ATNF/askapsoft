/// @file ReductionLogic.cc
///
/// @copyright (c) 2009 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "ReductionLogic.h"

// System includes
#include <cmath>

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

// Using
using namespace askap::cp;

ASKAP_LOGGER(logger, ".ReductionLogic");

ReductionLogic::ReductionLogic(int id, int numNodes)
    : itsId(id), itsNumNodes(numNodes)
{
}

ReductionLogic::~ReductionLogic()
{
}

int ReductionLogic::responsible(void)
{
    const int accumulatorStep = getAccumulatorStep();
    int responsible = 0;

    if (itsId == 0) {
        // Master
        if (itsNumNodes <= accumulatorStep) {
            responsible = itsNumNodes - 1;
        } else {
            responsible += accumulatorStep - 1; // First n workers
            float accumulators = ceil((float)itsNumNodes / (float)accumulatorStep) - 1.0;
            responsible += static_cast<int>(accumulators); // Accumulators 
        }
    } else if (itsId % accumulatorStep == 0) {
        // Accumulator + worker
        if ((itsId + accumulatorStep) > itsNumNodes) {
            responsible = itsNumNodes - itsId - 1;
        } else {
            responsible = accumulatorStep - 1;
        }

    } else {
        // If execution got here, the process is just a worker and is only
        // responsible for itself
        responsible = 0;
    }

    return responsible;
}

int ReductionLogic::getAccumulatorStep(void)
{
    if (itsNumNodes <= 16) {
        return 4;
    } else {
        return static_cast<int>(ceil(sqrt(itsNumNodes)));
    }
}

/// @file tMSfieldAccess.cc
///
/// @copyright (c) 2019 CSIRO
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

// System includes
#include <iostream>
#include <string>

// ASKAPsoft includes
#include "askap/Application.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"

#include "dataaccess/FeedSubtableHandler.h"

#include "casacore/casa/Quanta/MVTime.h"
#include "casacore/casa/Quanta/Quantum.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/measures/Measures/MEpoch.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"

// Using
using namespace askap;
using namespace casa;

//ASKAP_LOGGER(logger, ".tMSfieldAccess");


int main(int argc, char *argv[])
{
    if (argc < 2){
        return 1;
    }
    std::string filename(argv[1]);
            
    casa::MeasurementSet ms(filename, casa::Table::Old);
    ROMSColumns msc(ms);

    // Extract observation start and stop time
    const casa::Int obsId = msc.observationId()(0);
    const ROMSObservationColumns& obsc = msc.observation();
    const casa::Vector<casa::MEpoch> timeRange = obsc.timeRangeMeas()(obsId);
    casa::MEpoch itsObsStart = timeRange(0);
    casa::MEpoch itsObsEnd = timeRange(1);
            
    const ROMSFieldColumns& fieldc = msc.field();

    accessors::FeedSubtableHandler fsh(ms);


    TableColumn feedcol(ms, "FEED1");
    int feed = feedcol.asInt(0);
    std::cout << filename << " (obsID = " << obsId << ") is beam " << feed <<"\n";

    // Iterate over all rows, creating a ScanElement for each scan
    casa::Int lastScanId = -1;
    casa::uInt row = 0;
    while (row < msc.nrow()) {
        const casa::Int scanNum = msc.scanNumber()(row);
        if (scanNum > lastScanId) {
            lastScanId = scanNum;
            // 1: Collect scan metadata that is expected to remain constant for the whole scan
            const casa::MEpoch startTime = msc.timeMeas()(row);
                    
            // Field
            const casa::Int fieldId = msc.fieldId()(row);
            const casa::Vector<MDirection> dirVec = fieldc.phaseDirMeasCol()(fieldId);
            const MDirection fieldDirection = dirVec(0);
            const std::string fieldName = fieldc.name()(fieldId);

            const casa::Vector<casa::RigidVector<casa::Double, 2> > offsets = fsh.getAllBeamOffsets(startTime,0);

            MDirection beamDir = fieldDirection;
            beamDir.shift(-offsets[feed](0),offsets[feed](1),casa::True);
            std::cout << "Direction = " << printDirection(beamDir.getAngle("deg")) << "\n";
        } else {
            ++row;
        }
    }

    return 0;
}

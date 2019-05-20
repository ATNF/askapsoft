/// @file MonitoringPointManager.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "MonitoringPointManager.h"

// System includes
#include <string>

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "cpcommon/VisChunk.h"
#include "monitoring/MonitoringSingleton.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/MeasTable.h"

using namespace askap::cp::ingest;

MonitoringPointManager::MonitoringPointManager()
{
}

MonitoringPointManager::~MonitoringPointManager()
{
    submitPointNull("obs.ScanId");
    submitPointNull("obs.FieldName");
    submitPointNull("obs.dir1");
    submitPointNull("obs.dir2");
    submitPointNull("obs.CoordSys");
    submitPointNull("obs.Interval");
    submitPointNull("obs.SourceStartFreq");
    submitPointNull("obs.SourceNChan");
    submitPointNull("obs.SourceChanWidth");
    submitPointNull("obs.DataRate");

    submitPointNull("PacketsLostCount");
    submitPointNull("PacketsLostPercent");

    submitPointNull("VisCornerTurnDuration");

    submitPointNull("PacketsBuffered");
    submitPointNull("BufferUsagePercent");

    submitPointNull("dUTC");
    submitPointNull("dUT1");

    submitPointNull("MisalignedStreamsCount");
    submitPointNull("MisalignedStreamsPercent");
    submitPointNull("MSWritingDuration");
    submitPointNull("nFeedTableTimeRanges");
}

void MonitoringPointManager::submitMonitoringPoints(const askap::cp::common::VisChunk& chunk) const
{
    const casacore::MDirection target = chunk.targetPointingCentre()[0];
    submitPoint<int32_t>("obs.ScanId", chunk.scan());
    submitPoint<string>("obs.FieldName", chunk.targetName());
    submitPoint<string>("obs.dir1", askap::printLon(target));
    submitPoint<string>("obs.dir2", askap::printLat(target));
    submitPoint<string>("obs.CoordSys", casacore::MDirection::showType(target.type()));
    submitPoint<int32_t>("obs.Interval", chunk.interval() * 1000);
    submitPoint<float>("obs.SourceStartFreq", chunk.frequency()[0]/ 1000 / 1000);
    submitPoint<int32_t>("obs.SourceNChan", chunk.nChannel());
    submitPoint<float>("obs.SourceChanWidth", chunk.channelWidth() / 1000);
    
    /*
    // we can add another monitoring point if desired, but obs.DataRate
    // has been moved to sink as it is more appropriate for the full MPI case
    if (chunk.interval() > 0) {
        const float nVis = chunk.nChannel() * chunk.nRow() * chunk.nPol();
        // data estimated as 8byte vis, 4byte sigma + nRow * 100 bytes (mdata)
        const float nDataInMB = (12. * nVis + 100. * chunk.nRow()) / 1048576.;
        submitPoint<float>("obs.DataRate", nDataInMB / chunk.interval());
    } else {
       submitPointNull("obs.DataRate");
    }   
    */

    // casa measures "constants"
    const double mjd = chunk.time().get(); // current mjd
    submitPoint<float>("dUTC", casacore::MeasTable::dUTC(mjd));
    submitPoint<float>("dUT1", casacore::MeasTable::dUT1(mjd));
}

void MonitoringPointManager::submitPointNull(const std::string& key) const
{
    MonitoringSingleton::invalidatePoint(key);
}

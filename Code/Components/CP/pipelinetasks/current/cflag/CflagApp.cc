/// @file CflagApp.cc
///
/// @copyright (c) 2012-2014 CSIRO
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
#include "cflag/CflagApp.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>
#include <iomanip>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "Common/ParameterSet.h"
#include "askap/StatReporter.h"
#include "casacore/casa/aipstype.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"

// Local package includes
#include "cflag/FlaggerFactory.h"
#include "cflag/IFlagger.h"
#include "cflag/FlaggingStats.h"
#include "cflag/MSFlaggingSummary.h"

#include "casacore/tables/DataMan/TiledStManAccessor.h"

// Using
using namespace std;
using namespace askap;
using namespace askap::cp::pipelinetasks;
using namespace casa;

ASKAP_LOGGER(logger, ".CflagApp");

int CflagApp::run(int argc, char* argv[])
{
    StatReporter stats;
    const LOFAR::ParameterSet subset = config().makeSubset("Cflag.");

    // Open the measurement set
    const std::string dataset = subset.getString("dataset");
    ASKAPLOG_INFO_STR(logger, "Opening Measurement Set: " << dataset);
    casacore::MeasurementSet ms(dataset, casacore::Table::Update);
    MSColumns msc(ms);
    IPosition tileShape(3,0,0,0);

    // If data is tiled and is uniform in shape we can process in tiles
    TableDesc td = ms.actualTableDesc();
    const ColumnDesc& cdesc = td[msc.data().columnDesc().name()];
    String dataManType = cdesc.dataManagerType();
    String dataManGroup = cdesc.dataManagerGroup();
    Bool tiled = (dataManType.contains("Tiled"));
    if (tiled && msc.dataDescription().nrow() == 1) {
        ROTiledStManAccessor tsm(ms, dataManGroup);
        // Find the biggest tile, use it if it has 3 dimensions
        uInt nHyper = tsm.nhypercubes();
        uInt maxIndex = 0;
        uInt maxTile = 0;
        for (uInt i=0; i< nHyper; i++) {
            uInt product = tsm.getTileShape(i).product();
            if (product > maxTile) {
                maxIndex = i;
                maxTile = product;
            }
        }
        if (tsm.getTileShape(maxIndex).nelements() == 3)
            tileShape = tsm.getTileShape(maxIndex);
        // cout << "Input tile shape = " << tileShape <<  endl;
    }

    // Create a vector of all the flagging strategies specified in the parset
    std::vector< boost::shared_ptr<IFlagger> > flaggers = FlaggerFactory::build(subset, ms);
    if (flaggers.empty()) {
        ASKAPLOG_ERROR_STR(logger, "No flaggers configured - Aborting");
        return 1;
    }

    // Print a summary if needed
    if (subset.getBool("summary", true)) {
        MSFlaggingSummary::printToLog(msc);
    }

    // Is this a dry run?
    const bool dryRun = subset.getBool("dryrun", false);
    if (dryRun) {
        ASKAPLOG_INFO_STR(logger, "!!!!! DRY RUN ONLY - MeasurementSet will not be updated !!!!!");
    }

    // Iterate over each row in the main table
    const casacore::uInt nRows = msc.nrow();
    std::vector< boost::shared_ptr<IFlagger> >::iterator it;
    unsigned long rowsAlreadyFlagged = 0;
    casacore::Bool passRequired = casacore::True;
    casacore::uInt pass = 0;
    casacore::uInt step = 1;
    if (tileShape(2) > 1) step = tileShape(2);
    while (passRequired) {
        if (step > 1) {
            // Read data in whole tiles
            for (casacore::uInt i = 0; i < nRows; i+=step) {
                casacore::uInt rowsToProcess = min(step, nRows - i);
                // count flagged rows
                casacore::uInt flagged = 0;
                for (casacore::uInt j = i; j < i+rowsToProcess ; j++) {
                    if (msc.flagRow()(j)) flagged++;
                }
                rowsAlreadyFlagged += flagged;
                // If there are unflagged rows, do more flagging
                if (flagged < rowsToProcess) {
                    // Invoke each flagger
                    for (it = flaggers.begin(); it != flaggers.end(); ++it) {
                        if ((*it)->processingRequired(pass)) {
                            (*it)->processRows(msc, pass, i, rowsToProcess, dryRun);
                        }
                    }
                }
            }

        } else{
            for (casacore::uInt i = 0; i < nRows; ++i) {
                if (!msc.flagRow()(i)) {
                    // Invoke each flagger for this row, but only while the row isn't flagged
                    for (it = flaggers.begin(); it != flaggers.end(); ++it) {
                        if (msc.flagRow()(i)) {
                            break;
                        }
                        if ((*it)->processingRequired(pass)) {
                            (*it)->processRow(msc, pass, i, dryRun);
                        }
                    }
                } else {
                    rowsAlreadyFlagged++;
                }
            }
        }
        pass++;
        passRequired = casacore::False;
        for (it = flaggers.begin(); it != flaggers.end(); ++it) {
            if ((*it)->processingRequired(pass)) {
                passRequired = casacore::True;
            }
        }
    }

    // Write out flagging statistics
    ASKAPLOG_INFO_STR(logger, "Summary:");
    float rowPercent = static_cast<float>(rowsAlreadyFlagged) / nRows * 100.0;
    ASKAPLOG_INFO_STR(logger, "  Rows already flagged: " << rowsAlreadyFlagged
            << " (" << setprecision(2) << rowPercent << "%)");
    for (it = flaggers.begin(); it != flaggers.end(); ++it) {
        const FlaggingStats stats = (*it)->stats();
        rowPercent = static_cast<float>(stats.rowsFlagged) / nRows * 100.0;
        ASKAPDEBUGASSERT(rowPercent <= 100.0);
        ASKAPLOG_INFO_STR(logger, "  " << stats.name
                              << " - Entire rows flagged: " << stats.rowsFlagged
                              << " (" << setprecision(2) << rowPercent << "%)"
                              << ", Visibilities flagged: " << stats.visFlagged);
    }

    stats.logSummary();
    //RODataManAccessor(ms, "TiledData", False).showCacheStatistics (cout);

    return 0;
}

/// @file cpingest.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Must be included first
#include "askap_cpingest.h"

// System includes
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <mpi.h>

// ASKAPsoft includes
#include "cpcommon/ParallelCPApplication.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "askap/StatReporter.h"

// Local package includes
#include "ingestpipeline/IngestPipeline.h"

// Using
using namespace askap;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, ".main");

class CpIngestApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run()
   {
       StatReporter stats;

       // Run the pipeline
       IngestPipeline pipeline(config(), rank(), numProcs(), nodeName());
       pipeline.start();

       stats.logSummary();
   }
};

int main(int argc, char *argv[])
{
    CpIngestApp app;
    app.addParameter("standalone", "s", "Run in standalone/single-process mode (no MPI)", false);
    return app.main(argc, argv);
}

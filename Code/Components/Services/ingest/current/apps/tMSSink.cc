/// @file tMSSink.cc
/// @details
///   This application runs MSSink task with mock up data. This is handy
///   for performance testing. 
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

// System includes
#include <iostream>
#include <iomanip>
#include <string>

// casa
#include "casacore/casa/OS/Timer.h"
 
// ASKAPsoft includes
#include "cpcommon/ParallelCPApplication.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"
#include "cpcommon/VisDatagram.h"

// Local package includes
#include "ingestpipeline/mssink/MSSink.h"
#include "ingestpipeline/sourcetask/VisConverterADE.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too
#include "cpcommon/VisChunk.h"


// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, "tMSSink");

class TestParallelMetadataSnoopApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run() {
      //ASKAPCHECK(!isStandAlone() && numProcs() > 1, "This test application is specific to parallel multi-rank case and can't be used in stand-alone mode");
      int count = config().getInt32("count", 10);
      ASKAPCHECK(count > 0, "Expect positive number of timestamps to receive, you have = "<<count);
      uint32_t expectedCount = static_cast<uint32_t>(count);
    
      ASKAPLOG_INFO_STR(logger, "Setting up mock up data structure for rank="<<rank());
      Configuration cfg(config(), rank(), numProcs());
      VisConverter<VisDatagramADE> conv(config(), cfg, rank());
      conv.initVisChunk(4976749386006000ul, cfg.lookupCorrelatorMode("standard"));
      boost::shared_ptr<common::VisChunk> chunk = conv.visChunk();
      ASKAPASSERT(chunk);
      casa::Timer timer;
      float processingTime = 0.;
      size_t actualCount = 0;

      ASKAPLOG_INFO_STR(logger, "Initialising MSSink constructor for rank="<<rank());
      
      timer.mark();
      MSSink sink(config(), cfg);
      const float initTime = timer.real();
      ASKAPLOG_INFO_STR(logger, "MSSink initialisation time: "<<initTime<<" seconds");
      
      ASKAPLOG_INFO_STR(logger, "Running the test for rank="<<rank());

      for (uint32_t count = 0; count < expectedCount; ++count) {
           ASKAPLOG_INFO_STR(logger, "Received "<<count + 1<<" integration(s) for rank="<<rank());
           timer.mark();
           sink.process(chunk);
           const float runTime = timer.real();
           ASKAPLOG_INFO_STR(logger, "   - mssink took "<<runTime<<" seconds");
           processingTime += runTime;
           ++actualCount;
           chunk->time() += casa::Quantity(5.,"s");
      }
      if (actualCount > 0) {
          ASKAPLOG_INFO_STR(logger, "Average running time per cycle: "<<processingTime / actualCount<<
                     " seconds, "<<actualCount<<" iteratons averaged");
      }
   }
private:

};

int main(int argc, char *argv[])
{
    TestParallelMetadataSnoopApp app;
    return app.main(argc, argv);
}

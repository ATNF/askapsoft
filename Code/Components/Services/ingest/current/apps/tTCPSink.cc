/// @file tMSSink.cc
/// @details
///   This application runs TCPSink task with mock up data. This is handy
///   for performance testing and vis/spd debugging.
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
#include "casacore/casa/Quanta/MVTime.h"
#include "casacore/casa/Quanta/MVEpoch.h"
 
// ASKAPsoft includes
#include "cpcommon/ParallelCPApplication.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"
#include "cpcommon/VisDatagram.h"
#include "utils/ComplexGaussianNoise.h"

// Local package includes
#include "ingestpipeline/tcpsink/TCPSink.h"
#include "ingestpipeline/sourcetask/VisConverterADE.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too
#include "cpcommon/VisChunk.h"


// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, "tMSSink");

class TCPSinkTestApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run() {
      //ASKAPCHECK(!isStandAlone() && numProcs() > 1, "This test application is specific to parallel multi-rank case and can't be used in stand-alone mode");
      int count = config().getInt32("count", 0);
      ASKAPCHECK(count >= 0, "Expect non-negative number of timestamps to receive, you have = "<<count);
      uint32_t expectedCount = static_cast<uint32_t>(count);
      if (expectedCount == 0) {
          ASKAPLOG_INFO_STR(logger, "Running cycles indefinitely, use Ctrl+C to interrupt");
      }
    
      ASKAPLOG_INFO_STR(logger, "Setting up mock up data structure for rank="<<rank());
      Configuration cfg(config(), rank(), numProcs());
      VisConverter<VisDatagramADE> conv(config(), cfg);
      const CorrelatorMode &corrMode = cfg.lookupCorrelatorMode("standard");
      conv.initVisChunk(4976749386006000ul, corrMode);
      const float corrInterval = static_cast<float>(corrMode.interval()) / 1e6; // in seconds
      boost::shared_ptr<common::VisChunk> chunk = conv.visChunk();
      ASKAPASSERT(chunk);
      chunk->flag().set(false);
      for (casa::uInt chan = 0; chan < chunk->nChannel(); ++chan) {
           chunk->frequency()[chan] = 1e9 + 1e6/54.*double(chan);
      }
      casa::Timer timer;
      float processingTime = 0.;
      size_t actualCount = 0;

      ASKAPLOG_INFO_STR(logger, "Initialising TCPSink constructor for rank="<<rank());
      
      timer.mark();
      TCPSink sink(config(), cfg);
      const float initTime = timer.real();
      ASKAPLOG_INFO_STR(logger, "TCPSink initialisation time: "<<initTime<<" seconds");

      scimath::ComplexGaussianNoise cns(casa::square(config().getFloat("rms",1.)));
      
      ASKAPLOG_INFO_STR(logger, "Running the test for rank="<<rank());

      for (uint32_t count = 0; (count < expectedCount) || !expectedCount; ++count) {
           // prepare the integration
           timer.mark();
           casa::Quantity q;
           ASKAPCHECK(casa::MVTime::read(q, "today"), "MVTime::read failed");
           chunk->time() = casa::MVEpoch(q);
           ASKAPDEBUGASSERT(chunk->visibility().contiguousStorage());
           casa::Complex *data =chunk->visibility().data();
           casa::Complex *endData = data + chunk->visibility().nelements();
           for (; data != endData; ++data) {
                *data = cns();
           }
           float generationTime = timer.real();
           //
           ASKAPLOG_INFO_STR(logger, "Received "<<count + 1<<" integration(s) for rank="<<rank()<<" time: "<<chunk->time());
           timer.mark();
           sink.process(chunk);
           ASKAPCHECK(chunk, "TCP Sink is not supposed to change data distribution pattern");
           const float runTime = timer.real();
           ASKAPLOG_INFO_STR(logger, "   - tcpsink took "<<runTime<<" seconds, generation of visibilities took "<<generationTime<<" seconds");
           generationTime += runTime;
           processingTime += runTime;
           ++actualCount;
           if (generationTime < corrInterval) {
               sleep(corrInterval - generationTime);
           } else {
               ASKAPLOG_INFO_STR(logger, "Not keeping up! overheads = "<<generationTime<<" seconds, interval = "<<corrInterval<<" seconds");
           }
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
    TCPSinkTestApp app;
    return app.main(argc, argv);
}

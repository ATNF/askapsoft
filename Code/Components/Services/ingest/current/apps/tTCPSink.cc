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
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "boost/shared_ptr.hpp"
#include <boost/thread/thread.hpp>
#include "cpcommon/VisDatagram.h"
#include "utils/ComplexGaussianNoise.h"

// Local package includes
#include "ingestpipeline/tcpsink/TCPSink.h"
#include "ingestpipeline/sourcetask/VisConverterADE.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too
#include "cpcommon/VisChunk.h"
#include "askap/askap/AskapUtil.h"

// I am not very happy to have MPI includes here, we may abstract this interaction
// eventually. This task is specific for the parallel case, so there is no reason to
// hide MPI for now.
#include <mpi.h>


// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, "tMSSink");

class ParallelGenerator {
public:
   explicit ParallelGenerator(float rms, unsigned int nThreads = 1, int seed = 0) : itsRMS(rms), itsNThreads(nThreads), itsSeed(seed) {}
   void generate(boost::shared_ptr<casacore::Complex> &data, casacore::uInt size) const {
        ASKAPASSERT(itsNThreads > 0);
        const casacore::uInt chunkSize = size / itsNThreads;
        casacore::uInt offset = 0;
        for (casacore::uInt part = 1; part < itsNThreads; ++part, ++itsSeed) {
             boost::shared_ptr<casacore::Complex> start(data.get() + offset, utility::NullDeleter());
             itsGroup.create_thread(boost::bind(&ParallelGenerator::generatePart, this, start, chunkSize, itsSeed));
             offset += chunkSize;
        }
        // process the remaining data in the main thread
        ASKAPDEBUGASSERT(offset < size);
        const casacore::uInt remainder  = size - offset;
        ASKAPDEBUGASSERT(remainder >= chunkSize);
        boost::shared_ptr<casacore::Complex> start(data.get() + offset, utility::NullDeleter());
        generatePart(start, remainder);
        itsGroup.join_all();
   }

private:

   void generatePart(boost::shared_ptr<casacore::Complex> &data, casacore::uInt size, casacore::Int seed1 = 0) const {
      scimath::ComplexGaussianNoise cns(casacore::square(itsRMS),seed1);
      casacore::Complex *rawPtr = data.get();
      casacore::Complex *endMark = rawPtr + size;
      for (; rawPtr != endMark; ++rawPtr) {
           *rawPtr = cns();
      }
   }
   
   /// @brief rms of the simulated gaussian numbers
   const float  itsRMS;  
   /// @brief number of parallel threads
   const unsigned int itsNThreads;

   mutable boost::thread_group itsGroup;
   
   mutable casacore::Int itsSeed;
};

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
      for (casacore::uInt chan = 0; chan < chunk->nChannel(); ++chan) {
           chunk->frequency()[chan] = 1e9 + 1e6/54.*double(chan);
      }
      chunk->scan() = 0;
      if (numProcs() > 1) {
          // patch beam IDs in the chunk to ensure different ranks have different beams
          const casacore::uInt maxBeams = config().getUint("maxbeams");
          const int beamStep = maxBeams * rank();
          ASKAPLOG_INFO_STR(logger, "Adding "<<beamStep<<" to beam indices simulated by this rank");
          casacore::Vector<casacore::uInt> &beam1 = chunk->beam1();
          casacore::Vector<casacore::uInt> &beam2 = chunk->beam2();
          ASKAPDEBUGASSERT(chunk->nRow() == chunk->beam1().nelements());
          ASKAPDEBUGASSERT(chunk->nRow() == chunk->beam2().nelements());
          for (casacore::uInt row=0; row<beam1.nelements(); ++row) {
               beam1[row] += beamStep;
               beam2[row] += beamStep;
          }
      }
      casacore::Timer timer;
      float processingTime = 0.;
      size_t actualCount = 0;

      ASKAPLOG_INFO_STR(logger, "Initialising TCPSink constructor for rank="<<rank());
      
      timer.mark();
      TCPSink sink(config(), cfg);
      const float initTime = timer.real();
      ASKAPLOG_INFO_STR(logger, "TCPSink initialisation time: "<<initTime<<" seconds");

      scimath::ComplexGaussianNoise cns(casacore::square(config().getFloat("rms",1.)));
      const casacore::uInt nThreads = config().getUint("nthreads",10);
      ParallelGenerator pg(config().getFloat("rms",1.),nThreads, isStandAlone() ? 0 : rank() * nThreads);
      
      ASKAPLOG_INFO_STR(logger, "Running the test for rank="<<rank());

      for (uint32_t count = 0; (count < expectedCount) || !expectedCount; ++count) {
           // prepare the integration
           timer.mark();
           casacore::Quantity q;
           ASKAPCHECK(casacore::MVTime::read(q, "today"), "MVTime::read failed");
           chunk->time() = casacore::MVEpoch(q);
           // if we want synchronised times, it may be interesting to have this code optional
           if (numProcs() > 1) {
               double timeRecvBuf[2];
               if (rank() == 0) {
                   timeRecvBuf[0] = chunk->time().getDay();
                   timeRecvBuf[1] = chunk->time().getDayFraction();
               }
               const int response = MPI_Bcast(timeRecvBuf, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
               ASKAPCHECK(response == MPI_SUCCESS, "Error gathering times, response from MPI_Bcast = "<<response);
               if (rank() != 0) {
                   const casacore::MVEpoch receivedTime(timeRecvBuf[0], timeRecvBuf[1]);
                   chunk->time() = receivedTime;
               }
           }
           ASKAPDEBUGASSERT(chunk->visibility().contiguousStorage());
           boost::shared_ptr<casacore::Complex> data(chunk->visibility().data(), utility::NullDeleter());
           pg.generate(data, chunk->visibility().nelements());
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

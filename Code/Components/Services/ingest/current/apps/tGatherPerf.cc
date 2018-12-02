/// @file tGatherPerf.cc
/// @details
///   This application runs MPI Gather call with mock up data. This is handy
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

// boost includes
#include "boost/shared_array.hpp"
#include "boost/shared_ptr.hpp"


#include <mpi.h>


// Using
using namespace askap;
using namespace askap::cp;

ASKAP_LOGGER(logger, "tGatherPref");

class GatherTestApp : public askap::cp::common::ParallelCPApplication
{
public:
   void testGather(const std::vector<float> &data, casa::uInt count) {
      boost::shared_array<float> dataRecvBuf(new float[data.size() * numProcs()]);
      for (casa::uInt i = 0; i < count; ++i) {
           const int response = MPI_Gather(const_cast<float*>(data.data()), data.size(), MPI_FLOAT, dataRecvBuf.get(), data.size(), MPI_FLOAT, 0, MPI_COMM_WORLD);
           ASKAPCHECK(response == MPI_SUCCESS, "Error gathering visibilities, response from MPI_Gather = "<<response);
      }
   }

   virtual void run() {
      //ASKAPCHECK(!isStandAlone() && numProcs() > 1, "This test application is specific to parallel multi-rank case and can't be used in stand-alone mode");
      int count = config().getInt32("count", 10);
      ASKAPCHECK(count > 0, "Expect positive number of timestamps to receive, you have = "<<count);
      uint32_t expectedCount = static_cast<uint32_t>(count);
    
      ASKAPLOG_INFO_STR(logger, "Setting up mock up data structure for rank="<<rank());
     
      uint32_t size = config().getUint32("chunksize", 216 * 36 * 4 * 78);
      ASKAPLOG_INFO_STR(logger, "Chumk size = "<<size<<" complex floats");

      std::vector<float> data(size * 2, 1.1);


      casa::Timer timer;
      float processingTime = 0.;
      float maxProcessingTime = 0.;
      size_t actualCount = 0;

      ASKAPLOG_INFO_STR(logger, "Running the test for rank="<<rank());

      for (uint32_t count = 0; count < expectedCount; ++count) {
           ASKAPLOG_INFO_STR(logger, "Received "<<count + 1<<" integration(s) for rank="<<rank());
           timer.mark();
           testGather(data, 3);
           const float runTime = timer.real();
           ASKAPLOG_INFO_STR(logger, "   - gather took "<<runTime<<" seconds");
           processingTime += runTime;
           ++actualCount;
           if (runTime > maxProcessingTime) {
               maxProcessingTime = runTime;
           }
      }
      if (actualCount > 0) {
          ASKAPLOG_INFO_STR(logger, "Average running time per cycle: "<<processingTime / actualCount<<
                     " seconds, "<<actualCount<<" iteratons averaged, peak = "<<maxProcessingTime<<" seconds");
      }
   }
private:

};

int main(int argc, char *argv[])
{
    GatherTestApp app;
    return app.main(argc, argv);
}

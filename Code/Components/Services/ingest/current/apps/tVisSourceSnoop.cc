/// @file tVisSourceSnoop.cc
/// @details
///   This application runs VisSource in a parallel mode and prints some stats 
///   similar to vsnoop utility (but it works via VisSource). It is handy for
///   ingest scaling tests as it mimics the behaviour of the receiving part
///   without metadata logic which simplifies tests.
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

// ASKAPsoft includes
#include "cpcommon/ParallelCPApplication.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"
#include "cpcommon/VisDatagram.h"

// Local package includes
#include "ingestpipeline/sourcetask/VisSource.h"
#include "ingestpipeline/sourcetask/test/MockMetadataSource.h"

// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, "tVisSourceSnoop");

class TestVisSourceSnoopApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run() {
      //ASKAPCHECK(!isStandAlone() && numProcs() > 1, "This test application is specific to parallel multi-rank case and can't be used in stand-alone mode");
      int count = config().getInt32("count", 10);
      ASKAPCHECK(count > 0, "Expect positive number of timestamps to receive, you have = "<<count);
      uint32_t expectedCount = static_cast<uint32_t>(count);
    
      ASKAPLOG_INFO_STR(logger, "Setting up VisSource object for rank="<<rank());
      itsSrc.reset(new VisSource(config(), rank()));
      itsLastBAT=0u;

      for (;hasMore() && (count > 0);--count) {
           ASKAPASSERT(count > 0);
           ASKAPLOG_INFO_STR(logger, "Received "<<expectedCount + 1 - count<<" integration(s) for rank="<<rank());
      }
      ASKAPCHECK(count == 0, "Early termination detected - perhaps some card stopped streaming data; left over = "<<count);
   }
private:

   /// @brief get one more integration
   /// @return false if the end mark reached, true otherwise
   bool hasMore() {
       ASKAPASSERT(itsSrc);
       uint32_t nDgReceived = 0;
       uint32_t nDgEarlierTimestamp = 0;
       for (bool currentIntegration = true; currentIntegration;) {
            if (!itsDatagram) {
                if (!getNext()) {
                    return false;    
                }
                // this is the first datagram
                itsLastBAT = itsDatagram->timestamp;
            }
            const uint64_t currentBAT = itsDatagram->timestamp;
            if (currentBAT < itsLastBAT) {
                ++nDgEarlierTimestamp;
            } else if (currentBAT == itsLastBAT) {
                ++nDgReceived;
            } 
            if (currentBAT > itsLastBAT) {
              ASKAPLOG_INFO_STR(logger, "Rank "<<rank()<<" got new integration: "<<bat2epoch(itsLastBAT)<<" BAT = "<<std::hex<<itsLastBAT);
              currentIntegration = false;
            } else {
               if (!getNext()) {
                   return false;
               }
            } 
       }

       ASKAPLOG_INFO_STR(logger, "   - rank "<<rank()<<" received "<<nDgReceived<<" datagrams for "<<bat2epoch(itsLastBAT)<<" BAT = "<<std::hex<<itsLastBAT);
       const std::pair<uint32_t, uint32_t> bufferStats = itsSrc->bufferUsage();
       ASKAPLOG_INFO_STR(logger, "   - buffer stats: "<<bufferStats.first<<" datagrams queued out of "<<bufferStats.second<<" possible");
       ASKAPASSERT(itsDatagram);
       itsLastBAT = itsDatagram->timestamp;
       
       return true;
   }

   bool getNext() {
       const long TENSECONDS = 10000000;
       ASKAPASSERT(itsSrc);
       const uint32_t nRetries = 10;
       for (uint32_t attempt = 0; attempt<nRetries; ++attempt) {
            itsDatagram = itsSrc->next(TENSECONDS);
            if (itsDatagram) {
                return true;
            }
       }
       if (!itsDatagram) {
           ASKAPLOG_WARN_STR(logger, "Received empty shared pointer from VisSource after "<<nRetries<<" attempts, perhaps no streaming");
           return false;    
       }
       return true;
   }
   
   
   /// @brief vis source object
   boost::shared_ptr<VisSource> itsSrc;

   /// @brief current datagram
   boost::shared_ptr<askap::cp::VisDatagram> itsDatagram;

   /// @brief previous BAT
   uint64_t itsLastBAT;
};

int main(int argc, char *argv[])
{
    TestVisSourceSnoopApp app;
    return app.main(argc, argv);
}

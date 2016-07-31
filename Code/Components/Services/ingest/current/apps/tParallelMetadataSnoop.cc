/// @file tParallelMetadataSnoop.cc
/// @details
///   This application runs parallel metadata adapter prints some stats 
///   similar to msnoop utility (but it works via MetadataSource). It is handy for
///   ingest scaling tests as it mimics the behaviour of the receiving part
///   without visibility/synchronisation logic which simplifies tests.
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
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/MetadataSource.h"
#include "ingestpipeline/sourcetask/ParallelMetadataSource.h"

// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, "tParallelMetadataSnoop");

class TestParallelMetadataSnoopApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run() {
      //ASKAPCHECK(!isStandAlone() && numProcs() > 1, "This test application is specific to parallel multi-rank case and can't be used in stand-alone mode");
      int count = config().getInt32("count", 10);
      ASKAPCHECK(count > 0, "Expect positive number of timestamps to receive, you have = "<<count);
      uint32_t expectedCount = static_cast<uint32_t>(count);
    
      ASKAPLOG_INFO_STR(logger, "Setting up MetadataSource object for rank="<<rank());

      boost::shared_ptr<IMetadataSource> msrc;
      if ((numProcs() == 1) || (rank() == 0)) {
          ASKAPLOG_INFO_STR(logger, "Setting up actual source for rank = "<<rank());
          
          const std::string mdLocatorHost = config().getString("ice.locator_host");
          const std::string mdLocatorPort = config().getString("ice.locator_port");
          const std::string mdTopicManager = config().getString("icestorm.topicmanager");
          const std::string mdTopic = config().getString("topic");
          const unsigned int mdBufSz = 12; 
          const std::string mdAdapterName = "tParallelMetadataSnoop";
          msrc.reset(new MetadataSource(mdLocatorHost,
              mdLocatorPort, mdTopicManager, mdTopic, mdAdapterName, mdBufSz));
      } else {
          ASKAPLOG_INFO_STR(logger, "Bypass setting up metadata source - slave rank; rank="<<rank());
      }

      if (numProcs() == 1) {
          ASKAPLOG_INFO_STR(logger, "Serial case - just use the metadata source without an adapter");
          itsSrc = msrc;
      } else  {
          ASKAPLOG_INFO_STR(logger, "Parallel case - setting up metadata source adapter");
          itsSrc.reset(new ParallelMetadataSource(msrc));
      }

      //sleep(1);

      itsLastBAT=0u;

      for (;hasMore() && (count > 0);--count) {
           ASKAPASSERT(count > 0);
           ASKAPLOG_INFO_STR(logger, "Received "<<expectedCount + 1 - count<<" integration(s) for rank="<<rank());
      }
      ASKAPCHECK(count == 0, "Early termination detected - perhaps metadata streaming ceased; left over = "<<count);
   }
private:

   /// @brief get one more integration
   /// @return false if the end mark reached, true otherwise
   bool hasMore() {
       ASKAPASSERT(itsSrc);
       const bool firstRun = !itsMetadata;
       if (!getNext()) {
           return false;    
       }
       const uint64_t currentBAT = itsMetadata->time();
       if ((currentBAT < itsLastBAT) && !firstRun) {
           ASKAPLOG_WARN_STR(logger, "Received metadata for earlier time than before!");
           ASKAPLOG_WARN_STR(logger, "   was: "<<bat2epoch(itsLastBAT)<<" BAT = "<<std::hex<<itsLastBAT<< 
                      " now: "<<bat2epoch(currentBAT)<<" BAT = "<<std::hex<<currentBAT);
       } else if (currentBAT == itsLastBAT) {
           ASKAPLOG_WARN_STR(logger, "Received duplicated metadata for "<<bat2epoch(currentBAT)<<" BAT = "<<std::hex<<currentBAT);
       } else {

           ASKAPLOG_INFO_STR(logger, "   - rank "<<rank()<<" received metadata for "<<bat2epoch(currentBAT)<<" BAT = "<<std::hex<<currentBAT<<" scan: "<<std::dec<<itsMetadata->scanId()<<" source: "<<itsMetadata->targetName()<<" freq: "<<itsMetadata->centreFreq());
       }
       itsLastBAT = itsMetadata->time();
       return true;
   }

   bool getNext() {
       const long TENSECONDS = 10000000;
       ASKAPASSERT(itsSrc);
       const uint32_t nRetries = 10;
       for (uint32_t attempt = 0; attempt<nRetries; ++attempt) {
            itsMetadata = itsSrc->next(TENSECONDS);
            if (itsMetadata) {
                return true;
            }
       }
       if (!itsMetadata) {
           ASKAPLOG_WARN_STR(logger, "Received empty shared pointer from MetadataSource after "<<nRetries<<" attempts, perhaps no metadata streaming");
           return false;    
       }
       return true;
   }
   
   
   /// @brief vis source object
   boost::shared_ptr<IMetadataSource> itsSrc;

   /// @brief current datagram
   boost::shared_ptr<askap::cp::TosMetadata> itsMetadata;

   /// @brief previous BAT
   uint64_t itsLastBAT;
};

int main(int argc, char *argv[])
{
    TestParallelMetadataSnoopApp app;
    return app.main(argc, argv);
}

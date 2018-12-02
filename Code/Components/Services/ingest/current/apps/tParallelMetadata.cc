/// @file tParallelMetadata.cc
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
#include "cpcommon/TosMetadata.h"

// Local package includes
#include "ingestpipeline/sourcetask/ParallelMetadataSource.h"
#include "ingestpipeline/sourcetask/test/MockMetadataSource.h"

// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, ".tParallelMetadata");

class TestParallelMetaDataSourceApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run() {
      ASKAPCHECK(!isStandAlone() && numProcs() > 1, "This test application is specific to parallel multi-rank case and can't be used in stand-alone mode");
      int count = config().getInt32("count", 10);
      ASKAPCHECK(count > 0, "Expect positive number of messages to simulate, you have = "<<count);
      uint32_t expectedCount = static_cast<uint32_t>(count);
      setUp(expectedCount);
      for (--count;hasMore("Received");--count) {
           ASKAPCHECK(count > 0, "Expected end of observations flag has not been reached, count="<<count);

           // deliberately purge some messages on some ranks emulating mismatch between visibility stream
           // and metadata stream

           // ranks 1, 5, 9, etc skip 1 message
           bool forceSkip = (rank() % 4 == 1) && (count == 7);
           // ranks 9, 21, etc skip the following message as well
           forceSkip |= (rank() % 12 == 9) && (count == 6);
           if (forceSkip && !hasMore("Skipped")) {
                   break;
           }
      }
      if ((rank() % 12 == 9) && (expectedCount>6)) {
          ASKAPCHECK(count > 0, "Message wasn't skipped as expected; left over = "<<count);
          --count;
      } 
      if ((rank() % 4 == 1) && (expectedCount>7)) {
          ASKAPCHECK(count > 0, "Message wasn't skipped as expected; left over = "<<count);
          --count;
      } 
      ASKAPCHECK(count == 0, "Some messages left over in the queue; unexpected result; left over = "<<count);
   }
private:
   /// @brief set up mock up source on the master rank with the given number of messages
   /// @details
   /// @param[in] count number of messages to add
   void setUp(uint32_t count) {
      const int masterRank = config().getInt32("master_rank", 0);
      boost::shared_ptr<MockMetadataSource> mdSrc;
      if (rank() == masterRank) {
          mdSrc.reset(new MockMetadataSource);
          const uint32_t count = config().getUint32("count", 10);
          const uint64_t startTime = 0x1197c90000000 + static_cast<uint64_t>(config().getUint32("start_time", 0x4000000));
          const uint64_t period = config().getUint32("period", 4976640ul);
          ASKAPLOG_INFO_STR(logger, "Setting up mock up metadata source for "<<count<<" messages starting at 0x"<<
                  std::hex<<startTime<<" with period of "<<double(period) / 1e6 <<" s");
          uint64_t time = startTime;
          for (uint32_t i = 0; i < count; ++i, time += period) {
               boost::shared_ptr<TosMetadata> md(new TosMetadata);
               md->time(time);
               md->scanId(i + 1 != count ? static_cast<int>(i) : -2);
               mdSrc->add(md);
          }
      }
      itsSrc.reset(new ParallelMetadataSource(mdSrc));
   }

   /// @brief get one more message
   /// @param[in] action string for the message
   /// @return false if the end mark reached, true otherwise
   bool hasMore(const std::string &action) const {
       const long ONESECOND = 1000000;
       ASKAPASSERT(itsSrc);
       boost::shared_ptr<askap::cp::TosMetadata> md = itsSrc->next(ONESECOND);
       ASKAPCHECK(md, "next call returns an empty shared pointer. This is unexpected.");
       
       ASKAPLOG_INFO_STR(logger, action<<" metadata with BAT=0x"<<std::hex<<md->time()<<" scanId="<<std::dec<<md->scanId());
       
       return md->scanId() != -2;
   }
   
   boost::shared_ptr<IMetadataSource> itsSrc;
};

int main(int argc, char *argv[])
{
    TestParallelMetaDataSourceApp app;
    return app.main(argc, argv);
}

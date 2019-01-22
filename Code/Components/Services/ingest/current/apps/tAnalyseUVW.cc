/// @file tAnalyseUVW.cc
/// @details
///   This application runs metadata source and does some cross-checks of uvw 
///   coordinates for unflagged samples w.r.t. FCM layout available through the parset.
///   It is expected that the parset is essentially the same as available for ingest invocation.
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
#include "configuration/Configuration.h"
#include "configuration/Antenna.h"

// Local package includes
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/MetadataSource.h"
#include "ingestpipeline/sourcetask/ParallelMetadataSource.h"

// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, "tAnalyseUVW");

class TestAnalyseUVWApp : public askap::cp::common::ParallelCPApplication
{
public:
   virtual void run() {
      ASKAPCHECK(numProcs() == 1, "This test application is intended to be executed in the serial/single rank mode");
      int count = config().getInt32("count", -1);
      ASKAPCHECK(count > 0 || count == -1, "Expect positive number of timestamps to receive or -1 for indefinite cycling, you have = "<<count);
      uint32_t expectedCount = count > 0 ? static_cast<uint32_t>(count) : 0;

      ASKAPLOG_INFO_STR(logger, "Setting up MetadataSource object");

      ASKAPLOG_INFO_STR(logger, "Setting up actual source for rank = "<<rank());
          
      const std::string mdLocatorHost = config().getString("metadata_source.ice.locator_host");
      const std::string mdLocatorPort = config().getString("metadata_source.ice.locator_port");
      const std::string mdTopicManager = config().getString("metadata_source.icestorm.topicmanager");
      const std::string mdTopic = config().getString("metadata.topic");
      const unsigned int mdBufSz = 12; 
      const std::string mdAdapterName = "tAnalyseUVW";
      itsSrc.reset(new MetadataSource(mdLocatorHost,
          mdLocatorPort, mdTopicManager, mdTopic, mdAdapterName, mdBufSz));

      makeBaselines();
    
      itsLastBAT=0u;

      for (count = 0; hasMore() && (count < static_cast<int>(expectedCount) || expectedCount == 0);++count) {
           ASKAPLOG_INFO_STR(logger, "Received "<<count + 1<<" integration(s) for rank="<<rank());
           analyseUVW();
           //analyseUVWVectorLength();
      }
      ASKAPCHECK(count < static_cast<int>(expectedCount), "Early termination detected - perhaps metadata streaming ceased; left over = "<<(expectedCount > 0 ? static_cast<int>(expectedCount) - count: -1));
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
           std::string infoStr;
           if (checkAllFlagged()) {
               infoStr += "all data flagged";
           }
           ASKAPLOG_INFO_STR(logger, "   - metadata for "<<bat2epoch(currentBAT)<<" BAT = "<<std::hex<<currentBAT<<" scan: "<<std::dec<<itsMetadata->scanId()<<" source: "<<itsMetadata->targetName()<<" "<<infoStr);
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
   
   bool checkAllFlagged() {
       ASKAPASSERT(itsMetadata);
       if (itsMetadata->flagged()) {
           return true;
       }
       const std::vector<std::string> names = itsMetadata->antennaNames();
       casa::uInt count = 0;
       for (std::vector<std::string>::const_iterator ci = names.begin(); ci != names.end(); ++ci) {
            const TosMetadataAntenna& tma = itsMetadata->antenna(*ci);
            if (!tma.flagged() && tma.onSource()) {
                ++count;
            }
       }
       // need at least two antennas to form unflagged data sample
       return count < 2;
   }

   void makeBaselines() {
      ASKAPASSERT(itsBaselines.size() == 0);
      Configuration cfg(config(), rank(), numProcs());
      const std::vector<Antenna>& antennas = cfg.antennas();
      double minLength = -1;
      double maxLength = -1;
      std::string minName, maxName;
      for (size_t ant1 = 0; ant1 < antennas.size(); ++ant1) {
           for (size_t ant2 = ant1 + 1; ant2 < antennas.size(); ++ant2) {
                const casa::Vector<casa::Double> bsln = antennas[ant2].position() - antennas[ant1].position();
                ASKAPASSERT(bsln.nelements() == 3);
                const double length = casa::sqrt(bsln[0] * bsln[0] + bsln[1]*bsln[1] + bsln[2]*bsln[2]);
                const std::string key = antennas[ant1].name() + "-" + antennas[ant2].name();
                if (minLength<0) {
                    minLength = length;
                    maxLength = length;
                    minName = maxName = key;
                } else {
                    if (minLength > length) {
                        minLength = length;
                        minName = key;
                    }
                    if (maxLength < length) {
                        maxLength = length;
                        maxName = key;
                    }
                }
                ASKAPCHECK(itsBaselines.find(key) == itsBaselines.end(), "Duplicated baseline "<<key<<" - this shouldn't happen");
                itsBaselines[key] = bsln.tovector();
           }
      }
      ASKAPLOG_INFO_STR(logger, "Loaded layout with "<<itsBaselines.size()<<" baselines, the shortest is "<<minName<<" "<<minLength<<" metres, the longest "<<maxName<<" "<<maxLength<<" metres");
   }
   
   void analyseUVWVectorLength() {
      ASKAPASSERT(itsMetadata);
      if (itsMetadata->flagged()) {
          return;
      }
      const std::vector<std::string> names = itsMetadata->antennaNames();
      for (std::vector<std::string>::const_iterator ci=names.begin(); ci != names.end(); ++ci) {
           const TosMetadataAntenna& tma = itsMetadata->antenna(*ci);
           if (!tma.flagged() && tma.onSource()) {
               const casa::Vector<casa::Double>& uvw = tma.uvw();
               ASKAPCHECK(uvw.nelements() % 3 == 0, "Expect 3 elements per beam in uvw");
               const casa::uInt nBeams = uvw.nelements() / 3;
               double minLength = -1, maxLength = -1;
               for (casa::uInt beam = 0; beam < nBeams; ++beam) {
                    const double length = casa::sqrt(uvw[beam*3]*uvw[beam*3] + uvw[beam*3+1]*uvw[beam*3+1] + uvw[beam*3+2]*uvw[beam*3+2]);
                    if (length < minLength || minLength < 0) {
                        minLength = length;
                    }
                    if (length > maxLength || maxLength < 0) {
                        maxLength = length;
                    }
               }
               ASKAPLOG_INFO_STR(logger, "Antenna "<<*ci<<": |uvw| range from "<<minLength/1e3<<" km to "<<maxLength/1e3<<" km");
           }
      }
   }
  
   // returns the number of verified baselines
   size_t analyseUVW() {
      ASKAPASSERT(itsMetadata);
      if (itsMetadata->flagged()) {
          return 0;
      }
      const std::vector<std::string> names = itsMetadata->antennaNames();
      size_t count = 0;
      size_t countGood = 0;
      for (std::map<std::string, std::vector<double> >::const_iterator ci = itsBaselines.begin(); ci != itsBaselines.end(); ++ci) {
           const size_t pos = ci->first.find("-");
           ASKAPASSERT(pos != std::string::npos);
           ASKAPASSERT(pos + 1 < ci->first.size());
           ASKAPASSERT(ci->first.find("-",pos+1) == std::string::npos);
           const std::string ant1 = ci->first.substr(0,pos);
           const std::string ant2 = ci->first.substr(pos+1);
           ASKAPCHECK(find(names.begin(),names.end(), ant1) != names.end(), "Unable to find antenna "<<ant1<<" in metadata");
           ASKAPCHECK(find(names.begin(),names.end(), ant2) != names.end(), "Unable to find antenna "<<ant2<<" in metadata");
           const TosMetadataAntenna& tma1 = itsMetadata->antenna(ant1);
           const TosMetadataAntenna& tma2 = itsMetadata->antenna(ant2);
           if (tma1.flagged() || tma2.flagged() || !tma1.onSource() || !tma2.onSource()) {
               continue;
           }
           const casa::Vector<casa::Double> bsln = tma2.uvw() - tma1.uvw();
               //ASKAPLOG_INFO_STR(logger, " uvw1: "<<tma1.uvw()<<" uvw2:"<<tma2.uvw());
           ASKAPCHECK(bsln.nelements() % 3 == 0, "Expect 3 elements per beam in uvw vector, size = "<<bsln.nelements()<<" for "<<ci->first<<" baseline");
           const casa::uInt nBeams = bsln.nelements() / 3;
           ++count;
           size_t nGoodBeams = 0;
           double maxDiff = -1;
           for (casa::uInt beam = 0; beam < nBeams; ++beam) {
                double diff2_meas = 0.;
                double diff2_exp = 0.;
                for (casa::uInt coord = 0; coord < 3; ++coord) {
                     diff2_meas += bsln[beam * 3 + coord] * bsln[beam * 3 + coord];
                     diff2_exp += ci->second[coord] * ci->second[coord];
                }
                const double diff = casa::sqrt(diff2_meas) - casa::sqrt(diff2_exp);
                //ASKAPLOG_INFO_STR(logger, "Beam: "<<beam<<" bsln: "<<bsln<<" expected: "<<ci->second);
                //ASKAPCHECK(diff < 0.001, "Difference exceeds 1mm for beam "<<beam<<" baseline "<<ci->first<<" max difference: "<<diff<<" metres");
                if (diff < 0.001) {
                    ++nGoodBeams;
                }
                if (diff < maxDiff || maxDiff < 0) {
                     maxDiff = diff;
                }
           }
           if (nGoodBeams != nBeams) {
               ASKAPLOG_INFO_STR(logger, "Baseline "<<ci->first<<" discrepancy over 1mm for "<<nBeams - nGoodBeams<<" beams, largest difference "<<maxDiff<<" metres");
           } else {   
               ++countGood;
           }
      }
      ASKAPLOG_INFO_STR(logger, "Analysed "<<count<<" baselines, found "<<countGood<<" to be within 1mm of expected length");
      return count;
   }
   
   
   /// @brief vis source object
   boost::shared_ptr<IMetadataSource> itsSrc;

   /// @brief current datagram
   boost::shared_ptr<askap::cp::TosMetadata> itsMetadata;

   /// @brief previous BAT
   uint64_t itsLastBAT;

   /// @brief baselines, key is akXX-akYY
   std::map<std::string, std::vector<double> > itsBaselines;
};

int main(int argc, char *argv[])
{
    TestAnalyseUVWApp app;
    return app.main(argc, argv);
}

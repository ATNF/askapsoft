/// @file tSubstitutionRules.cc
/// @details
///   This application is inteded for testing of various MPI-dependent substitution rules
///   which are hard to test inside the unit test frame work. There is no cross-subsystem 
///   dependence, just one step closer to real life operations.
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
#include "ingestpipeline/mssink/GenericSubstitutionRule.h"
#include "ingestpipeline/mssink/DateTimeSubstitutionRule.h"
#include "ingestpipeline/mssink/BeamSubstitutionRule.h"
#include "configuration/SubstitutionHandler.h"
#include "cpcommon/ParallelCPApplication.h"
#include "cpcommon/VisChunk.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"

// boost includes
#include "boost/shared_ptr.hpp"


#include <mpi.h>


// Using
using namespace askap;
using namespace askap::cp;

ASKAP_LOGGER(logger, "tSubstitutionRules");

class SubstitutionRulesTestApp : public askap::cp::common::ParallelCPApplication
{
public:
   
   std::string substitute(const std::string &str) {
      ingest::SubstitutionHandler sh;
      ingest::Configuration cfg(config(), rank(), numProcs());
      ingest::GenericSubstitutionRule gsr("r", rank(), cfg);
      ingest::GenericSubstitutionRule gsr2("s", cfg.receiverId(), cfg);
      ingest::DateTimeSubstitutionRule dtsr(cfg);
      sh.add(boost::shared_ptr<ingest::GenericSubstitutionRule>(&gsr, utility::NullDeleter()));
      sh.add(boost::shared_ptr<ingest::DateTimeSubstitutionRule>(&dtsr, utility::NullDeleter()));
      sh.add(boost::shared_ptr<ingest::GenericSubstitutionRule>(&gsr2, utility::NullDeleter()));
      if (itsBeamSR) {
          sh.add(itsBeamSR);
      }
      return sh(str);
   }

   virtual void run() {
      itsBeamSR.reset(new ingest::BeamSubstitutionRule("b", ingest::Configuration(config(), rank(), numProcs())));
      boost::shared_ptr<common::VisChunk> chunk(new common::VisChunk(100, 10, 4, 6));
      chunk->beam1().set(static_cast<casa::uInt>(rank()));
      chunk->beam2().set(static_cast<casa::uInt>(rank()));
      itsBeamSR->setupFromChunk(chunk);

      std::vector<std::string>  strs;
      strs.push_back(config().getString("filename", "test_%r.dat"));
      strs.push_back("test%{_%d%}%{_%r%}");
      strs.push_back("%d_%t%{_%d:%s%r%}");
      strs.push_back("%d_%t%{_%b%}");
      for (std::vector<std::string>::const_iterator ci = strs.begin(); ci != strs.end(); ++ci) {
           const std::string res = substitute(*ci);
           ASKAPLOG_INFO_STR(logger, "Input: "<<*ci<<" output: "<<res);
      }
      itsBeamSR->verifyChunk(chunk);
      
   }
private:
   boost::shared_ptr<ingest::BeamSubstitutionRule> itsBeamSR;
};

int main(int argc, char *argv[])
{
    SubstitutionRulesTestApp app;
    return app.main(argc, argv);
}

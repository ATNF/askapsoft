//
// @file tTableCalSolution.cc : evolving test/demonstration program of the
//                        calibration table accessor
//
/// @copyright (c) 2007 CSIRO
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


#include <calibaccess/TableCalSolutionConstSource.h>
#include <askap_accessors.h>
#include <askap/AskapLogging.h>
ASKAP_LOGGER(logger, "");

#include <askap/AskapError.h>

// casa
#include <casacore/casa/OS/Timer.h>


// std
#include <stdexcept>
#include <iostream>

// boost
#include <boost/shared_ptr.hpp>

using std::cout;
using std::cerr;
using std::endl;

using namespace askap;
using namespace accessors;

void doReadOnlyTest(const ICalSolutionConstSource &src) 
{
   const long id = src.mostRecentSolution();
   boost::shared_ptr<ICalSolutionConstAccessor> acc = src.roSolution(id);
   ASKAPASSERT(acc);
   const bool doBandpass = false;
   const casa::uInt nAnt = 36;
   if (doBandpass) {
       const casa::uInt nChan = 288;
       for (casa::uInt chan = 0; chan < nChan; ++chan) {
            std::cout<<chan;
            for (casa::uInt ant = 0; ant < nAnt; ++ant) {
                 const JonesIndex index(ant, 0);
                 const JonesJTerm value = acc->bandpass(index, chan);
                 std::cout<<" "<<casa::abs(value.g1())<<" "<<casa::arg(value.g1()) / casa::C::pi * 180.<<" "<<value.g1IsValid() << " " <<
                            casa::abs(value.g2())<<" "<<casa::arg(value.g2()) / casa::C::pi * 180.<<" "<<value.g2IsValid();
            }
            std::cout<<std::endl;
       }
   } else {
       for (casa::uInt ant = 0; ant < nAnt; ++ant) {
            const JonesIndex index(ant, 0);
            const JonesJTerm value = acc->gain(index);
            std::cout<<casa::abs(value.g1())<<" "<<casa::arg(value.g1()) / casa::C::pi * 180.<<" "<<value.g1IsValid() << " " <<
                        casa::abs(value.g2())<<" "<<casa::arg(value.g2()) / casa::C::pi * 180.<<" "<<value.g2IsValid()<<std::endl;
       }
       
   }
}

int main(int argc, char **argv) {
  try {
     if (argc!=2) {
         cerr<<"Usage "<<argv[0]<<" cal_table"<<endl;
	 return -2;
     }

     casa::Timer timer;

     timer.mark();
     TableCalSolutionConstSource cs(argv[1]);     
     std::cerr<<"Initialization: "<<timer.real()<<std::endl;
     timer.mark();
     doReadOnlyTest(cs);
     std::cerr<<"Job: "<<timer.real()<<std::endl;
     
  }
  catch(const AskapError &ce) {
     cerr<<"AskapError has been caught. "<<ce.what()<<endl;
     return -1;
  }
  catch(const std::exception &ex) {
     cerr<<"std::exception has been caught. "<<ex.what()<<endl;
     return -1;
  }
  catch(...) {
     cerr<<"An unexpected exception has been caught"<<endl;
     return -1;
  }
  return 0;
}

/// @file
/// @brief an utility to extract level of autocorrelations per beam for port mapping test
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


#include <dataaccess/TableDataSource.h>
#include <askap_accessors.h>
#include <askap/AskapLogging.h>
#include <askap/AskapUtil.h>
ASKAP_LOGGER(logger, "");

#include <askap/AskapError.h>
#include <dataaccess/SharedIter.h>

#include <dataaccess/TableManager.h>
#include <dataaccess/IDataConverterImpl.h>
#include <utils/DelayEstimator.h>
#include <swcorrelator/BasicMonitor.h>

// casa
#include <measures/Measures/MFrequency.h>
#include <tables/Tables/Table.h>
#include <casa/OS/Timer.h>
#include <casa/Arrays/ArrayMath.h>
#include <casa/Arrays/MatrixMath.h>



// std
#include <stdexcept>
#include <iostream>
#include <vector>
#include <iomanip>

using std::cout;
using std::cerr;
using std::endl;

using namespace askap;
using namespace askap::accessors;

struct DeadBeamsList {
   explicit DeadBeamsList(const std::string &fname) : itsOS(fname.c_str()) { ASKAPCHECK(itsOS, "Unable to create file "<<fname); itsDeadBeams.reserve(72);}
   
   void add(casa::uInt beam)  { itsDeadBeams.push_back(beam); }
   void flushAndReset(double time) {
      itsOS<<time;
      if (itsDeadBeams.size()) {
          for (size_t i=0; i<itsDeadBeams.size(); ++i) {
               itsOS<<" "<<itsDeadBeams[i];
          }
          itsDeadBeams.resize(0);
      } else {
          itsOS<<" all valid";
      }
      itsOS<<std::endl;
   }
   
   ~DeadBeamsList() { if (itsDeadBeams.size() > 0) { std::cout<<"Dead beams list not flushed before the destructor, last integration will be lost"; }}
private:
   std::vector<casa::uInt> itsDeadBeams;
   std::ofstream itsOS;
};


void process(const IConstDataSource &ds, const int ctrl = -1) {
  IDataSelectorPtr sel=ds.createSelector();
  //sel->chooseFeed(0);
  //sel->chooseCrossCorrelations();
  sel->chooseAutoCorrelations();
  if (ctrl >=0 ) {
      //sel->chooseUserDefinedIndex("CONTROL",casa::uInt(ctrl));
      sel->chooseUserDefinedIndex("SCAN_NUMBER",casa::uInt(ctrl));
  }
  IDataConverterPtr conv=ds.createConverter();  
  conv->setFrequencyFrame(casa::MFrequency::Ref(casa::MFrequency::TOPO),"MHz");
  conv->setEpochFrame(casa::MEpoch(casa::Quantity(55913.0,"d"),
                      casa::MEpoch::Ref(casa::MEpoch::UTC)),"s");
  conv->setDirectionFrame(casa::MDirection::Ref(casa::MDirection::J2000));                    
  casa::Vector<double> freq;
  size_t counter = 0;
  size_t nGoodRows = 0;
  size_t nBadRows = 0;
  casa::uInt nChan = 0;
  casa::uInt nRow = 0;
  double startTime = 0;
  double stopTime = 0;
  
  casa::Vector<casa::uInt> ant1ids, ant2ids, beamids;

  std::ofstream os3("autoamps.dat");

  std::vector<boost::shared_ptr<DeadBeamsList> > dbl(3);
  dbl[0].reset(new DeadBeamsList("ak04_deadbeams.dat"));
  dbl[1].reset(new DeadBeamsList("ak05_deadbeams.dat"));
  dbl[2].reset(new DeadBeamsList("ak12_deadbeams.dat"));

  bool firstTimeStamp = true;
  casa::uInt numberOfRowsPerBeam = 0;
  for (IConstDataSharedIter it=ds.createConstIterator(sel,conv);it!=it.end();++it, ++counter) {  
       if (firstTimeStamp) {
           startTime = it->time();
           firstTimeStamp = false;
       }
       stopTime = it->time() + 5; // 5s integration time is hardcoded

       

       if (nChan == 0) {
           nChan = it->nChannel();
           nRow = it->nRow();
           freq = it->frequency();
           ant1ids = it->antenna1();
           ant2ids = it->antenna2();
           beamids = it->feed1();
           std::cout<<"Baseline order is as follows: "<<std::endl;
           numberOfRowsPerBeam = 0;
           for (casa::uInt row = 0; row<nRow; ++row) {
                if (beamids[row] == 0) {
                    std::cout<<"baseline (1-based) = "<<row+1<<" is "<<ant1ids[row]<<" - "<<ant2ids[row]<<std::endl; 
                } else {
                    if (numberOfRowsPerBeam == 0) {
                        numberOfRowsPerBeam = row;
                    }
                    ASKAPCHECK(numberOfRowsPerBeam != 0, "First beam should have an ID of zero");
                    const casa::uInt firstBeamRow = row % numberOfRowsPerBeam;
                    ASKAPCHECK(ant1ids[firstBeamRow] == ant1ids[row], "Inconsistent antenna 1 ids at row = "<<row<<" for beam "<<beamids[row]);
                    ASKAPCHECK(ant2ids[firstBeamRow] == ant2ids[row], "Inconsistent antenna 2 ids at row = "<<row<<" for beam "<<beamids[row]);             
                }
           }           
       } else { 
           ASKAPCHECK(nChan == it->nChannel(), 
                  "Number of channels seem to have been changed, previously "<<nChan<<" now "<<it->nChannel());
           if (nRow != it->nRow()) {
               std::cerr<<"Number of rows changed was "<<nRow<<" now "<<it->nRow()<<std::endl;
               continue;
           }
           ASKAPCHECK(nRow == it->nRow(), 
                  "Number of rows seem to have been changed, previously "<<nRow<<" now "<<it->nRow());
       }
       
       ASKAPASSERT(it->nPol() == 4);
       ASKAPASSERT(it->nChannel() > 1);
       // check that the products come in consistent way across the interations
       for (casa::uInt row = 0; row<nRow; ++row) {
            ASKAPCHECK(it->antenna1()[row] == ant1ids[row], "Inconsistent antenna 1 ids at row = "<<row);
            ASKAPCHECK(it->antenna2()[row] == ant2ids[row], "Inconsistent antenna 2 ids at row = "<<row);             
       }
       
       // add new spectrum to the buffer

       os3<< (it->time() - startTime)/60.;

       bool somethingFlaggedThisTimestamp = false;

       for (casa::uInt row=0; row<nRow; ++row) {
            
          
            casa::Vector<casa::Bool> flagsX = it->flag().xyPlane(0).row(row);
            casa::Vector<casa::Bool> flagsY = it->flag().xyPlane(3).row(row);
            bool flagged = false;
            bool allFlagged = true;
            for (casa::uInt ch = 0; ch < flagsX.nelements(); ++ch) {
                 ASKAPDEBUGASSERT(ch < flagsY.nelements());
                 flagged |= flagsX[ch];
                 flagged |= flagsY[ch];
                 allFlagged &= flagsX[ch];
                 allFlagged &= flagsY[ch];
            }
            
            casa::Vector<casa::Complex> measuredRowX = it->visibility().xyPlane(0).row(row);
            casa::Vector<casa::Complex> measuredRowY = it->visibility().xyPlane(3).row(row);
            
            if (ant1ids[row] >= 3) {
                casa::Complex sumX(0.,0.);
                casa::Complex sumY(0.,0.);
                size_t nGoodChX = 0;
                size_t nGoodChY = 0;
                for (casa::uInt ch=0; ch < measuredRowX.nelements(); ++ch) {
                     ASKAPDEBUGASSERT(ch < flagsX.nelements());
                     ASKAPDEBUGASSERT(ch < flagsY.nelements());
                     ASKAPDEBUGASSERT(ch < measuredRowY.nelements());
                     if (!flagsX[ch]) {
                         ++nGoodChX;
                         sumX += measuredRowX[ch];
                     }
                     if (!flagsY[ch]) {
                         ++nGoodChY;
                         sumY += measuredRowY[ch];
                     }
                }
                const float curAmpX  = nGoodChX == 0 ? 0. : abs(sumX / float(nGoodChX));
                const float curAmpY  = nGoodChY == 0 ? 0. : abs(sumY / float(nGoodChY));
                os3<<" "<<curAmpX<<" "<<curAmpY;
                if (ant1ids[row] < 6) {
                    const size_t index = ant1ids[row] - 3;
                    ASKAPDEBUGASSERT(index < dbl.size());
                    DeadBeamsList& curDBL = *(dbl[index]);
                    // sequential numeration of polarisations, to match the hardware numbering scheme
                    const double threshold = 50;
                    if (curAmpX < threshold) {
                        curDBL.add(beamids[row]  * 2);
                    }
                    if (curAmpY < threshold) {
                        curDBL.add(beamids[row]  * 2 + 1);
                    }
                }
            }


            /*
            if ((counter == 10) && (row==12)) {
                std::ofstream os4("testsp.dat");
                for (casa::uInt ch=0; ch<measuredRow.nelements(); ++ch) {
                     os4<<ch<<" "<<arg(measuredRow[ch])/casa::C::pi*180.<<" "<<arg(reducedResVis[ch/54])/casa::C::pi*180.<<std::endl;
                }
            }
            */
            
            
            // to disable flagging
            //flagged = false; 
            //flagged = allFlagged; 
            

            if (flagged) {
               ++nBadRows;
               somethingFlaggedThisTimestamp = true;
            } else {
                ++nGoodRows;
            }
       }
       if (somethingFlaggedThisTimestamp) {
           os3<<" flagged";
       }
       os3<<std::endl;
       for (size_t i = 0; i < dbl.size(); ++i) {
            dbl[i]->flushAndReset((it->time() - startTime)/60.);
       }
       if ((counter == 0) && (nGoodRows == 0)) {
           // all data are flagged, completely ignoring this iteration and consider the next one to be first
           nChan = 0;
           continue;
       }
       /*
       // optionally reset integration to provide multiple chunks integrated 
       if ((counter>1) && ((it->time() - startTime)/60. >= 29.9999999)) {
           counter = 0;
           nChan = 0;
       } 
       //
       */
      
  }
  if (counter>1) {
      //ASKAPCHECK(nGoodRows % nRow == 0, "Number of good rows="<<nGoodRows<<" is supposed to be integral multiple of number of rows in a cycle="<<nRow);
      std::cout<<"Each integration has "<<nRow<<" rows"<<std::endl;
      casa::uInt nGoodCycles = nGoodRows / nRow;
      std::cout<<"Processed "<<nGoodCycles<<" integration cycles, "<<nGoodRows<<" good and "<<nBadRows<<" bad rows, time span "<<(stopTime-startTime)/60.<<" minutues, cycles="<<counter<<std::endl;
  } else {
     std::cout<<"No data found!"<<std::endl;
  }
}


int main(int argc, char **argv) {
  try {
     if ((argc!=2) && (argc!=3)) {
         cerr<<"Usage: "<<argv[0]<<" [ctrl] measurement_set"<<endl;
	 return -2;
     }

     casa::Timer timer;
     const std::string msName = argv[argc - 1];
     const int ctrl = argc == 2 ? -1 : utility::fromString<int>(argv[1]);

     timer.mark();
     TableDataSource ds(msName,TableDataSource::MEMORY_BUFFERS);     
     std::cerr<<"Initialization: "<<timer.real()<<std::endl;
     timer.mark();
     process(ds,ctrl);
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

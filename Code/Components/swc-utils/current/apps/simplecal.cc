/// @file
/// @brief an utility to "calibrate" 3-antenna experiment with the sw-correlation
/// @details The number of measurements is not enough to do a proper calibration.
/// This is why the ccalibrator cannot be used. However, we can align the data to
/// get a basic effect of the calibration and also optionally adjust amplitudes assuming
/// a strong source has been observed
///
///
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
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapUtil.h>
ASKAP_LOGGER(logger, "");

#include <askap/askap/AskapError.h>
#include <dataaccess/SharedIter.h>

#include <dataaccess/TableManager.h>
#include <dataaccess/IDataConverterImpl.h>
#include <swcorrelator/BasicMonitor.h>

// casa
#include <casacore/measures/Measures/MFrequency.h>
#include <casacore/tables/Tables/Table.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/Arrays/MatrixMath.h>



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

std::string printComplex(const casacore::Complex &val) {
  return std::string("[")+utility::toString<float>(casacore::real(val))+" , "+utility::toString<float>(casacore::imag(val))+"]";
}

void process(const IConstDataSource &ds, const float flux, const int ctrl = -1) {
  IDataSelectorPtr sel=ds.createSelector();
  sel->chooseCrossCorrelations();
  //sel->chooseFeed(7);
  if (ctrl >=0 ) {
      sel->chooseUserDefinedIndex("CONTROL",casacore::uInt(ctrl));
  }
  IDataConverterPtr conv=ds.createConverter();  
  conv->setFrequencyFrame(casacore::MFrequency::Ref(casacore::MFrequency::TOPO),"MHz");
  conv->setEpochFrame(casacore::MEpoch(casacore::Quantity(55913.0,"d"),
                      casacore::MEpoch::Ref(casacore::MEpoch::UTC)),"s");
  conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));                    
  casacore::Matrix<casacore::Complex> buf;
  casacore::Vector<double> freq;
  size_t counter = 0;
  size_t nGoodRows = 0;
  size_t nBadRows = 0;
  casacore::uInt nChan = 0;
  casacore::uInt nRow = 0;
  double startTime = 0;
  double stopTime = 0;

  casacore::Vector<casacore::uInt> ant1IDs;
  casacore::Vector<casacore::uInt> ant2IDs;
    
  // the assumed baseline order depends on this parameter
  const bool useSWCorrelator = false;
  const bool useADECorrelator = true;

  casacore::uInt cPol = 3;
  for (IConstDataSharedIter it=ds.createConstIterator(sel,conv);it!=it.end();++it) {  
       // for every iteration we first build an index into all unflagged rows
       std::vector<casacore::uInt> rowIndex;
       rowIndex.reserve(it->nRow());
       ASKAPASSERT(cPol<it->nPol());

       if (counter >= 5800) break;

       for (casacore::uInt row = 0; row<it->nRow(); ++row) {
            casacore::Vector<casacore::Bool> flags = it->flag().xyPlane(cPol).row(row);
            
            // this code ensures that all channels are unflagged
            bool flagged = false;
            for (casacore::uInt ch = 0; ch < flags.nelements(); ++ch) {
                 flagged |= flags[ch];
            }
            
            /*
            // this code ensures that at least one channel is unflagged
            bool flagged = true;
            for (casacore::uInt ch = 0; ch < flags.nelements(); ++ch) {
                 flagged &= flags[ch];
            }
            */

            if (!flagged) {
                rowIndex.push_back(row);
            }
       }
       if (nChan == 0) {
           nChan = it->nChannel();
           nRow = rowIndex.size();//it->nRow();
           buf.resize(nRow,nChan);
           buf.set(casacore::Complex(0.,0.));
           freq = it->frequency();
           ant1IDs = it->antenna1().copy();
           ant2IDs = it->antenna2().copy();
           //std::cout<<ant1IDs<<" "<<ant2IDs<<" "<<rowIndex<<std::endl;
       } else { 
           ASKAPCHECK(nChan == it->nChannel(), 
                  "Number of channels seem to have been changed, previously "<<nChan<<" now "<<it->nChannel());
           //ASKAPCHECK(nRow == rowIndex.size(), 
           //       "Number of rows seem to have been changed, previously "<<nRow<<" now "<<it->nRow());
           if (nRow != rowIndex.size()) {
               // quick and dirty hack for now as a work around for the condition where one antenna is unflagged before the others (as it normally happens
               // now for fringe tracking under control of the ingest pipeline)
               // --> allow to increase the amount of unflagged data, but skip rows if their number decreases
               // currently skip one cycle when it happens, but it is not too bad as we normally have FR settling on a new rate
               if (nRow < rowIndex.size()) {
                   std::cerr<<"Number of unflagged rows increased, initially "<<nRow<<" now "<<rowIndex.size()<<", integration cycle = "<<counter+1<<" reset the expected number of rows"<<std::endl;
                   nChan = 0;
                   continue;
               }
               std::cerr<<"Number of unflagged rows has been changed, initially "<<nRow<<" now "<<rowIndex.size()<<", integration cycle = "<<counter+1<<std::endl;
               continue;
           }
           //
           ASKAPDEBUGASSERT(ant1IDs.nelements() == it->nRow());
           ASKAPDEBUGASSERT(ant2IDs.nelements() == it->nRow());
           for (casacore::uInt row = 0; row<it->nRow(); ++row) {
                ASKAPCHECK(ant1IDs[row] == it->antenna1()[row], "Mismatch of antenna 1 index for row "<<row<<
                           " - got "<<it->antenna1()[row]<<" expected "<<ant1IDs[row]);
                ASKAPCHECK(ant2IDs[row] == it->antenna2()[row], "Mismatch of antenna 2 index for row "<<row<<
                           " - got "<<it->antenna2()[row]<<" expected "<<ant2IDs[row]);
           }
       }
       
       ASKAPASSERT(it->nPol() >= 1);
       ASKAPASSERT(it->nChannel() > 1);
       // we require that 3 baselines come in certain order, so we can hard code conjugation for calculation
       // of the closure phase.
       // the order is different for software and hardware correlator. Just hard code the differences
       for (casacore::uInt validRow = 0; validRow<nRow; validRow+=3) {
            const casacore::uInt row = rowIndex[validRow];
            if (useSWCorrelator) {
                ASKAPCHECK(it->antenna2()[row] == it->antenna1()[row+1], "Expect baselines in the order 1-2,2-3 and 1-3");
                ASKAPCHECK(it->antenna1()[row] == it->antenna1()[row+2], "Expect baselines in the order 1-2,2-3 and 1-3");
                ASKAPCHECK(it->antenna2()[row+1] == it->antenna2()[row+2], "Expect baselines in the order 1-2,2-3 and 1-3");
             } else { /*
                if (useADECorrelator) {
                   ASKAPCHECK(it->antenna2()[row] == it->antenna2()[row+1], "Expect baselines in the order 4-5,4-12 and 5-12");
                   ASKAPCHECK(it->antenna1()[row] == it->antenna2()[row+2], "Expect baselines in the order 4-5,4-12 and 5-12");
                   ASKAPCHECK(it->antenna1()[row+1] == it->antenna1()[row+2], "Expect baselines in the order 4-5,4-12 and 5-12");
                } else { */
                   ASKAPCHECK(it->antenna2()[row] == it->antenna1()[row+2], "Expect baselines in the order 1-2,1-3 and 2-3");
                   ASKAPCHECK(it->antenna1()[row] == it->antenna1()[row+1], "Expect baselines in the order 1-2,1-3 and 2-3");
                   ASKAPCHECK(it->antenna2()[row+1] == it->antenna2()[row+2], "Expect baselines in the order 1-2,1-3 and 2-3");
                //}
             }
       }
       //
       
       // add new spectrum to the buffer
       for (casacore::uInt validRow=0; validRow<nRow; ++validRow) {
            const casacore::uInt row = rowIndex[validRow];
            casacore::Vector<casacore::Bool> flags = it->flag().xyPlane(cPol).row(row);
      
            bool flagged = false;
            /*
            // to ensure nothing is flagged
            for (casacore::uInt ch = 0; ch < flags.nelements(); ++ch) {
                 // to ensure nothing is flagged
                 flagged |= flags[ch];
            }
            */
            if (flagged) {
               ++nBadRows;
            } else {
                casacore::Vector<casacore::Complex> thisRow = buf.row(validRow);
                //thisRow += it->visibility().xyPlane(cPol).row(row);
                for (casacore::uInt ch = 0; ch<thisRow.nelements(); ++ch) {
                     if (!flags[ch]) {
                         thisRow[ch] += it->visibility()(row,ch,cPol);
                     }
                }
                ++nGoodRows;
            }
       }
       if ((counter == 0) && (nGoodRows == 0)) {
           // all data are flagged, completely ignoring this iteration and consider the next one to be first
           nChan = 0;
           continue;
       }
      
       if (++counter == 1) {
           startTime = it->time();
       }
       stopTime = it->time() + (useSWCorrelator ? 1 : 5); // 1s or 5s integration time is hardcoded
       /*
       if (counter == 3) {
           break;
       }
       */
  }
  if (counter!=0) {
      buf /= float(counter);
      std::cout<<"Averaged "<<counter<<" integration cycles, "<<nGoodRows<<" good and "<<nBadRows<<" bad rows, time span "<<(stopTime-startTime)/60.<<" minutues"<<std::endl;
      { // export averaged spectrum
        ASKAPDEBUGASSERT(freq.nelements() == nChan);
        std::ofstream os("avgspectrum.dat");
        for (casacore::uInt chan=0; chan<nChan; ++chan) {
             os<<chan<<" "<<freq[chan];
             for (casacore::uInt row=0; row<nRow; ++row) {
                  os<<" "<<casacore::abs(buf(row,chan))<<" "<<casacore::arg(buf(row,chan))/casacore::C::pi*180.;
             }
             os<<std::endl;
        }
      }

      ASKAPCHECK(buf.ncolumn()>0, "Need at least 1 spectral channel!");
      std::ofstream os("roughcalib.in");
      if (flux>0) {
          os<<"# amplitudes adjusted to match flux = "<<flux<<" Jy of the 'calibrator'"<<std::endl;
      } else {
          os<<"# all gain amplitudes are 1."<<std::endl;
      }
      for (casacore::uInt row = 0; row<buf.nrow(); row+=3) {
           ASKAPDEBUGASSERT(row+2<buf.nrow());
           casacore::Vector<casacore::Complex> spAvg(3,casacore::Complex(0.,0.));
           for (casacore::uInt baseline = 0; baseline<spAvg.nelements(); ++baseline) {
                casacore::Vector<casacore::Complex> thisRow = buf.row(row+baseline);
                //casacore::Vector<casacore::Complex> thisRow = buf.row(row+baseline)(casacore::Slice(38,32));
                spAvg[baseline] = casacore::sum(thisRow);
                spAvg[baseline] /= float(buf.ncolumn());
           }
           if (!useSWCorrelator) {
               // the hw-correlator has a different baseline order: 0-1, 0-2 and 1-2, we need to swap last two baselines to get 0-1,1-2,0-2 everywhere 
               const casacore::Complex tempBuf = spAvg[2];
               spAvg[2] = spAvg[1];
               spAvg[1] = tempBuf;
               /*
               if (useADECorrelator) {
                   // for ADE need to conjugate all; antenna order 4,5,12
                   for (casacore::uInt i=0; i<spAvg.nelements();++i) {
                        spAvg[i] = conj(spAvg[i]);
                   }
               }
               */
           }
           const float ph1 = -arg(spAvg[0]);
           const float ph2 = -arg(spAvg[2]);
           const float closurePh = arg(spAvg[0]*spAvg[1]*conj(spAvg[2]));
           
           const casacore::uInt beam = row/3;
           os<<"# Beam "<<beam<<" closure phase: "<<closurePh/casacore::C::pi*180.<<" deg"<<std::endl;
           std::string blStr = "(0-1,1-2,0-2)";
           if (useADECorrelator) {
               blStr = "(5-4,5-12,5-12)";
           }
               
           os<<"# measured phases              "<<blStr<<": "<<arg(spAvg[0])/casacore::C::pi*180.<<" "<<arg(spAvg[1])/casacore::C::pi*180.<<" "<<arg(spAvg[2])/casacore::C::pi*180.<<std::endl;
           os<<"# measured amplitudes          "<<blStr<<": "<<casacore::abs(spAvg[0])<<" "<<casacore::abs(spAvg[1])<<" "<<casacore::abs(spAvg[2])<<std::endl;
           float amp0 = 1.;
           float amp1 = 1.;
           float amp2 = 1.;
           if (flux > 0) {
               ASKAPCHECK((casacore::abs(spAvg[0])> 1e-6) && (casacore::abs(spAvg[1])> 1e-6) && (casacore::abs(spAvg[2])> 1e-6), "One of the measured amplitudes is too close to 0.: "<<spAvg);
               amp0 = sqrt(casacore::abs(spAvg[2]) * casacore::abs(spAvg[0]) / casacore::abs(spAvg[1]) / flux);
               amp1 = sqrt(casacore::abs(spAvg[1]) * casacore::abs(spAvg[0]) / casacore::abs(spAvg[2]) / flux);
               amp2 = sqrt(casacore::abs(spAvg[2]) * casacore::abs(spAvg[1]) / casacore::abs(spAvg[0]) / flux);
           }
           const casacore::Complex g0(amp0,0.);
           const casacore::Complex g1 = casacore::Complex(cos(ph1),sin(ph1)) * amp1;
           const casacore::Complex g2 = casacore::Complex(cos(ph2),sin(ph2)) * amp2;

           os<<"# phases after calibration     "<<blStr<<": "<<arg(spAvg[0]/g0/conj(g1))/casacore::C::pi*180.<<" "<<arg(spAvg[1]/g1/conj(g2))/casacore::C::pi*180.<<" "<<arg(spAvg[2]/g0/conj(g2))/casacore::C::pi*180.<<std::endl;
           os<<"# amplitudes after calibration "<<blStr<<": "<<casacore::abs(spAvg[0]/g0/conj(g1))<<" "<<casacore::abs(spAvg[1]/g1/conj(g2))<<" "<<casacore::abs(spAvg[2]/g0/conj(g2))<<std::endl;

           int baseAnt = 0;
           if (useADECorrelator) {
               baseAnt = 3;
           }
           os<<"gain.g11."<<baseAnt<<"."<<beam<<" = "<<printComplex(g0)<<std::endl;
           os<<"gain.g22."<<baseAnt<<"."<<beam<<" = "<<printComplex(g0)<<std::endl;
           os<<"gain.g11."<<baseAnt+1<<"."<<beam<<" = "<<printComplex(g1)<<std::endl;
           os<<"gain.g22."<<baseAnt+1<<"."<<beam<<" = "<<printComplex(g1)<<std::endl;
           os<<"gain.g11."<<baseAnt+2<<"."<<beam<<" = "<<printComplex(g2)<<std::endl;
           os<<"gain.g22."<<baseAnt+2<<"."<<beam<<" = "<<printComplex(g2)<<std::endl;
      }
  } else {
     std::cout<<"No data found!"<<std::endl;
  }
}


int main(int argc, char **argv) {
  try {
     if ((argc!=2) && (argc!=3)) {
         cerr<<"Usage: "<<argv[0]<<" [flux] measurement_set"<<endl;
	 return -2;
     }

     casacore::Timer timer;
     const std::string msName = argv[argc - 1];
     const float flux = argc == 2 ? -1 : utility::fromString<float>(argv[1]);

     timer.mark();
     TableDataSource ds(msName,TableDataSource::MEMORY_BUFFERS);     
     std::cerr<<"Initialization: "<<timer.real()<<std::endl;
     timer.mark();
     process(ds,flux);
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

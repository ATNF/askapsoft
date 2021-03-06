/// @file
/// @brief an utility to extract delays for averaged measurement set produced by sw-correlation
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

void process(const IConstDataSource &ds, const int ctrl = -1) {
  IDataSelectorPtr sel=ds.createSelector();
  sel->chooseFeed(0);
  sel->chooseAntenna(0);
  sel->chooseCrossCorrelations();
  //sel->chooseAutoCorrelations();
  if (ctrl >=0 ) {
      //sel->chooseUserDefinedIndex("CONTROL",casa::uInt(ctrl));
      sel->chooseUserDefinedIndex("SCAN_NUMBER",casa::uInt(ctrl));
  }
  IDataConverterPtr conv=ds.createConverter();  
  conv->setFrequencyFrame(casa::MFrequency::Ref(casa::MFrequency::TOPO),"MHz");
  conv->setEpochFrame(casa::MEpoch(casa::Quantity(55913.0,"d"),
                      casa::MEpoch::Ref(casa::MEpoch::UTC)),"s");
  conv->setDirectionFrame(casa::MDirection::Ref(casa::MDirection::J2000));                    
  casa::Matrix<casa::Complex> buf;
  casa::Matrix<casa::Complex> buf2;
  casa::Vector<double> freq;
  size_t counter = 0;
  size_t nGoodRows = 0;
  size_t nBadRows = 0;
  casa::uInt nChan = 0;
  casa::uInt nRow = 0;
  double startTime = 0;
  double stopTime = 0;
  double timeIntervalInMin = 0.;
  
  casa::Vector<casa::uInt> ant1ids, ant2ids;
  casa::Vector<casa::uInt> nGoodRowsThisProduct;

  scimath::DelayEstimator de(1e6);
    
  std::ofstream os2("avgts.dat");  
  std::ofstream os3("delayts.dat");
  bool firstTimeStamp = true;
  for (IConstDataSharedIter it=ds.createConstIterator(sel,conv);it!=it.end();++it, ++counter) {  
       if (firstTimeStamp) {
           startTime = it->time();
           firstTimeStamp = false;
       }
       stopTime = it->time() + 5; // 1s integration time is hardcoded

       if (nChan == 0) {
           nChan = it->nChannel();
           nRow = it->nRow();
           buf.resize(nRow,nChan);
           buf.set(casa::Complex(0.,0.));
           buf2.resize(nRow,nChan);
           buf2.set(casa::Complex(0.,0.));
           nGoodRowsThisProduct.resize(nRow);
           nGoodRowsThisProduct.set(0u);
           freq = it->frequency();
           ant1ids = it->antenna1();
           ant2ids = it->antenna2();
           std::cout<<"Baseline order is as follows: "<<std::endl;
           for (casa::uInt row = 0; row<nRow; ++row) {
                std::cout<<"baseline (1-based) = "<<row+1<<" is "<<ant1ids[row]<<" - "<<ant2ids[row]<<std::endl; 
           }           
       } else { 
           ASKAPCHECK(nChan == it->nChannel(), 
                  "Number of channels seem to have been changed, previously "<<nChan<<" now "<<it->nChannel());
           if (nRow != it->nRow()) {
               std::cerr<<"Number of rows changed was "<<nRow<<" now "<<it->nRow()<<std::endl;
               // reset averaging, for simplicity skip this integration too, although it may be good
               nChan = 0;
               nGoodRows = 0;
               nBadRows = 0;
               counter = 0;
               continue;
           }
           ASKAPCHECK(nRow == it->nRow(), 
                  "Number of rows seem to have been changed, previously "<<nRow<<" now "<<it->nRow());
       }
       
       ASKAPASSERT(it->nPol() >= 1);
       ASKAPASSERT(it->nChannel() > 1);
       // check that the products come in consistent way across the interations
       for (casa::uInt row = 0; row<nRow; ++row) {
            ASKAPCHECK(it->antenna1()[row] == ant1ids[row], "Inconsistent antenna 1 ids at row = "<<row);
            ASKAPCHECK(it->antenna2()[row] == ant2ids[row], "Inconsistent antenna 2 ids at row = "<<row);             
       }
       
       // add new spectrum to the buffer
       const casa::uInt pol2use = 0;
       ASKAPDEBUGASSERT(pol2use < it->nPol());

       if (counter == 0) {
           os3<<"0.0 ";
       } else {
           os3<< (it->time() - startTime)/60.;
       }

       bool somethingFlaggedThisTimestamp = false;

       for (casa::uInt row=0; row<nRow; ++row) {
            
          
            casa::Vector<casa::Bool> flags = it->flag().xyPlane(pol2use).row(row);
            bool flagged = false;
            bool allFlagged = true;
            for (casa::uInt ch = 0; ch < flags.nelements(); ++ch) {
                 flagged |= flags[ch];
                 allFlagged &= flags[ch];
            }
            
            casa::Vector<casa::Complex> measuredRow = it->visibility().xyPlane(pol2use).row(row);
            

            // delay estimate for the given row
            
            de.setResolution(1e6/54.);
            const double coarseDelay = de.getDelayWithFFT(measuredRow);
            de.setResolution(1e6);
            
            casa::Vector<casa::Complex> reducedResVis(measuredRow.nelements() / 54, casa::Complex(0.,0.));
            for (casa::uInt ch=0; ch < reducedResVis.nelements(); ++ch) {
                 casa::Complex sum(0.,0.);
                 for (casa::uInt fineCh = 0; fineCh < 54; ++fineCh) {
                      const casa::uInt chan = ch * 54 + fineCh;
                      ASKAPDEBUGASSERT(chan < measuredRow.nelements());
                      const float phase =  -casa::C::_2pi * (float(chan) / 54. * 1e6) * coarseDelay;
                      const casa::Complex phasor(cos(phase), sin(phase));
                      sum += measuredRow[chan] * phasor;
                 }
                 reducedResVis[ch] = sum / float(54.);
            }
            const double curDelay  = (coarseDelay + de.getDelay(reducedResVis)) * 1e9;
            
            //if (row >= 3) {
            //if (row >= 12) {
            {
                casa::Complex sum(0.,0.);
                size_t nGoodCh = 0;
                for (casa::uInt ch=0; ch < measuredRow.nelements(); ++ch) {
                     ASKAPDEBUGASSERT(ch < flags.nelements());
                     if (!flags[ch]) {
                         ++nGoodCh;
                         sum += measuredRow[ch];
                     }
                }
                const float curPhase  = nGoodCh == 0 ? 0. : arg(sum / float(nGoodCh)) / casa::C::pi *180.;
                const float curAmp  = nGoodCh == 0 ? 0. : abs(sum / float(nGoodCh));
                os3<<" "<<curAmp<<" "<<curPhase<<" "<<curDelay;
            }


            /*
            if ((counter == 10) && (row==12)) {
                std::ofstream os4("testsp.dat");
                for (casa::uInt ch=0; ch<measuredRow.nelements(); ++ch) {
                     os4<<ch<<" "<<arg(measuredRow[ch])/casa::C::pi*180.<<" "<<arg(reducedResVis[ch/54])/casa::C::pi*180.<<std::endl;
                }
            }
            */
            
            // flagging based on the amplitude (to remove extreme outliers)
            //casa::Complex currentAvgVis = casa::sum(measuredRow) / float(it->nChannel());
            
            /*
            if ((casa::abs(currentAvgVis) > 0.5) && (row % 3 == 2)) {
                flagged = true;
            } 
            */
            
            /*
            // optional flagging based on time-range
            if ((counter>1) && ((it->time() - startTime)/60.>1050.)) {
                flagged = true;
            }
            */
            
            /*
            // uncomment to store the actual amplitude time-series
            if ((counter>1) && (row % 3 == 0)) {
                os2<<counter<<" "<<(it->time() - startTime)/60.<<" "<<casa::abs(currentAvgVis)<<std::endl;
            }
            */
            //

            
            // to disable flagging
            //flagged = false; 
            flagged = allFlagged; 
            

            if (flagged) {
               ++nBadRows;
               somethingFlaggedThisTimestamp = true;
            } else {
                casa::Vector<casa::Complex> thisRow = buf.row(row);
                for (casa::uInt ch = 0; ch<thisRow.nelements(); ++ch) {
                     if (!flags[ch]) {
                         thisRow[ch] += measuredRow[ch];
                         buf2(row,ch) += casa::Complex(casa::square(casa::real(measuredRow[ch])), casa::square(casa::imag(measuredRow[ch])));
                     }
                }
                ++nGoodRows;
                ++nGoodRowsThisProduct[row];


                // uncomment to store averaged time-series
                if ((counter>1) && (row % 15 == 0) && (it->feed1()[row] == 0)) {
                    timeIntervalInMin += 1./12.;
                    const casa::Vector<casa::Complex> currentSpectrum = thisRow.copy() / float(counter);                    
                    const casa::Complex avgVis = casa::sum(currentSpectrum) / float(currentSpectrum.nelements());
                    casa::Complex avgSqr(0.,0.);
                    for (casa::uInt ch = 0; ch<currentSpectrum.nelements(); ++ch) {
                         avgSqr += casa::Complex(casa::square(casa::real(currentSpectrum[ch])),casa::square(casa::imag(currentSpectrum[ch])));                         
                    }   
                    avgSqr /= float(currentSpectrum.nelements());
                    const float varReal = casa::real(avgSqr) - casa::square(casa::real(avgVis)); 
                    const float varImag = casa::imag(avgSqr) - casa::square(casa::imag(avgVis)); 
                     
                    const double intervalInMin = (it->time() - startTime)/60.;


                    os2<<counter<<" "<<intervalInMin<<" "<<1/sqrt(timeIntervalInMin)<<" "<<casa::real(avgVis)<<" "<<
                         sqrt(varReal)<<" "<<casa::imag(avgVis)<<" "<<sqrt(varImag)<<std::endl;
                }               
            }
       }
       if (somethingFlaggedThisTimestamp) {
           os3<<" flagged";
       }
       os3<<std::endl;
       if ((counter == 0) && (nGoodRows == 0)) {
           // all data are flagged, completely ignoring this iteration and consider the next one to be first
           nChan = 0;
           nBadRows = 0;
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
       
       /*
       // optionally break integration after given time
       if ((counter > 1) && ((it->time() - startTime)/60. >= 10.)) {
       //if (max(nGoodRowsThisProduct) > 0) {
           break;
       }
       */
       
  }
  if (counter>1) {
      casa::uInt nNoDataProducts = 0;
      for (casa::uInt row=0; row < nRow; ++row) {
           if (nGoodRowsThisProduct[row] > 0) {
               for (casa::uInt chan = 0; chan < nChan; ++chan) {
                    buf(row,chan) /= float(nGoodRowsThisProduct[row]);
                    buf2(row,chan) /= float(nGoodRowsThisProduct[row]);
               }
           } else {
              ++nNoDataProducts;
           }
      }
      std::cout<<"Averaged maximum of "<<max(nGoodRowsThisProduct)<<" integration cycles, "<<nGoodRows<<" good and "<<nBadRows<<" bad rows, time span "<<(stopTime-startTime)/60.<<" minutues, cycles="<<counter<<std::endl;
      casa::MVEpoch startEpoch(casa::Quantity(55913.0,"d"));
      startEpoch += casa::MVEpoch(casa::Quantity(startTime, "s"));
      std::cout<<"Start time "<<startEpoch<<std::endl;

      { // export averaged spectrum
        ASKAPDEBUGASSERT(freq.nelements() == nChan);
        std::ofstream os("avgspectrum.dat");
        for (casa::uInt chan=0; chan<nChan; ++chan) {
             os<<chan<<" "<<freq[chan];
             for (casa::uInt row=0; row<nRow; ++row) {
                  const float varReal = casa::real(buf2(row,chan)) - casa::square(casa::real(buf(row,chan))); 
                  const float varImag = casa::imag(buf2(row,chan)) - casa::square(casa::imag(buf(row,chan))); 
                  os<<" "<<casa::abs(buf(row,chan))<<" "<<casa::arg(buf(row,chan))/casa::C::pi*180.<<" "<<sqrt(varReal + varImag)<<" ";
             }
             os<<std::endl;
        }
      }
      // delay estimate
      casa::Vector<casa::Float> delays =  swcorrelator::BasicMonitor::estimateDelays(buf);
      for (casa::uInt row = 0; row<delays.nelements(); ++row) {
           std::cout<<"row="<<row<<" delay = "<<delays[row]*1e9<<" ns or "<<delays[row]*1e9/1.3<<" DRx samples"<<std::endl;
      }
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

/// @file
/// @brief utility to make an image demonstrating fringes for sw-correlation experiment
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


#include <askap/dataaccess/TableDataSource.h>
#include <askap/askap_accessors.h>
#include <askap/askap/AskapLogging.h>
ASKAP_LOGGER(logger, "");

#include <askap/askap/AskapError.h>
#include <askap/dataaccess/SharedIter.h>

#include <askap/dataaccess/TableManager.h>
#include <askap/dataaccess/IDataConverterImpl.h>
#include <askap/scimath/fft/FFTWrapper.h>

// casa
#include <casacore/measures/Measures/MFrequency.h>
#include <casacore/tables/Tables/Table.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/Arrays/MatrixMath.h>
#include <askap/scimath/utils/ImageUtils.h>
#include <casacore/casa/Arrays/Cube.h>




// std
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;

using namespace askap;
using namespace askap::accessors;

void analyseDelay(const casacore::Matrix<casacore::Complex> &fringes, const casacore::uInt padding, double avgTime, 
                  const accessors::IConstDataAccessor &acc)
{
  ASKAPDEBUGASSERT(acc.nRow() == fringes.ncolumn());
  ASKAPDEBUGASSERT(acc.nChannel() * padding == fringes.nrow());
  for (casacore::uInt row = 0; row < acc.nRow(); ++row) {
       
  }
}

casacore::Matrix<casacore::Complex> flagOutliers(const casacore::Matrix<casacore::Complex> &in) {
  return in;
  casacore::Matrix<casacore::Complex> result(in);
  for (casacore::uInt row=0;row<result.nrow(); ++row) {
       for (casacore::uInt col=0; col<result.ncolumn(); ++col) {
            if (casacore::abs(result(row,col))>1) {
                result(row,col) = 0.;
            }
       }
  }
  return result;
}

casacore::Matrix<casacore::Complex> padSecond(const casacore::Matrix<casacore::Complex> &in, const casacore::uInt factor) {
   if (factor == 1) {
       return in;
   }
   ASKAPDEBUGASSERT(factor>0);
   ASKAPDEBUGASSERT(in.ncolumn()>0);
   ASKAPDEBUGASSERT(in.nrow()>0);
   casacore::Matrix<casacore::Complex> result(in.nrow(), in.ncolumn()*factor,casacore::Complex(0.,0.));
   const casacore::uInt start = in.ncolumn()*(factor-1)/2;
   result(casacore::IPosition(2,0,start), casacore::IPosition(2, in.nrow() - 1, start + in.ncolumn() - 1)) = in;
   return result;
}

void process(const IConstDataSource &ds, size_t nAvg, size_t padding = 1) {
  IDataSelectorPtr sel=ds.createSelector();
  //sel->chooseBaseline(0,1);
  sel->chooseCrossCorrelations();
  sel->chooseFeed(0);
  IDataConverterPtr conv=ds.createConverter();  
  conv->setFrequencyFrame(casacore::MFrequency::Ref(casacore::MFrequency::TOPO),"MHz");
  conv->setEpochFrame(casacore::MEpoch(casacore::Quantity(56150.0,"d"),
                      casacore::MEpoch::Ref(casacore::MEpoch::UTC)),"s");
  conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));                    
  casacore::Matrix<casacore::Complex> buf;
  double avgTime = 0.;
  size_t counter = 0;
  casacore::Cube<casacore::Complex> imgBuf;
  const casacore::uInt maxSteps = 2000;
  casacore::uInt currentStep = 0;
  casacore::Vector<casacore::uInt> ant1IDs;
  casacore::Vector<casacore::uInt> ant2IDs;
    
  for (IConstDataSharedIter it=ds.createConstIterator(sel,conv);it!=it.end();++it) {  
       if (buf.nelements() == 0) {
           buf.resize(it->nRow(),it->frequency().nelements()*padding);
           buf.set(casacore::Complex(0.,0.));
           ant1IDs = it->antenna1().copy();
           ant2IDs = it->antenna2().copy();
           for (casacore::uInt row = 0; row<it->nRow();++row) {
                std::cout<<"plane "<<row<<" corresponds to "<<ant1IDs[row]<<" - "<<ant2IDs[row]<<" baseline"<<std::endl;
           }
           imgBuf.resize(buf.ncolumn(),maxSteps,it->nRow());
           imgBuf.set(casacore::Complex(0.,0.));
       } else { 
           ASKAPCHECK(buf.ncolumn() == padding*it->frequency().nelements(), 
                  "Number of channels seem to have been changed, previously "<<buf.ncolumn()<<" now "<<it->frequency().nelements());
           if (imgBuf.nplane() != it->nRow()) {
               std::cerr << "The number of rows in the accessor is "<<it->nRow()<<", previously "<<imgBuf.nplane()<<" - ignoring"<<std::endl;
               continue;
           }
           ASKAPCHECK(imgBuf.nplane() == it->nRow(), "The number of rows in the accessor "<<it->nRow()<<
                      " is different to the maximum number of baselines");
           ASKAPDEBUGASSERT(ant1IDs.nelements() == it->nRow());
           ASKAPDEBUGASSERT(ant2IDs.nelements() == it->nRow());
           for (casacore::uInt row = 0; row<it->nRow(); ++row) {
                ASKAPCHECK(ant1IDs[row] == it->antenna1()[row], "Mismatch of antenna 1 index for row "<<row<<
                           " - got "<<it->antenna1()[row]<<" expected "<<ant1IDs[row]);
                ASKAPCHECK(ant2IDs[row] == it->antenna2()[row], "Mismatch of antenna 2 index for row "<<row<<
                           " - got "<<it->antenna2()[row]<<" expected "<<ant2IDs[row]);
           }
       }
       ASKAPASSERT(it->nRow() == buf.nrow());
       ASKAPASSERT(it->nChannel()*padding == buf.ncolumn());
       ASKAPASSERT(it->nPol() >= 1);
       const casacore::uInt pol = 3;
       ASKAPASSERT(pol < it->nPol());
       buf += flagOutliers(padSecond(it->visibility().xyPlane(pol),padding));
       avgTime += it->time();
       if (++counter == nAvg) {
           buf /= float(nAvg);
           avgTime /= float(nAvg);
           for (casacore::uInt row = 0; row<buf.nrow(); ++row) {
                casacore::Vector<casacore::Complex> curRow = buf.row(row);
                //scimath::fft(curRow, true);
           }
           ASKAPCHECK(currentStep < imgBuf.ncolumn(), "Image buffer is too small (in time axis)");
           imgBuf.xzPlane(currentStep++) = casacore::transpose(buf);
           buf.set(casacore::Complex(0.,0.));
           avgTime = 0.;
           counter = 0;
       }
       //cout<<"time: "<<it->time()<<endl;
  }
  if (counter!=0) {
      buf /= float(counter);
      avgTime /= double(counter);
      for (casacore::uInt row = 0; row<buf.nrow(); ++row) {
           casacore::Vector<casacore::Complex> curRow = buf.row(row);
           //scimath::fft(curRow, true);
      }
      ASKAPCHECK(currentStep < imgBuf.ncolumn(), "Image buffer is too small (in time axis)");
      imgBuf.xzPlane(currentStep) = casacore::transpose(buf);
  } else if (currentStep > 0) {
      --currentStep;
  }
  std::cout<<imgBuf.shape()<<" "<<currentStep<<std::endl;
  scimath::saveAsCasaImage("fringe.img", casacore::amplitude(imgBuf(casacore::IPosition(3,0,0,0),
                 casacore::IPosition(3,imgBuf.nrow()-1,currentStep,imgBuf.nplane()-1))));
  //scimath::saveAsCasaImage("fringe.img", casacore::phase(imgBuf(casacore::IPosition(3,0,0,0),
  //               casacore::IPosition(3,imgBuf.nrow()-1,currentStep,imgBuf.nplane()-1))));
  // exporting first row into a dat file
  if ((currentStep>0) || (counter!=0)) {
      std::ofstream os("fringe.dat");
      for (casacore::uInt chan=0; chan<imgBuf.nrow(); ++chan) {
           os<<chan<<" ";
           for (casacore::uInt baseline_beam = 0; baseline_beam < imgBuf.nplane(); ++baseline_beam) {
                os<<" "<<casacore::abs(imgBuf(casacore::IPosition(3,chan,0,baseline_beam)))<<" "<<casacore::arg(imgBuf(casacore::IPosition(3,chan,0,baseline_beam)))*180./casacore::C::pi;
           }
           os<<std::endl;
      }     
  }
}


int main(int argc, char **argv) {
  try {
     if (argc!=2) {
         cerr<<"Usage "<<argv[0]<<" measurement_set"<<endl;
	 return -2;
     }

     casacore::Timer timer;

     timer.mark();
     TableDataSource ds(argv[1],TableDataSource::MEMORY_BUFFERS);     
     std::cerr<<"Initialization: "<<timer.real()<<std::endl;
     timer.mark();
     // number of cycles to average
     const size_t nAvg = 1;
     // padding factor
     const size_t padding = 1;
     process(ds, nAvg, padding);
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

/// @file
/// @brief an utility to extract holography measurement from the measurement set produced by sw-correlation
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
#include <askap/askap/AskapUtil.h>
ASKAP_LOGGER(logger, "");

#include <askap/askap/AskapError.h>
#include <askap/dataaccess/SharedIter.h>

#include <askap/dataaccess/TableManager.h>
#include <askap/dataaccess/IDataConverterImpl.h>
#include <swcorrelator/BasicMonitor.h>

// casa
#include <casacore/measures/Measures/MFrequency.h>
#include <casacore/tables/Tables/Table.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/casa/Arrays/MatrixMath.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/lattices/Lattices/ArrayLattice.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/LinearCoordinate.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>



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

const casacore::uInt refAnt = 1; // the one which doesn't move
const casacore::uInt maxMappedAnt = 5;
const casacore::uInt maxMappedBeam = 9;

// converts antenna index into plane index (i.e. bypasses the reference antenna
casacore::uInt getAntPlaneIndex(const casacore::uInt &in) {
  if (in<refAnt) {
      return in;
  }
  ASKAPDEBUGASSERT(in>0);
  return in - 1;
}

casacore::Matrix<casacore::Complex> processOnePoint(const IConstDataSource &ds, const int ctrl = -1) {
  IDataSelectorPtr sel=ds.createSelector();
  if (ctrl >=0 ) {
      //sel->chooseUserDefinedIndex("CONTROL",casacore::uInt(ctrl));
      sel->chooseUserDefinedIndex("SCAN_NUMBER",casacore::uInt(ctrl));
  }
  sel->chooseCrossCorrelations();
  //sel->chooseFeed(0);
  IDataConverterPtr conv=ds.createConverter();  
  conv->setFrequencyFrame(casacore::MFrequency::Ref(casacore::MFrequency::TOPO),"MHz");
  conv->setEpochFrame(casacore::MEpoch(casacore::Quantity(55913.0,"d"),
                      casacore::MEpoch::Ref(casacore::MEpoch::UTC)),"s");
  conv->setDirectionFrame(casacore::MDirection::Ref(casacore::MDirection::J2000));                    

  casacore::Matrix<casacore::Complex> result(maxMappedAnt,maxMappedBeam, casacore::Complex(0.,0.));
  casacore::Matrix<casacore::uInt>  counts(maxMappedAnt,maxMappedBeam, 0u);

  casacore::Vector<double> freq;
  size_t counter = 0;
  size_t nGoodRows = 0;
  size_t nBadRows = 0;
  casacore::uInt nChan = 0;
  casacore::uInt nRow = 0;
  double startTime = 0;
  double stopTime = 0;
    
  for (IConstDataSharedIter it=ds.createConstIterator(sel,conv);it!=it.end();++it) {  
       if (nChan == 0) {
           nChan = it->nChannel();
           nRow = it->nRow();
           freq = it->frequency();
       } else { 
           ASKAPCHECK(nChan == it->nChannel(), 
                  "Number of channels seem to have been changed, previously "<<nChan<<" now "<<it->nChannel());
       }
       
       ASKAPASSERT(it->nPol() >= 1);
       ASKAPASSERT(it->nChannel() > 1);
       
       // add new value to the buffer
       for (casacore::uInt row=0; row<nRow; ++row) {
            casacore::Vector<casacore::Bool> flags = it->flag().xyPlane(0).row(row);
            bool flagged = false;
            bool allFlagged = true;
            for (casacore::uInt ch = 0; ch < flags.nelements(); ++ch) {
                 //flagged |= flags[ch];
                 allFlagged &= flags[ch];
            }
            if (allFlagged) {
                flagged = true;
            }
            
            casacore::Vector<casacore::Complex> measuredRow = it->visibility().xyPlane(0).row(row);
            
            
            //casacore::Complex currentAvgVis = casacore::sum(measuredRow) / float(it->nChannel());
            ASKAPDEBUGASSERT(measuredRow.nelements() == flags.nelements());
            casacore::Complex currentAvgVis(0.,0.);
            casacore::uInt currentNChan = 0;
            for (casacore::uInt ch = 0;  ch < flags.nelements(); ++ch) {
                 if (!flags[ch]) {
                     currentAvgVis += measuredRow[ch];
                     ++currentNChan;
                 }
            }
            if (currentNChan > 0) {
                currentAvgVis /= float(currentNChan);
            } else {
                ASKAPASSERT(flagged);
            }

            /*
            if ((casacore::abs(currentAvgVis) > 0.05) && (row % 3 == 2)) {
                flagged = true;
            } 
            
            // optional flagging based on time-range
            if ((counter>1) && ((it->time() - startTime)/60.>1050.)) {
                flagged = true;
            }
            */
            
            // we have to discard rows which do not contain the reference antenna
            if ((it->antenna1()[row] != refAnt) && (it->antenna2()[row] != refAnt)) {
                flagged = true;
            }
            //
            
            if (flagged) {
               ++nBadRows;
            } else {

                ++nGoodRows;
                const casacore::uInt beam = it->feed1()[row];
                ASKAPDEBUGASSERT(beam == it->feed2()[row]);
                casacore::uInt ant = it->antenna2()[row];
                bool needConjugate = false;
                if (ant == refAnt) {
                    ant = it->antenna1()[row];
                    needConjugate = true;
                }
                ASKAPASSERT(ant != refAnt);
                const casacore::uInt planeIndex = getAntPlaneIndex(ant);
                ASKAPDEBUGASSERT(planeIndex < result.nrow());
                ASKAPDEBUGASSERT(beam < result.ncolumn());
                if (needConjugate) {
                   result(planeIndex,beam) += conj(currentAvgVis);
                } else {
                   result(planeIndex,beam) += currentAvgVis;
                }
                counts(planeIndex,beam) += 1;
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
       stopTime = it->time() + 1; // 1s integration time is hardcoded
  }
  if (counter>1) {
      for (casacore::uInt row=0; row<result.nrow(); ++row) {
           for (casacore::uInt col=0; col<result.ncolumn(); ++col) {
                if (counts(row,col) > 0) {
                    result(row,col) /= float(counts(row,col));
                }
           }
      }
      std::cout<<"Processed "<<counter<<" integration cycles for ctrl="<<ctrl<<", "<<nGoodRows<<" good and "<<nBadRows<<" bad rows, time span "<<(stopTime-startTime)/60.<<" minutues"<<std::endl;
  } else {
     std::cout<<"No data found for ctrl="<<ctrl<<std::endl;
  }
  return result;
}

void process(const IConstDataSource &ds, const casacore::uInt size) {
   const double resolutionInRad = 0.5 / 180. * casacore::C::pi; // in radians
   ASKAPDEBUGASSERT(size % 2 == 1);
   ASKAPDEBUGASSERT(size > 1);
   const int halfSize = (int(size) - 1) / 2;
   //const casacore::IPosition targetShape(4,int(size),int(size),maxMappedBeam,maxMappedAnt);
   const casacore::IPosition targetShape(3,int(size),int(size),maxMappedBeam*maxMappedAnt);
   casacore::Array<float> buf(targetShape,0.);

   int counter = 0;
   for (int x = -halfSize; x <= halfSize; ++x) {
       //const int dir = (x + halfSize) % 2 == 0 ? 1 : -1; // the first scan is in the increasing order
       const int dir = 1.; // always the same direction
       for (int y = -halfSize; y <= halfSize; ++y) {

            ++counter; // effectively a 1-based counter

            int planeCounter = 0;
            casacore::Matrix<casacore::Complex> result = processOnePoint(ds,counter-1);
            for (casacore::uInt ant = 0; ant < result.nrow(); ++ant) {
                 for (casacore::uInt beam = 0; beam < result.ncolumn(); ++beam,++planeCounter) {
                      //const casacore::IPosition curPos(4, x + halfSize, halfSize - y * dir,int(ant),int(beam));
                      ASKAPDEBUGASSERT(planeCounter < targetShape(2));
                      const casacore::IPosition curPos(3, x + halfSize, halfSize - y * dir,planeCounter);
                      buf(curPos) = casacore::abs(result(ant,beam));
                 }
            }
       }
   }

   // storing the image
   size_t nDim = buf.shape().nonDegenerate().nelements();
   ASKAPASSERT(nDim>=2);
      
   casacore::Matrix<double> xform(2,2,0.);
   xform.diagonal() = 1.;
   casacore::DirectionCoordinate dc(casacore::MDirection::AZEL, casacore::Projection(casacore::Projection::SIN),0.,0.,
         resolutionInRad, -resolutionInRad, xform, double(halfSize), double(halfSize));
         
   casacore::CoordinateSystem coords;
   coords.addCoordinate(dc);
      
   for (size_t dim=2; dim<nDim; ++dim) {
        casacore::Vector<casacore::String> addname(1);
        if (dim == 2) {
            addname[0] = targetShape.nelements() == 4 ? "beam" : "";
        } else if (dim == 3) {
            addname[0] = "antenna";
        } else {
           addname[0]="addaxis"+utility::toString<size_t>(dim-3);
        }
        casacore::Matrix<double> xform(1,1,1.);
        casacore::LinearCoordinate lc(addname, addname,
        casacore::Vector<double>(1,0.), casacore::Vector<double>(1,1.),xform, 
            casacore::Vector<double>(1,0.));
        coords.addCoordinate(lc);
   }
   casacore::PagedImage<casacore::Float> resimg(casacore::TiledShape(buf.nonDegenerate().shape()), coords, "beammap.img");
   casacore::ArrayLattice<casacore::Float> lattice(buf.nonDegenerate());
   resimg.copyData(lattice);
}
   
int main(int argc, char **argv) {
  try {
     if (argc!=2) {
         cerr<<"Usage: "<<argv[0]<<" measurement_set"<<endl;
	 return -2;
     }

     casacore::Timer timer;
     const std::string msName = argv[1];

     timer.mark();
     TableDataSource ds(msName,TableDataSource::MEMORY_BUFFERS);     
     std::cerr<<"Initialization: "<<timer.real()<<std::endl;
     timer.mark();
     process(ds,9);
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

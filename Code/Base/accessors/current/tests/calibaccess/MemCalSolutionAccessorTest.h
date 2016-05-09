/// @file
///
/// Unit test for the memory-based implementation of the interface to access
/// calibration solutions. It is also used in the table-based implementation. 
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#include <casacore/casa/aipstype.h>
#include <cppunit/extensions/HelperMacros.h>

// own includes
#include <calibaccess/JonesIndex.h>
#include <calibaccess/MemCalSolutionAccessor.h>
#include <calibaccess/ICalSolutionFiller.h>
#include <askap/AskapUtil.h>


// boost includes
#include <boost/shared_ptr.hpp>


namespace askap {

namespace accessors {

class MemCalSolutionAccessorTest : public CppUnit::TestFixture,
                                   virtual public ICalSolutionFiller
{
   CPPUNIT_TEST_SUITE(MemCalSolutionAccessorTest);
   CPPUNIT_TEST(testRead);
   CPPUNIT_TEST(testCache);
   CPPUNIT_TEST(testWriteGains);
   CPPUNIT_TEST(testWriteLeakages);
   CPPUNIT_TEST(testWriteBandpasses);   
   CPPUNIT_TEST_EXCEPTION(testOverwriteROGains,AskapError);
   CPPUNIT_TEST_EXCEPTION(testOverwriteROLeakages,AskapError);
   CPPUNIT_TEST_EXCEPTION(testOverwriteROBandpasses,AskapError);
   CPPUNIT_TEST_EXCEPTION(testOverwriteXX,AskapError);
   CPPUNIT_TEST_EXCEPTION(testOverwriteXY,AskapError);
   CPPUNIT_TEST_EXCEPTION(testOverwriteBPElement,AskapError);   
   CPPUNIT_TEST_SUITE_END();
protected:
   static void fillCube(casa::Cube<casa::Complex> &cube) {
      for (casa::uInt row=0; row<cube.nrow(); ++row) {
           for (casa::uInt column=0; column<cube.ncolumn(); ++column) {
                for (casa::uInt plane=0; plane<cube.nplane(); ++plane) {
                     const float scale = (row / 2 + 1) * (row % 2 == 0 ? 1. : -1.);
                     cube(row,column,plane) = casa::Complex(scale*(float(column)/100. + float(plane)/10.),
                                                  -scale*(float(column)/100. + float(plane)/10.));
                }
           }
      }
   }
   
   void testValue(const casa::Complex &val, const JonesIndex &index, const casa::uInt row) const {
      const casa::uInt ant = casa::uInt(index.antenna());
      const casa::uInt beam = casa::uInt(index.beam());
      CPPUNIT_ASSERT(ant < itsNAnt);
      CPPUNIT_ASSERT(beam < itsNBeam);
      const float scale = (row / 2 + 1) * (row % 2 == 0 ? 1. : -1.);
      const casa::Complex expected(scale*(float(ant)/100. + float(beam)/10.),
                                                  -scale*(float(ant)/100. + float(beam)/10.));      
      CPPUNIT_ASSERT_DOUBLES_EQUAL(real(expected), real(val), 1e-6);
      CPPUNIT_ASSERT_DOUBLES_EQUAL(imag(expected), imag(val), 1e-6);      
   }
   
   boost::shared_ptr<ICalSolutionAccessor> initAccessor(const bool roFlag) {
      boost::shared_ptr<ICalSolutionFiller> csf(this, utility::NullDeleter());
      boost::shared_ptr<MemCalSolutionAccessor> acc(new MemCalSolutionAccessor(csf,roFlag));
      CPPUNIT_ASSERT(acc);
      return acc;
   }
   
public:
   void setUp() {
      itsNAnt = 36;
      itsNBeam = 30;
      itsNChan = 16;
      // flags showing  that write operation has taken place
      itsGainsWritten  = false;
      itsLeakagesWritten  = false;
      itsBandpassesWritten  = false;      
      // flags showing that read operation has taken place
      itsGainsRead  = false;
      itsLeakagesRead  = false;
      itsBandpassesRead  = false;            
   }

  // methods of the solution filler
  /// @brief gains filler  
  /// @details
  /// @param[in] gains pair of cubes with gains and validity flags (to be resised to 2 x nAnt x nBeam)
  virtual void fillGains(std::pair<casa::Cube<casa::Complex>, casa::Cube<casa::Bool> > &gains) const {
     gains.first.resize(2, itsNAnt, itsNBeam);
     gains.second.resize(2, itsNAnt, itsNBeam);
     gains.second.set(true);     
     fillCube(gains.first);
     itsGainsRead = true;
  }
  
  /// @brief leakage filler  
  /// @details
  /// @param[in] leakages pair of cubes with leakages and validity flags (to be resised to 2 x nAnt x nBeam)
  virtual void fillLeakages(std::pair<casa::Cube<casa::Complex>, casa::Cube<casa::Bool> > &leakages) const {
     leakages.first.resize(2, itsNAnt, itsNBeam);
     leakages.second.resize(2, itsNAnt, itsNBeam);
     leakages.second.set(true);     
     fillCube(leakages.first);
     itsLeakagesRead = true;
  }

  /// @brief bandpass filler  
  /// @details
  /// @param[in] bp pair of cubes with bandpasses and validity flags (to be resised to (2*nChan) x nAnt x nBeam)
  virtual void fillBandpasses(std::pair<casa::Cube<casa::Complex>, casa::Cube<casa::Bool> > &bp) const {
     bp.first.resize(2*itsNChan, itsNAnt, itsNBeam);
     bp.second.resize(2*itsNChan, itsNAnt, itsNBeam);
     bp.second.set(true);     
     fillCube(bp.first);  
     itsBandpassesRead = true;
  }
  
  /// @brief gains writer
  /// @details
  /// @param[in] gains pair of cubes with gains and validity flags (should be 2 x nAnt x nBeam)
  virtual void writeGains(const std::pair<casa::Cube<casa::Complex>, casa::Cube<casa::Bool> > &gains) const  {
     CPPUNIT_ASSERT(gains.first.shape() == gains.second.shape());
     CPPUNIT_ASSERT_EQUAL(size_t(2u),gains.first.nrow());
     CPPUNIT_ASSERT_EQUAL(size_t(itsNAnt),gains.first.ncolumn());
     CPPUNIT_ASSERT_EQUAL(size_t(itsNBeam),gains.first.nplane());      
     itsGainsWritten = true;
  }
  
  /// @brief leakage writer  
  /// @details
  /// @param[in] leakages pair of cubes with leakages and validity flags (should be 2 x nAnt x nBeam)
  virtual void writeLeakages(const std::pair<casa::Cube<casa::Complex>, casa::Cube<casa::Bool> > &leakages) const {
     CPPUNIT_ASSERT(leakages.first.shape() == leakages.second.shape());
     CPPUNIT_ASSERT_EQUAL(size_t(2u),leakages.first.nrow());
     CPPUNIT_ASSERT_EQUAL(size_t(itsNAnt),leakages.first.ncolumn());
     CPPUNIT_ASSERT_EQUAL(size_t(itsNBeam),leakages.first.nplane());      
     itsLeakagesWritten = true;  
  }

  /// @brief bandpass writer  
  /// @details
  /// @param[in] bp pair of cubes with bandpasses and validity flags (should be (2*nChan) x nAnt x nBeam)
  virtual void writeBandpasses(const std::pair<casa::Cube<casa::Complex>, casa::Cube<casa::Bool> > &bp) const {
     CPPUNIT_ASSERT(bp.first.shape() == bp.second.shape());
     CPPUNIT_ASSERT_EQUAL(size_t(2*itsNChan),bp.first.nrow());
     CPPUNIT_ASSERT_EQUAL(size_t(itsNAnt),bp.first.ncolumn());
     CPPUNIT_ASSERT_EQUAL(size_t(itsNBeam),bp.first.nplane());      
     itsBandpassesWritten = true;    
  }
   
  // test methods
  void testRead() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     CPPUNIT_ASSERT(!itsGainsRead);
     CPPUNIT_ASSERT(!itsLeakagesRead);
     CPPUNIT_ASSERT(!itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesJTerm gain = acc->gain(index);
               CPPUNIT_ASSERT(gain.g1IsValid());
               CPPUNIT_ASSERT(gain.g2IsValid());               
               testValue(gain.g1(),index,0);
               testValue(gain.g2(),index,1);               
          }
     }
     CPPUNIT_ASSERT(itsGainsRead);
     CPPUNIT_ASSERT(!itsLeakagesRead);
     CPPUNIT_ASSERT(!itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);

     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesDTerm leakage = acc->leakage(index);
               CPPUNIT_ASSERT(leakage.d12IsValid());
               CPPUNIT_ASSERT(leakage.d21IsValid());               
               testValue(leakage.d12(),index,0);
               testValue(leakage.d21(),index,1);               
          }
     }
     CPPUNIT_ASSERT(itsGainsRead);
     CPPUNIT_ASSERT(itsLeakagesRead);
     CPPUNIT_ASSERT(!itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);

     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               for (casa::uInt chan = 0; chan<itsNChan; ++chan) {
                    const JonesJTerm bp = acc->bandpass(index,chan);
                    CPPUNIT_ASSERT(bp.g1IsValid());
                    CPPUNIT_ASSERT(bp.g2IsValid());               
                    testValue(bp.g1(), index, 2 * chan);
                    testValue(bp.g2(), index, 2 * chan + 1);               
               }
          }
     }
     CPPUNIT_ASSERT(itsGainsRead);
     CPPUNIT_ASSERT(itsLeakagesRead);
     CPPUNIT_ASSERT(itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);
     // reset accessor and check that there was no write
     acc.reset();
     CPPUNIT_ASSERT(itsGainsRead);
     CPPUNIT_ASSERT(itsLeakagesRead);
     CPPUNIT_ASSERT(itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
  }
  
  void testCache() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);     
     // the following should read gains, bandpasses and leakages
     acc->jones(0,0,0);
     CPPUNIT_ASSERT(itsGainsRead);
     CPPUNIT_ASSERT(itsLeakagesRead);
     CPPUNIT_ASSERT(itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);
     // reset read flags
     itsGainsRead = false;
     itsLeakagesRead = false;
     itsBandpassesRead = false;
     // now read operation shouldn't happen because it has been done already
     acc->jones(0,0,0);
     CPPUNIT_ASSERT(!itsGainsRead);
     CPPUNIT_ASSERT(!itsLeakagesRead);
     CPPUNIT_ASSERT(!itsBandpassesRead);
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
  }
  
  void testWriteGains() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(false);
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesJTerm gains(casa::Complex(1.,-1.), (ant % 2 == 0), casa::Complex(-1.,1.), (beam % 2 == 0));
               acc->setGain(index,gains);
          }
      }
     CPPUNIT_ASSERT(itsGainsRead);
     CPPUNIT_ASSERT(!itsLeakagesRead);
     CPPUNIT_ASSERT(!itsBandpassesRead);
     // no write happened yet, the values are cached
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
     // check values
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesJTerm gain = acc->gain(index);
               CPPUNIT_ASSERT_EQUAL((ant % 2 == 0),gain.g1IsValid());
               CPPUNIT_ASSERT_EQUAL((beam % 2 == 0),gain.g2IsValid());               
               CPPUNIT_ASSERT_DOUBLES_EQUAL(1.,real(gain.g1()),1e-6);
               CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.,imag(gain.g1()),1e-6);
               CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.,real(gain.g2()),1e-6);
               CPPUNIT_ASSERT_DOUBLES_EQUAL(1.,imag(gain.g2()),1e-6);
          }
     }
     acc.reset();
     // now write should be done as the accessor has gone out of scope
     CPPUNIT_ASSERT(itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
  }

  void testWriteLeakages() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(false);
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesDTerm leakages(casa::Complex(1.,-1.), (ant % 2 == 0), casa::Complex(-1.,1.), (beam % 2 == 0));
               acc->setLeakage(index,leakages);
          }
      }
     CPPUNIT_ASSERT(!itsGainsRead);
     CPPUNIT_ASSERT(itsLeakagesRead);
     CPPUNIT_ASSERT(!itsBandpassesRead);
     // no write happened yet, the values are cached
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
     // check values
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesDTerm leakage = acc->leakage(index);
               CPPUNIT_ASSERT_EQUAL((ant % 2 == 0),leakage.d12IsValid());
               CPPUNIT_ASSERT_EQUAL((beam % 2 == 0),leakage.d21IsValid());               
               CPPUNIT_ASSERT_DOUBLES_EQUAL(1.,real(leakage.d12()),1e-6);
               CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.,imag(leakage.d12()),1e-6);
               CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.,real(leakage.d21()),1e-6);
               CPPUNIT_ASSERT_DOUBLES_EQUAL(1.,imag(leakage.d21()),1e-6);
          }
     }
     acc.reset();
     // now write should be done as the accessor has gone out of scope
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
  }

  void testWriteBandpasses() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(false);
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               const JonesJTerm bp(casa::Complex(1.,-1.), (ant % 2 == 0), casa::Complex(-1.,1.), (beam % 2 == 0));
               for (casa::uInt chan = 0; chan<itsNChan; chan+=2) {
                    acc->setBandpass(index,bp,chan);
               }
          }
     }
     CPPUNIT_ASSERT(!itsGainsRead);
     CPPUNIT_ASSERT(!itsLeakagesRead);
     CPPUNIT_ASSERT(itsBandpassesRead);
     // no write happened yet, the values are cached
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(!itsBandpassesWritten);     
     // check values
     for (casa::uInt ant = 0; ant<itsNAnt; ++ant) {
          for (casa::uInt beam = 0; beam<itsNBeam; ++beam) {
               const JonesIndex index(ant,beam);
               for (casa::uInt chan = 0; chan<itsNChan; ++chan) {
                    const JonesJTerm bp = acc->bandpass(index,chan);
                    if (chan % 2 == 0) {
                        CPPUNIT_ASSERT_EQUAL((ant % 2 == 0),bp.g1IsValid());
                        CPPUNIT_ASSERT_EQUAL((beam % 2 == 0),bp.g2IsValid());               
                        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.,real(bp.g1()),1e-6);
                        CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.,imag(bp.g1()),1e-6);
                        CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.,real(bp.g2()),1e-6);
                        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.,imag(bp.g2()),1e-6);
                    } else {
                        CPPUNIT_ASSERT(bp.g1IsValid());
                        CPPUNIT_ASSERT(bp.g2IsValid());               
                        testValue(bp.g1(), index, 2 * chan);
                        testValue(bp.g2(), index, 2 * chan + 1);                    
                    }
               }
          }
     }
     acc.reset();
     // now write should be done as the accessor has gone out of scope
     CPPUNIT_ASSERT(!itsGainsWritten);
     CPPUNIT_ASSERT(!itsLeakagesWritten);
     CPPUNIT_ASSERT(itsBandpassesWritten);     
  }
  
  void testOverwriteROGains() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     const JonesJTerm gain;
     acc->setGain(JonesIndex(0u,0u),gain);     
  }

  void testOverwriteROLeakages() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     const JonesDTerm leakage;
     acc->setLeakage(JonesIndex(0u,0u),leakage);     
  }

  void testOverwriteROBandpasses() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     const JonesJTerm gain;
     acc->setBandpass(JonesIndex(0u,0u),gain,0);     
  }
  
  void testOverwriteXX() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     try {
       acc->setJonesElement(0,0,casa::Stokes::XX, casa::Complex(0.));       
     }
     catch (const AskapError &) {
       // we should read the gains before write is attempted
       CPPUNIT_ASSERT(itsGainsRead); 
       throw;
     }
  }

  void testOverwriteXY() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     try {
       acc->setJonesElement(0,0,casa::Stokes::XY, casa::Complex(0.));       
     }
     catch (const AskapError &) {
       // we should read the leakages before write is attempted
       CPPUNIT_ASSERT(itsLeakagesRead); 
       throw;
     }
  }

  void testOverwriteBPElement() {
     boost::shared_ptr<ICalSolutionAccessor> acc = initAccessor(true);
     try {
       acc->setBandpassElement(0,0,casa::Stokes::XX, 0, casa::Complex(0.));       
     }
     catch (const AskapError &) {
       // we should read the bandpass before write is attempted
       CPPUNIT_ASSERT(itsBandpassesRead); 
       throw;
     }
  }
  
private:
  casa::uInt itsNAnt;
  casa::uInt itsNBeam;
  casa::uInt itsNChan; 
  mutable bool itsGainsWritten;
  mutable bool itsLeakagesWritten;
  mutable bool itsBandpassesWritten;
  mutable bool itsGainsRead;
  mutable bool itsLeakagesRead;
  mutable bool itsBandpassesRead;    
};

} // namespace accessors

} // namespace askap



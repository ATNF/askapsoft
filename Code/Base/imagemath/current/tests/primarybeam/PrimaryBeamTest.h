#include "primarybeam/PrimaryBeam.h"
#include "primarybeam/GaussianPB.h"
#include "primarybeam/PrimaryBeamFactory.h"
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/SpectralCoordinate.h>
#include <casacore/coordinates/Coordinates/LinearCoordinate.h>
#include <casacore/coordinates/Coordinates/StokesCoordinate.h>

#include <casacore/coordinates/Coordinates/CoordinateSystem.h>

#include <askap/AskapError.h>

#include <cppunit/extensions/HelperMacros.h>

#include <stdexcept>
#include <boost/shared_ptr.hpp>



using namespace casa;


namespace askap
{
  namespace imagemath
  {

    class PrimaryBeamTest : public CppUnit::TestFixture
    {

      CPPUNIT_TEST_SUITE(PrimaryBeamTest);

      CPPUNIT_TEST(testCreateGaussian);
      CPPUNIT_TEST_EXCEPTION(testCreateAbstract,AskapError);
      CPPUNIT_TEST(testEvaluateGaussian);
      CPPUNIT_TEST_SUITE_END();

  private:
      /// @brief method to access image for primary beam tests

      casa::CoordinateSystem makeCoords() {

          // Direction Coordinate
          casa::Matrix<casa::Double> xform(2,2);                                    // 1
          xform = 0.0; xform.diagonal() = 1.0;                          // 2
          casa::DirectionCoordinate radec(casa::MDirection::J2000,                  // 3
              casa::Projection(casa::Projection::SIN),        // 4
              135*casa::C::pi/180.0, 60*casa::C::pi/180.0,    // 5
              -0.005*casa::C::pi/180.0, 0.005*casa::C::pi/180,        // 6
              xform,                              // 7
              128.0, 128.0,                       // 8
              999.0, 999.0);

          casa::Vector<casa::String> units(2); units = "deg";                        //  9
          radec.setWorldAxisUnits(units);                               // 10

          casa::Vector<casa::Double> world(2), pixel(2);                            // 11
          pixel = 138.0;                                                // 12

          CPPUNIT_ASSERT(radec.toWorld(world, pixel));                        // 13
                                                                  // 17
          cout << world << " <--- " << pixel << endl;           // 18
          CPPUNIT_ASSERT(radec.toPixel(pixel, world));                             // 19

          cout << world << " ---> " << pixel << endl;

          // StokesCoordinate
          Vector<Int> iquv(4);                                         // 20
          iquv(0) = Stokes::I; iquv(1) = Stokes::Q;                    // 21
          iquv(2) = Stokes::U; iquv(3) = Stokes::V;                    // 22
          StokesCoordinate stokes(iquv);                               // 23
          Int plane;                                                   // 24
          CPPUNIT_ASSERT(stokes.toPixel(plane, Stokes::Q));                       // 25

          cout << "Stokes Q is plane " << plane << endl;

          CPPUNIT_ASSERT(stokes.toPixel(plane, Stokes::XX) == 0); // expecting to fail wrong Basis                      // 26



          // SpectralCoordinate
          SpectralCoordinate spectral(MFrequency::TOPO,               // 27
              1400 * 1.0E+6,                  // 28
              20 * 1.0E+3,                    // 29
              0,                              // 30
              1420.40575 * 1.0E+6);           // 31
          units.resize(1); pixel.resize(1); world.resize(1);
          units = "MHz";
          spectral.setWorldAxisUnits(units);

          pixel = 50;
          CPPUNIT_ASSERT(spectral.toWorld(world, pixel));

          cout << world << " <--- " << pixel << endl;

          CPPUNIT_ASSERT(spectral.toPixel(pixel, world));

          cout << world << " ---> " << pixel << endl;

          // CoordinateSystem
          CoordinateSystem coordsys;
          coordsys.addCoordinate(radec);
          // coordsys.addCoordinate(stokes);
          // coordsys.addCoordinate(spectral);


          return coordsys;
      }


  public:

      void setUp() {
          // create and write a constant into image

      }
      void tearDown() {
      }
      void testEvaluateGaussian() {
          LOFAR::ParameterSet parset;

          const casa::IPosition shape(2,256,256);
          casa::Array<float> arr(shape);
          arr.set(1.);
          casa::CoordinateSystem coordsys(makeCoords());


          casa::Vector<double> pixel(2,0.);
          casa::MVDirection world;
          double offsetBeam = 0;
          double frequency = 1.0E9; // 1GHz


          parset.add("aperture", "12");
          parset.add("fwhmscaling", "0.5");
          parset.add("primarybeam","GaussianPB");
          PrimaryBeam::ShPtr GaussPB =  PrimaryBeamFactory::make(parset);


          // reference direction of input coords
          casa::MVDirection beamcentre(135*C::pi/180.0, 60*C::pi/180.0);


          // get coordinates of the direction axes
          const int dcPos = coordsys.findCoordinate(casa::Coordinate::DIRECTION,-1);
          const casa::DirectionCoordinate outDC = coordsys.directionCoordinate(dcPos);

          casa::Array<float> BeamArr(shape);

          for (int x=0; x<shape[0];++x) {
              for (int y=0; y<shape[1];++y) {

                  // get the current pixel location and distance from beam centre
                  pixel[0] = double(x);
                  pixel[1] = double(y);
                  cout << " pixel --- " << pixel << endl;
                  outDC.toWorld(world,pixel);

                  cout << " world --- " << world << endl;
                  cout << " beamcentres -- " << beamcentre << endl;

                  offsetBeam = beamcentre.separation(world);
                  cout << " offset -- " << offsetBeam << endl;

                  double beamval = GaussPB->evaluateAtOffset(offsetBeam,frequency);
                  cout << " beamval --- " << beamval << endl;

                  const casa::IPosition index(2,int(x),int(y));
                  BeamArr(index) = beamval*beamval;


              }
          }
          const casa::IPosition index(2,200,200);
          double testVal = BeamArr(index);
          // 0.49583 is the val at 200,200
          pixel[0] = 200;
          pixel[1] = 200;
          outDC.toWorld(world,pixel);

          offsetBeam = beamcentre.separation(world);
          double beamval = GaussPB->evaluateAtOffset(offsetBeam,frequency);
          cout << "testVal: " << testVal << " beamVal: " << beamval*beamval << endl;

          CPPUNIT_ASSERT(abs(testVal - beamval*beamval) < 1E-7);


      }
      void testCreateGaussian()
      {

         LOFAR::ParameterSet parset;
         parset.add("aperture", "12");
         parset.add("fwhmscaling", "0.5");
         parset.add("primarybeam","GaussianPB");
         PrimaryBeam::ShPtr PB = PrimaryBeamFactory::make(parset);

      }
      void testCreateAbstract()
      {
          // PrimaryBeam is still an abstract class
          // calling createPriamryBeam static method should raise an
          // exception
          LOFAR::ParameterSet parset;
          PrimaryBeam::createPrimaryBeam(parset);

      }




  }; // class
  } // synthesis
} //askap

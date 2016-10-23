/// @file VisChunkTest.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include "askap/AskapError.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"
#include "Blob/BlobIStream.h"
#include "Blob/BlobIBufVector.h"
#include "Blob/BlobOStream.h"
#include "Blob/BlobOBufVector.h"

// Classes to test
#include "cpcommon/VisChunk.h"

using namespace askap::cp::common;
using namespace casa;

namespace askap {
namespace cp {

class VisChunkTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(VisChunkTest);
        CPPUNIT_TEST(testConstructor);
        CPPUNIT_TEST(testResizeChans);
        CPPUNIT_TEST(testResizeRows);
        CPPUNIT_TEST(testResizePols);
        CPPUNIT_TEST(testCopy);
        //CPPUNIT_TEST(testSerialize);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
        };

        void tearDown() {
        }

        void testConstructor() {
            VisChunk::ShPtr chunk(new VisChunk(nRows, nChans, nPols, nAnt));
            CPPUNIT_ASSERT_EQUAL(nRows, chunk->nRow());
            CPPUNIT_ASSERT_EQUAL(nChans, chunk->nChannel());
            CPPUNIT_ASSERT_EQUAL(nPols, chunk->nPol());

            // Verify visibility cube
            CPPUNIT_ASSERT_EQUAL(nRows, static_cast<unsigned int>(chunk->visibility().nrow()));
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(chunk->visibility().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(chunk->visibility().nplane()));

            // Verify flag cube
            CPPUNIT_ASSERT_EQUAL(nRows, static_cast<unsigned int>(chunk->flag().nrow()));
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(chunk->flag().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(chunk->flag().nplane()));

            // Verify frequency vector
            CPPUNIT_ASSERT_EQUAL(nChans,
                    static_cast<unsigned int>(chunk->frequency().size()));
        }

        void testResizeChans()
        {
            resizeDriver(nRows, nChans, nPols,
                    nRows, 304, nPols);
        }

        void testResizeRows()
        {
            CPPUNIT_ASSERT_THROW(
                    resizeDriver(nRows, nChans, nPols, nRows+1, nChans, nPols),
                    askap::AskapError);
        }

        void testResizePols()
        {
            CPPUNIT_ASSERT_THROW(
                    resizeDriver(nRows, nChans, nPols, nRows, nChans, nPols+1),
                    askap::AskapError);

        }


    private:
        void resizeDriver(const unsigned int initialRows,
                const unsigned int initialChans,
                const unsigned int initialPols,
                const unsigned int newRows,
                const unsigned int newChans,
                const unsigned int newPols)
        {
            VisChunk::ShPtr chunk(new VisChunk(initialRows, initialChans, initialPols, nAnt));

            // Create and assign the containers
            casa::Cube<casa::Complex> vis(newRows, newChans, newPols);
            casa::Cube<casa::Bool> flag(newRows, newChans, newPols);
            casa::Vector<casa::Double> frequency(newChans);
            chunk->resize(vis, flag, frequency);

            // Verify the result
            CPPUNIT_ASSERT_EQUAL(newRows, chunk->nRow());
            CPPUNIT_ASSERT_EQUAL(newChans, chunk->nChannel());
            CPPUNIT_ASSERT_EQUAL(newPols, chunk->nPol());

            // Verify visibility cube
            CPPUNIT_ASSERT_EQUAL(newRows, static_cast<unsigned int>(chunk->visibility().nrow()));
            CPPUNIT_ASSERT_EQUAL(newChans, static_cast<unsigned int>(chunk->visibility().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(newPols, static_cast<unsigned int>(chunk->visibility().nplane()));

            // Verify flag cube
            CPPUNIT_ASSERT_EQUAL(newRows, static_cast<unsigned int>(chunk->flag().nrow()));
            CPPUNIT_ASSERT_EQUAL(newChans, static_cast<unsigned int>(chunk->flag().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(newPols, static_cast<unsigned int>(chunk->flag().nplane()));

            // Verify frequency vector
            CPPUNIT_ASSERT_EQUAL(newChans,
                    static_cast<unsigned int>(chunk->frequency().size()));
        }

        void testCopy() {
            // setup some data
            VisChunk source(nRows, nChans, nPols, nAnt);
            source.flag().set(true);
            source.visibility().set(casa::Complex(2.048, -1.11)); 
            const casa::MVEpoch epoch(55902., 0.13);
            source.time()=epoch;
            source.targetName() = "Virgo";
            source.interval() = 5.;
            source.scan() = 1u;
            source.antenna1().set(3u);
            source.antenna2().set(4u);
            source.beam1().set(5u);
            source.beam2().set(6u);
            source.beam1PA().set(1.0);
            source.beam2PA().set(2.0);
            const casa::MVDirection dir(0.35, -0.85);
            source.phaseCentre().set(dir);
            source.targetPointingCentre().set(casa::MDirection(dir,casa::MDirection::J2000));
            const casa::MDirection dir1(casa::MVDirection(0.5, -0.7), casa::MDirection::J2000);
            source.actualPointingCentre().set(dir1);
            const casa::Quantity pa(92.668, "deg");
            source.actualPolAngle().set(pa);
            const casa::Quantity az(131.195, "deg");
            source.actualAzimuth().set(az);
            const casa::Quantity el(37.166, "deg");
            source.actualElevation().set(el);
            source.onSourceFlag().set(true);
            const casa::RigidVector<casa::Double, 3> uvw(-102.345, 12.333, 1.002);
            source.uvw().set(uvw);
            const double freq = 939.5e6;
            source.frequency().set(freq);
            const double resolution = 1e6/54;
            source.channelWidth() = resolution;
            source.stokes().set(casa::Stokes::XX);
            const casa::MDirection::Ref dirFrame(casa::MDirection::J2000);
            source.directionFrame() = dirFrame;
            

            // make a copy
            const VisChunk target(source);

            // corrupt the original container to test whether reference semantics has been
            // dealt with properly
            source.flag().set(false);
            source.visibility().set(casa::Complex(0.,0)); 
            source.time()=casa::MVEpoch(0., 0.);
            source.targetName() = "Junk";
            source.interval() = 10.;
            source.scan() = 0u;
            source.antenna1().set(0u);
            source.antenna2().set(0u);
            source.beam1().set(0u);
            source.beam2().set(0u);
            source.beam1PA().set(0.0);
            source.beam2PA().set(0.0);
            source.phaseCentre().set(casa::MVDirection(0.,0.));
            source.targetPointingCentre().set(casa::MVDirection(0.,0.));
            source.actualPointingCentre().set(casa::MVDirection(0.,0.));
            source.actualPolAngle().set(az);
            source.actualAzimuth().set(el);
            source.actualElevation().set(pa);
            source.onSourceFlag().set(false);
            source.uvw().set(casa::RigidVector<casa::Double, 3>());
            source.frequency().set(0.);
            source.channelWidth() = 0.01;
            source.stokes().set(casa::Stokes::RR);
            source.directionFrame() = casa::MDirection::Ref(casa::MDirection::AZEL);

            
            // test the result
           
            CPPUNIT_ASSERT_EQUAL(nRows, target.nRow());
            CPPUNIT_ASSERT_EQUAL(nChans, target.nChannel());
            CPPUNIT_ASSERT_EQUAL(nPols, target.nPol());
            CPPUNIT_ASSERT_EQUAL(nAnt, target.nAntenna());

            // Verify sizes for visibility cube
            CPPUNIT_ASSERT_EQUAL(nRows, static_cast<unsigned int>(target.visibility().nrow()));
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(target.visibility().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(target.visibility().nplane()));

            // Verify sizes flag cube
            CPPUNIT_ASSERT_EQUAL(nRows, static_cast<unsigned int>(target.flag().nrow()));
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(target.flag().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(target.flag().nplane()));

            // antenna-based parameters
            CPPUNIT_ASSERT_EQUAL(nAnt, static_cast<unsigned int>(target.actualPointingCentre().nelements()));
            CPPUNIT_ASSERT_EQUAL(nAnt, static_cast<unsigned int>(target.actualPolAngle().nelements()));
            CPPUNIT_ASSERT_EQUAL(nAnt, static_cast<unsigned int>(target.actualAzimuth().nelements()));
            CPPUNIT_ASSERT_EQUAL(nAnt, static_cast<unsigned int>(target.actualElevation().nelements()));
            CPPUNIT_ASSERT_EQUAL(nAnt, static_cast<unsigned int>(target.onSourceFlag().nelements()));
     
            // size of the frequency vector
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(target.frequency().nelements()));

            // size of the stokes vector
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(target.stokes().nelements()));
            

            // check content 
            checkCube(target.flag(), true);
            checkCube(target.visibility(), casa::Complex(2.048, -1.11));
            CPPUNIT_ASSERT(epoch.near(target.time()));
            CPPUNIT_ASSERT(target.targetName() == "Virgo");
            CPPUNIT_ASSERT_DOUBLES_EQUAL(5., target.interval(), 1e-6);
            CPPUNIT_ASSERT_EQUAL(1u, target.scan());
            checkVector(target.antenna1(), 3u);
            checkVector(target.antenna2(), 4u);
            checkVector(target.beam1(), 5u);
            checkVector(target.beam2(), 6u);
            checkVector(target.beam1PA(), float(1.0));
            checkVector(target.beam2PA(), float(2.0));
            checkVector(target.phaseCentre(), dir);
            checkVector(target.targetPointingCentre(), casa::MDirection(dir,casa::MDirection::J2000));
            checkVector(target.actualPolAngle(), pa);
            checkVector(target.actualAzimuth(), az);
            checkVector(target.actualElevation(), el);
            checkVector(target.onSourceFlag(), true);
            checkVector(target.uvw(), uvw);
            checkVector(target.frequency(), freq);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(resolution, target.channelWidth(), 1e-6);
            checkVector(target.stokes(), casa::Stokes::XX);
            CPPUNIT_ASSERT_EQUAL(dirFrame.getType(), target.directionFrame().getType());
        }

        
        /*
        // MV: commented out. It looks like the appropriate serialization operations
        // have never been implemented. Everything compiled because this particular
        // test was not plugged in
        void testSerialize() {
            VisChunk source(nRows, nChans, nPols, nAnt);
            VisChunk target(1, 1, 1, 1);

            // Encode
            std::vector<int8_t> buf;
            LOFAR::BlobOBufVector<int8_t> obv(buf, expandSize);
            LOFAR::BlobOStream out(obv);
            out.putStart("VisChunk", 1);
            out << source;
            out.putEnd();

            // Decode
            LOFAR::BlobIBufVector<int8_t> ibv(buf);
            LOFAR::BlobIStream in(ibv);
            int version = in.getStart("VisChunk");
            ASKAPASSERT(version == 1);
            in >> target;
            in.getEnd();

            CPPUNIT_ASSERT_EQUAL(nRows, target.nRow());
            CPPUNIT_ASSERT_EQUAL(nChans, target.nChannel());
            CPPUNIT_ASSERT_EQUAL(nPols, target.nPol());

            // Verify visibility cube
            CPPUNIT_ASSERT_EQUAL(nRows, static_cast<unsigned int>(target.visibility().nrow()));
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(target.visibility().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(target.visibility().nplane()));

            // Verify flag cube
            CPPUNIT_ASSERT_EQUAL(nRows, static_cast<unsigned int>(target.flag().nrow()));
            CPPUNIT_ASSERT_EQUAL(nChans, static_cast<unsigned int>(target.flag().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(nPols, static_cast<unsigned int>(target.flag().nplane()));

            // Verify frequency vector
            CPPUNIT_ASSERT_EQUAL(nChans,
                    static_cast<unsigned int>(target.frequency().size()));
        }
        */

        //
        // Test values
        //

        // This is the size of a BETA VisChunk, 21 baselines (including
        // auto correlations) * 9 beams (maximum number of beams)
        static const unsigned int nRows = 21 * 9;

        static const unsigned int nChans = 216;

        // Polarisations
        static const unsigned int nPols = 4;

        // number of antennas
        static const unsigned int nAnt = 6;

        // Expand size. Size of increment for Blob BufVector storage.
        // Too small and there is lots of overhead in expanding the vector.
        static const unsigned int expandSize = 4 * 1024 * 1024;
private:
        template<typename T>
        static void checkVal(const T& val1, const T& val2) {
            CPPUNIT_ASSERT_EQUAL(val2, val1);
        }

        static void checkVal(const double& val1, const double& val2) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(val2, val1, 1e-6);
        }

        static void checkVal(const float& val1, const float& val2) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(val2, val1, 1e-6);
        }

        static void checkVal(const casa::Complex& val1, const casa::Complex& val2) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(casa::real(val2), casa::real(val1), 1e-6);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(casa::imag(val2), casa::imag(val1), 1e-6);
        }

        static void checkVal(const casa::MVDirection& val1, const casa::MVDirection& val2) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(0., val1.separation(val2), 1e-6);
        }

        static void checkVal(const casa::MDirection& val1, const casa::MDirection& val2) {
            // we're only concerned with an exact match of two measures, don't need to cater
            // for the case where two directions in different frames actually point to the same
            // physical direction on the sky
            CPPUNIT_ASSERT_EQUAL(val1.getRef().getType(), val2.getRef().getType());
            CPPUNIT_ASSERT_DOUBLES_EQUAL(0., val1.getValue().separation(val2.getValue()), 1e-6);
        }

        static void checkVal(const casa::Quantity& val1, const casa::Quantity& val2) {
            CPPUNIT_ASSERT(val1.getFullUnit() == val2.getFullUnit());
            CPPUNIT_ASSERT_DOUBLES_EQUAL(val1.getValue(), val2.getValue(), 1e-6);
        }

        static void checkVal(const casa::RigidVector<casa::Double, 3> &val1,
                             const casa::RigidVector<casa::Double, 3> &val2) {
            CPPUNIT_ASSERT_DOUBLES_EQUAL(val1(0), val2(0), 1e-6);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(val1(1), val2(1), 1e-6);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(val1(2), val2(2), 1e-6);
        }

        template<typename T>
        static void checkCube(const casa::Cube<T> &in, const T& val) 
        {
           for (casa::uInt row = 0; row < in.nrow(); ++row) {
                for (casa::uInt column = 0; column < in.ncolumn(); ++column) {
                     for (casa::uInt plane = 0; plane < in.nplane(); ++plane) {
                          checkVal(in(row, column, plane), val);
                     }
                }
           }
        }

        template<typename T>
        static void checkVector(const casa::Vector<T> &in, const T& val) 
        {
           for (casa::uInt row = 0; row < in.nelements(); ++row) {
                checkVal(in[row], val);
           }
        }
};

const unsigned int VisChunkTest::nRows;
const unsigned int VisChunkTest::nChans;
const unsigned int VisChunkTest::nPols;
const unsigned int VisChunkTest::nAnt;

}   // End namespace cp

}   // End namespace askap


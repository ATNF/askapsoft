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

// Classes to test
#include "cpcommon/VisChunk.h"

using namespace casacore;
using askap::cp::common::VisChunk;

namespace askap {
namespace cp {
namespace ingest {

class VisChunkTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(VisChunkTest);
        CPPUNIT_TEST(testConstructor);
        CPPUNIT_TEST(testResizeChans);
        CPPUNIT_TEST(testResizeRows);
        CPPUNIT_TEST(testResizePols);
        CPPUNIT_TEST(testRawAccess);
        CPPUNIT_TEST_SUITE_END();

    public:
        void setUp() {
        };

        void tearDown() {
        }

        void testConstructor() {
            VisChunk::ShPtr chunk(new VisChunk(nRows, nChans, nPols, nAntennas));
            CPPUNIT_ASSERT_EQUAL(size_t(nRows), size_t(chunk->nRow()));
            CPPUNIT_ASSERT_EQUAL(size_t(nChans), size_t(chunk->nChannel()));
            CPPUNIT_ASSERT_EQUAL(size_t(nPols), size_t(chunk->nPol()));

            // Verify visibility cube
            CPPUNIT_ASSERT_EQUAL(size_t(nRows), size_t(chunk->visibility().nrow()));
            CPPUNIT_ASSERT_EQUAL(size_t(nChans), size_t(chunk->visibility().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(size_t(nPols), size_t(chunk->visibility().nplane()));

            // Verify flag cube
            CPPUNIT_ASSERT_EQUAL(size_t(nRows), size_t(chunk->flag().nrow()));
            CPPUNIT_ASSERT_EQUAL(size_t(nChans), size_t(chunk->flag().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(size_t(nPols), size_t(chunk->flag().nplane()));

            // Verify frequency vector
            CPPUNIT_ASSERT_EQUAL(nChans,
                                 static_cast<unsigned int>(chunk->frequency().size()));
        }

        void testRawAccess() {
            // unfortunately, we need low level access to large cubes for performance
            // (casacore's slicers do not provide an adequate solution)
            // This test method tests assumed data distribution as this particular 
            casacore::Matrix<casacore::uInt> mtr(3,5);
            for (casacore::uInt row=0; row<mtr.nrow(); ++row) {
                 for (casacore::uInt col=0; col<mtr.ncolumn(); ++col) {
                      mtr(row, col) = row * 10 + col;
                 }
            }
            CPPUNIT_ASSERT(mtr.contiguousStorage());
            casacore::uInt *ptr = mtr.data();
            for (casacore::uInt row=0; row<mtr.nrow(); ++row) {
                 for (casacore::uInt col=0; col<mtr.ncolumn(); ++col) {
                      casacore::uInt index = col * mtr.nrow() + row;
                      CPPUNIT_ASSERT_EQUAL(mtr(row, col), *(ptr + index));
                 }
            }
            // now test similar thing for a cube
            casacore::Cube<casacore::uInt> cube(3,5,7);
            for (casacore::uInt row=0; row<cube.nrow(); ++row) {
                 for (casacore::uInt col=0; col<cube.ncolumn(); ++col) {
                      for (casacore::uInt plane=0; plane<cube.nplane(); ++plane) {
                           cube(row, col, plane) = row * 100 + col * 10 + plane;
                      }
                 }
            }
            CPPUNIT_ASSERT(cube.contiguousStorage());
            CPPUNIT_ASSERT(!cube.yzPlane(0).contiguousStorage());
            casacore::uInt *ptr1 = cube.data();
            for (casacore::uInt row=0; row<cube.nrow(); ++row) {
                 for (casacore::uInt col=0; col<cube.ncolumn(); ++col) {
                      for (casacore::uInt plane=0; plane<cube.nplane(); ++plane) {
                           casacore::uInt index = (plane * cube.ncolumn() + col) * cube.nrow() + row;
                           CPPUNIT_ASSERT_EQUAL(cube(row, col, plane), *(ptr1 + index));
                      }
                 }
            }
            
        }

        void testResizeChans() {
            resizeDriver(nRows, nChans, nPols,
                         nRows, 304, nPols);
        }

        void testResizeRows() {
            CPPUNIT_ASSERT_THROW(
                resizeDriver(nRows, nChans, nPols, nRows + 1, nChans, nPols),
                askap::AskapError);
        }

        void testResizePols() {
            CPPUNIT_ASSERT_THROW(
                resizeDriver(nRows, nChans, nPols, nRows, nChans, nPols + 1),
                askap::AskapError);

        }


    private:
        void resizeDriver(const unsigned int initialRows,
                          const unsigned int initialChans,
                          const unsigned int initialPols,
                          const unsigned int newRows,
                          const unsigned int newChans,
                          const unsigned int newPols) {
            VisChunk::ShPtr chunk(new VisChunk(initialRows, initialChans, initialPols, nAntennas));

            // Create and assign the containers
            casacore::Cube<casacore::Complex> vis(newRows, newChans, newPols);
            casacore::Cube<casacore::Bool> flag(newRows, newChans, newPols);
            casacore::Vector<casacore::Double> frequency(newChans);
            chunk->resize(vis, flag, frequency);

            // Verify the result
            CPPUNIT_ASSERT_EQUAL(size_t(newRows), size_t(chunk->nRow()));
            CPPUNIT_ASSERT_EQUAL(size_t(newChans), size_t(chunk->nChannel()));
            CPPUNIT_ASSERT_EQUAL(size_t(newPols), size_t(chunk->nPol()));

            // Verify visibility cube
            CPPUNIT_ASSERT_EQUAL(size_t(newRows), size_t(chunk->visibility().nrow()));
            CPPUNIT_ASSERT_EQUAL(size_t(newChans), size_t(chunk->visibility().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(size_t(newPols), size_t(chunk->visibility().nplane()));

            // Verify flag cube
            CPPUNIT_ASSERT_EQUAL(size_t(newRows), size_t(chunk->flag().nrow()));
            CPPUNIT_ASSERT_EQUAL(size_t(newChans), size_t(chunk->flag().ncolumn()));
            CPPUNIT_ASSERT_EQUAL(size_t(newPols), size_t(chunk->flag().nplane()));

            // Verify frequency vector
            CPPUNIT_ASSERT_EQUAL(newChans,
                                 static_cast<unsigned int>(chunk->frequency().size()));
        }

        //
        // Test values
        //

        static const unsigned int nAntennas = 6;

        // This is the size of a BETA VisChunk, 21 baselines (including
        // auto correlations) * 36 beams (maximum number of beams)
        static const unsigned int nRows = 21 * 36;

        // 304 coarse channels with 54 fine channels per coarse
        static const unsigned int nChans = 54 * 304;

        // Polarisations
        static const unsigned int nPols = 4;
};

const unsigned int VisChunkTest::nRows;
const unsigned int VisChunkTest::nChans;
const unsigned int VisChunkTest::nPols;

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

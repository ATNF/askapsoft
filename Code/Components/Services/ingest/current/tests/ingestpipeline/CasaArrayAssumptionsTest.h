/// @file CasaArrayAssumptionsTest.cc
/// @details Ingest pipeline uses MPI and often had to pass storage 
/// by raw pointers. For complex types (e.g. multi-dimensional arrays)
/// this may cause problems when we change compilers and/or platform, or
/// if implementation details of the casacore types change. The unit tests
/// in this file assert the current assumptions (also handy to remember details).
///
/// @copyright (c) 2011 CSIRO
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

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include "casacore/casa/aips.h"
#include "configuration/Configuration.h"
#include "ConfigurationHelper.h"

// casacore includes
#include <casacore/casa/Arrays/Cube.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/BasicSL/Complex.h>

#include <inttypes.h>

using namespace casa;

namespace askap {
namespace cp {
namespace ingest {

class CasaArrayAssumptionsTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(CasaArrayAssumptionsTest);
        CPPUNIT_TEST(testTypeSizes);
        CPPUNIT_TEST(testCubeAxes);
        CPPUNIT_TEST(testTrimVector);
        CPPUNIT_TEST_SUITE_END();

    public:

        void testTypeSizes() {
           CPPUNIT_ASSERT_EQUAL(sizeof(char), sizeof(casa::Bool));
           CPPUNIT_ASSERT_EQUAL(2 * sizeof(uint32_t), sizeof(std::pair<casa::uInt, casa::uInt>));
           CPPUNIT_ASSERT_EQUAL(2 * sizeof(float), sizeof(casa::Complex));
        };
 
        void testCubeAxes() {
           casa::Cube<casa::Int> buffer(5,3,2,-1);
           const size_t size = buffer.nelements();
           // populate with unique value
           for (casa::uInt row = 0; row<buffer.nrow(); ++row) {
                for (casa::uInt column = 0; column<buffer.ncolumn(); ++column) {
                     for (casa::uInt plane = 0; plane<buffer.nplane(); ++plane) {
                          const casa::uInt val = (row * buffer.ncolumn() + column)* buffer.nplane() + plane;
                          //buffer(row,column,plane) = static_cast<casa::Int>(val);
                          // row is the fastest changing coordinate
                          const size_t index = (plane * buffer.ncolumn() + column)* buffer.nrow() + row;
                          CPPUNIT_ASSERT(index < size);
                          *(buffer.data() + index) = val;
                     }
                }
           }
           // test values
           for (casa::uInt row = 0; row<buffer.nrow(); ++row) {
                for (casa::uInt column = 0; column<buffer.ncolumn(); ++column) {
                     for (casa::uInt plane = 0; plane<buffer.nplane(); ++plane) {
                          const casa::uInt val = (row * buffer.ncolumn() + column )* buffer.nplane() + plane;
                          CPPUNIT_ASSERT_EQUAL(static_cast<casa::Int>(val), buffer(row,column,plane));
                     }
                }
           }
        }

        void testTrimVector() {
             casa::Vector<casa::Int> buffer(100);
             std::vector<int> stlBuffer(100);
             for (size_t i = 0; i < buffer.nelements(); ++i) {
                  buffer[i] = static_cast<casa::Int>(i);
                  stlBuffer[i] = static_cast<int>(i);
             }
             // the following doesn't seem to work without explicit copy (second argument set to true) -
             // so no performance benefit over direct copy which would give a cleaner code.
             buffer.resize(30, true);
             // it does work with STL vectors, though (presumably, without data copy)
             stlBuffer.resize(30);
             CPPUNIT_ASSERT(stlBuffer.size() == buffer.nelements());
             for (size_t i = 0; i < buffer.nelements(); ++i) {
                  CPPUNIT_ASSERT_EQUAL(static_cast<casa::Int>(i), buffer[i]);
                  CPPUNIT_ASSERT_EQUAL(static_cast<int>(i), stlBuffer[i]);
             }
        }
};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

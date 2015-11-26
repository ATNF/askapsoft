/// @file 
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

// casa stuff
#include "casa/OS/Path.h"

// Support classes
#include <string>

// Classes to test
#include "configuration/BaselineMap.h"

using namespace std;

namespace askap {
namespace cp {
namespace ingest {

class BaselineMapTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(BaselineMapTest);
        CPPUNIT_TEST(testLookup);
        CPPUNIT_TEST(testNoMatchAnt1);
        CPPUNIT_TEST(testNoMatchAnt2);
        CPPUNIT_TEST(testNoMatchPol);
        CPPUNIT_TEST(testSliceMap);
        CPPUNIT_TEST(testLowerTriangle);
        CPPUNIT_TEST(testDefaultMap);
        CPPUNIT_TEST_EXCEPTION(testUnknownMap, askap::CheckError);
        CPPUNIT_TEST_EXCEPTION(testMixedParam, askap::CheckError);
        CPPUNIT_TEST_SUITE_END();

    public:

        void testLookup() {
            LOFAR::ParameterSet params;
            params.add("baselineids","[0,1,4]");
            params.add("0","[0,0,XX]");
            params.add("1","[1,3,XY]");
            params.add("4","[3,1,YY]");
            BaselineMap bm(params);

            CPPUNIT_ASSERT_EQUAL(1, bm.getID(1,3,casa::Stokes::XY));
            CPPUNIT_ASSERT_EQUAL(4, bm.maxID());
            CPPUNIT_ASSERT_EQUAL(size_t(3), bm.size());
            CPPUNIT_ASSERT_EQUAL(0, bm.idToAntenna1(0));
            CPPUNIT_ASSERT_EQUAL(0, bm.idToAntenna2(0));
            CPPUNIT_ASSERT_EQUAL(casa::Stokes::XX, bm.idToStokes(0));
            CPPUNIT_ASSERT_EQUAL(1, bm.idToAntenna1(1));
            CPPUNIT_ASSERT_EQUAL(3, bm.idToAntenna2(1));
            CPPUNIT_ASSERT_EQUAL(casa::Stokes::XY, bm.idToStokes(1));
            CPPUNIT_ASSERT_EQUAL(3, bm.idToAntenna1(4));
            CPPUNIT_ASSERT_EQUAL(1, bm.idToAntenna2(4));
            CPPUNIT_ASSERT_EQUAL(casa::Stokes::YY, bm.idToStokes(4));

            CPPUNIT_ASSERT_EQUAL(0, testNoMatch(3,1,casa::Stokes::XX));
        };

        int32_t testNoMatch(const int32_t ant1, const int32_t ant2, const casa::Stokes::StokesTypes pol) {
            LOFAR::ParameterSet params;
            params.add("baselineids","[0]");
            params.add("0","[3,1,XX]");
            BaselineMap bm(params);

            CPPUNIT_ASSERT_EQUAL(0, bm.getID(3,1,casa::Stokes::XX));
            CPPUNIT_ASSERT_EQUAL(0, bm.maxID());
            CPPUNIT_ASSERT_EQUAL(size_t(1), bm.size());
            return bm.getID(ant1,ant2,pol);
        }
        
        void testNoMatchAnt1() {
            CPPUNIT_ASSERT_EQUAL(-1, testNoMatch(1, 1, casa::Stokes::XX));
        }

        void testNoMatchAnt2() {
            CPPUNIT_ASSERT_EQUAL(-1, testNoMatch(3,2,casa::Stokes::XX));
        }

        void testNoMatchPol() {
            CPPUNIT_ASSERT_EQUAL(-1, testNoMatch(3, 1, casa::Stokes::XY));
            CPPUNIT_ASSERT_EQUAL(-1, testNoMatch(3, 1, casa::Stokes::YY));
            CPPUNIT_ASSERT_EQUAL(-1, testNoMatch(3, 1, casa::Stokes::XY));
        }
        void testSliceMap() {
            LOFAR::ParameterSet params;
            // actual BETA3 configuration of correlation products
            params.add("baselineids","[1..21]");
            params.add("1","[0, 0, XX]");
            params.add("2","[0, 0, XY]");
            params.add("3","[0, 1, XX]");
            params.add("4","[0, 1, XY]");
            params.add("5","[0, 2, XX]");
            params.add("6","[0, 2, XY]");
            params.add("7","[0, 0, YY]");
            params.add("8","[0, 1, YX]");
            params.add("9","[0, 1, YY]");
            params.add("10","[0, 2, YX]");
            params.add("11","[0, 2, YY]");

            params.add("12","[1, 1, XX]");
            params.add("13","[1, 1, XY]");
            params.add("14","[1, 2, XX]");
            params.add("15","[1, 2, XY]");
            params.add("16","[1, 1, YY]");
            params.add("17","[1, 2, YX]");
            params.add("18","[1, 2, YY]");

            params.add("19","[2, 2, XX]");
            params.add("20","[2, 2, XY]");
            params.add("21","[2, 2, YY]");

            BaselineMap bm(params);

            CPPUNIT_ASSERT_EQUAL(size_t(21u), bm.size());
            CPPUNIT_ASSERT(!bm.isLowerTriangle());
            CPPUNIT_ASSERT(bm.isUpperTriangle());

            std::vector<int> ids(2);
            ids[0] = 0;
            ids[1] = 2;
            bm.sliceMap(ids);
            
            CPPUNIT_ASSERT_EQUAL(size_t(10u), bm.size());
            CPPUNIT_ASSERT(!bm.isLowerTriangle());
            CPPUNIT_ASSERT(bm.isUpperTriangle());
           
            CPPUNIT_ASSERT_EQUAL(21, bm.maxID());
            // check the first antenna
            const int ant1[21] = {0, 0, -1, -1, 0, 0, 0, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1};
            const int ant2[21] = {0, 0, -1, -1, 1, 1, 0, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1};
            const casa::Stokes::StokesTypes stokes[21] = {casa::Stokes::XX, casa::Stokes::XY, 
                      casa::Stokes::Undefined, casa::Stokes::Undefined, casa::Stokes::XX, casa::Stokes::XY, 
                      casa::Stokes::YY, casa::Stokes::Undefined, casa::Stokes::Undefined, casa::Stokes::YX,
                      casa::Stokes::YY,  casa::Stokes::Undefined, casa::Stokes::Undefined,  
                      casa::Stokes::Undefined, casa::Stokes::Undefined, casa::Stokes::Undefined, 
                      casa::Stokes::Undefined, casa::Stokes::Undefined, casa::Stokes::XX, 
                      casa::Stokes::XY, casa::Stokes::YY};
            for (int product = 0; product < 21; ++product) {
                 CPPUNIT_ASSERT_EQUAL(ant1[product], bm.idToAntenna1(product + 1));
                 CPPUNIT_ASSERT_EQUAL(ant2[product], bm.idToAntenna2(product + 1));
                 CPPUNIT_ASSERT_EQUAL(stokes[product], bm.idToStokes(product + 1));
            }
        }
     
        void testLowerTriangle() {
            // test of ADE-style of correlator product arrangement
            
            LOFAR::ParameterSet params;
            // the first 21 products of the ADE correlator
            params.add("baselineids","[1..21]");
            params.add("1","[0, 0, XX]");
            params.add("2","[0, 0, YX]");
            params.add("3","[0, 0, YY]");
            params.add("4","[1, 0, XX]");
            params.add("5","[1, 0, XY]");
            params.add("6","[1, 1, XX]");
            params.add("7","[1, 0, YX]");
            params.add("8","[1, 0, YY]");
            params.add("9","[1, 1, YX]");
            params.add("10","[1, 1, YY]");
            params.add("11","[2, 0, XX]");

            params.add("12","[2, 0, XY]");
            params.add("13","[2, 1, XX]");
            params.add("14","[2, 1, XY]");
            params.add("15","[2, 2, XX]");
            params.add("16","[2, 0, YX]");
            params.add("17","[2, 0, YY]");
            params.add("18","[2, 1, YX]");

            params.add("19","[2, 1, YY]");
            params.add("20","[2, 2, YX]");
            params.add("21","[2, 2, YY]");

            BaselineMap bm(params);

            CPPUNIT_ASSERT_EQUAL(size_t(21u), bm.size());
            CPPUNIT_ASSERT(bm.isLowerTriangle());
            CPPUNIT_ASSERT(!bm.isUpperTriangle());
        }
        void testDefaultMap() {
            // we store the test map in the same directory this file is
            const casa::Path thisFile(__FILE__);
            casa::Path path2map(thisFile.dirName());
            path2map.append("TestBaselineMap.parset");
            CPPUNIT_ASSERT(path2map.isValid());
            LOFAR::ParameterSet templateConfig(path2map.expandedName());
            BaselineMap bmTemplate(templateConfig.makeSubset("baselinemap."));
            CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2628u), bmTemplate.size());
            CPPUNIT_ASSERT_EQUAL(2628, bmTemplate.maxID());

            // set up default map
            LOFAR::ParameterSet params;
            params.add("name","standard");
            BaselineMap bm(params);
            
            // first test the number of elements
            CPPUNIT_ASSERT_EQUAL(bmTemplate.size(), bm.size());
            CPPUNIT_ASSERT_EQUAL(bmTemplate.maxID(), bm.maxID());
            
            // iterate over all products and check that they're the same
            // product 0 should be undefined, but access methods should just
            // return -1 or Stokes::Undefined for both the tested and template map
            for (int32_t id = 0; id <= bm.maxID(); ++id) {
                 CPPUNIT_ASSERT_EQUAL(bmTemplate.idToAntenna1(id), bm.idToAntenna1(id));
                 CPPUNIT_ASSERT_EQUAL(bmTemplate.idToAntenna2(id), bm.idToAntenna2(id));
                 CPPUNIT_ASSERT_EQUAL(bmTemplate.idToStokes(id), bm.idToStokes(id));
            }
        }
       
        void testUnknownMap() {
            // set up default map; only 'standard' name is defined, so
            // beta should throw an exception
            LOFAR::ParameterSet params;
            params.add("name","beta");
            BaselineMap bm(params);
        }
 
        void testMixedParam() {
            LOFAR::ParameterSet params;
            // the first 7 products of the ADE correlator
            params.add("baselineids","[1..7]");
            params.add("1","[0, 0, XX]");
            params.add("2","[0, 0, YX]");
            params.add("3","[0, 0, YY]");
            params.add("4","[1, 0, XX]");
            params.add("5","[1, 0, XY]");
            params.add("6","[1, 1, XX]");
            params.add("7","[1, 0, YX]");
        
            // this map should be alright
            BaselineMap bm1(params);
            CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(7u), bm1.size());
            CPPUNIT_ASSERT_EQUAL(7, bm1.maxID());
 
            // now add the default map which conflicts with explicit map
            params.add("name","standard");
            // the following should throw an exception
            BaselineMap bm(params);
        }
};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

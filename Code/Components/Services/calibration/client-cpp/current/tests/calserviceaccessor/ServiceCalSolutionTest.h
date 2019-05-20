/// @file
///
/// Unit test for the table-based implementation of the interface to access
/// calibration solutions
///
///
/// @copyright (c) 2017 CSIRO
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
/// @author Stephen Ord <stephen.ord@csiro.au>


#include <cppunit/extensions/HelperMacros.h>
#include <askap/calibaccess/JonesIndex.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <string>

namespace askap {

namespace accessors {


class ServiceCalSolutionTest : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(ServiceCalSolutionTest);
   CPPUNIT_TEST(testStub);
   CPPUNIT_TEST_SUITE_END();

protected:


public:

    void setUp() {
    /// spawn a thread to fire up the server

    }
    void tearDown() {
    /// shutdown the test server
    }

    void testStub() {
      CPPUNIT_ASSERT(true);

    }





};

} // namespace accessors

} // namespace askap

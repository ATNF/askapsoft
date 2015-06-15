/// @file VisConverterBaseTest.h
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>
/// based on Ben's code for MergedVisSource test

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <cmath>
#include <stdint.h>
#include "boost/shared_ptr.hpp"
#include "Common/ParameterSet.h"
#include "ingestpipeline/sourcetask/test/MockVisSource.h"
#include "cpcommon/VisDatagram.h"
#include "cpcommon/VisChunk.h"
#include "ConfigurationHelper.h"

// Classes to test
#include "ingestpipeline/sourcetask/VisConverterBase.h"

using askap::cp::common::VisChunk;

namespace askap {
namespace cp {
namespace ingest {

class VisConverterBaseTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(VisConverterBaseTest);
        CPPUNIT_TEST(testSumOfArithmeticSeries);
        CPPUNIT_TEST(testCalculateRow);
        CPPUNIT_TEST(testConstruct);
        CPPUNIT_TEST(testInitVisChunk);
        CPPUNIT_TEST_SUITE_END();

    public:

        void setUp() {
            itsVisSrc.reset(new MockVisSource);

            const Configuration config = ConfigurationHelper::createDummyConfig();
            LOFAR::ParameterSet params;
            params.add("n_channels.0", "0");
            params.add("n_channels.1", "16416");
            // input beams are one-based, MS requires zero-based:
            params.add("beammap","1:0,2:1,3:2,4:3,5:4");
            itsInstance.reset(new VisConverterBase(params, config, 1));
        }

        void tearDown() {
            itsInstance.reset();
            itsVisSrc.reset();
        }

        void testConstruct() {
            CPPUNIT_ASSERT(itsInstance);
            CPPUNIT_ASSERT_EQUAL(1, itsInstance->id());
        }

        void testInitVisChunk() {
            const unsigned long starttime = 1000000; // One second after epoch 0
            // constants that match the numbers set by ConfigurationHelper
            const double interval = 5.;
            const uint32_t nAntennas = 6;
            const uint32_t nBeams = 4;
            const uint32_t nChannels = 16416;
            const unsigned int nPol = 4;

            CPPUNIT_ASSERT(itsInstance);
            const CorrelatorMode corrMode = itsInstance->itsConfig.lookupCorrelatorMode("standard");
            itsInstance->initVisChunk(starttime, corrMode);
            VisChunk::ShPtr chunk = itsInstance->visChunk();
            CPPUNIT_ASSERT(chunk);
            CPPUNIT_ASSERT_EQUAL(nAntennas * (nAntennas + 1) / 2 * nBeams, chunk->nRow());
            CPPUNIT_ASSERT_EQUAL(nChannels, chunk->nChannel());
            CPPUNIT_ASSERT_EQUAL(nPol, chunk->nPol());
            
            // Ensure the timestamp represents the integration midpoint.
            const uint64_t midpoint = 3500000;
            uint64_t chunkMidpoint = epoch2bat(chunk->time());
            CPPUNIT_ASSERT_EQUAL(midpoint, chunkMidpoint);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(interval, chunk->interval(), 1.0E-10);
            CPPUNIT_ASSERT_DOUBLES_EQUAL(1e6/54, chunk->channelWidth(), 1.0E-3);

            // Check stokes
            CPPUNIT_ASSERT_EQUAL(size_t(nPol), chunk->stokes().size());
            CPPUNIT_ASSERT(chunk->stokes()(0) == casa::Stokes::XX);
            CPPUNIT_ASSERT(chunk->stokes()(1) == casa::Stokes::XY);
            CPPUNIT_ASSERT(chunk->stokes()(2) == casa::Stokes::YX);
            CPPUNIT_ASSERT(chunk->stokes()(3) == casa::Stokes::YY);

            // checking individual rows
            for (casa::uInt row=0; row < chunk->nRow(); ++row) {
                 const casa::uInt ant1 = chunk->antenna1()(row);
                 const casa::uInt ant2 = chunk->antenna2()(row);
                 const casa::uInt beam = chunk->beam1()(row);
                 // consistency check
                 CPPUNIT_ASSERT_EQUAL(row,
                                itsInstance->calculateRow(ant1, ant2, beam));
                 CPPUNIT_ASSERT_EQUAL(beam, chunk->beam2()(row));
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(0., chunk->beam1PA()(row), 1.0E-10);
                 CPPUNIT_ASSERT_DOUBLES_EQUAL(0., chunk->beam2PA()(row), 1.0E-10);
                 for (int i=0; i<3; ++i) {
                      CPPUNIT_ASSERT_DOUBLES_EQUAL(0., chunk->uvw()(row)(i), 1.0E-10);
                 }
            }
        }

        void testSumOfArithmeticSeries()
        {
            const uint32_t A = 0; // First term
            const uint32_t D = 1; // Common difference between the terms
            CPPUNIT_ASSERT_EQUAL(0u, VisConverterBase::sumOfArithmeticSeries(1, A, D));
            CPPUNIT_ASSERT_EQUAL(1u, VisConverterBase::sumOfArithmeticSeries(2, A, D));
            CPPUNIT_ASSERT_EQUAL(3u, VisConverterBase::sumOfArithmeticSeries(3, A, D));
            CPPUNIT_ASSERT_EQUAL(6u, VisConverterBase::sumOfArithmeticSeries(4, A, D));
            CPPUNIT_ASSERT_EQUAL(10u, VisConverterBase::sumOfArithmeticSeries(5, A, D));
            CPPUNIT_ASSERT_EQUAL(15u, VisConverterBase::sumOfArithmeticSeries(6, A, D));
        }

        void testCalculateRow()
        { 
            CPPUNIT_ASSERT(itsInstance);
            // the following parameters are setup in the dummy configuration
            const uint32_t nAntennas = 6;
            const uint32_t nBeams = 4;

            // This ensures the row mapping sees the second antenna index changing
            // the fastest, then the first antenna index, and finally the beam id 
            // changing the slowest
            for (uint32_t beam = 0,idx = 0; beam < nBeams; ++beam) {
                for (uint32_t ant1 = 0; ant1 < nAntennas; ++ant1) {
                    for (uint32_t ant2 = ant1; ant2 < nAntennas; ++ant2,++idx) {
                        CPPUNIT_ASSERT_EQUAL(idx,
                                itsInstance->calculateRow(ant1, ant2, beam));
                    }
                }
            } 
        }

    private:

        boost::shared_ptr< VisConverterBase > itsInstance;
        MockVisSource::ShPtr itsVisSrc;

};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

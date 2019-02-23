/// @file MergedSourceTest.h
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
/// Seriously modified during ADE transition by Max Voronkov

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <cmath>
#include <stdint.h>
#include "boost/shared_ptr.hpp"
#include "Common/ParameterSet.h"
#include "ingestpipeline/sourcetask/test/MockMetadataSource.h"
#include "ingestpipeline/sourcetask/test/MockVisSource.h"
#include "cpcommon/VisDatagram.h"
#include "cpcommon/TosMetadata.h"
#include "cpcommon/TosMetadataAntenna.h"
#include "cpcommon/VisChunk.h"
#include "casacore/measures/Measures.h"
#include "ConfigurationHelper.h"
#include "VisDatagramTestHelper.h"
#include "askap/AskapUtil.h"
#include "askap/AskapError.h"

// Classes to test
#include "ingestpipeline/sourcetask/MergedSource.h"

using askap::cp::common::VisChunk;

namespace askap {
namespace cp {
namespace ingest {

class MergedSourceTest : public CppUnit::TestFixture,
                         protected VisDatagramTestHelper<VisDatagram> {
        CPPUNIT_TEST_SUITE(MergedSourceTest);
        CPPUNIT_TEST(testMockMetadataSource);
        CPPUNIT_TEST(testMockVisSource);
        CPPUNIT_TEST(testSingle);
        CPPUNIT_TEST(testUVWMismatch);
        CPPUNIT_TEST_EXCEPTION(testUVWMismatchException, CheckError);
        CPPUNIT_TEST_SUITE_END();

    public:

        void setUp() {
            itsMetadataSrc.reset(new MockMetadataSource);
            itsVisSrc.reset(new MockVisSource);

            LOFAR::ParameterSet params;
            params.add("n_channels.0", utility::toString(nChannelsForTest()));
            params.add("baduvw_maxcycles", utility::toString(-1));
            const Configuration config = ConfigurationHelper::createDummyConfig();
            itsInstance.reset(new MergedSource(params, config, itsMetadataSrc, itsVisSrc));
        }

        void tearDown() {
            itsInstance.reset();
            itsVisSrc.reset();
            itsMetadataSrc.reset();
        }

        // Test the MockMetadataSource before using it
        void testMockMetadataSource() {
            const int64_t time = 1234;
            boost::shared_ptr<TosMetadata> md(new TosMetadata());
            md->time(time);
            itsMetadataSrc->add(md);
            CPPUNIT_ASSERT(itsMetadataSrc->next() == md);
        };

        // Test the MockVisSource before using it
        void testMockVisSource() {
            const int64_t time = 1234;
            boost::shared_ptr<VisDatagram> vis(new VisDatagram);
            vis->timestamp = time;
            itsVisSrc->add(vis);
            CPPUNIT_ASSERT(itsVisSrc->next() == vis);
        };

        void testSingle() {
            const Configuration config = ConfigurationHelper::createDummyConfig();
            const uint64_t starttime = 1000000; // One second after epoch
            const uint64_t period = 5 * 1000 * 1000;

            // Create a mock metadata object and program it, then
            // add to the MockMetadataSource
            fakeMetadata(config, starttime);

            fakeVisData(starttime, period, 2u);

            runMergedSourceTest(config, false);
       }
  
       void testUVWMismatch() {
            const Configuration config = ConfigurationHelper::createDummyConfig();
            const uint64_t starttime = 1000000; // One second after epoch
            const uint64_t period = 5 * 1000 * 1000;

            // Create a mock metadata object and program it, then
            // add to the MockMetadataSource, add 1m to first antenna uvw
            fakeMetadata(config, starttime, 1.);

            fakeVisData(starttime, period, 2u);

            runMergedSourceTest(config, true);
       }

       void testUVWMismatchException() {
            const Configuration config = ConfigurationHelper::createDummyConfig();
         
            // replace MergedSource with the object which has a different configuration
            // from the one used in normal test
            LOFAR::ParameterSet params;
            params.add("n_channels.0", utility::toString(nChannelsForTest()));
            params.add("baduvw_maxcycles", utility::toString(0));
            itsInstance.reset(new MergedSource(params, config, itsMetadataSrc, itsVisSrc));

            const uint64_t starttime = 1000000; // One second after epoch
            const uint64_t period = 5 * 1000 * 1000;

            // Create a mock metadata object and program it, then
            // add to the MockMetadataSource, add 1m to first antenna uvw
            fakeMetadata(config, starttime, 1.);

            fakeVisData(starttime, period, 2u);

            // it would generate an exception in itsInstance->next()
            runMergedSourceTest(config, true);
       }

    protected:

        /// @brief common functionality to test the output of MergedSource
        /// @details 
        /// @param config configuration object
        /// @param ant0Flag if true, antenna with index 0 is expected to be flagged
        /// due to uvw mismatch, otherwise the basic rule applies (i.e. if the was data,
        /// and there was due to the way the test is designed)
        void runMergedSourceTest(const Configuration &config, bool ant0Flag) {
            const uint32_t nCorr = 4;

            // Get the first VisChunk instance
            VisChunk::ShPtr chunk(itsInstance->next());
            CPPUNIT_ASSERT(chunk.get());

            // Ensure the timestamp represents the integration midpoint.
            // Note the TosMetadata timestamp is the integration start (in
            // microseconds) while the VisChunk timestamp is the integration
            // midpoint (in seconds). The later is that way because the
            // measurement set specification used integration midpoint in
            // seconds.
            const double midpoint = bat2epoch(3500000ul).getValue().getTime().getValue("s");
            const casa::Quantity chunkMidpoint = chunk->time().getTime();
            CPPUNIT_ASSERT_DOUBLES_EQUAL(midpoint, chunkMidpoint.getValue("s"), 1.0E-10);

            // Ensure other metadata is as expected
            CPPUNIT_ASSERT_EQUAL(nChannelsForTest(), chunk->nChannel());
            CPPUNIT_ASSERT_EQUAL(nCorr, chunk->nPol());
            const casa::uInt nAntennas = config.antennas().size();
            const casa::uInt nBaselines = nAntennas * (nAntennas + 1) / 2;
            const uint32_t nBeam = config.feed().nFeeds();
            CPPUNIT_ASSERT_EQUAL(nBaselines * nBeam, chunk->nRow());

            // Check stokes
            CPPUNIT_ASSERT(chunk->nPol() >= 4);
            CPPUNIT_ASSERT(chunk->stokes()(0) == casa::Stokes::XX);
            CPPUNIT_ASSERT(chunk->stokes()(1) == casa::Stokes::XY);
            CPPUNIT_ASSERT(chunk->stokes()(2) == casa::Stokes::YX);
            CPPUNIT_ASSERT(chunk->stokes()(3) == casa::Stokes::YY);

            // Ensure the visibilities that were supplied (most were not)
            // are not flagged, and that the rest are flagged

            for (uint32_t row = 0; row < chunk->nRow(); ++row) {
                 for (uint32_t pol = 0; pol < chunk->nPol(); ++pol) {
                      
                      const int32_t product = config.bmap().getID(chunk->antenna1()(row),
                                      chunk->antenna2()(row), chunk->stokes()(pol));
                      if (product < 0) {
                          if ((chunk->antenna1()(row) == chunk->antenna2()(row)) && (pol == 2)) {
                              // for autos pol==2 is obtained from pol==1
                              for (uint32_t chan = 0; chan < chunk->nChannel(); ++chan) {
                                   CPPUNIT_ASSERT_EQUAL(chunk->flag()(row, chan, 1), chunk->flag()(row, chan, pol));
                              }
                          } else {

                            // products are defined for the first 3 antennas only
                            CPPUNIT_ASSERT((chunk->antenna1()(row) > 2) || (chunk->antenna2()(row) > 2));

                            // appropriate visibilities should be flagged
                            for (uint32_t chan = 0; chan < chunk->nChannel(); ++chan) {
                                 CPPUNIT_ASSERT_EQUAL(true, chunk->flag()(row, chan, pol));
                            }
                          }
                          continue;
                      }
                      
                      // product is 1-based
                      CPPUNIT_ASSERT(product > 0);
                      const uint32_t beamid = 1u;

                      // antenna 0 baselines may be flagged if corruption of UVW is simulated
                      const bool rowWithGoodUVW = !ant0Flag || (chunk->antenna1()(row) != 0 && chunk->antenna2()(row) != 0);

                      for (uint32_t chan = 0; chan < chunk->nChannel(); ++chan) {
                           // by default, beams in the datagrams are 1-based and in the chunk are zero-based
                           if (validChannelAndProduct(chan, static_cast<uint32_t>(product)) &&
                                chunk->beam1()(row) + 1 == beamid && chunk->beam2()(row) + 1 == beamid && rowWithGoodUVW) {
                               // If this is one of the visibilities that were added above
                               CPPUNIT_ASSERT_EQUAL(false, chunk->flag()(row, chan, pol));
                           } else {
                               CPPUNIT_ASSERT_EQUAL(true, chunk->flag()(row, chan, pol));
                           }
                      }
                 }
            }

            // Check scan index
            CPPUNIT_ASSERT_EQUAL(0u, chunk->scan());

            // Check frequency vector
            CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(nChannelsForTest()),
                    chunk->frequency().size());
        }

        /// @brief made a mockup metadata datagram
        /// @details create a mock metadata object and program it, then
        /// add to the MockMetadataSource
        /// @param[in] config configuration of the array
        /// @param[in] time timestamp to use 
        /// @param[in] uvwOffset a number which is added to U, V and W of the first antenna in
        /// configuration (to test mismatch)
        void fakeMetadata(const Configuration &config, const uint64_t time, const double uvwOffset = 0.) {
            TosMetadata metadata;
            metadata.time(time);
            metadata.scanId(0);
            metadata.flagged(false);
            metadata.corrMode("standard");
            // need to specify the middle of the band for the first card 
            // want to get hardware channel 11 (accessor channel 55) to map to 1 GHz exactly
            // config helper sets up the full ASKAP band without inversion
            // start is 8208 fine channels lower, channel 55 is 1 GHz:
            metadata.centreFreq(casa::Quantity(1000. + double(8208 - 55) / 54, "MHz"));

            // antenna_names
            for (uint32_t i = 0; i < config.antennas().size(); ++i) {
                TosMetadataAntenna ant(config.antennas()[i].name());
                ant.onSource(true);
                ant.flagged(false);
                // there is a guard against zeros in the code because values are geocentric,
                // there is also a guard against wrong length of resulting per-baseline uvws and
                // uvws implying that antenna is not on the ground. So passing a large constant
                // as we once had no longer works. The easiest way to solve the problem without doing
                // full simulation is to pass antenna position as uvw for all beams.
                casa::Vector<casa::Double> dummyUVW(36*3,0.);
                const casa::Vector<casa::Double> antPos = config.antennas()[i].position();
                CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3u), antPos.nelements());
                CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0u), dummyUVW.nelements() % 3u);
                for (casa::uInt item = 0; item < dummyUVW.nelements(); ++item) {
                     dummyUVW[item] = antPos[item % 3] + (i == 0 ? uvwOffset : 0.);
                }
                ant.uvw(dummyUVW);
                metadata.addAntenna(ant);
            }

            // Make a copy of the metadata and add it to the mock
            // Metadata source
            boost::shared_ptr<TosMetadata> copy(new TosMetadata(metadata));
            itsMetadataSrc->add(copy);
        }

        /// @brief populate VisDatagram
        /// @param[in] starttime timestamp to use 
        /// @param[in] period cycle time to use 
        /// @param[in] nCycles number of cycles to generate
        void fakeVisData(const uint64_t starttime, const uint64_t period, const uint32_t nCycles = 2) {
            askap::cp::VisDatagram vis;
            vis.version = VisDatagramTraits<VisDatagram>::VISPAYLOAD_VERSION;
            vis.slice = 0;
            fillProtocolSpecificInfo(vis);
            vis.beamid = 1;
            vis.timestamp = starttime;

            for (uint32_t cycle = 0; cycle < nCycles; ++cycle) {
                 boost::shared_ptr<VisDatagram> copy(new VisDatagram(vis));
                 itsVisSrc->add(copy);

                 vis.timestamp += period;
            }
        }

    private:

        boost::shared_ptr< MergedSource > itsInstance;
        MockMetadataSource::ShPtr itsMetadataSrc;
        MockVisSource::ShPtr itsVisSrc;

};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

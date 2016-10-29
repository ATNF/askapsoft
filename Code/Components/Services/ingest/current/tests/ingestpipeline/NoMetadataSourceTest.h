/// @file NoMetadataSourceTest.h
///
/// @copyright (c) 2013 CSIRO
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
#include "boost/shared_ptr.hpp"
#include "Common/ParameterSet.h"
#include "ingestpipeline/sourcetask/test/MockVisSource.h"
#include "cpcommon/VisDatagram.h"
#include "cpcommon/VisChunk.h"
#include "ConfigurationHelper.h"
#include "VisDatagramTestHelper.h"
#include "askap/AskapUtil.h"

// Classes to test
#include "ingestpipeline/sourcetask/NoMetadataSource.h"

using askap::cp::common::VisChunk;

namespace askap {
namespace cp {
namespace ingest {

class NoMetadataSourceTest : public CppUnit::TestFixture,
                      protected VisDatagramTestHelper<VisDatagram> {
        CPPUNIT_TEST_SUITE(NoMetadataSourceTest);
        CPPUNIT_TEST(testMockVisSource);
        CPPUNIT_TEST(testSingle);
        CPPUNIT_TEST_SUITE_END();

    public:

        void setUp() {
            itsVisSrc.reset(new MockVisSource);

            LOFAR::ParameterSet params;
            params.add("n_channels.0", utility::toString(nChannelsForTest()));
            // config is setup with full ASKAP band, we test channel 55 which should
            // map to 1 GHz, i.e. centre frequency is (8208-55)/54 MHz above 1 GHz:
            //params.add("centre_freq", "0.9175GHz");
            params.add("centre_freq", "1.1509814814814815GHz");
            params.add("target_name", "test-field");
            params.add("target_direction","[12h30m49.43, +12d23m28.100, J2000]");
            params.add("correlator_mode", "standard");
            Configuration config = ConfigurationHelper::createDummyConfig();
            itsInstance.reset(new NoMetadataSource(params, config, itsVisSrc));
        }

        void tearDown() {
            itsInstance.reset();
            itsVisSrc.reset();
        }

        // Test the MockVisSource before using it
        void testMockVisSource() {
            const long time = 1234;
            boost::shared_ptr<VisDatagram> vis(new VisDatagram);
            vis->timestamp = time;
            itsVisSrc->add(vis);
            CPPUNIT_ASSERT(itsVisSrc->next() == vis);
        };

        
        void testSingle() {
            const unsigned long starttime = 1000000; // One second after epoch
            const unsigned long period = 5 * 1000 * 1000;
            const unsigned int nCorr = 4;
            const Configuration config = ConfigurationHelper::createDummyConfig();

            // Populate a VisDatagram to match the metadata
            askap::cp::VisDatagram vis;
            vis.version = VisDatagramTraits<VisDatagram>::VISPAYLOAD_VERSION;
            vis.slice = 0;
            fillProtocolSpecificInfo(vis);
            vis.beamid = 1;
            vis.timestamp = starttime;

            boost::shared_ptr<VisDatagram> copy1(new VisDatagram(vis));
            itsVisSrc->add(copy1);

            vis.timestamp = starttime + period;
            boost::shared_ptr<VisDatagram> copy2(new VisDatagram(vis));
            itsVisSrc->add(copy2);

            // Get the first VisChunk instance
            VisChunk::ShPtr chunk(itsInstance->next());
            CPPUNIT_ASSERT(chunk.get());

            // Ensure the timestamp represents the integration midpoint.
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

            for (unsigned int row = 0; row < chunk->nRow(); ++row) {
                 for (unsigned int pol = 0; pol < chunk->nPol(); ++pol) {
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

                      for (uint32_t chan = 0; chan < chunk->nChannel(); ++chan) {
                           // by default, beams in the datagrams are 1-based and in the chunk are zero-based
                           if (validChannelAndProduct(chan, static_cast<uint32_t>(product)) &&
                                chunk->beam1()(row) + 1 == vis.beamid && chunk->beam2()(row) + 1 == vis.beamid) {
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
        
    private:

        boost::shared_ptr< NoMetadataSource > itsInstance;
        MockVisSource::ShPtr itsVisSrc;

};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap

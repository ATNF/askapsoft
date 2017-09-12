/// @file GlobalSkyModelTest.h
///
/// @copyright (c) 2016 CSIRO
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
/// @author Daniel Collins <daniel.collins@csiro.au>

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <string>
#include <askap/AskapError.h>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/math/constants/constants.hpp>
#include <Common/ParameterSet.h>
#include <votable/VOTable.h>

// Classes to test
#include "datamodel/Common.h"
#include "datamodel/ContinuumComponent.h"
#include "datamodel/ComponentStats.h"
#include "service/GlobalSkyModel.h"

using namespace std;
using namespace askap::accessors;
using namespace boost;
using namespace boost::posix_time;

namespace askap {
namespace cp {
namespace sms {

using namespace datamodel;

class GlobalSkyModelTest : public CppUnit::TestFixture {

        CPPUNIT_TEST_SUITE(GlobalSkyModelTest);
        CPPUNIT_TEST(testGsmStatsEmpty);
        CPPUNIT_TEST(testGsmStatsSmall);
        CPPUNIT_TEST(testCreateFromParsetFile);
        CPPUNIT_TEST(testNside);
        CPPUNIT_TEST(testHealpixOrder);
        CPPUNIT_TEST(testGetMissingComponentById);
        CPPUNIT_TEST(testIngestVOTableToEmptyDatabase);
        CPPUNIT_TEST(testIngestVOTableFailsForBadCatalog);
        CPPUNIT_TEST(testIngestPolarisation);
        CPPUNIT_TEST(testMetadata);
        CPPUNIT_TEST(testNonAskapDataIngest);
        CPPUNIT_TEST(testSimpleConeSearch);
        CPPUNIT_TEST(testConeSearch_frequency_criteria);
        CPPUNIT_TEST(testConeSearch_flux_int);
        CPPUNIT_TEST(testSimpleRectSearch);
        CPPUNIT_TEST(testRectSearch_freq_range);
        CPPUNIT_TEST(testLargeAreaSearch);
        CPPUNIT_TEST(testPixelsPerDatabaseSearchIsMultipleOfPixelsInSearch);
        CPPUNIT_TEST_SUITE_END();

    public:
        GlobalSkyModelTest() :
            gsm(),
            parset(true),
            parsetFile("./tests/data/sms_parset.cfg"),
            small_components("./tests/data/votable_small_components.xml"),
            large_components("./tests/data/votable_large_components.xml"),
            invalid_components("./tests/data/votable_error_freq_units.xml"),
            small_polarisation("./tests/data/votable_small_polarisation.xml"),
            simple_cone_search("./tests/data/votable_simple_cone_search.xml")
        {
        }

        void setUp() {
            parset.clear();
            parset.adoptFile(parsetFile);
        }

        void tearDown() {
            parset.clear();
        }

        void initEmptyDatabase() {
            gsm = GlobalSkyModel::create(parset);
            gsm->createSchema();
        }

        void testGsmStatsEmpty() {
            initEmptyDatabase();
            ComponentStats stats = gsm->getComponentStats();
            CPPUNIT_ASSERT_EQUAL(std::size_t(0), stats.count);
        }

        void testGsmStatsSmall() {
            initSearch();
            ComponentStats stats = gsm->getComponentStats();
            CPPUNIT_ASSERT_EQUAL(std::size_t(10), stats.count);
        }

        void testCreateFromParsetFile() {
            initEmptyDatabase();
            CPPUNIT_ASSERT(gsm.get());
        }

        void testNside() {
            initEmptyDatabase();
            CPPUNIT_ASSERT_EQUAL(2l << 9, gsm->getHealpixNside());
        }

        void testHealpixOrder() {
            initEmptyDatabase();
            CPPUNIT_ASSERT_EQUAL(9l, gsm->getHealpixOrder());
        }

        void testGetMissingComponentById() {
            initEmptyDatabase();
            GlobalSkyModel::ComponentPtr component = gsm->getComponentByID(9);
            CPPUNIT_ASSERT(!component.get());
        }

        void testIngestVOTableToEmptyDatabase() {
            parset.replace("sqlite.name", "./tests/service/ingested.dbtmp");
            initEmptyDatabase();
            // perform the ingest
            GlobalSkyModel::IdListPtr ids = gsm->ingestVOTable(
                    small_components,
                    "",
                    10,
                    second_clock::universal_time());
            CPPUNIT_ASSERT_EQUAL(size_t(10), ids->size());

            // test that some expected components can be found
            boost::shared_ptr<ContinuumComponent> component(
                gsm->getComponentByID((*ids)[0]));
            CPPUNIT_ASSERT(component.get());
            CPPUNIT_ASSERT_EQUAL(
                string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_1a"),
                component->component_id);
        }

        void testIngestPolarisation() {
            parset.replace("sqlite.name", "./tests/service/polarisation.dbtmp");
            initEmptyDatabase();
            // perform the ingest
            GlobalSkyModel::IdListPtr ids = gsm->ingestVOTable(
                    small_components,
                    small_polarisation,
                    1337,
                    second_clock::universal_time());
            CPPUNIT_ASSERT_EQUAL(size_t(10), ids->size());

            // each component should have a corresponding polarisation object
            for (GlobalSkyModel::IdList::const_iterator it = ids->begin();
                it != ids->end();
                it++) {
                boost::shared_ptr<ContinuumComponent> component(gsm->getComponentByID(*it));

                // Check that we have a polarisation object
                CPPUNIT_ASSERT(component->polarisation.get());

                // Check that we have the correct polarisation object
                CPPUNIT_ASSERT_EQUAL(component->component_id, component->polarisation->component_id);
            }
        }

        void testNonAskapDataIngest() {
            parset.replace("sqlite.name", "./tests/service/data_source.dbtmp");
            initEmptyDatabase();

            // Non-ASKAP data sources need metadata that is assumed for ASKAP sources.
            boost::shared_ptr<DataSource> expectedDataSource(new DataSource());
            expectedDataSource->name = "Robby Dobby the Bear";
            expectedDataSource->catalogue_id = "RDTB";

            // perform the ingest
            GlobalSkyModel::IdListPtr ids = gsm->ingestVOTable(
                    small_components,
                    small_polarisation,
                    expectedDataSource);

            for (GlobalSkyModel::IdList::const_iterator it = ids->begin();
                it != ids->end();
                it++) {
                boost::shared_ptr<ContinuumComponent> component(gsm->getComponentByID(*it));
                // should have reference to the data source metadata
                CPPUNIT_ASSERT(component->data_source.get());
                // should not have a scheduling block ID
                CPPUNIT_ASSERT_EQUAL(datamodel::NO_SB_ID, component->sb_id);
                // Don't have an observation date for now, but this might change.
                CPPUNIT_ASSERT(component->observation_date == date_time::not_a_date_time),

                CPPUNIT_ASSERT_EQUAL(expectedDataSource->name, component->data_source->name);
                CPPUNIT_ASSERT_EQUAL(expectedDataSource->catalogue_id, component->data_source->catalogue_id);
            }
        }

        void testMetadata() {
            parset.replace("sqlite.name", "./tests/service/metadata.dbtmp");
            initEmptyDatabase();

            int64_t expected_sb_id = 71414;
            ptime expected_obs_date = second_clock::universal_time();

            // perform the ingest
            GlobalSkyModel::IdListPtr ids = gsm->ingestVOTable(
                    small_components,
                    "",
                    expected_sb_id,
                    expected_obs_date);

            for (GlobalSkyModel::IdList::const_iterator it = ids->begin();
                it != ids->end();
                it++) {
                boost::shared_ptr<ContinuumComponent> component(gsm->getComponentByID(*it));

                CPPUNIT_ASSERT_EQUAL(expected_sb_id, component->sb_id);
                // CPPUNIT_ASSERT_EQUAL chokes on the ptime values
                CPPUNIT_ASSERT(expected_obs_date == component->observation_date);
            }
        }

        void testIngestVOTableFailsForBadCatalog() {
            initEmptyDatabase();
            CPPUNIT_ASSERT_THROW(
                gsm->ingestVOTable(
                    invalid_components,
                    "",
                    14,
                    date_time::not_a_date_time),
                askap::AskapError);
        }

        void testSimpleConeSearch() {
            initSearch();
            GlobalSkyModel::ComponentListPtr results = gsm->coneSearch(Coordinate(70.2, -61.8), 1.0);
            CPPUNIT_ASSERT_EQUAL(size_t(1), results->size());
            CPPUNIT_ASSERT_EQUAL(
                string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_1a"),
                results->begin()->component_id);
        }

        /// @brief Simple functor for testing query results against expected component ID strings.
        class ComponentIdMatch
        {
        public:
            ComponentIdMatch(string targetComponentId) :
                itsTarget(targetComponentId)
            {
            }

            bool operator()(datamodel::ContinuumComponent& that)
            {
                return itsTarget == that.component_id;
            }
        private:
            std::string itsTarget;

        };

        void testConeSearch_frequency_criteria() {
            initSearch();

            // create a component query for frequencies in the range [1230..1250]
            GlobalSkyModel::ComponentQuery query(
                GlobalSkyModel::ComponentQuery::freq >= 1230.0 &&
                GlobalSkyModel::ComponentQuery::freq <= 1250.0);
            Coordinate centre(76.0, -71.0);
            double radius = 1.5;
            GlobalSkyModel::ComponentListPtr results = gsm->coneSearch(centre, radius, query);

            CPPUNIT_ASSERT_EQUAL(size_t(3), results->size());
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4b"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4c"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_5a"))));
        }

        void testConeSearch_flux_int() {
            initSearch();
            GlobalSkyModel::ComponentQuery query(
                GlobalSkyModel::ComponentQuery::flux_int >= 80.0);
            Coordinate centre(76.0, -71.0);
            double radius = 1.5;
            GlobalSkyModel::ComponentListPtr results = gsm->coneSearch(centre, radius, query);

            CPPUNIT_ASSERT_EQUAL(size_t(3), results->size());
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_2a"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_3a"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4a"))));
        }

        void testSimpleRectSearch() {
            initSearch();

            Rect roi(Coordinate(79.375, -71.5), Extents(0.75, 1.0));
            GlobalSkyModel::ComponentListPtr results = gsm->rectSearch(roi);

            CPPUNIT_ASSERT_EQUAL(size_t(4), results->size());
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_1b"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_1c"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4a"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4c"))));
        }

        void testRectSearch_freq_range() {
            initSearch();

            // use the same bounds as testSimpleRectSearch
            Rect roi(Coordinate(79.375, -71.5), Extents(0.75, 1.0));

            GlobalSkyModel::ComponentQuery query(
                GlobalSkyModel::ComponentQuery::freq >= 1200.0 &&
                GlobalSkyModel::ComponentQuery::freq <= 1260.0);

            GlobalSkyModel::ComponentListPtr results = gsm->rectSearch(roi, query);

            CPPUNIT_ASSERT_EQUAL(size_t(2), results->size());
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4a"))));
            CPPUNIT_ASSERT_EQUAL(1l, std::count_if(results->begin(), results->end(),
                ComponentIdMatch(string("SB1958_image.i.LMC.cont.sb1958.taylor.0.restored_4c"))));
        }

        void testLargeAreaSearch() {
            initSearch();
            GlobalSkyModel::ComponentListPtr results = gsm->coneSearch(Coordinate(70.2, -61.8), 20.0);
            CPPUNIT_ASSERT_EQUAL(size_t(10), results->size());
        }

        void testPixelsPerDatabaseSearchIsMultipleOfPixelsInSearch() {
            // With the initial implementation of search chunking, I was getting
            // a fencepost error when the total number of pixels in the query
            // region was evenly divisible by the database query chunk size.
            // The values chosen here have been selected by trial and error to
            // reproduce the bug (in order to fix it).
            parset.replace("database.max_pixels_per_query", "15");
            initSearch();

            // The search parameters map to 60 pixels at order 9, but if the GSM
            // NSide/Order is ever changed, then this test may be invalidated
            CPPUNIT_ASSERT_EQUAL((int64_t) 9, gsm->getHealpixOrder());

            CPPUNIT_ASSERT_NO_THROW(gsm->coneSearch(Coordinate(70.2, -61.8), 0.21));
        }

    private:
        GlobalSkyModel::IdListPtr initSearch() {
            // Generate the database file for use in functional tests
            //parset.replace("sqlite.name", "./tests/service/small_spatial_search.db");
            initEmptyDatabase();
            return gsm->ingestVOTable(
                simple_cone_search,
                small_polarisation,
                42,
                second_clock::universal_time());
        }

        boost::shared_ptr<GlobalSkyModel> gsm;
        LOFAR::ParameterSet parset;
        const string parsetFile;
        const string small_components;
        const string large_components;
        const string invalid_components;
        const string small_polarisation;
        const string simple_cone_search;
};

}
}
}

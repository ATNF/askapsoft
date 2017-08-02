/// @file GlobalSkyModel.cc
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

// Include own header file first
#include "GlobalSkyModel.h"

// Include package level header file
#include "askap_skymodel.h"

// System includes
#include <string>

// ASKAPsoft includes
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>
#include <votable/VOTable.h>

// ODB includes
#include <odb/mysql/database.hxx>
#include <odb/mysql/connection-factory.hxx>
#include <odb/pgsql/database.hxx>
#include <odb/pgsql/connection-factory.hxx>
#include <odb/sqlite/database.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/connection.hxx>
#include <odb/transaction.hxx>

// Local package includes
#include "datamodel/ContinuumComponent-odb.h"
#include "datamodel/ComponentStats-odb.h"
#include "Utility.h"
#include "VOTableData.h"

ASKAP_LOGGER(logger, ".GlobalSkyModel");

using namespace odb;
using namespace std;
using namespace boost;
using namespace askap::cp::sms;
using namespace askap::cp::sms::datamodel;
using namespace askap::accessors;


shared_ptr<GlobalSkyModel> GlobalSkyModel::create(const LOFAR::ParameterSet& parset)
{
    shared_ptr<GlobalSkyModel> pImpl;
    const string dbType = parset.get("database.backend");
    ASKAPLOG_DEBUG_STR(logger, "database backend: " << dbType);

    // Get the max number of HEALPix pixels per database query from the parset,
    // with clamping to a reasonable range
    const size_t DEFAULT_PIXELS_PER_QUERY = 2000;
    const size_t MAX_MAX_PIXELS_PER_QUERY = 40000;
    size_t maxPixelsPerQuery = parset.getUint("database.max_pixels_per_query", DEFAULT_PIXELS_PER_QUERY);
    if (maxPixelsPerQuery < 1)
        maxPixelsPerQuery = DEFAULT_PIXELS_PER_QUERY;
    else if (maxPixelsPerQuery >= MAX_MAX_PIXELS_PER_QUERY)
        maxPixelsPerQuery = MAX_MAX_PIXELS_PER_QUERY;

    ASKAPLOG_INFO_STR(logger, "Using " << maxPixelsPerQuery << " pixels per database query");

    if (dbType.compare("sqlite") == 0) {
        // get parameters
        const LOFAR::ParameterSet& dbParset = parset.makeSubset("sqlite.");
        const string dbName = dbParset.get("name");

        ASKAPLOG_INFO_STR(logger, "Instantiating sqlite file " << dbName);

        // TODO Parset flag for db creation control
        shared_ptr<odb::database> pDb(
            new sqlite::database(
                dbName,
                SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));
        ASKAPCHECK(pDb.get(), "GlobalSkyModel creation failed");

        // create the implementation
        pImpl.reset(new GlobalSkyModel(pDb, maxPixelsPerQuery));
    }
    else if (dbType.compare("mysql") == 0) {
        ASKAPLOG_INFO_STR(logger, "connecting to msql");

        ASKAPLOG_DEBUG_STR(logger, "creating connection pool factory");
        std::auto_ptr<odb::mysql::connection_factory> pConnectionFactory(
            new odb::mysql::connection_pool_factory(
                parset.getInt("mysql.max_connections"),
                parset.getInt("mysql.min_connections"),
                parset.getBool("mysql.ping_connections")));
        ASKAPCHECK(pConnectionFactory.get(), "MySQL connection factory failed");

        ASKAPLOG_DEBUG_STR(logger, "creating MySQL database");
        shared_ptr<odb::database> pDb(
            new mysql::database(
                parset.get("mysql.user"),
                parset.get("mysql.password"),
                parset.get("mysql.database"),
                parset.get("mysql.host"),
                parset.getUint("mysql.port"),
                parset.get("mysql.socket"),
                parset.get("mysql.charset"),
                0, // no flags yet
                pConnectionFactory));
        ASKAPCHECK(pDb.get(), "GlobalSkyModel creation failed");

        // create the implementation
        ASKAPLOG_DEBUG_STR(logger, "creating GlobalSkyModel");
        pImpl.reset(new GlobalSkyModel(pDb, maxPixelsPerQuery));
    }
    /* PostgreSQL support is being removed in order to simplify build
     * dependencies. MySQL has been chosen as the production backend, while unit
     * and functional tests run against sqlite.
     * See https://jira.csiro.au/browse/ASKAPSDP-2738
     */
    /*
    else if (dbType.compare("pgsql") == 0) {
        ASKAPLOG_INFO_STR(logger, "connecting to pgsql");

        ASKAPLOG_DEBUG_STR(logger, "creating connection pool factory");
        std::auto_ptr<odb::pgsql::connection_factory> pConnectionFactory(
            new odb::pgsql::connection_pool_factory(
                parset.getInt("pgsql.max_connections"),
                parset.getInt("pgsql.min_connections")));
        ASKAPCHECK(pConnectionFactory.get(), "pgsql connection factory failed");

        ASKAPLOG_DEBUG_STR(logger, "creating pgsql database");
        shared_ptr<odb::database> pDb(
            new pgsql::database(
                parset.get("pgsql.user"),
                parset.get("pgsql.password"),
                parset.get("pgsql.database"),
                parset.get("pgsql.host"),
                parset.getUint("pgsql.port"),
                "",
                pConnectionFactory));
        ASKAPCHECK(pDb.get(), "GlobalSkyModel creation failed");

        // create the implementation
        ASKAPLOG_DEBUG_STR(logger, "creating GlobalSkyModel");
        pImpl.reset(new GlobalSkyModel(pDb, maxPixelsPerQuery));
    }
    */
    else {
        ASKAPTHROW(AskapError, "Unsupported database backend: " << dbType);
    }

    ASKAPCHECK(pImpl.get(), "GlobalSkyModel creation failed");
    return pImpl;
}

GlobalSkyModel::GlobalSkyModel(
    boost::shared_ptr<odb::database> database,
    size_t maxPixelsPerQuery)
    :
    itsDb(database),
    itsHealPix(getHealpixOrder()),
    itsMaxPixelsPerQuery(maxPixelsPerQuery)
{
}

GlobalSkyModel::~GlobalSkyModel()
{
    ASKAPLOG_DEBUG_STR(logger, "GSM shutting down");
    // shutdown the database
}

ComponentStats GlobalSkyModel::getComponentStats() const
{
    transaction t(itsDb->begin());
    ComponentStats stats(itsDb->query_value<ComponentStats>());
    t.commit();
    return stats;
}

bool GlobalSkyModel::createSchema(bool dropTables)
{
    // SQLite has quirks that must be handled with DB-specific code...
    if (itsDb->id () == odb::id_sqlite) {
        ASKAPLOG_DEBUG_STR(logger, "Creating sqlite db");
        createSchemaSqlite(dropTables);
        return true;
    }
    else { // if (itsDb->id() == odb::id_mysql) {
        ASKAPLOG_DEBUG_STR(logger, "Creating schema");
        transaction t(itsDb->begin());
        schema_catalog::create_schema(*itsDb, "", dropTables);
        t.commit();
        return true;
    }

    return false;
}

void GlobalSkyModel::createSchemaSqlite(bool dropTables)
{
    // Create the database schema. Due to bugs in SQLite foreign key
    // support for DDL statements, we need to temporarily disable
    // foreign keys.
    connection_ptr c(itsDb->connection());

    c->execute ("PRAGMA foreign_keys=OFF");

    transaction t(c->begin());
    schema_catalog::create_schema(*itsDb, "", dropTables);
    t.commit();

    c->execute("PRAGMA foreign_keys=ON");
}

GlobalSkyModel::IdListPtr GlobalSkyModel::ingestVOTable(
    const std::string& componentsCatalog,
    const std::string& polarisationCatalog,
    int64_t sb_id,
    posix_time::ptime obs_date)
{
    return ingestVOTable(
            componentsCatalog,
            polarisationCatalog,
            shared_ptr<datamodel::DataSource>(),
            sb_id,
            obs_date);
}

GlobalSkyModel::IdListPtr GlobalSkyModel::ingestVOTable(
    const std::string& componentsCatalog,
    const std::string& polarisationCatalog,
    shared_ptr<datamodel::DataSource> dataSource)
{
    ASKAPASSERT(dataSource.get());
    return ingestVOTable(
            componentsCatalog,
            polarisationCatalog,
            dataSource,
            NO_SB_ID,
            date_time::not_a_date_time);
}

GlobalSkyModel::IdListPtr GlobalSkyModel::ingestVOTable(
    const std::string& componentsCatalog,
    const std::string& polarisationCatalog,
    shared_ptr<datamodel::DataSource> dataSource,
    int64_t sb_id,
    posix_time::ptime obs_date)
{
    ASKAPLOG_INFO_STR(logger,
        "Starting VO Table ingest. Component catalog: '" << componentsCatalog <<
        "' polarisationCatalog: '" << polarisationCatalog << "'");

    shared_ptr<VOTableData> pCatalog(
        VOTableData::create(
            componentsCatalog,
            polarisationCatalog,
            getHealpixOrder()));

    IdListPtr results(new std::vector<datamodel::id_type>());

    if (pCatalog.get()) {
        VOTableData::ComponentList& components = pCatalog->getComponents();

        ASKAPLOG_DEBUG_STR(logger, "starting transaction");
        transaction t(itsDb->begin());

        // If we have a data source object, persist it
        if (dataSource.get())
            itsDb->persist(dataSource);

        // bulk persist is only supported for SQLServer and Oracle.
        // So we have to fall back to a manual loop persisting one component at
        // a time...
        for (VOTableData::ComponentList::iterator it = components.begin();
             it != components.end();
             it++) {
            it->sb_id = sb_id;
            it->observation_date = obs_date;
            it->data_source = dataSource;

            // If this component has polarisation data, then persist it
            if (it->polarisation.get())
                itsDb->persist(it->polarisation);

            results->push_back(itsDb->persist(*it));
        }

        t.commit();
        ASKAPLOG_DEBUG_STR(logger, "transaction committed. Ingested " << results->size() << " components");
    }

    return results;
}

GlobalSkyModel::ComponentPtr GlobalSkyModel::getComponentByID(datamodel::id_type id) const
{
    ASKAPLOG_INFO_STR(logger, "getComponentByID: id = " << id);

    transaction t(itsDb->begin());
    boost::shared_ptr<ContinuumComponent> component(itsDb->find<ContinuumComponent>(id));
    t.commit();

    return component;
}

GlobalSkyModel::ComponentListPtr GlobalSkyModel::coneSearch(
    Coordinate centre,
    double radius) const
{
    return coneSearch(centre, radius, ComponentQuery());
}

GlobalSkyModel::ComponentListPtr GlobalSkyModel::coneSearch(
    Coordinate centre,
    double radius,
    ComponentQuery query) const
{
    ASKAPLOG_DEBUG_STR(logger, "ra=" << centre.ra << ", dec=" << centre.dec << ", radius=" << radius);
    ASKAPASSERT(radius > 0);
    return queryComponentsByPixel(
            itsHealPix.queryDisk(centre, radius),
            query);
}

GlobalSkyModel::ComponentListPtr GlobalSkyModel::rectSearch(Rect rect) const
{
    return rectSearch(rect, ComponentQuery());
}

GlobalSkyModel::ComponentListPtr GlobalSkyModel::rectSearch(
    Rect rect, ComponentQuery query) const
{
    ASKAPLOG_DEBUG_STR(logger, "centre=" << rect.centre.ra << ", " <<
        rect.centre.dec << ". extents=" << rect.extents.width << ", " << rect.extents.height);
    return queryComponentsByPixel(
            itsHealPix.queryRect(rect),
            query);
}

GlobalSkyModel::ComponentListPtr GlobalSkyModel::queryComponentsByPixel(
    HealPixFacade::IndexListPtr pixels,
    ComponentQuery query) const
{
    ASKAPASSERT(pixels.get());
    ASKAPLOG_DEBUG_STR(logger, "healpixQuery against : " << pixels->size() << " pixels");

    // We need somewhere to store the results
    ComponentListPtr results(new ComponentList());

    if (pixels->size() > 0) {
        // Break the query into multiple database hits, so we don't overwhelm the
        // database with too many pixels per query
        const ldiv_t chunks = std::ldiv(pixels->size(), getMaxPixelsPerQuery());
        size_t cumulative = 0;  // track the total number of pixels searched so far
        HealPixFacade::IndexList::iterator it = pixels->begin();

        transaction t(itsDb->begin());

        // We don't require the final iteration if the remainder is 0
        long requiredIterations = chunks.rem ? chunks.quot + 1 : chunks.quot;
        for (long i = 0; i < requiredIterations; i++) {
            size_t size = i == chunks.quot ? chunks.rem : getMaxPixelsPerQuery();
            cumulative += size;
            //cout << "i: " << i << " size: " << size << " cumulative: " << cumulative << endl;
            Result r = itsDb->query<ContinuumComponent>(
                    ComponentQuery::healpix_index.in_range(it, it + size)
                    && query);
            results->insert(results->end(), r.begin(), r.end());
            it += size;
        }

        t.commit();
        ASKAPASSERT(cumulative == pixels->size());  // loop post-condition
    }

    ASKAPLOG_DEBUG_STR(logger, results->size() << " results");
    return results;
}

GlobalSkyModel::IdListPtr GlobalSkyModel::uploadComponents(ComponentList& components)
{
    IdListPtr results(new std::vector<datamodel::id_type>());

    // OK, first we need to index the components
    ASKAPLOG_DEBUG_STR(logger, "Starting HEALPix indexation");
    HealPixFacade hp(getHealpixOrder());
    for (ComponentList::iterator it = components.begin(); it != components.end(); it++) {
        it->healpix_index = hp.calcHealPixIndex(Coordinate(it->ra, it->dec));
    }
    ASKAPLOG_DEBUG_STR(logger, "HEALPix indexation complete");

    ASKAPLOG_DEBUG_STR(logger, "Starting upload");
    transaction t(itsDb->begin());

    // bulk persist is only supported for SQLServer and Oracle.
    // So we have to fall back to a manual loop persisting one component at
    // a time...
    for (ComponentList::iterator it = components.begin(); it != components.end(); it++) {
        if (it->polarisation.get())
            itsDb->persist(it->polarisation);
        results->push_back(itsDb->persist(*it));
    }

    t.commit();
    ASKAPLOG_DEBUG_STR(logger, "Uploaded " << results->size() << " components");

    return results;
}

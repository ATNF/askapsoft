/// @file sms_tools.cc
/// @brief Entry point for Sky Model Service tools and utility functions.
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

// Include package level header file
#include <askap_skymodel.h>

// System includes
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Boost includes
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/random/uniform_real.hpp"
#include "boost/random/variate_generator.hpp"
#include "boost/random/linear_congruential.hpp"
#include "boost/program_options.hpp"

// ASKAPsoft includes
#include <askap/Application.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askap/StatReporter.h>
#include <Common/ParameterSet.h>

// ODB includes
#include <odb/exception.hxx>

// Local Package includes
#include "service/GlobalSkyModel.h"
#include "service/SkyModelService.h"
#include "service/Utility.h"


using namespace boost;
using namespace askap;
using namespace askap::cp::sms;

ASKAP_LOGGER(logger, ".sms_tools");

#define CREATE_SCHEMA "create-schema"
#define INGEST_COMPONENTS "ingest-components"
#define INGEST_POLARISATION "ingest-polarisation"
#define STATS "gsm-stats"
#define SB_ID "sbid"
#define OBS_DATE "observation-date"
#define RANDOMISE "gen-random-components"
#define CONE_SEARCH "cone-search"
#define RA "ra"
#define DEC "dec"
#define RADIUS "radius"
#define H_LINE "\n------------------------------------------------------------\n"

class SmsToolsApp : public askap::Application {
    public:
        virtual int run(int argc, char* argv[])
        {
            StatReporter stats;
            int exit_code = 0;

            try {
                // Dispatch to the requested utility function
                if (parameterExists(CREATE_SCHEMA)) {
                    exit_code = createSchema();
                }
                else if (parameterExists(INGEST_COMPONENTS)) {
                    exit_code = ingestVoTable();
                }
                else if (parameterExists(RANDOMISE)) {
                    int64_t count = lexical_cast<int64_t>(parameter(RANDOMISE));
                    exit_code = generateRandomComponents(count);
                }
                else if (parameterExists(CONE_SEARCH)) {
                    coneSearchTest();
                }
                else if (parameterExists(STATS)) {
                    printGsmStats();
                }
            } catch (const AskapError& e) {
                ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << e.what());
                exit_code = 1;
            } catch (const odb::exception& e) {
                ASKAPLOG_FATAL_STR(logger, "Database exception in " << argv[0] << ": " << e.what());
                exit_code = 2;
            } catch (const std::exception& e) {
                ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << e.what());
                exit_code = 3;
            }

            stats.logSummary();

            return exit_code;
        }

    private:

        int createSchema()
        {
            const LOFAR::ParameterSet& parset = config();
            bool dropTables = parset.getBool("database.create_schema.droptables", true);

            boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
            return pGsm->createSchema(dropTables) ? 0 : 4;
        }

        int ingestVoTable()
        {
            ASKAPASSERT(parameterExists(INGEST_COMPONENTS));
            ASKAPASSERT(parameterExists(SB_ID));
            ASKAPASSERT(parameterExists(OBS_DATE));

            string components = parameter(INGEST_COMPONENTS);
            string polarisation = parameterExists(INGEST_POLARISATION) ? parameter(INGEST_POLARISATION) : "";
            int64_t sbid = lexical_cast<int64_t>(parameter(SB_ID));
            posix_time::ptime obsDate = date_time::parse_delimited_time<posix_time::ptime>(parameter(OBS_DATE), 'T');

            ASKAPLOG_INFO_STR(
                logger,
                "Ingesting catalogs. Components: '" << components << "', " <<
                "Polarisation: '" << polarisation << "', " <<
                "Scheduling block: " << sbid << ", " <<
                "Observation date: " << obsDate);

            boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
            pGsm->ingestVOTable(
                components,
                polarisation,
                sbid,
                obsDate);
            return 0;
        }

        int generateRandomComponents(int64_t componentCount)
        {
            ASKAPLOG_INFO_STR(logger, "Generating " << componentCount << " components");
            if (componentCount > 0) {
                boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
                int64_t sbid = parameterExists(SB_ID) ? lexical_cast<int64_t>(parameter(SB_ID)) : -1;

                GlobalSkyModel::ComponentList components(componentCount);
                populateRandomComponents(components, sbid);
                pGsm->uploadComponents(components);
            }

            return 0;
        }

        void populateRandomComponents(
            GlobalSkyModel::ComponentList& components,
            int64_t sbid)
        {
            // Set up (painfully!) the boost PRNGs
            // I am not worried about repeating number cycles, so reusing the underlying generator should be fine
            boost::minstd_rand generator(147u);

            // RA over [0..360)
            boost::uniform_real<double> ra_dist(0, 360);
            boost::variate_generator<boost::minstd_rand&, boost::uniform_real<double> > ra_rng(generator, ra_dist);

            // Dec over [-90..90)
            boost::uniform_real<double> dec_dist(-90, 90);
            boost::variate_generator<boost::minstd_rand&, boost::uniform_real<double> > dec_rng(generator, dec_dist);

            int i = 0;
            for (GlobalSkyModel::ComponentList::iterator it = components.begin();
                 it != components.end();
                 it++, i++) {
                it->component_id = boost::str(boost::format("randomly generated component %d") % i);
                it->ra = ra_rng();
                it->dec = dec_rng();
                it->sb_id = sbid;
            }
        }
        void coneSearchTest()
        {
            ASKAPASSERT(parameterExists(RA));
            ASKAPASSERT(parameterExists(DEC));
            ASKAPASSERT(parameterExists(RADIUS));

            double ra = lexical_cast<double>(parameter(RA));
            double dec = lexical_cast<double>(parameter(DEC));
            double radius = lexical_cast<double>(parameter(RADIUS));
            boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));

            std::cout <<
                "Cone search test. RA: " << ra << ", " <<
                "Dec: " << dec << ", " <<
                "Radius: " << radius << std::endl;

            GlobalSkyModel::ComponentListPtr pComponents = pGsm->coneSearch(
                Coordinate(ra, dec),
                radius);

            std::cout << "Retrieved " << pComponents->size() << " components" << std::endl;
        }

        void printGsmStats()
        {
            boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
            datamodel::ComponentStats stats = pGsm->getComponentStats();
            std::cout << H_LINE <<
                "GSM stats:" << std::endl <<
                "\tComponents: " << stats.count <<
                H_LINE;
        }
};

int main(int argc, char *argv[])
{
    // make a copy of the command-line argument array
    std::vector<char*> args(argv, argv + argc);

    // use boost program_options to parse the actual args to check for the 
    // config option
    program_options::options_description desc;
    desc.add_options()
        ("config,c", program_options::value<string>(), "");
    program_options::variables_map vm;
    program_options::store(program_options::parse_command_line(argc, argv, desc), vm);
    program_options::notify(vm);

    if (vm.count("config") == 0) 
    {
        // parset file was not supplied, so try to read parset config location
        // from environment variable
        char* parset_env = std::getenv("ASKAP_SMS_PARSET");
        if (parset_env)
        {
            // We have the environment variable, so add it to the argument array 
            // that will be passed to the Application base class:
            args.push_back((char*)"--config");
            args.push_back(parset_env);
            argc+=2;
        }
    }

    SmsToolsApp app;
    app.addParameter(STATS, "v", "Output some database statistics", false);
    app.addParameter(CREATE_SCHEMA, "s", "Initialises an empty database", false);
    app.addParameter(INGEST_COMPONENTS, "g", "Ingest/upload a VO Table of components to the global sky model", true);
    app.addParameter(INGEST_POLARISATION, "p", "Optional polarisation data catalog", true);
    app.addParameter(SB_ID, "i", "Scheduling block ID for ingested catalog", true);
    app.addParameter(OBS_DATE, "d", "Observation date for ingested catalog, in form YYYY-MM-DDTHH:MM:SS", true);
    app.addParameter(RANDOMISE, "t", "Populate the database by randomly generating the specified number of components", "0");
    app.addParameter(CONE_SEARCH, "w", "Test cone search (does not output any results, just for testing)", false);
    app.addParameter(RA, "x", "Right-ascension for cone search tests", "0");
    app.addParameter(DEC, "y", "Declination for cone search tests", "0");
    app.addParameter(RADIUS, "z", "Radius for cone search tests", "0.1");

    // Note that we pass the address of the first element in std::vector<char*>
    // args, effectively getting a char*[] as expected by Application::main
    return app.main(argc, &args[0]);
}

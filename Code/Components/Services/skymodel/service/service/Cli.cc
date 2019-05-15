/// @file Cli.cc
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

#include "Cli.h"

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
#include <askap/askap/Application.h>
#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>
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

#define INGEST_COMPONENTS "ingest-components"
#define INGEST_POLARISATION "ingest-polarisation"
#define STATS "gsm-stats"
#define SB_ID "sbid"
#define OBS_DATE "observation-date"
#define CONE_SEARCH "cone-search"
#define RA "ra"
#define DEC "dec"
#define RADIUS "radius"
#define H_LINE "\n------------------------------------------------------------\n"


int Cli::main(int argc, char *argv[])
{
    // If the parset file is not specified, use a default from the environment
    // First make a copy of the command-line argument array
    // so we can search for the config arg, and then manipulate the args array
    // if required.
    // Not using vector<string> as it makes getting the char*[] harder
    std::vector<char*> args(argv, argv + argc);

    // I could use std::find, but that requires either C++11 support for lambda
    // expressions, or a bunch of code to define the unary comparison for
    // char* comparisons against the target string.
    bool found_config = false;
    for (std::vector<char*>::iterator it = args.begin(); it != args.end(); it++)
    {
        // check both help and config args. If help is supplied then we don't
        // need the config anyway.
        if (strcmp(*it, (char*)"--help") == 0 ||
            strcmp(*it, (char*)"-h") == 0 ||
            strcmp(*it, (char*)"--config") == 0 ||
            strcmp(*it, (char*)"-c") == 0)
        {
            found_config = true;
            break;
        }
    }

    if (!found_config)
    {
        std::cerr << "No configuration file, checking ASKAP_SMS_PARSET environment variable." << std::endl;

        char* parset_env = std::getenv("ASKAP_SMS_PARSET");
        if (parset_env)
        {
            std::cerr << "Using SMS parset: " << parset_env << std::endl;

            // We have the environment variable, so add it to the argument array
            // that will be passed to the Application base class:
            args.push_back((char*)"--config");
            args.push_back(parset_env);
            argc += 2;
        }
        else
        {
            std::cerr << "ASKAP_SMS_PARSET not found. Please set, or supply a parset on the command line." << std::endl;
        }
    }

    // add common parameters
    addParameter(STATS, "v", "Output some database statistics", false);
    addParameter(INGEST_COMPONENTS, "g", "Ingest/upload a VO Table of components to the global sky model", true);
    addParameter(INGEST_POLARISATION, "p", "Optional polarisation data catalog", true);
    addParameter(SB_ID, "i", "Scheduling block ID for ingested catalog", true);
    addParameter(OBS_DATE, "d", "Observation date for ingested catalog, in form YYYY-MM-DDTHH:MM:SS", true);
    addParameter(CONE_SEARCH, "w", "Test cone search (does not output any results, just for testing)", false);
    addParameter(RA, "x", "Right-ascension for cone search tests", "0");
    addParameter(DEC, "y", "Declination for cone search tests", "0");
    addParameter(RADIUS, "z", "Radius for cone search tests", "0.1");

    // allow subclasses the chance to add additional parameters
    doAddParameters();

    // Note that we pass the address of the first element in std::vector<char*>
    // effectively getting a char*[] as expected by Application::main
    return Application::main(argc, &args[0]);
}

int Cli::run(int argc, char* argv[])
{
    StatReporter stats;
    int exit_code = 0;

    try {
        exit_code = dispatch();
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

void Cli::doAddParameters()
{
}

int Cli::doCommandDispatch()
{
    return 0;
}

int Cli::dispatch()
{
    int exit_code = 0;

    // Dispatch to the requested utility function
    if (parameterExists(INGEST_COMPONENTS)) {
        exit_code = ingestVoTable();
    }
    else if (parameterExists(CONE_SEARCH)) {
        coneSearch();
    }
    else if (parameterExists(STATS)) {
        printGsmStats();
    }
    else {
        // delegate to the base class (if any)
        exit_code = doCommandDispatch();
    }

    return exit_code;
}

int Cli::ingestVoTable()
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

void Cli::printGsmStats()
{
    boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
    datamodel::ComponentStats stats = pGsm->getComponentStats();
    std::cout << H_LINE <<
        "GSM stats:" << std::endl <<
        "\tComponents: " << stats.count <<
        H_LINE;
}

void Cli::coneSearch()
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

    ComponentListPtr pComponents = pGsm->coneSearch(
            Coordinate(ra, dec),
            radius);

    std::cout << "Retrieved " << pComponents->size() << " components" << std::endl;
}

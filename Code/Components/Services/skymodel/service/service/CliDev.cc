/// @file CliDev.cc
/// @brief Defines some additional developer-only command-line utilities.
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

#include "CliDev.h"

// Include package level header file
#include <askap_skymodel.h>

// System includes
#include <vector>

// Boost includes
#include "boost/format.hpp"
#include "boost/random/uniform_real.hpp"
#include "boost/random/variate_generator.hpp"
#include "boost/random/linear_congruential.hpp"

// ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <Common/ParameterSet.h>

// ODB includes
#include <odb/exception.hxx>

// Local Package includes


using namespace boost;
using namespace askap;
using namespace askap::cp::sms;

ASKAP_LOGGER(logger, ".sms_dev_tools");

#define CREATE_SCHEMA "create-schema"
#define RANDOMISE "gen-random-components"

void CliDev::doAddParameters()
{
    addParameter(CREATE_SCHEMA, "s", "Initialises an empty database", false);
    addParameter(RANDOMISE, "t", "Populate the database by randomly generating the specified number of components", "0");
}

int CliDev::doCommandDispatch() 
{
    int exit_code = 0;

    // Dispatch to the requested utility function
    if (parameterExists(CREATE_SCHEMA)) {
        exit_code = createSchema();
    }
    else if (parameterExists(RANDOMISE)) {
        int64_t count = lexical_cast<int64_t>(parameter(RANDOMISE));
        exit_code = generateRandomComponents(count);
    }

    return exit_code;
}

int CliDev::createSchema()
{
    const LOFAR::ParameterSet& parset = config();
    bool dropTables = parset.getBool("database.create_schema.droptables", true);

    boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
    return pGsm->createSchema(dropTables) ? 0 : 4;
}

int CliDev::generateRandomComponents(int64_t componentCount)
{
    ASKAPLOG_INFO_STR(logger, "Generating " << componentCount << " components");
    if (componentCount > 0) {
        boost::shared_ptr<GlobalSkyModel> pGsm(GlobalSkyModel::create(config()));
        int64_t sbid = -1;

        ComponentList components(componentCount);
        populateRandomComponents(components, sbid);
        pGsm->uploadComponents(components);
    }

    return 0;
}

void CliDev::populateRandomComponents(
        ComponentList& components,
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
    for (ComponentList::iterator it = components.begin();
            it != components.end();
            it++, i++) {
        it->component_id = boost::str(boost::format("randomly generated component %d") % i);
        it->ra = ra_rng();
        it->dec = dec_rng();
        it->sb_id = sbid;
    }
}

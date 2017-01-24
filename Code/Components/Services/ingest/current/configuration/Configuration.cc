/// @file Configuration.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "Configuration.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <vector>
#include <map>
#include <limits>
#include <utility>
#include <stdint.h>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "askap/AskapLogging.h"
#include "askap/AskapUtil.h"
#include "boost/scoped_ptr.hpp"
#include "Common/ParameterSet.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/BasicSL.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Vector.h"

// Local package includes
#include "configuration/TaskDesc.h"
#include "configuration/Antenna.h"
#include "configuration/CorrelatorMode.h"
#include "configuration/ServiceConfig.h"
#include "configuration/TopicConfig.h"
#include "configuration/MonitoringProviderConfig.h"

using namespace std;
using namespace askap;
using namespace askap::cp::ingest;

ASKAP_LOGGER(logger, ".Configuration");

Configuration::Configuration(const LOFAR::ParameterSet& parset, int rank, int nprocs)
    : itsParset(parset), itsRank(rank), itsNProcs(nprocs), itsReceiverId(-1), itsNReceivingProcs(-1)
{
    buildRanksInfo();
    buildTasks();
    buildFeeds();
    buildAntennas();
    buildBaselineMap();
    buildCorrelatorModes();
}

int Configuration::rank(void) const
{
    return itsRank;
}

int Configuration::nprocs(void) const
{
    return itsNProcs;
}

/// @brief populate receiver Id and calculate number of receivers
void Configuration::buildRanksInfo()
{
   // by default, all ranks are receiving ranks
   std::vector<unsigned int> nonReceivingRanks = itsParset.getUint32Vector("service_ranks",std::vector<unsigned int>());
   if (rank() < 0) { 
       // serial case, not really our use case in the post BETA era anyway but do these checks for completeness
       ASKAPCHECK(nprocs() == 1, "Number of processes is expected to be 1 in the serial case; you have "<<nprocs());
       ASKAPCHECK(nonReceivingRanks.size() == 0, "Non-receiving (a.k.a service_ranks)  are not supported in the serial case");
       itsReceiverId = 0;
       itsNReceivingProcs = nprocs();
   } else {
       ASKAPDEBUGASSERT(itsReceiverId == -1);
       ASKAPCHECK(rank() < nprocs(), "Rank "<<rank()<<" should not exceed the number of processes = "<<nprocs());
       // by default all are receiving ranks
       itsNReceivingProcs = nprocs();
       bool thisProcessIsAReceiver = true;
       int nServiceRanksBeforeThis = 0, nServiceRanksIgnored = 0;
       for (std::vector<unsigned int>::const_iterator ci = nonReceivingRanks.begin();
            ci != nonReceivingRanks.end(); ++ci) {
            ASKAPCHECK(std::count(nonReceivingRanks.begin(), nonReceivingRanks.end(), *ci) == 1,
                       "Duplicated element was found in service_ranks field: "<<nonReceivingRanks);
            ASKAPCHECK(*ci >= 0, "Negative values are not allowed in the list of service ranks: "<<nonReceivingRanks);
            if (*ci >= static_cast<unsigned int>(nprocs())) {
                ++nServiceRanksIgnored;
            } else {
                --itsNReceivingProcs;
                if (*ci < static_cast<unsigned int>(rank())) {
                    ++nServiceRanksBeforeThis;
                } else if (*ci == static_cast<unsigned int>(rank())) {
                    thisProcessIsAReceiver = false;
                }
            }
       }
       if (thisProcessIsAReceiver) {
           // get receiver Id for it
           itsReceiverId = rank() - nServiceRanksBeforeThis;
           ASKAPASSERT(itsReceiverId >= 0);
           ASKAPASSERT(itsReceiverId < itsNReceivingProcs);
       }
       if ((nServiceRanksIgnored > 0) && (rank() == 0)) {
           ASKAPLOG_WARN_STR(logger, "Given the number of ranks available ("<<nprocs()<<"), "<<nServiceRanksIgnored<<" service rank(s) is/are ignored");
       }
   }    
   //ASKAPLOG_DEBUG_STR(logger, "Rank "<<rank()<<" out of "<<nprocs()<<" available has receiverId = "<<receiverId()<<"; number of receivers = "<<nReceivingProcs());
}

casa::String Configuration::arrayName(void) const
{
    return itsParset.getString("array.name");
}

const std::vector<TaskDesc>& Configuration::tasks(void) const
{
    return itsTasks;
}

const FeedConfig& Configuration::feed(void) const
{
    if (itsFeedConfig.get() == 0) {
        ASKAPTHROW(AskapError, "Feed config not initialised");
    }
    return *itsFeedConfig;
}

const std::vector<Antenna>& Configuration::antennas(void) const
{
    return itsAntennas;
}

const BaselineMap& Configuration::bmap(void) const
{
    if (!itsBaselineMap) {
        ASKAPTHROW(AskapError, "BaselineMap not initialised");
    }
    return *itsBaselineMap;
}

const CorrelatorMode& Configuration::lookupCorrelatorMode(const std::string& modename) const
{
    const std::map<std::string, CorrelatorMode>::const_iterator it = itsCorrelatorModes.find(modename);
    if (it == itsCorrelatorModes.end()) {
        ASKAPTHROW(AskapError, "Correlator mode " << modename << " not found");
    }
    return it->second;
}

casa::uInt Configuration::schedulingBlockID(void) const
{
    return itsParset.getUint32("sbid", 0);
}

ServiceConfig Configuration::calibrationDataService(void) const
{
    const string registryHost = itsParset.getString("cal_data_service.ice.locator_host");
    const string registryPort = itsParset.getString("cal_data_service.ice.locator_port");
    const string serviceName = itsParset.getString("cal_data_service.servicename");
    return ServiceConfig(registryHost, registryPort, serviceName);
}

MonitoringProviderConfig Configuration::monitoringConfig(void) const
{
    if (itsParset.isDefined("monitoring.enabled") && itsParset.getBool("monitoring.enabled", false)) {
        const string registryHost = itsParset.getString("monitoring.ice.locator_host");
        const string registryPort = itsParset.getString("monitoring.ice.locator_port");
        const string serviceName = itsParset.getString("monitoring.servicename");
        const string adapterName = itsParset.getString("monitoring.adaptername");
        return MonitoringProviderConfig(registryHost, registryPort, serviceName, adapterName);
    } else {
        return MonitoringProviderConfig("", "", "", "");
    }
}

TopicConfig Configuration::metadataTopic(void) const
{
    const string registryHost = itsParset.getString("metadata_source.ice.locator_host");
    const string registryPort = itsParset.getString("metadata_source.ice.locator_port");
    const string topicManager = itsParset.getString("metadata_source.icestorm.topicmanager");
    const string topic = itsParset.getString("metadata.topic");
    return TopicConfig(registryHost, registryPort, topicManager, topic);
}

void Configuration::buildTasks(void)
{
    // Iterator over all tasks
    const vector<string> names = itsParset.getStringVector("tasks.tasklist");
    vector<string>::const_iterator it;
    for (it = names.begin(); it != names.end(); ++it) {
        itsTasks.push_back(taskByName(*it));
    }
}

/// @brief task description by logical name
/// @param[in] name logical name of the task
/// @return task descriptor
TaskDesc Configuration::taskByName(const std::string &name) const
{
        const string keyBase = "tasks." + name;
        const string typeStr = itsParset.getString(keyBase + ".type");
        const TaskDesc::Type type = TaskDesc::toType(typeStr);
        const LOFAR::ParameterSet params = itsParset.makeSubset(keyBase + ".params.");
        return TaskDesc(name, type, params);
}

void Configuration::buildAntennas(void)
{
    // Build a map of name->Antenna
    const vector<string> antId = itsParset.getStringVector("antennas");
    const casa::Quantity defaultDiameter = asQuantity(itsParset.getString("antenna.ant.diameter"));
    const string defaultMount = itsParset.getString("antenna.ant.mount");
    const casa::Quantity defaultDelay = asQuantity(itsParset.getString("antenna.ant.delay", "0s"));
    map<string, Antenna> antennaMap;

    for (vector<string>::const_iterator it = antId.begin(); it != antId.end(); ++it) {
        const string keyBase = "antenna." + *it + ".";
        const string name = itsParset.getString(keyBase + "name");
        ASKAPCHECK(name.find(" ") == std::string::npos, 
              "Antenna names are expected to be single words. For "<<*it<<", you have: "<<name);
        const vector<double> location = itsParset.getDoubleVector(keyBase + "location.itrf");

        casa::Quantity diameter;
        if (itsParset.isDefined(keyBase + "diameter")) {
            diameter = asQuantity(itsParset.getString(keyBase + "diameter"));
        } else {
            diameter = defaultDiameter;
        }

        string mount;
        if (itsParset.isDefined(keyBase + "mount")) {
            mount = itsParset.getString(keyBase + "mount");
        } else {
            mount = defaultMount;
        }

        casa::Quantity delay;
        if (itsParset.isDefined(keyBase + "delay")) {
            delay = asQuantity(itsParset.getString(keyBase + "delay"));
        } else {
            delay = defaultDelay;
        }

        antennaMap.insert(make_pair(name, Antenna(name, mount, location, diameter, delay)));
    }
    
    // Now read "baselinemap.antennaidx" and build the antenna vector with the
    // ordering that maps to the baseline mapping
    const vector<string> antOrdering = itsParset.getStringVector("baselinemap.antennaidx");
    for (vector<string>::const_iterator it = antOrdering.begin(); it != antOrdering.end(); ++it) {
        map<string, Antenna>::const_iterator antit = antennaMap.find(*it);
        if (antit == antennaMap.end()) {
            ASKAPTHROW(AskapError, "Antenna " << *it << " is not configured");
        }
        ASKAPLOG_DEBUG_STR(logger, "Adding "<<antit->first<<": "<<antit->second.position()<<" as "<<antit->second.name());
        itsAntennas.push_back(antit->second);
    }
    ASKAPLOG_DEBUG_STR(logger, "Defined "<<itsAntennas.size()<<" antennas in the configuration");
}

/// @brief build vector of indices of the given antennas in the full map
/// @details This is a helper method to provide default indices for a selected list
/// of antennas (for ADE antennas are present in the natural order).
/// @param[in] ants vector of antenna names
/// @return vector of indices
/// @note Antenna names are assumed to be in the form of "??NN" where ? is an 
/// arbitrary letter and NN is an integer number 0..99 with a leading zero, if
/// necessary.
std::vector<int32_t> Configuration::buildValidAntIndices(const std::vector<std::string> &ants) {
    ASKAPLOG_DEBUG_STR(logger, "Default antenna indices will be derived from antenna names for "<<ants.size()<<
                               " antennas for which data to be ingested");
    std::vector<int32_t> result(ants.size());
    for (size_t ant = 0; ant < result.size(); ++ant) {
         ASKAPCHECK(ants[ant].size() == 4u, "Expect 4-letter antenna names e.g. ak01. You have "<<ants[ant]);
         result[ant] = utility::fromString<int32_t>(ants[ant].substr(2)) - 1;
         ASKAPCHECK(result[ant] >= 0, "Negative antenna indices are not expected, antenna numbers should be 1-based; antenna: "<<ants[ant]);
    }
    return result;
}

void Configuration::buildBaselineMap(void)
{
    itsBaselineMap.reset(new BaselineMap(itsParset.makeSubset("baselinemap.")));
    ASKAPDEBUGASSERT(itsBaselineMap);

    // the following code exist to assist early commissioning with sparse arrays
    // it introduces an additional mapping and probably should be removed when we 
    // transition to proper operations
    if (itsParset.isDefined("baselinemap.antennaindices")) {
        ASKAPLOG_INFO_STR(logger, "A subset of antenna indices will be selected from the defined correlator product configuration");
        const vector<string> antOrdering = itsParset.getStringVector("baselinemap.antennaidx");
        const vector<int32_t> parsetAntIndices = itsParset.getInt32Vector("baselinemap.antennaindices");
        const vector<int32_t> validAntIndices = parsetAntIndices.size() != 0 ? parsetAntIndices : buildValidAntIndices(antOrdering);
        ASKAPCHECK(validAntIndices.size() == antOrdering.size(),
                 "Number of antenna indices should match baselinemap.antennaidx; valid indices = "<<validAntIndices);
        for (size_t ant=0; ant<validAntIndices.size(); ++ant) {
             ASKAPLOG_DEBUG_STR(logger, "Re-mapping antenna "<<validAntIndices[ant]<<" ("<<antOrdering[ant]<<
                           ") to the new antenna index of "<<ant);
        }
        const size_t nMappedProductsBefore = itsBaselineMap->size();
        itsBaselineMap->sliceMap(validAntIndices);
        const size_t nMappedProductsAfter = itsBaselineMap->size();
        ASKAPLOG_DEBUG_STR(logger, "Reduced number of accepted correlation products from "<<nMappedProductsBefore<<
                      " to "<<nMappedProductsAfter);
    }
    //
}

void Configuration::buildCorrelatorModes(void)
{
    const vector<string> modes = itsParset.getStringVector("correlator.modes");
    vector<string>::const_iterator it;
    for (it = modes.begin(); it != modes.end(); ++it) {
        const string name = *it;
        const string keyBase = "correlator.mode." + name + ".";
        const casa::Quantity chanWidth = asQuantity(itsParset.getString(keyBase + "chan_width"));
        const casa::uInt nChan = itsParset.getUint32(keyBase + "n_chan");

        vector<casa::Stokes::StokesTypes> stokes;
        const vector<string> stokesStrings = itsParset.getStringVector(keyBase + "stokes");
        for (vector<string>::const_iterator it = stokesStrings.begin();
                it != stokesStrings.end(); ++it) {
            stokes.push_back(casa::Stokes::type(*it));
        }

        const casa::uInt interval = itsParset.getUint32(keyBase + "interval");

        const CorrelatorMode mode(name, chanWidth, nChan, stokes, interval);
        itsCorrelatorModes.insert(make_pair(name, mode));
    }
}

void Configuration::buildFeeds(void)
{
    const uint32_t N_RECEPTORS = 2; // Only support receptors "X Y"
    const uint32_t nFeeds = itsParset.getUint32("feeds.n_feeds");
    const casa::Quantity spacing = asQuantity(itsParset.getString("feeds.spacing"), "rad");

    // Get offsets for each feed/beam
    casa::Matrix<casa::Quantity> offsets(nFeeds, N_RECEPTORS);
    for (uint32_t i = 0; i < nFeeds; ++i) {
        const string key = "feeds.feed" + utility::toString(i);
        if (!itsParset.isDefined(key)) {
            ASKAPTHROW(AskapError, "Expected " << nFeeds << " feed offsets");
        }
        const vector<casa::Double> xy = itsParset.getDoubleVector(key);
        offsets(i, 0) = spacing * xy.at(0);
        offsets(i, 1) = spacing * xy.at(1);
    }
    casa::Vector<casa::String> pols(nFeeds, "X Y");

    itsFeedConfig.reset(new FeedConfig(offsets, pols));
}

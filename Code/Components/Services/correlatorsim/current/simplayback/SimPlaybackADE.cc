/// @file SimPlaybackADE.cc
///
/// @copyright (c) 2009 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>

// Include own header file first
#include "SimPlaybackADE.h"

// Include package level header file
#include "askap_correlatorsim.h"

// System includes
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <mpi.h>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "askap/AskapLogging.h"
#include "Common/ParameterSet.h"
#include "boost/shared_ptr.hpp"
#include "casa/OS/Timer.h"

// Local package includes
#include "simplayback/ISimulator.h"
#include "simplayback/CorrelatorSimulatorADE.h"
#include "simplayback/TosSimulator.h"

//#define VERBOSE

// Using
using namespace askap::cp;
using namespace std;

ASKAP_LOGGER(logger, ".SimPlaybackADE");

SimPlaybackADE::SimPlaybackADE(const LOFAR::ParameterSet& parset)
    : itsParset(parset.makeSubset("playback."))
{
    MPI_Comm_rank(MPI_COMM_WORLD, &itsRank);
    MPI_Comm_size(MPI_COMM_WORLD, &itsNumProcs);
    if (itsRank == 0) {
#ifdef VERBOSE
        cout << "MPI rank " << itsRank << 
                " is validating configuration" << endl;
#endif
        validateConfig();
    }
#ifdef VERBOSE
    else {
        cout << "MPI rank " << itsRank << " is waiting" << endl;
    }
#endif
}


SimPlaybackADE::~SimPlaybackADE()
{
}


void SimPlaybackADE::validateConfig(void)
{
#ifdef VERBOSE
	cout << "SimPlaybackADE::validateConfig..." << endl;
#endif
	const std::string nShelvesKey = "corrsim.n_shelves";

	const int nShelves = itsParset.getInt32(nShelvesKey);
	ASKAPCHECK(itsNumProcs == (nShelves+1),
			"Incorrect number of ranks for the requested configuration");
#ifdef VERBOSE
	cout << "nShelves: " << nShelves << endl;
#endif

	const std::string inputMode = itsParset.getString("input_mode");
	ASKAPCHECK(inputMode == "zero","Illegal input mode");
	
	// Build a list of required keys    
	std::vector<std::string> requiredKeys;
	requiredKeys.push_back(nShelvesKey);
	requiredKeys.push_back("tossim.ice.locator_host");
	requiredKeys.push_back("tossim.ice.locator_port");
	requiredKeys.push_back("tossim.icestorm.topicmanager");
	requiredKeys.push_back("tossim.icestorm.topic");
	
	for (int i = 0; i < nShelves; ++i) {
		std::ostringstream ss;
		ss << "corrsim.shelf" << i+1 << ".";

		if (inputMode == "expand") {
			std::string dataset = ss.str();
			dataset.append("dataset");
			requiredKeys.push_back(dataset);
		}

		std::string hostname = ss.str();
		hostname.append("out.hostname");
		requiredKeys.push_back(hostname);

		std::string port = ss.str();
		port.append("out.port");
		requiredKeys.push_back(port);
	}

	// Now check the required keys are present
	std::vector<std::string>::const_iterator it;
	it = requiredKeys.begin();

	while (it != requiredKeys.end()) {
		if (!itsParset.isDefined(*it)) {
			ASKAPTHROW(AskapError, 
					"Required key not present in parset: " << *it);
		}
		++it;
	}
#ifdef VERBOSE
	cout << "SimPlaybackADE::validateConfig: done" << endl; 
#endif
}


boost::shared_ptr<TosSimulator> SimPlaybackADE::makeTosSim(void)
{
#ifdef VERBOSE
	std::cout << "makeTosSim ... " << std::endl;
#endif
    const std::string filename = itsParset.getString("corrsim.shelf1.dataset","");
    const std::string locatorHost = itsParset.getString("tossim.ice.locator_host");
    const std::string locatorPort = itsParset.getString("tossim.ice.locator_port");
    const std::string topicManager = itsParset.getString(
			"tossim.icestorm.topicmanager");
    const std::string topic = itsParset.getString("tossim.icestorm.topic");
    const double failureChance = itsParset.getDouble(
			"tossim.random_metadata_send_fail", 0.0);

    return boost::shared_ptr<TosSimulator>(new TosSimulator(filename,
            locatorHost, locatorPort, topicManager, topic, failureChance));
#ifdef VERBOSE
	std::cout << "makeTosSim: done" << std::endl;
#endif
}


boost::shared_ptr<CorrelatorSimulatorADE> 
        SimPlaybackADE::makeCorrelatorSim(void)
{
#ifdef VERBOSE
	std::cout << "makeCorrelatorSim ... " << std::endl;
#endif
	std::ostringstream ss;
	ss << "corrsim.shelf" << itsRank << ".";
	const LOFAR::ParameterSet subset = itsParset.makeSubset(ss.str());
	std::string dataset = subset.getString("dataset", "");
	std::string hostname = subset.getString("out.hostname");
	std::string port = subset.getString("out.port");
	
    const unsigned int nAntenna = 
            itsParset.getUint32("corrsim.n_antennas", 1);
    const unsigned int nCoarseChannel =
            itsParset.getUint32("corrsim.n_coarse_channels", 304);
    const unsigned int nChannelSub =
            itsParset.getUint32("corrsim.n_channel_subdivision", 54);
    const double coarseBandwidth =
            itsParset.getDouble("corrsim.coarse_channel_bandwidth", 1000000);
    const unsigned int delay =
            itsParset.getUint32("corrsim.delay", 0);
    return boost::shared_ptr<CorrelatorSimulatorADE>(
            new CorrelatorSimulatorADE(dataset, hostname, port, 
            itsRank, nAntenna, nCoarseChannel, nChannelSub,
            coarseBandwidth, itsInputMode, delay));
#ifdef VERBOSE
	std::cout << "makeCorrelatorSim: done" << std::endl;
#endif
}


void SimPlaybackADE::run(void)
{
    // Wait for all processes to get here. The master alone checks the config
    // file so this barrier ensures the configuration has been validated
    // before all processes go and use it. If the master finds a problem
    // an MPI_Abort is called.
    MPI_Barrier(MPI_COMM_WORLD);

	itsInputMode = itsParset.getString ("input_mode","zero");
#ifdef VERBOSE
	std::cout << "itsInputMode: " << itsInputMode << std::endl;
#endif
	
    if (itsRank == 0) {
		if (itsInputMode == "expand") {
			boost::shared_ptr<ISimulator> sim = makeTosSim();
			bool moreData = true;
			while (moreData) {
				moreData = sim->sendNext();
			}
		}
    } else {
        boost::shared_ptr<ISimulator> sim = makeCorrelatorSim();
		bool moreData = true;
		while (moreData) {
			moreData = sim->sendNext();
		}
    }
	
    MPI_Barrier(MPI_COMM_WORLD);
	
#ifdef VERBOSE
	cout << "SimPlaybackADE::run: done" << endl;
#endif

}

#ifdef VERBOSE
#undef VERBOSE
#endif

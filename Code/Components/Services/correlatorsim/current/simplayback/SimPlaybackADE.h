/// @file SimPlaybackADE.h
///
/// @copyright (c) 2015 CSIRO
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

#ifndef ASKAP_CP_SIMPLAYBACKADE_H
#define ASKAP_CP_SIMPLAYBACKADE_H

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "boost/shared_ptr.hpp"

// Local package includes
#include "simplayback/CorrelatorSimulatorADE.h"
#include "simplayback/TosSimulator.h"

namespace askap {
namespace cp {

/// @brief Main class which simulates the ASKAP Correlator and Telescope
/// Operating System for the Central Processor.
///
/// The purpose of this software is to simulate the ASKAP correlator
/// for the purposes of testing the central processor. This simulator
/// described here is actually a playback simulator and relies on other
/// software (e.g. csimulator) to actually create a simulated measurement
/// set which will be played back by this software.

class SimPlaybackADE {
    public:
        /// @brief Constructor.
        ///
        /// @param[in] parset   configuration parameter set.
        SimPlaybackADE(const LOFAR::ParameterSet& parset);

        /// @brief Destructor.
        ~SimPlaybackADE();

        /// @brief Starts the playback.
        void run(void);

    private:
	
		//const static double TIMEMULTIPLIER = 1000000.0;
		
        // Validates the configuration parameter set, throwing an
        // exception if it is not suitable.
        void validateConfig(void);

        // Factory method of sorts, creates the TosSimulator instance.
        boost::shared_ptr<TosSimulator> makeTosSim(void);

        // Factory method of sorts, creates the Correlator Simulator instance.
        boost::shared_ptr<CorrelatorSimulatorADE> makeCorrelatorSim(void);

		// Get the number of antennas from parset
		//uint32_t getNAntenna();

		// Set parameter prefixes that are possibly used for key names.
		// Each call accumulates to a list of prefixes.
		// Note: do not forget that no prefix (ie. "") is also a prefix!
		void setParPrefixes(const string& prefix);

		// Check whether a parameter key exists.
		// The key might have a numbe of possible prefixes
		bool isParDefined(const string& key);

		// Get key name with its prefix
		string getPrefixAndKey(const string& key);

		// Get the values of parameter.
		// The key might have a number of possible prefixes
		uint32_t getPar(const string& key, const uint32_t defValue);
        string getPar(const string& key, const string& defValue);


        // ParameterSet (configuration)
        const LOFAR::ParameterSet itsParset;

        // Rank of this process
        int itsRank;

        // Total number of processes
        int itsNumProcs;

        // Playback mode
        string itsMode;

		// The number of times the input measurement set is played back
		uint32_t itsPlaybackLoop;

		// A set of parameter fixes
		vector<string> itsParPrefixes;
};
};

};

#endif

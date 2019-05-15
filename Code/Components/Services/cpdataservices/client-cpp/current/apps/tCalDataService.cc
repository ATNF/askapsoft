/// @file tCalibrationDataService.cc
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

// System includes
#include <fstream>
#include <string>
#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <sys/times.h>
#include <stdexcept>

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"
#include "Common/ParameterSet.h"
#include "CommandLineParser.h"

// Local package includes
#include "calibrationclient/CalibrationDataServiceClient.h"
#include "calibaccess/JonesIndex.h"
#include "calibaccess/JonesJTerm.h"
#include "calibaccess/JonesDTerm.h"
#include "calibrationclient/GenericSolution.h"

using namespace std;
using namespace askap::cp::caldataservice;

#include <sys/times.h>

class Stopwatch {
    public:
        Stopwatch() : m_start(static_cast<clock_t>(-1)) { }

        void start()
        {
            struct tms t;
            m_start = times(&t);

            if (m_start == static_cast<clock_t>(-1)) {
                throw runtime_error("Error calling times()");
            }

        }

        // Returns elapsed time (since start() was called) in seconds.
        double stop()
        {
            struct tms t;
            clock_t stop = times(&t);

            if (m_start == static_cast<clock_t>(-1)) {
                throw runtime_error("Start time not set");
            }

            if (stop == static_cast<clock_t>(-1)) {
                throw runtime_error("Error calling times()");
            }

            return (static_cast<double>(stop - m_start)) / (static_cast<double>(sysconf(_SC_CLK_TCK)));

        }

    private:
        clock_t m_start;
};

void addGainSolution(CalibrationDataServiceClient& svc, const casacore::Long id,
        const casacore::Double timestamp,
        const casacore::Short nAntenna, const casacore::Short nBeam)
{
    GainSolution sol(timestamp);
    // Create a map entry for each antenna/beam combination
    for (casacore::Short antenna = 1; antenna <= nAntenna; ++antenna) {
        for (casacore::Short beam = 1; beam <= nBeam; ++beam) {
            JonesJTerm jterm(casacore::Complex(1.0, 1.0), true,
                    casacore::Complex(1.0, 1.0), true);
            sol.map()[askap::accessors::JonesIndex(antenna, beam)] = jterm;
        }
    }

    svc.addGainSolution(id,sol);
}

void addLeakageSolution(CalibrationDataServiceClient& svc, const casacore::Long id,
        const casacore::Double timestamp,
        const casacore::Short nAntenna, const casacore::Short nBeam)
{
    LeakageSolution sol(timestamp);
    // Create a map entry for each antenna/beam combination
    for (casacore::Short antenna = 1; antenna <= nAntenna; ++antenna) {
        for (casacore::Short beam = 1; beam <= nBeam; ++beam) {
            sol.map()[askap::accessors::JonesIndex(antenna, beam)] = askap::accessors::JonesDTerm(
                    casacore::Complex(1.0, 1.0), casacore::Complex(1.0, 1.0));
        }
    }

    svc.addLeakageSolution(id,sol);
}

void addBandpassSolution(CalibrationDataServiceClient& svc, const casacore::Long id,
        const casacore::Double timestamp,
        const casacore::Short nAntenna, const casacore::Short nBeam, const casacore::Int nChan)
{
    BandpassSolution sol(timestamp);
    // Create a map entry for each antenna/beam combination
    for (casacore::Short antenna = 1; antenna <= nAntenna; ++antenna) {
        for (casacore::Short beam = 1; beam <= nBeam; ++beam) {
            JonesJTerm jterm(casacore::Complex(1.0, 1.0), true,
                    casacore::Complex(1.0, 1.0), true);
            std::vector<askap::accessors::JonesJTerm> jterms(nChan, jterm);
            sol.map()[askap::accessors::JonesIndex(antenna, beam)] = jterms;
        }
    }

    svc.addBandpassSolution(id,sol);
}

// main()
int main(int argc, char *argv[])
{
    // Command line parser
    cmdlineparser::Parser parser;

    // Command line parameter
    cmdlineparser::FlaggedParameter<string> inputsPar("-inputs", "tCalDataService.in");

    // Throw an exception if the parameter is not present
    parser.add(inputsPar, cmdlineparser::Parser::return_default);
    parser.process(argc, const_cast<char**> (argv));

    // Create a subset
    LOFAR::ParameterSet parset(inputsPar);

    const string locatorHost = parset.getString("ice.locator.host");
    const string locatorPort = parset.getString("ice.locator.port");
    const string serviceName = parset.getString("calibrationdataservice.name");
    const casacore::Short nAntenna = parset.getInt16("test.nantenna");
    const casacore::Short nBeam = parset.getInt16("test.nbeam");
    const casacore::Int nChan = parset.getInt32("test.nchannel");

    CalibrationDataServiceClient svc(locatorHost, locatorPort, serviceName);

    const casacore::Double timestamp = 55790.1;

    Stopwatch sw;
    sw.start();
    casacore::Long newID = svc.newSolutionID();
    double time = sw.stop();
    std::cout << "Time to get new solution ID: " << time << std::endl;

    sw.start();
    addGainSolution(svc, newID, timestamp, nAntenna, nBeam);
    time = sw.stop();
    std::cout << "Time to add gains solution: " << time << std::endl;

    sw.start();
    addLeakageSolution(svc, newID, timestamp, nAntenna, nBeam);
    time = sw.stop();
    std::cout << "Time to add leakage solution: " << time << std::endl;

    sw.start();
    addBandpassSolution(svc, newID, timestamp, nAntenna, nBeam, nChan);
    time = sw.stop();
    std::cout << "Time to add bandpass solution: " << time << std::endl;


    sw.start();
    GainSolution gsol = svc.getGainSolution(newID);
    time = sw.stop();
    std::cout << "Time to get gains solution: " << time << std::endl;

    sw.start();
    LeakageSolution lsol = svc.getLeakageSolution(newID);
    time = sw.stop();
    std::cout << "Time to get leakage solution: " << time << std::endl;

    sw.start();
    BandpassSolution bsol = svc.getBandpassSolution(newID);
    time = sw.stop();
    std::cout << "Time to get bandpass solution: " << time << std::endl;

    return 0;
}

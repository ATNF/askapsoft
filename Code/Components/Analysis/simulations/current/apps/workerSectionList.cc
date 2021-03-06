///
/// @file : List the subsection string for each worker
///
/// Control parameters are passed in from a LOFAR ParameterSet file.
///
/// @copyright (c) 2007 CSIRO
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
#include <askap_simulations.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <casacore/casa/Logging/LogIO.h>
#include <askap/Log4cxxLogSink.h>

#include <Common/ParameterSet.h>
#include <casacore/casa/OS/Timer.h>

#include <FITS/FITSparallel.h>
#include <FITS/FITSfile.h>
#include <analysisparallel/SubimageDef.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <stdlib.h>
#include <time.h>

using namespace askap;
using namespace askap::simulations;
using namespace askap::simulations::FITS;

ASKAP_LOGGER(logger, "workerSectionList.log");

// Move to Askap Util?
std::string getInputs(const std::string& key, const std::string& def, int argc,
                      const char** argv)
{
    if (argc > 2) {
        for (int arg = 0; arg < (argc - 1); arg++) {
            std::string argument = std::string(argv[arg]);

            if (argument == key) {
                return std::string(argv[arg + 1]);
            }
        }
    }

    return def;
}

// Main function
int main(int argc, const char** argv)
{
    askap::askapparallel::AskapParallel comms(argc, argv);
//     try {
    // Ensure that CASA log messages are captured
    casa::LogSinkInterface* globalSink = new Log4cxxLogSink();
    casa::LogSink::globalSink(globalSink);
    casa::Timer timer;
    timer.mark();
    srandom(time(0));
    std::string parsetFile(getInputs("-inputs", "createFITS.in", argc, argv));
    ASKAPLOG_INFO_STR(logger,  "parset file " << parsetFile);
    LOFAR::ParameterSet parset(parsetFile);
    LOFAR::ParameterSet subset(parset.makeSubset("createFITS."));

    std::string outfile = subset.getString("workerList", "workerSectionList.txt");
    int nsubx = subset.getInt32("nsubx", 1);
    int nsuby = subset.getInt32("nsuby", 1);
    int nsubz = subset.getInt32("nsubz", 1);
    int nworkers = nsubx * nsuby * nsubz;
    ASKAPLOG_DEBUG_STR(logger, nsubx << " " << nsuby << " " << nsubz << " " << nworkers);
    if (nworkers <= 1) {
        ASKAPLOG_WARN_STR(logger, "Number of workers required by parset is only one!");
        exit(0);
    }
    size_t dim = subset.getInt32("dim", 2);
    std::vector<int> axes = subset.getInt32Vector("axes");
    analysisutilities::SubimageDef subdef = analysisutilities::SubimageDef(subset);
    subdef.define(dim);
    subdef.setImageDim(axes);
    subdef.setInputSubsection(duchamp::nullSection(dim));
    subdef.defineAllSections();

    std::ofstream output(outfile.c_str());
    output << "# Worker Subsection\n";
    for (int i = 0; i < nworkers; i++) {
        output << i + 1 << "  " << subdef.section(i).getSection() << "\n";
    }


    ASKAPLOG_INFO_STR(logger, "Time for execution of workerSectionList = " <<
                      timer.real() << " sec");

//     } catch (const askap::AskapError& x) {
//         ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
//         std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
//         exit(1);
//     } catch (const std::exception& x) {
//         ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
//         std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what() << std::endl;
//         exit(1);
//     }

    return 0;
}


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

// System includes
#include <cstring>
#include <boost/chrono/include.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>

// ASKAPsoft includes
#include <askap_askap.h>
#include <askap/Application.h>
#include <askap/AskapLogging.h>

using namespace askap;


ASKAP_LOGGER(logger, ".logAllApp");

class LogAllApp : public askap::Application {
    public:

        LogAllApp() : 
            NUM_LOOPS("loops"),
            SLEEP_SECONDS("sleep")
        {
            addParameter(NUM_LOOPS, "n", "Number of loops/repeats", "1");
            addParameter(SLEEP_SECONDS, "s", "Number of seconds to sleep between repeats", "2");
        }

        // Overriding main so I can skip the otherwise required configuration
        // file. It would be easier if the Application class used the
        // template-method pattern
        virtual int main(int argc, char *argv[])
        {
            int status = EXIT_FAILURE;

            try {
                processCmdLineArgs(argc, argv);
                initLogging(argv[0]);
                status = run(argc, argv);
            } catch (const std::exception& e) {
                if (ASKAPLOG_ISCONFIGURED) {
                    ASKAPLOG_FATAL_STR(logger, "Error: " << e.what());
                } else {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
                return EXIT_FAILURE;
            }

            return status;
        }

        virtual int run(int argc, char* argv[])
        {
            // Sigh, the default values are not working as they should (and the
            // code in Application looks correct wrt to the Boost docs). Doing
            // this ugly workaround instead...
            int loops = 1;
            if (parameterExists(NUM_LOOPS))
                loops = boost::lexical_cast<int>(parameter(NUM_LOOPS));

            int sleepSeconds = 2;
            if (parameterExists(SLEEP_SECONDS))
                sleepSeconds = boost::lexical_cast<int>(parameter(SLEEP_SECONDS));

            ASKAPLOG_DEBUG_STR(logger, "loops: " << loops);
            ASKAPLOG_DEBUG_STR(logger, "sleep: " << sleepSeconds << " seconds");
            loop(loops, sleepSeconds);

            return 0;
        }

    private:

        void emitAllLogLevels(int i) 
        {
            ASKAPLOG_DEBUG_STR(logger, "Debug message #" << i);
            ASKAPLOG_INFO_STR(logger, "Info message #" << i);
            ASKAPLOG_WARN_STR(logger, "Warn message #" << i);
            ASKAPLOG_ERROR_STR(logger, "Error message #" << i);
            ASKAPLOG_FATAL_STR(logger, "Fatal message #" << i);
        }

        void loop(int loops, int sleepSeconds) 
        {
            for (int i = 0; i < loops; ++i) {
                emitAllLogLevels(i);
                if (i < loops - 1) {
                    boost::this_thread::sleep_for(
                        boost::chrono::seconds(sleepSeconds));
                }
            }
        }

        const char* NUM_LOOPS;
        const char* SLEEP_SECONDS;
};

int main(int argc, char* argv[]) 
{
    LogAllApp app;
    return app.main(argc, argv);
}

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

// ASKAPsoft includes
#include <askap_askap.h>
#include <askap/Application.h>
#include <askap/AskapLogging.h>

using namespace askap;

#define NUM_LOOPS "loops"
#define SLEEP_SECONDS "sleep"

ASKAP_LOGGER(logger, ".logAllApp");

class LogAllApp : public askap::Application {
    public:
        virtual int run(int argc, char* argv[])
        {
            // TODO: replace with boost::lexical_cast on the parameter values
            int loops = 1;
            int sleepSeconds = 2;

            loop(loops, sleepSeconds);

            return 0;
        }

    private:
        void emitAllLogLevels() {
            ASKAPLOG_DEBUG(logger, "Debug message");
            ASKAPLOG_INFO(logger, "Info message");
            ASKAPLOG_WARN(logger, "Warn message");
            ASKAPLOG_ERROR(logger, "Error message");
            ASKAPLOG_FATAL(logger, "Fatal message");
        }

        void loop(int loops, int sleepSeconds) {
            for (int i = 0; i < loops; ++i) {
                emitAllLogLevels();

                if (i < loops - 1) {
                    //TODO: sleep
                }
            }
        }
};

int main(int argc, char* argv[]) {

    LogAllApp app;
    app.addParameter(NUM_LOOPS, "n", "Number of loops/repeats", "1");
    app.addParameter(SLEEP_SECONDS, "s", "Number of seconds to sleep between repeats", "2");
    return app.main(argc, argv);
/*
  // Now set its level. Normally you do not need to set the
  // level of a logger programmatically. This is usually done
  // in configuration files.
  ASKAPLOG_INIT("tLogging.log_cfg");
  int i = 1;
  ASKAP_LOGGER(locallog, ".test");

  ASKAPLOG_WARN(locallog,"Warning. This is a warning.");
  ASKAPLOG_INFO(locallog,"This is an automatic (subpackage) log");
  ASKAPLOG_INFO_STR(locallog,"This is " << i << " log stream test.");
  return 0;
*/
}

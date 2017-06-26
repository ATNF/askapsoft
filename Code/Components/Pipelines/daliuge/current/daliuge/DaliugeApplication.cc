/// @file DaliugeApplication.cc
///
/// @copyright (c) 2017 CSIRO
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
/// @author Stephen Ord <stephen.ord@csiro.au>

// Include own header file first
#include "daliuge/DaliugeApplication.h"

// Include package level header file
#include "askap_daliuge_pipeline.h"

// System includes
#include <string>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <iostream>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/Log4cxxLogSink.h"
#include "log4cxx/logger.h"
#include "log4cxx/logmanager.h"
#include "log4cxx/consoleappender.h"
#include "log4cxx/patternlayout.h"
#include "boost/program_options.hpp"
#include "casacore/casa/Logging/LogIO.h"
#include "casacore/casa/Logging/LogSinkInterface.h"

ASKAP_LOGGER(logger, ".DaliugeApplication");

// Using/namespace
using namespace askap;

DaliugeApplication::DaliugeApplication() {
    ASKAPLOG_DEBUG_STR(logger,"DaliugeApplication default constructor");
}

DaliugeApplication::~DaliugeApplication() {
    ASKAPLOG_DEBUG_STR(logger,"DaliugeApplication default destructor");
}

DaliugeApplication::ShPtr DaliugeApplication::createDaliugeApplication(const std::string& name)
{
   ASKAPTHROW(AskapError, "createDaliugeApplication is supposed to be defined for every derived application, "
                          "DaliugeApplication::createDaliugeApplication should never be called");
   return DaliugeApplication::ShPtr();
}

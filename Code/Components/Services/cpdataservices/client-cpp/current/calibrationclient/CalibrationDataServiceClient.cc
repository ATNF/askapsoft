/// @file CalibrationDataServiceClient.cc
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
#include "CalibrationDataServiceClient.h"

// Include package level header file
#include "askap_cpdataservices.h"

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"
#include <askap/askap/AskapLogging.h>
#include "askap/AskapError.h"
#include "Ice/Ice.h"
#include "iceutils/CommunicatorConfig.h"
#include "iceutils/CommunicatorFactory.h"
#include "CalibrationDataService.h" // Ice generated interface

// Local package includes
#include "calibrationclient/GenericSolution.h"
#include "calibrationclient/IceMapper.h"

// Using
using namespace std;
using namespace askap;
using namespace askap::cp::caldataservice;
using askap::interfaces::caldataservice::UnknownSolutionIdException;
using askap::interfaces::caldataservice::AlreadyExists;

ASKAP_LOGGER(logger, ".CalibrationDataServiceClient");

CalibrationDataServiceClient::CalibrationDataServiceClient(const std::string& locatorHost,
        const std::string& locatorPort,
        const std::string& serviceName)
{
    askap::cp::icewrapper::CommunicatorConfig config(locatorHost, locatorPort);
    config.setProperty("Ice.MessageSizeMax", "131072");
    askap::cp::icewrapper::CommunicatorFactory commFactory;
    itsComm = commFactory.createCommunicator(config);

    ASKAPDEBUGASSERT(itsComm);

    Ice::ObjectPrx base = itsComm->stringToProxy(serviceName);
    itsService = askap::interfaces::caldataservice::ICalibrationDataServicePrx::checkedCast(base);

    if (!itsService) {
        ASKAPTHROW(AskapError, "CalibrationDataService proxy is invalid");

    }
    ASKAPLOG_INFO_STR(logger,"Conected to CalibrationDataService");
}

CalibrationDataServiceClient::~CalibrationDataServiceClient()
{
    itsComm->destroy();
}

void CalibrationDataServiceClient::addGainSolution(casacore::Long id, const GainSolution& sol)
{
    itsService->addGainsSolution(id,IceMapper::toIce(sol));
}

void CalibrationDataServiceClient::addLeakageSolution(casacore::Long id, const LeakageSolution& sol)
{
    itsService->addLeakageSolution(id, IceMapper::toIce(sol));
}

void CalibrationDataServiceClient::addBandpassSolution(casacore::Long id, const BandpassSolution& sol)
{
    itsService->addBandpassSolution(id, IceMapper::toIce(sol));
}

casacore::Long CalibrationDataServiceClient::getLatestSolutionID(void)
{
    return itsService->getLatestSolutionID();
}

/// Create new solution ID to use with add functions
///
/// @return ID of the brand new entry to with calibration solutions can be attached
casacore::Long CalibrationDataServiceClient::newSolutionID()
{
    return itsService->newSolutionID();
}


GainSolution CalibrationDataServiceClient::getGainSolution(const casacore::Long id)
{
    askap::interfaces::calparams::TimeTaggedGainSolution ice_sol;
    try {
        ice_sol = itsService->getGainSolution(id);
    } catch (const UnknownSolutionIdException& e) {
        ASKAPTHROW(AskapError, "Unknown Solution ID");
    }
    return IceMapper::fromIce(ice_sol);
}

LeakageSolution CalibrationDataServiceClient::getLeakageSolution(const casacore::Long id)
{
    askap::interfaces::calparams::TimeTaggedLeakageSolution ice_sol;
    try {
        ice_sol = itsService->getLeakageSolution(id);
    } catch (const UnknownSolutionIdException& e) {
        ASKAPTHROW(AskapError, "Unknown Solution ID");
    }
    return IceMapper::fromIce(ice_sol);
}

BandpassSolution CalibrationDataServiceClient::getBandpassSolution(const casacore::Long id)
{
    askap::interfaces::calparams::TimeTaggedBandpassSolution ice_sol;
    try {
        ice_sol = itsService->getBandpassSolution(id);
    } catch (const UnknownSolutionIdException& e) {
        ASKAPTHROW(AskapError, "Unknown Solution ID");
    }
    return IceMapper::fromIce(ice_sol);

}
void CalibrationDataServiceClient::adjustGains(casacore::Long id, GainSolution &sol)
{
  try {
    itsService->adjustGains(id, IceMapper::toIce(sol));
  } catch (const AlreadyExists& e) {
      ASKAPTHROW(AskapError, "Gain Solution already added");
  }

}
void CalibrationDataServiceClient::adjustLeakages(long id, LeakageSolution &sol)
{
  try {
    itsService->adjustLeakages(id, IceMapper::toIce(sol));
  } catch (const AlreadyExists& e) {
      ASKAPTHROW(AskapError, "Leakage Solution already added");
  }

}
void CalibrationDataServiceClient::adjustBandpass(long id, BandpassSolution &sol)
{
  try {
    itsService->adjustBandpass(id, IceMapper::toIce(sol));
  } catch (const AlreadyExists& e) {
      ASKAPTHROW(AskapError, "Bandpass Solution already added");
  }

}

bool CalibrationDataServiceClient::hasGainSolution(long id) {
  return itsService->hasGainSolution(id);
}
bool CalibrationDataServiceClient::hasLeakageSolution(long id) {
  return itsService->hasLeakageSolution(id);
}
bool CalibrationDataServiceClient::hasBandpassSolution(long id) {
  return itsService->hasBandpassSolution(id);
}

/**
 * Obtain smallest solution ID corresponding to the time >= the given timestamp
 * @param timestamp absolute time given as MJD in the UTC frame (same as timestamp
 *                  in solutions - can be compared directly)
 * @return solution ID
 * @note gain, bandpass and leakage solutions corresponding to one solution ID
 *       can have different timestamps. Use the greatest for comparison.
 * if all the timestamps in the stored solutions are less than the given timestamp,
 * this method is equivalent to getLatestSolutionID().
 */
long CalibrationDataServiceClient::getLowerBoundID(double timestamp)
{
  try {
    return itsService->getLowerBoundID(timestamp);
  } catch (const UnknownSolutionIdException& e) {
      ASKAPTHROW(AskapError, "Unknown Solution ID"); // that is not very informative

  }
}
long CalibrationDataServiceClient::getUpperBoundID(double timestamp)
{
  try {
    return itsService->getUpperBoundID(timestamp);
  } catch (const UnknownSolutionIdException& e) {
      ASKAPTHROW(AskapError, "Unknown Solution ID"); // that is not very informative

  }

}

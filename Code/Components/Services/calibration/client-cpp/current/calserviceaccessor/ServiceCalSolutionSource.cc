/// @file
/// @brief Service based implementation of the calibration solution source
/// @details This implementation is to be used with the Calibration Data Service
/// Main functionality is implemented in the corresponding ServiceCalSolutionAccessor class.
/// This class just creates an instance of the accessor and manages it.
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
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>
/// @author Stephen Ord <Stephen.Ord@csiro.au>


#include <calibaccess/ServiceCalSolutionSourceStub.h>
#include "ServiceCalSolutionSource.h"
#include "ServiceCalSolutionAccessor.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>


// logging stuff
#include <askap_accessors.h>
#include <askap/askap/AskapLogging.h>
ASKAP_LOGGER(logger, ".calibaccess");

namespace askap {

namespace accessors {

/// @brief constructor
/// @details Creates solution source object for a given parset file
/// (whether it is for writing or reading depends on the actual methods
/// used). Also need to decide whether it is the source or the accessor that
/// creates the client. I think it should be the source then an accessor of any
/// particular type can be instaniated when required.
/// But the accessor does need a communicator....
/// how about the source instantiates the client and the accessor is instantiaed using the client
///
/// @param[in] parset parset file name
ServiceCalSolutionSource::ServiceCalSolutionSource(const LOFAR::ParameterSet &parset) : ServiceCalSolutionSourceStub(parset)
  {
  ASKAPLOG_WARN_STR(logger, "ServiceCalSolutionSource constructor - override the stub");

  // Need to generate the calibrationclient and set up all the solutions


  const string locatorHost = parset.getString("ice.locator.host");
  const string locatorPort = parset.getString("ice.locator.port");
  const string serviceName = parset.getString("calibrationdataservice.name");
  long solution = parset.getInt("solution.id",-1);
  const double solutionTime = parset.getDouble("solution.time",-1.0);
  bool newSol = parset.getBool("solution.new",false);


  itsClient = boost::make_shared<askap::cp::caldataservice::CalibrationDataServiceClient> (locatorHost, locatorPort, serviceName);

  if (solutionTime > 0) {
    if (solution > 0) {
      ASKAPTHROW(AskapError, "Ambiguous parameters: Specified a solution ID and a time");
    }
    if (!newSol) {
      solution = this->solutionID(solutionTime);
    }
    else {
      // new solution requested
      // need to know the solution size for this to work
      solution = this->newSolutionID(solutionTime);


    }
  }

  // should have a valid solution ID now.


  if (solution > 0) {
    /// Requesting a specific solutionID
    if (!newSol) {
        ASKAPLOG_WARN_STR(logger, "ServiceCalSolutionAccessor Read Only with a known solution ID");
        itsAccessor.reset(new ServiceCalSolutionAccessor(itsClient, solution, true));
    }
    else {
        ASKAPLOG_WARN_STR(logger, "ServiceCalSolutionAccessor RW with a new solution ID");

        itsAccessor.reset(new ServiceCalSolutionAccessor(itsClient, solution,false));

        int nAnt = parset.getInt("solution.nant",0);
        if (nAnt == 0) {
          nAnt = parset.getInt("nAnt",0);
        }
        int nBeam = parset.getInt("solution.nbeam",0);
        if (nBeam == 0) {
          nBeam = parset.getInt("nBeam",0);
        }

        if (nAnt == 0 || nBeam == 0) {
          ASKAPTHROW(AskapError, "Ambiguous parameters:Specified new solution but did not provide nAnt or NBeam");
        }
        else {

          this->addDefaultGainSolution(solution, solutionTime, nAnt, nBeam);
          this->addDefaultLeakageSolution(solution, solutionTime, nAnt, nBeam);
        }
        int nChan = parset.getInt("solution.nchan",0);
        if (nChan == 0) {
          nChan = parset.getInt("nChan",0);
        }
        if (nChan == 0) {
          ASKAPLOG_WARN_STR(logger, "Cannot add a bandpass solution for this ID - no chan in parset");

        }
        else {
          this->addDefaultBandpassSolution(solution, solutionTime, nAnt, nBeam,nChan);
        }
        boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());
        acc->solutionsValid = true; // valid but default ....

    }
  }
  else if (solution == 0){
    ASKAPLOG_WARN_STR(logger, "ServiceCalSolutionAccessor Read Only with the most recent ID");
    itsAccessor.reset(new ServiceCalSolutionAccessor(itsClient,this->mostRecentSolution(),true));
  }
  else if (solution < 0) {
    /// Going to make a completely new solution
    /// specify a blank one?
    ASKAPLOG_WARN_STR(logger, "ServiceCalSolutionAccessor NO ACCESSOR");
    if (solutionTime < 0) {
      ASKAPTHROW(AskapError, "Ambiguous parameters: Specified a new solution but did not give a timestamp");
    }

  }
}

/// @brief obtain ID for the most recent solution
/// @return ID for the most recent solution
/// @note This particular implementation doesn't support multiple
/// solutions and, therefore, always returns the same ID.
long ServiceCalSolutionSource::mostRecentSolution() const
{

  return itsClient->getLatestSolutionID();

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

long ServiceCalSolutionSource::solutionID(const double timetag) const
{

  itsClient->getUpperBoundID(timetag);

  return 0;
}

/// @brief obtain read-only accessor for a given solution ID
/// @details This method returns a shared pointer to the solution accessor, which
/// can be used to read the parameters. If a solution with the given ID doesn't
/// exist, an exception is thrown. Existing solutions with undefined parameters
/// are managed via validity flags of gains, leakages and bandpasses
/// @return shared pointer to an accessor object
/// @note This particular implementation doesn't support multiple solutions and
/// always returns the same accessor (for both reading and writing)
boost::shared_ptr<ICalSolutionConstAccessor> ServiceCalSolutionSource::roSolution(const long ID) const
{

  ASKAPDEBUGASSERT(accessor());
  boost::shared_ptr<ICalSolutionAccessor> acc = boost::dynamic_pointer_cast<ICalSolutionAccessor>(accessor());
  ASKAPCHECK(acc,
     "Unable to cast solution accessor to read-write type, CalSolutionSourceStub has been initialised with an incompatible object");
  return acc;
}

/// @brief obtain a solution ID to store new solution
/// @details This method provides a solution ID for a new solution. It must
/// be called before any write operation (one needs a writable accessor to
/// write the actual solution and to get this accessor one needs an ID).
/// @param[in] time time stamp of the new solution in seconds since MJD of 0.
/// @return solution ID

long ServiceCalSolutionSource::newSolutionID(const double timetag)
{
  // the time tag is added to the solution when it is created.
  return itsClient->newSolutionID();

}
/// @brief obtain a writeable accessor for a given solution ID
/// @details This method returns a shared pointer to the solution accessor, which
/// can be used to both read the parameters and write them back. If a solution with
/// the given ID doesn't exist, an exception is thrown. Existing solutions with undefined
/// parameters are managed via validity flags of gains, leakages and bandpasses
/// @return shared pointer to an accessor object
/// @note This particular implementation returns the same accessor regardless of the
/// chosen ID (for both reading and writing)
boost::shared_ptr<ICalSolutionAccessor> ServiceCalSolutionSource::rwSolution(const long ID) const
{

  ASKAPDEBUGASSERT(accessor());
  boost::shared_ptr<ICalSolutionAccessor> acc = boost::dynamic_pointer_cast<ICalSolutionAccessor>(accessor());
  ASKAPCHECK(acc,
     "Unable to cast solution accessor to read-write type, CalSolutionSourceStub has been initialised with an incompatible object");
  return acc;
}
void ServiceCalSolutionSource::addDefaultGainSolution(const long id,
        const double timestamp,
        const short nAntenna, const short nBeam)
{
    ASKAPASSERT(accessor());
    boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());

    ASKAPLOG_INFO_STR(logger, "addDefaultGainSolution");
    askap::cp::caldataservice::GainSolution sol(timestamp);
    // Create a map entry for each antenna/beam combination
    for (short  antenna = 0; antenna < nAntenna; ++antenna) {
        for (short beam = 0; beam < nBeam; ++beam) {
            JonesJTerm jterm(casacore::Complex(1.0, 1.0), true,
                    casacore::Complex(1.0, 1.0), true);
            sol.map()[askap::accessors::JonesIndex(antenna, beam)] = jterm;
        }
    }

    acc->addGainSolution(sol);
}


void ServiceCalSolutionSource::addDefaultLeakageSolution( const long id,
        const double timestamp,
        const short nAntenna, const short nBeam)
{
    ASKAPASSERT(accessor());
    boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());
    ASKAPLOG_INFO_STR(logger, "addDefaultLeakageSolution");
    askap::cp::caldataservice::LeakageSolution sol(timestamp);
    // Create a map entry for each antenna/beam combination
    for (short antenna = 0; antenna < nAntenna; ++antenna) {
        for (short beam = 0; beam < nBeam; ++beam) {
            sol.map()[askap::accessors::JonesIndex(antenna, beam)] = askap::accessors::JonesDTerm(
                    casacore::Complex(1.0, 1.0), casacore::Complex(1.0, 1.0));
        }
    }

    acc->addLeakageSolution(sol);
}

void ServiceCalSolutionSource::addDefaultBandpassSolution(const long id,
        const double timestamp,
        const short nAntenna, const short nBeam, const int nChan)
{
    ASKAPASSERT(accessor());
    boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());
    ASKAPLOG_INFO_STR(logger, "addDefaultBandpassSolution");
    askap::cp::caldataservice::BandpassSolution sol(timestamp);
    // Create a map entry for each antenna/beam combination
    for (short antenna = 0; antenna < nAntenna; ++antenna) {
        for (short beam = 0; beam < nBeam; ++beam) {
            JonesJTerm jterm(casacore::Complex(1.0, 1.0), true,
                    casacore::Complex(1.0, 1.0), true);
            std::vector<askap::accessors::JonesJTerm> jterms(nChan, jterm);
            sol.map()[askap::accessors::JonesIndex(antenna, beam)] = jterms;
        }
    }

    acc->addBandpassSolution(sol);
}

/// @brief this source will be used to solve for the following:
/// @details this works by setting flags in the accessor to allow it to
/// update the solution in the database. Without these the solution will still
/// be found but <not> pushed to the service

void ServiceCalSolutionSource::solveGains() {
  ASKAPASSERT(accessor());
  boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());
  acc->willPushGains();

}
void ServiceCalSolutionSource::solveLeakages() {
  ASKAPASSERT(accessor());
  boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());
  acc->willPushLeakages();

}
void ServiceCalSolutionSource::solveBandpass() {
  ASKAPASSERT(accessor());
  boost::shared_ptr<ServiceCalSolutionAccessor> acc = boost::dynamic_pointer_cast<ServiceCalSolutionAccessor>(accessor());
  acc->willPushBandpass();

}


} // accessors

} // namespace askap

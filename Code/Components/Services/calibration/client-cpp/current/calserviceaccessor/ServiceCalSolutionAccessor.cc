/// @file
/// @brief implementation of the calibration solution accessor returning values from the Calibration Data Service
/// @details This class supports all calibration products (i.e. gains, bandpasses and leakages)
/// it accesses the Calibration Data Service directly for the information.
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



#include <Ice/Ice.h>

#include <iceutils/CommunicatorConfig.h>
#include <iceutils/CommunicatorFactory.h>
#include <CalibrationDataService.h> // Ice generated interface
#include <calibrationclient/CalibrationDataServiceClient.h>
#include <calibrationclient/GenericSolution.h>

#include <askap/calibaccess/ICalSolutionAccessor.h>

#include "ServiceCalSolutionAccessor.h"

#include <calibrationclient/IceMapper.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>
#include <Common/ParameterSet.h>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace askap::accessors;
ASKAP_LOGGER(logger, ".ServiceCalSolutionAccessor");
namespace askap {

namespace accessors {


/// @brief constructor
/// @details It reads the given parset file, configures the service client
/// @param[in] parset parset file name
/// @param[in] the iD of the solution to get - or to make
/// @param[in] readonly if true, additional checks are done that file exists
ServiceCalSolutionAccessor::ServiceCalSolutionAccessor(const LOFAR::ParameterSet &parset, casacore::Long iD, bool readonly) : itsGainSolution(0), itsLeakageSolution(0), itsBandpassSolution(0), itsReadOnly(readonly), pushGains(false),pushLeakages(false),pushBandpass(false)

{

  // Need to generate the calibrationclient and set up all the solutions
  ASKAPLOG_INFO_STR(logger,"Setting up client");
  const string locatorHost = parset.getString("ice.locator.host");
  const string locatorPort = parset.getString("ice.locator.port");
  const string serviceName = parset.getString("calibrationdataservice.name");

  theClientPtr = boost::make_shared<askap::cp::caldataservice::CalibrationDataServiceClient> (locatorHost, locatorPort, serviceName);
  ASKAPLOG_INFO_STR(logger,"Done - client connected");

  this->solutionID = iD;

  ASKAPLOG_INFO_STR(logger, "Current ID " << this->solutionID);
  ASKAPLOG_INFO_STR(logger, "Latest ID " << theClientPtr->getLatestSolutionID());

  if (itsReadOnly) { // solutions exist and are being pulled from the service
    try {
      this->pullSolutions();
    }
    catch (const interfaces::caldataservice::UnknownSolutionIdException& e) {
      ASKAPTHROW(AskapError, "Unknown Solution ID");
    }
  } // else the solution source has filled the local solutions with defaults.



}

ServiceCalSolutionAccessor::ServiceCalSolutionAccessor(boost::shared_ptr<askap::cp::caldataservice::CalibrationDataServiceClient> inClient, casacore::Long iD, bool readonly) : itsGainSolution(0), itsLeakageSolution(0), itsBandpassSolution(0),itsReadOnly(readonly),pushGains(false),pushLeakages(false),pushBandpass(false)

{
  ASKAPLOG_INFO_STR(logger,"Constructed with CalibrationDataServiceClient");
  theClientPtr = inClient;

  this->solutionID = iD;
  // debug ...
  // debug ...
  ASKAPLOG_INFO_STR(logger, "Current ID " << this->solutionID);
  ASKAPLOG_INFO_STR(logger, "Latest ID " << theClientPtr->getLatestSolutionID());
  if (itsReadOnly) { // ReadOnly solutions pulled from server
    try {
      this->pullSolutions();
    }
    catch (const interfaces::caldataservice::UnknownSolutionIdException& e) {
      ASKAPTHROW(AskapError, "Unknown Solution ID");
    }
  }
}
/// @brief obtain gains (J-Jones)
/// @details This method retrieves parallel-hand gains for both
/// polarisations (corresponding to XX and YY). If no gains are defined
/// for a particular index, gains of 1. with invalid flags set are
/// returned.
/// @param[in] index ant/beam index
/// @return JonesJTerm object with gains and validity flags
accessors::JonesJTerm ServiceCalSolutionAccessor::gain(const accessors::JonesIndex &index) const
{
  ASKAPASSERT(this->solutionsValid);
  map<accessors::JonesIndex,accessors::JonesJTerm> gains = itsGainSolution.map();
  return gains[index];


}

/// @brief obtain leakage (D-Jones)
/// @details This method retrieves cross-hand elements of the
/// Jones matrix (polarisation leakages). There are two values
/// (corresponding to XY and YX) returned (as members of JonesDTerm
/// class). If no leakages are defined for a particular index,
/// zero leakages are returned with invalid flags set.
/// @param[in] index ant/beam index
/// @return JonesDTerm object with leakages and validity flags
accessors::JonesDTerm ServiceCalSolutionAccessor::leakage(const accessors::JonesIndex &index) const
{
  ASKAPASSERT(this->solutionsValid);
  map<accessors::JonesIndex,accessors::JonesDTerm> leakages = itsLeakageSolution.map();
  return leakages[index];
}

/// @brief obtain bandpass (frequency dependent J-Jones)
/// @details This method retrieves parallel-hand spectral
/// channel-dependent gain (also known as bandpass) for a
/// given channel and antenna/beam. The actual implementation
/// does not necessarily store these channel-dependent gains
/// in an array. It could also implement interpolation or
/// sample a polynomial fit at the given channel (and
/// parameters of the polynomial could be in the database). If
/// no bandpass is defined (at all or for this particular channel),
/// gains of 1.0 are returned (with invalid flag is set).
/// @param[in] index ant/beam index
/// @param[in] chan spectral channel of interest
/// @return JonesJTerm object with gains and validity flags
accessors::JonesJTerm ServiceCalSolutionAccessor::bandpass(const accessors::JonesIndex &index, const casacore::uInt chan) const
{

  ASKAPASSERT(this->solutionsValid);
  typedef accessors::JonesIndex keyType;
  typedef std::vector<accessors::JonesJTerm> valueType;

  // ASKAPLOG_INFO_STR(logger, "searching for antenna " << index.antenna() << " beam " << index.beam());

  const std::map< keyType, valueType >& bandpass = itsBandpassSolution.map();
  const valueType& terms = (bandpass.find(index))->second;
  return terms[chan];

  // std::map<keyType, valueType>::const_iterator it;
  // for (it = bandpass.begin(); it != bandpass.end(); ++it) {
  //    const valueType& terms = it->second;
  //    const keyType ind = it->first;
  //
  //    std::cout << "Found antenna " << ind.antenna() << " beam " << ind.beam() << std::endl;
  //
  //    for (size_t ch = 0; ch < terms.size(); ++ch) {
  //
  //        std::cout << "chan " << ch << " g1 " << terms[ch].g1() << " g2 " << terms[ch].g2() << std::endl;
  //    }
  //    if (ind == index) {
  //       std::cout << "Match " << std::endl;
  //       return terms[chan];
  //    }
  // }


}

/// @brief set gains (J-Jones)
/// @details This method writes parallel-hand gains for both
/// polarisations (corresponding to XX and YY)
/// @param[in] index ant/beam index
/// @param[in] gains JonesJTerm object with gains and validity flags
void ServiceCalSolutionAccessor::setGain(const accessors::JonesIndex &index, const accessors::JonesJTerm &gains)
{

    itsGainSolution.map()[index] = gains;
}

/// @brief set leakages (D-Jones)
/// @details This method writes cross-pol leakages
/// (corresponding to XY and YX)
/// @param[in] index ant/beam index
/// @param[in] leakages JonesDTerm object with leakages and validity flags
void ServiceCalSolutionAccessor::setLeakage(const accessors::JonesIndex &index, const accessors::JonesDTerm &leakages)
{
    itsLeakageSolution.map()[index] = leakages;
}

/// @brief set gains for a single bandpass channel
/// @details This method writes parallel-hand gains corresponding to a single
/// spectral channel (i.e. one bandpass element).
/// @param[in] index ant/beam index
/// @param[in] bp JonesJTerm object with gains for the given channel and validity flags
/// @param[in] chan spectral channel
/// @note We may add later variants of this method assuming that the bandpass is
/// approximated somehow, e.g. by a polynomial. For simplicity, for now we deal with
/// gains set explicitly for each channel.
void ServiceCalSolutionAccessor::setBandpass(const accessors::JonesIndex &index, const accessors::JonesJTerm &bp, const casacore::uInt chan)
{
    itsBandpassSolution.map()[index][chan] = bp;
}

/// private member functions
void ServiceCalSolutionAccessor::pullSolutions() {

  try {
    ASKAPLOG_INFO_STR(logger, "Attempting to pull Gain Solution from client");
    itsGainSolution = this->theClientPtr->getGainSolution(this->solutionID);

    ASKAPLOG_INFO_STR(logger, "Attempting to pull Leakage Solution from client");
    itsLeakageSolution = this->theClientPtr->getLeakageSolution(this->solutionID);

    ASKAPLOG_INFO_STR(logger, "Attempting to pull Bandpass Solution from client");
    itsBandpassSolution = this->theClientPtr->getBandpassSolution(this->solutionID);

    this->solutionsValid = true;
  }
  catch (const interfaces::caldataservice::UnknownSolutionIdException& e) {
      ASKAPTHROW(AskapError, "Unknown Solution ID");
  }

}
void ServiceCalSolutionAccessor::pushSolutions() {

  /// should I split this into 3 different calls ....
  /// These need to be around conditionals as the service does not allow solutions to
  /// be adjusted


  if (pushGains) {
    ASKAPLOG_INFO_STR(logger, "Pushing Gain solution for ID " << this->solutionID);
    theClientPtr->addGainSolution(this->solutionID,(this->itsGainSolution));
  }
  if (pushLeakages) {
    ASKAPLOG_INFO_STR(logger, "Pushing Leakage solution for ID " << this->solutionID);
    theClientPtr->addLeakageSolution(this->solutionID,(this->itsLeakageSolution));
  }
  if (pushBandpass) {
    ASKAPLOG_INFO_STR(logger, "Pushing Bandpass solution for ID " << this->solutionID);
    theClientPtr->addBandpassSolution(this->solutionID,(this->itsBandpassSolution));
  }

  //ASKAPLOG_INFO_STR(logger, "Latest (after push) ID " << theClientPtr->getLatestSolutionID());

}
/// @brief destructor
/// @details Do we need it to call pushSolutions at the end
ServiceCalSolutionAccessor::~ServiceCalSolutionAccessor()
{
  if (!itsReadOnly) {
    this->pushSolutions();
  }
}



} // namespace accessors

} // namespace askap

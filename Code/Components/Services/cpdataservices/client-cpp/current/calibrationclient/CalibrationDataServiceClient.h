/// @file CalibrationDataServiceClient.h
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

#ifndef ASKAP_CP_CALDATASERVICE_CALIBRATIONDATASERVICE_H
#define ASKAP_CP_CALDATASERVICE_CALIBRATIONDATASERVICE_H

// System includes
#include <string>

// ASKAPsoft includes
#include "Ice/Ice.h"
#include "CalibrationDataService.h" // Ice generated interface

// Local package includes
#include "calibrationclient/GenericSolution.h"

namespace askap {
namespace cp {
namespace caldataservice {

/// @brief C++ client wrapper for the Calibration Data Service.
class CalibrationDataServiceClient {

    public:
        /// Constructor.
        ///
        /// The three parameters passed allow an instance of the calibration
        /// data service to be located in an ICE registry.
        ///
        /// @param[in] locatorHost  host of the ICE locator service.
        /// @param[in] locatorPort  port of the ICE locator service.
        /// @param[in] serviceName  identity of the calibration data service
        ///                         in the ICE registry.
        CalibrationDataServiceClient(const std::string& locatorHost,
                                     const std::string& locatorPort,
                                     const std::string& serviceName = "CalibrationDataService");

        /// Destructor.
        ~CalibrationDataServiceClient();


        /// Create new solution ID to use with add functions
        ///
        /// @return ID of the brand new entry to with calibration solutions can be attached
        casa::Long newSolutionID();

        /// Add a new gain solution to the data service. This method is
        /// intended to be used by the calibratin pipeine, and is called
        /// to submit new gain solutions.
        ///
        /// @param[in] id solution id to work with
        /// @param[in] sol  the gain solution to add.
        void addGainSolution(casa::Long id, const GainSolution& sol);

        /// Add a new leakage solution to the data service. This method is
        /// intended to be used by the calibratin pipeine, and is called
        /// to submit new leakage solutions.
        ///
        /// @param[in] id solution id to work with
        /// @param[in] sol  the leakage solution to add.
        /// @return a unique id referencing the solution in the data service.
        void addLeakageSolution(casa::Long id, const LeakageSolution& sol);

        /// Add a new bandpass solution to the data service. This method is
        /// intended to be used by the calibratin pipeine, and is called
        /// to submit new bandpass solutions.
        ///
        /// @param[in] id solution id to work with
        /// @param[in] sol  the bandpass solution to add.
        /// @return a unique id referencing the solution in the data service.
        void addBandpassSolution(casa::Long id, const BandpassSolution& sol);

        /// Obtain the ID for the latest solution.
        ///
        /// @return the ID of the latest solution
        casa::Long getLatestSolutionID(void);

        /// Get a gain solution.
        ///
        /// @param[in] id   id of the gain solution to obtain.
        /// @return the gain solution.
        /// @throw AskapError if the solution specified by the id does not
        ///         exist.
        GainSolution getGainSolution(const casa::Long id);

        /// Get a leakage solution.
        ///
        /// @param[in] id   id of the leakage solution to obtain.
        /// @return the leakage solution.
        /// @throw AskapError if the solution specified by the id does not
        ///         exist.
        LeakageSolution getLeakageSolution(const casa::Long id);

        /// Get a bandpass solution.
        ///
        /// @param[in] id   id of the bandpass solution to obtain.
        /// @return the bandpass solution.
        /// @throw AskapError if the solution specified by the id does not
        ///         exist.
        BandpassSolution getBandpassSolution(const casa::Long id);

    private:

        // Ice Communicator
        Ice::CommunicatorPtr itsComm;

        // Proxy object for remote service
        askap::interfaces::caldataservice::ICalibrationDataServicePrx itsService;

        // No support for assignment
        CalibrationDataServiceClient& operator=(const CalibrationDataServiceClient& rhs);

        // No support for copy constructor
        CalibrationDataServiceClient(const CalibrationDataServiceClient& src);
};

};
};
};

#endif

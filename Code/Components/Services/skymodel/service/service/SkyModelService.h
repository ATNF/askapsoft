/// @file SkyModelService.h
///
/// @copyright (c) 2016 CSIRO
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#ifndef ASKAP_CP_SMS_SKYMODELSERVICE_H
#define ASKAP_CP_SMS_SKYMODELSERVICE_H

// ASKAPsoft includes
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <Ice/Ice.h>
#include <iceutils/ServiceManager.h>
#include <Common/ParameterSet.h>

// Local package includes


namespace askap {
namespace cp {
namespace sms {

// forward declaration
class SkyModelServiceImpl;

/// @brief Main class for the Sky Model Service
class SkyModelService : private boost::noncopyable {
    public:
        /// @brief Construct a Sky Model Service Instance
        ///
        /// @param[in]  parset  the parameter set containing
        ///                     the configuration.
        SkyModelService(const LOFAR::ParameterSet& parset);

        /// @brief Destructor.
        ~SkyModelService();

        /// @brief Run the service
        void run(void);

    private:

        // Parameter set
        const LOFAR::ParameterSet& itsParset;
        Ice::CommunicatorPtr itsComm;
        boost::scoped_ptr<askap::cp::icewrapper::ServiceManager> itsServiceManager;
};

};
};
};

#endif

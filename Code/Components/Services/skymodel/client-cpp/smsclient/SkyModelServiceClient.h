/// @file SkyModelServiceClient.h
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#ifndef ASKAP_CP_SMS_SKYMODELSERVICECLIENT_H
#define ASKAP_CP_SMS_SKYMODELSERVICECLIENT_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include <boost/noncopyable.hpp>
#include "boost/shared_ptr.hpp"
#include "casacore/casa/Quanta/Quantum.h"
#include "Ice/Ice.h"

// Ice interfaces
#include <SkyModelService.h>
#include <SkyModelServiceDTO.h>

// Local package includes
#include "Component.h"

namespace askap {
namespace cp {
namespace sms {
namespace client {

typedef std::vector<askap::cp::sms::client::Component> ComponentList;
typedef boost::shared_ptr<ComponentList> ComponentListPtr;

class SkyModelServiceClient :
    private boost::noncopyable {

    public:
        friend class SkyModelServiceClientTest;

        /// Constructor
        /// The three parameters passed allow an instance of the sky model
        /// service to be located in an ICE registry.
        ///
        /// @param[in] locatorHost  host of the ICE locator service.
        /// @param[in] locatorPort  port of the ICE locator service.
        /// @param[in] serviceName  identity of the calibration data service
        ///                         in the ICE registry.
        SkyModelServiceClient(
                const std::string& locatorHost,
                const std::string& locatorPort,
                const std::string& serviceName = "SkyModelService");

        /// Destructor.
        ~SkyModelServiceClient();

        /// Cone search.
        ///
        /// @param ra   the right ascension of the centre of the
        ///             search area (Unit conformance: decimal degrees).
        /// @param dec  the declination of the centre of the search
        ///              area (Unit conformance: decimal degrees).
        /// @param searchRadius the search radius (Unit conformance:
        ///                      decimal degrees).
        /// @param fluxLimit    low limit on flux on sources returned all
        ///                     returned sources shall have flux >= fluxLimit
        ///                     (Unit conformance: Jy).
        /// @throw  AskapError  in the case one ore more of the Quantities does not
        ///                     conform to the appropriate unit.
        ComponentListPtr coneSearch(const casa::Quantity& ra,
                const casa::Quantity& dec,
                const casa::Quantity& searchRadius,
                const casa::Quantity& fluxLimit);

        /// @brief Rectangular search. Searches for components matching the criteria
        /// in the spatial region defined by a top-left (ra, dec) and
        /// bottom-right (ra, dec) point pair.
        ///
        /// @param[in] rect The rectangular region of interest (J2000 decimal degrees)
        /// @param[in] query the additional component query.
        /// @throw  AskapError  in the case one ore more of the Quantities does not
        ///                     conform to the appropriate unit.
        ///
        /// @return a sequence of components matching the query.
        ComponentListPtr rectSearch(
            askap::interfaces::skymodelservice::Rect roi,
            askap::interfaces::skymodelservice::SearchCriteria criteria);

    private:
        /// Default ctor used only for unit tests.
        /// Do not call any search methods after using this constructor.
        SkyModelServiceClient() {}

        /// Transforms a sequence of ICE ContinuumComponent objects into
        /// a sequence of askap::cp::sms::client::Component objects.
        ///
        /// @param ice_resultset    The ICE Component sequence.
        ComponentListPtr transformData(const askap::interfaces::skymodelservice::ComponentSeq& ice_resultset) const;

        // Ice Communicator
        Ice::CommunicatorPtr itsComm;

        // Proxy object for remote service
        askap::interfaces::skymodelservice::ISkyModelServicePrx itsService;
};

};
};
};
};

#endif

/// @file SkyModelServiceImpl.h
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

#ifndef ASKAP_CP_SMS_SKYMODELSERVICEIMPL_H
#define ASKAP_CP_SMS_SKYMODELSERVICEIMPL_H

// System includes
#include <string>

// ASKAPsoft includes
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <Common/ParameterSet.h>
#include <Ice/Ice.h>

// Ice interfaces
#include <SkyModelService.h>
#include <SkyModelServiceDTO.h>
#include <SkyModelServiceCriteria.h>

// Local package includes
#include "GlobalSkyModel.h"

namespace askap {
namespace cp {
namespace sms {

namespace sms_interface = askap::interfaces::skymodelservice;

/// @brief Implementation of the "ISkyModelService" Ice interface.
class SkyModelServiceImpl :
    public sms_interface::ISkyModelService,
    private boost::noncopyable {
    public:

        /// @brief Factory method for constructing the SkyModelService
        /// implementation.
        ///
        /// @return The SkyModelServiceImpl instance.
        /// @throw AskapError   If the implementation cannot be constructed.
        static SkyModelServiceImpl* create(const LOFAR::ParameterSet& parset);

        /// @brief Destructor.
        virtual ~SkyModelServiceImpl();

        virtual std::string getServiceVersion(const Ice::Current&);

        virtual sms_interface::ComponentSeq coneSearch(
            const sms_interface::Coordinate& centre,
            double radius,
            const sms_interface::SearchCriteria& criteria,
            const Ice::Current&);

        virtual sms_interface::ComponentSeq rectSearch(
            const sms_interface::Rect& roi,
            const sms_interface::SearchCriteria& criteria,
            const Ice::Current&);

    private:
        /// @brief Constructor.
        /// Private. Use the factory method to create.
        /// @param gsm The GlobalSkyModel instance.
        SkyModelServiceImpl(boost::shared_ptr<GlobalSkyModel> gsm);

        /// @brief The GlobalSkyModel instance
        boost::shared_ptr<GlobalSkyModel> itsGsm;
};

}
}
}

#endif

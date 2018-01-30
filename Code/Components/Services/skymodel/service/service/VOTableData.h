/// @file VOTableData.h
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

#ifndef ASKAP_CP_SMS_VOTABLEDATA_H
#define ASKAP_CP_SMS_VOTABLEDATA_H

// System includes
#include <string>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>

// ASKAPsoft includes
#include <votable/VOTable.h>

// Local package includes
#include "datamodel/ContinuumComponent.h"
#include "datamodel/Polarisation.h"
#include "HealPixFacade.h"
#include "SmsTypes.h"

namespace askap {
namespace cp {
namespace sms {

/// @brief VO table data container, with data stored in structure-of-arrays form
/// suitable for threaded computations.
class VOTableData :
    private boost::noncopyable {
    public:
        typedef std::vector<datamodel::ContinuumComponent> ComponentList;

        /// @brief Factory method for constructing the VOTableData implementation.
        ///
        /// @param components_file File name of the VO Table catalogue containing the components data
        /// @param polarisation_file File name of the VO Table catalogue containing the polarisation data for the components
        /// @param healpix_order HealPix Order to use for indexation (NSIDE = 2^order)
        /// @return The VOTableData instance.
        /// @throw AskapError   If the implementation cannot be constructed.
        static boost::shared_ptr<VOTableData> create(
            std::string components_file,
            std::string polarisation_file,
            boost::int64_t healpix_order);

        /// @brief Destructor.
        virtual ~VOTableData();

        /// @brief Get the number of components
        ///
        /// @return Component count
        inline long getCount() const {
            return itsComponents.size();
        }

        /// @brief Get the vector of ContinuumComponent objects.
        ///
        /// @return reference to the vector of ContinuumComponent objects.
        inline ComponentList& getComponents() {
            return itsComponents;
        }

        /// @brief Get the vector of ContinuumComponent objects.
        ///
        /// @return Const reference to the vector of ContinuumComponent objects.
        inline const ComponentList& getComponents() const {
            return itsComponents;
        }

    private:
        /// @brief Constructor.
        /// Private. Use the factory method to create.
        ///
        /// @param num_components The number of components for which space should be preallocated.
        VOTableData(unsigned long num_components);

        void calcHealpixIndicies(boost::int64_t healpix_order);

        ComponentList itsComponents;
};

}
}
}

#endif

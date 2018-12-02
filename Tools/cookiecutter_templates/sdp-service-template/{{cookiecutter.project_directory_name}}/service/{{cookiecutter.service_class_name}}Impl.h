/// @file {{cookiecutter.service_class_name}}Impl.h
///
/// @copyright (c) {{cookiecutter.year}} CSIRO
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
/// @author {{cookiecutter.user_name}} <{{cookiecutter.user_email}}>

#pragma once

// System includes
#include <string>

// ASKAPsoft includes
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <Common/ParameterSet.h>
#include <Ice/Ice.h>

// Ice interfaces
#include <{{cookiecutter.ice_interface_filename}}.h>

// Local package includes
//#include ".h"

namespace askap {
namespace cp {
namespace {{cookiecutter.namespace}} {

namespace {{cookiecutter.namespace}}_interface = askap::interfaces::{{cookiecutter.ice_interface_namespace}};

/// @brief Implementation of the "I{{cookiecutter.ice_service_name}}" Ice interface.
class {{cookiecutter.service_class_name}}Impl :
    public {{cookiecutter.namespace}}_interface::I{{cookiecutter.ice_service_name}},
    private boost::noncopyable {
    public:

        /// @brief Factory method for constructing the {{cookiecutter.service_class_name}}
        /// implementation.
        ///
        /// @return The {{cookiecutter.service_class_name}}Impl instance.
        /// @throw AskapError   If the implementation cannot be constructed.
        static {{cookiecutter.service_class_name}}Impl* create(const LOFAR::ParameterSet& parset);

        /// @brief Destructor.
        virtual ~{{cookiecutter.service_class_name}}Impl();

        virtual std::string getServiceVersion(const Ice::Current&);

    private:
        /// @brief Constructor.
        /// Private. Use the factory method to create.
        {{cookiecutter.service_class_name}}Impl();
};

}
}
}

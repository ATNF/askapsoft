/// @file {{cookiecutter.service_class_name}}.h
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

// ASKAPsoft includes
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <Ice/Ice.h>
#include <iceutils/ServiceManager.h>
#include <Common/ParameterSet.h>

// Local package includes


namespace askap {
namespace cp {
namespace {{cookiecutter.namespace}} {

// forward declaration
class {{cookiecutter.service_class_name}}Impl;

/// @brief Main class for the Sky Model Service
class {{cookiecutter.service_class_name}} : private boost::noncopyable {
    public:
        /// @brief Construct a Sky Model Service Instance
        ///
        /// @param[in]  parset  the parameter set containing
        ///                     the configuration.
        {{cookiecutter.service_class_name}}(const LOFAR::ParameterSet& parset);

        /// @brief Destructor.
        ~{{cookiecutter.service_class_name}}();

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

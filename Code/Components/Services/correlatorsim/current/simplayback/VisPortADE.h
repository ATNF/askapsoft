/// @file VisPortADE.h
///
/// @copyright (c) 2010 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>

#ifndef ASKAP_CP_VISPORTADE_H
#define ASKAP_CP_VISPORTADE_H

// System includes
#include <vector>

// ASKAPsoft includes
#include "boost/asio.hpp"
#include "cpcommon/VisDatagramADE.h"

// Local package includes

namespace askap {
namespace cp {

/// @brief This class acts as a port to the visibility reciever. This class
/// encapsulates a UDP port which is related to a specific host & port as is
/// specified in the constructor. VisDatagramADE objects can be "sent" 
/// using this port.
class VisPortADE {
    public:

        /// @brief Constructor.
        //
        /// @param[in] hostname hostname or IP address of the host to which the
        ///                     UDP data stream will be sent.
        /// @param[in] port     UDP port number to which the UDP data stream 
        ///                     will be sent.
        VisPortADE(const std::string& hostname, const std::string& port);
        
        /// @brief Destructor.
        ~VisPortADE();

        /// @brief Sends all payload objects in the vector to the host/port
        /// that was specified when the object was instantiated.
        ///
        /// @param[in] payload  vector of VisDatagramADE objects to send.
        void send(const std::vector<askap::cp::VisDatagramADE>& payload);

        /// @brief Sends the payload object to the host/port that was specified
        /// when the object was instantiated.
        ///
        /// @param[in] payload  VisDatagramADE object to send.
        void send(const askap::cp::VisDatagramADE& payload);

    private:
        // io_service
        boost::asio::io_service itsIOService;

        // Network socket
        boost::asio::ip::udp::socket itsSocket;
};
};

};

#endif

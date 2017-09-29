/// @file PublisherApp.h
///
/// @copyright (c) 2014 CSIRO
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

#ifndef ASKAP_CP_VISPUBLISHER_PUBLISHERAPP_H
#define ASKAP_CP_VISPUBLISHER_PUBLISHERAPP_H

// System includes
#include <stdint.h>

// ASKAPsoft includes
#include "askap/Application.h"

// Local package includes
#include "publisher/InputMessage.h"
#include "publisher/SpdOutputMessage.h"

#include "publisher/VisMessageBuilder.h"
#include "publisher/ZmqPublisher.h"
#include "publisher/ZmqVisControlPort.h"

// to be moved to base
#include "publisher/CircularBuffer.h"

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

namespace askap {
namespace cp {
namespace vispublisher {

/// @brief Implementation of the VisPublisher application.
class PublisherApp : public askap::Application {
    public:
        /// constructor
        PublisherApp();

        /// destructor
        ~PublisherApp();
 
        /// Run the application
        virtual int run(int argc, char* argv[]);

    private:
        /// actual loop of the program receiving and publishing messages
        void receiveAndPublishLoop(boost::asio::ip::tcp::socket &socket);

        /// Build an Spd message for a givn beam and polarisation
        static SpdOutputMessage buildSpdOutputMessage(const InputMessage& in,
                                                      uint32_t beam,
                                                      uint32_t pol);

        /// @brief parallel thread entry point 
        /// @details This is the code of the parallel thread
        /// @param[in] stream unique stream number (e.g. for logging and storing data to avoid synchronisation)
        void parallelThread(int stream);

        /// shared pointer to zmq publisher for vis messages
        boost::shared_ptr<ZmqPublisher> itsVisMsgPublisher;

        /// shared pointer to zmq publisher for spd messages
        boost::shared_ptr<ZmqPublisher> itsSpdMsgPublisher;

        /// shared pointer to zmq publisher for control port
        boost::shared_ptr<ZmqVisControlPort> itsVisCtrlPort;

        /// thread group to manage connections from ingest
        mutable boost::thread_group itsThreadGroup;

        /// stop flag
        mutable bool itsStopRequested;
        
        // circular buffer handling jobs for individual threads
        ingest::CircularBuffer<boost::asio::ip::tcp::socket> itsBuffer;

        /// @details synchronisation to ensure vis and spd messages
        /// are published in one chunk
        mutable boost::mutex itsMutex;
};

}
}
}

#endif

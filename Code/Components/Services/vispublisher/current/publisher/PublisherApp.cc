/// @file PublisherApp.cc
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

// Include own header file first
#include "publisher/PublisherApp.h"

// Include package level header file
#include "askap_vispublisher.h"

// System includes
#include <vector>
#include <set>
#include <complex>
#include <stdint.h>

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "Common/ParameterSet.h"
#include "askap/StatReporter.h"
#include <zmq.hpp>
#include <boost/asio.hpp>
#include <casacore/casa/OS/Timer.h>

// Local package includes
#include "publisher/SpdOutputMessage.h"
#include "publisher/InputMessage.h"
#include "publisher/SubsetExtractor.h"
#include "publisher/VisMessageBuilder.h"
#include "publisher/ZmqPublisher.h"
#include "publisher/ZmqVisControlPort.h"

// Using
using namespace std;
using namespace askap;
using namespace askap::cp::vispublisher;
using boost::asio::ip::tcp;

ASKAP_LOGGER(logger, ".PublisherApp");

/// actual loop of the program receiving and publishing messages
void PublisherApp::receiveAndPublishLoop(boost::asio::ip::tcp::socket &socket)
{
    ASKAPASSERT(itsVisMsgPublisher);
    ASKAPASSERT(itsSpdMsgPublisher);
    ASKAPASSERT(itsVisCtrlPort);
    casacore::Timer timer;
    const uint32_t N_POLS = 4;

    while (socket.is_open()) {
        try {
            InputMessage inMsg = InputMessage::build(socket);
            timer.mark();
            ASKAPLOG_DEBUG_STR(logger, "Received a message - Timestamp: "
                    << inMsg.timestamp() << " Scan: " << inMsg.scan());
   
            boost::mutex::scoped_lock lock(itsMutex);

            ///////////////////
            // Publish SPD data
            ///////////////////
            const vector<uint32_t> beamvector(inMsg.beam());
            const set<uint32_t> beamset(beamvector.begin(), beamvector.end());
            for (set<uint32_t>::const_iterator beamit = beamset.begin();
                    beamit != beamset.end(); ++beamit) {
                for (uint32_t pol = 0; pol < N_POLS; ++pol) {
                    SpdOutputMessage outmsg = SubsetExtractor::subset(inMsg, *beamit, pol);
                    //ASKAPLOG_DEBUG_STR(logger, "Publishing Spd message for beam " << *beamit << " pol " << pol);
                    itsSpdMsgPublisher->publish(outmsg);
                }
            }

            ///////////////////
            // Publish VIS data
            ///////////////////

            /*
            // commissioning hack - only publish vis message for the first beam if it is a single beam case
            if (beamset.size() == 1) {
                if (*beamset.begin() != 0) {
                    continue;
                }
            }
            //
            */

            // Get and check the tvchan setting
            uint32_t tvChanBegin = 0;
            uint32_t tvChanEnd = inMsg.nChannels() - 1;
            if (itsVisCtrlPort->isTVChanSet()) {
                const pair<uint32_t, uint32_t> tvchan = itsVisCtrlPort->tvChan();
                tvChanBegin = tvchan.first;
                tvChanEnd = tvchan.second;
            }

            if (tvChanEnd < tvChanBegin
                    || (tvChanEnd - tvChanBegin + 1) > inMsg.nChannels()) {
                ASKAPLOG_WARN_STR(logger, "Invalid TV Chan range: "
                        << tvChanBegin << "-" << tvChanEnd);
                continue;
            }

            // Create and send the output message
            VisOutputMessage outmsg = VisMessageBuilder::build(inMsg,
                    tvChanBegin, tvChanEnd);
            ASKAPLOG_DEBUG_STR(logger, "Publishing Vis message - tvchan: "
                    << tvChanBegin << " - " << tvChanEnd);
            itsVisMsgPublisher->publish(outmsg);
            ASKAPLOG_DEBUG_STR(logger, "Time to handle " << timer.real() << "s");

        } catch (AskapError& e) {
            ASKAPLOG_DEBUG_STR(logger, "Error reading input message: " << e.what()
                    << ", closing input socket");
            socket.close();
        }
    }
}

/// constructor
PublisherApp::PublisherApp() : itsStopRequested(false), itsBuffer(36) //subset.getUint16("nthreads",1))
{
}

/// destructor
PublisherApp::~PublisherApp() 
{
   itsStopRequested = true; 
   itsThreadGroup.join_all();
}


/// @brief parallel thread entry point 
/// @details This is the code of the parallel thread
/// @param[in] stream unique stream number (e.g. for logging and storing data to avoid synchronisation)
void PublisherApp::parallelThread(int stream)
{
   ASKAPLOG_DEBUG_STR(logger, "Started thread to handle stream = "<<stream);
   const long ONE_SECOND = 1000000;
   while (!itsStopRequested) {
      boost::shared_ptr<tcp::socket> nextConnection = itsBuffer.next(ONE_SECOND);
      if (nextConnection) {
          ASKAPLOG_DEBUG_STR(logger, "Assigning incoming connection from: "<<nextConnection->remote_endpoint().address()<<
                                     " to stream: "<<stream);
          receiveAndPublishLoop(*nextConnection);
      }
   }
}


int PublisherApp::run(int argc, char* argv[])
{
    StatReporter stats;
    const LOFAR::ParameterSet subset = config().makeSubset("vispublisher.");
    const uint16_t inPort = subset.getUint16("in.port");
    const uint16_t spdPort = subset.getUint16("spd.port");
    const uint16_t visPort = subset.getUint16("vis.port");
    const uint16_t visControlPort = subset.getUint16("viscontrol.port");
    const uint16_t nThreads = itsBuffer.capacity();

    ASKAPLOG_INFO_STR(logger, "ASKAP Vis Publisher " << ASKAP_PACKAGE_VERSION);
    ASKAPLOG_INFO_STR(logger, "Input Port: " << inPort);
    ASKAPLOG_INFO_STR(logger, "Spd Output Port: " << spdPort);
    ASKAPLOG_INFO_STR(logger, "Vis Output Port: " << visPort);
    ASKAPLOG_INFO_STR(logger, "Vis Control Port: " << visControlPort);
    ASKAPLOG_INFO_STR(logger, "Will setup "<<nThreads<<" threads to receive messages from ingest");

    // Setup the ZeroMQ publisher and control objects
    itsSpdMsgPublisher.reset(new ZmqPublisher(spdPort));
    itsVisMsgPublisher.reset(new ZmqPublisher(visPort));
    itsVisCtrlPort.reset(new ZmqVisControlPort(visControlPort));

    // Setup the TCP socket to receive data from the ingest pipeline
    boost::asio::io_service io_service;
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), inPort));
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

    // create the fixed number of parallel threads to handle connections
    for (size_t thread = 0; thread < itsBuffer.capacity(); ++thread) {
         itsThreadGroup.create_thread(boost::bind(&PublisherApp::parallelThread, this, thread));
    }

    //tcp::socket socket(io_service);
    casacore::Timer timer;
    while (true) {
        boost::shared_ptr<tcp::socket> socket(new tcp::socket(io_service));
        ASKAPASSERT(socket);
        acceptor.accept(*socket);
        ASKAPLOG_DEBUG_STR(logger, "Accepted incoming connection from: "
                << socket->remote_endpoint().address());
        itsBuffer.add(socket);
    }
    ASKAPLOG_INFO_STR(logger, "Stopping ASKAP Vis Publisher");
    itsStopRequested = true;
    itsThreadGroup.join_all();

    stats.logSummary();
    return 0;
}

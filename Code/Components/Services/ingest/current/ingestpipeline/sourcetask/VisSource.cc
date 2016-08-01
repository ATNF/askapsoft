/// @file VisSource.cc
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

// Include own header file first
#include "VisSource.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"

#include <iomanip>

// Using
using namespace askap;
using namespace askap::cp;
using namespace askap::cp::ingest;
using boost::asio::ip::udp;

ASKAP_LOGGER(logger, ".VisSource");

/// @brief access to beam rejection criterion
/// @details This method encapsulates access to parset parameter defining 
/// beam rejection at the receiver side (i.e. before the datagram is even 
/// put in the buffer).
/// @param[in] parset parameter set to work with
/// @return maximum beam Id to be kept
uint32_t VisSource::getMaxBeamId(const LOFAR::ParameterSet &parset)
{
   // BETA value is currently the default
   return parset.getUint32("vis_source.max_beamid", 9); 
}

/// @brief access to slice rejection criterion
/// @details This method encapsulates access to parset parameter defining 
/// slice rejection at the receiver side (i.e. before the datagram is even 
/// put in the buffer).
/// @param[in] parset parameter set to work with
/// @return maximum beam Id to be kept
uint32_t VisSource::getMaxSlice(const LOFAR::ParameterSet &parset)
{
   // this will not drop datagrams for either BETA or ADE
   return parset.getUint32("vis_source.max_slice", 15); 
}
       

/// @brief constructor
/// @param[in] parset parameters (such as port, buffer_size, etc)
/// @param[in] portOffset this number is added to the port number
///            given in the parset (to allow parallel processes
///            to listen different ports)
VisSource::VisSource(const LOFAR::ParameterSet &parset, const unsigned int portOffset) :
    itsBuffer(parset.getUint32("buffer_size", 78 * 36 * 16 * 2)),  // default is tuned for BETA
    itsStopRequested(false), 
    itsMaxBeamId(getMaxBeamId(parset)), itsMaxSlice(getMaxSlice(parset)), 
    itsOldTimestamp(0ul)
#ifdef ASKAP_DEBUG
    ,
    itsCard(portOffset + 1)
#endif
{
    const uint32_t recvBufferSize = parset.getUint32("vis_source.receive_buffer_size", 
                                                     1024 * 1024 * 16); // BETA value is the default
    const unsigned int port = parset.getUint32("vis_source.port") + portOffset;

    ASKAPLOG_INFO_STR(logger, "Setting up VisSource to listen up port "<<port);
    ASKAPLOG_INFO_STR(logger, "     - receive  buffer size: "<<recvBufferSize / 1024 / 1024 <<" Mb");
    ASKAPLOG_INFO_STR(logger, "     - circular buffer size: "<<itsBuffer.capacity()<<" datagrams");    
    ASKAPLOG_INFO_STR(logger, "     - beams with Id > "<<itsMaxBeamId<<" will be ignored"); 
    ASKAPLOG_INFO_STR(logger, "     - slices > "<<itsMaxSlice<<" will be ignored"); 

    bat2epoch(4943907678000000ul);

    // Create socket
    itsSocket.reset(new udp::socket(itsIOService, udp::endpoint(udp::v4(), port)));


    // set up receive buffer to help deal with the bursty nature of the communication
    boost::asio::socket_base::receive_buffer_size option(recvBufferSize);

    boost::system::error_code soerror;
    itsSocket->set_option(option, soerror);
    if (soerror) {
        ASKAPLOG_WARN_STR(logger, "Setting UDP receive buffer size failed. " <<
                "This may result in dropped datagrams");
    }
    //

    start_receive();

    // Start the thread
    itsThread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&VisSource::run, this)));
}

VisSource::~VisSource()
{
    // Signal stopped so now more calls to start_receive() will be made
    itsStopRequested = true;

    // Stop the io_service (non-blocking) and cancel an outstanding requests
    itsIOService.stop();
    itsSocket->cancel();

    // Wait for the thread running the io_service to finish
    if (itsThread.get()) {
        itsThread->join();
    }

    // Finally close the socket
    itsSocket->close();
}

/// @brief query buffer status
/// @details Typical implementation involves buffering of data. 
/// Exceeding the buffer capacity will cause data loss. This method
/// is intended for monitoring the usage of the buffer.
/// @return a pair with number of datagrams in the queue and the buffer size
std::pair<uint32_t, uint32_t> VisSource::bufferUsage() const
{
   return std::pair<uint32_t, uint32_t>(itsBuffer.size(),itsBuffer.capacity());
}

void VisSource::start_receive(void)
{
    itsRecvBuffer.reset(new VisDatagram);
    itsSocket->async_receive_from(
            boost::asio::buffer(boost::asio::buffer(itsRecvBuffer.get(), sizeof(VisDatagram))),
            itsRemoteEndpoint,
            boost::bind(&VisSource::handle_receive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}

void VisSource::handle_receive(const boost::system::error_code& error,
        std::size_t bytes)
{
    if (!error || error == boost::asio::error::message_size) {
        if (bytes != sizeof(VisDatagram)) {
            ASKAPLOG_WARN_STR(logger, "Error: Failed to read a full VisDatagram struct");
        }
        if (itsRecvBuffer->version != VisDatagramTraits<VisDatagram>::VISPAYLOAD_VERSION) {
            ASKAPLOG_ERROR_STR(logger, "Version mismatch. Expected "
                    << VisDatagramTraits<VisDatagram>::VISPAYLOAD_VERSION
                    << " got " << itsRecvBuffer->version);
        }

        // message for debugging
        if (itsOldTimestamp != itsRecvBuffer->timestamp) {
            itsOldTimestamp = itsRecvBuffer->timestamp;
/*
// for debugging, temporary commented out due to performance issues in the logger
#ifdef ASKAP_DEBUG
            ASKAPLOG_DEBUG_STR(logger, "VisSource("<<itsCard<<"): queuing new timestamp :"<<bat2epoch(itsOldTimestamp)<<" BAT=0x"<<std::hex<<itsOldTimestamp);
#else
            ASKAPLOG_DEBUG_STR(logger, "VisSource: queuing new timestamp :"<<bat2epoch(itsOldTimestamp)<<" BAT=0x"<<std::hex<<itsOldTimestamp);
#endif
*/
        }
        //

        if (itsRecvBuffer->beamid <= itsMaxBeamId) {
            if (itsRecvBuffer->slice <= itsMaxSlice) {
                // Add a pointer to the message to the back of the circular buffer.
                // Waiters are notified.
                itsBuffer.add(itsRecvBuffer);
            }
        }
        itsRecvBuffer.reset();
    } else {
        ASKAPLOG_WARN_STR(logger, "Error reading visibilities from UDP socket. Error Code: "
                << error);
    }

    if (!itsStopRequested) {
        start_receive();
    }
}

void VisSource::run(void)
{
    itsIOService.run();    
}

boost::shared_ptr<VisDatagram> VisSource::next(const long timeout)
{
    return itsBuffer.next(timeout);
}

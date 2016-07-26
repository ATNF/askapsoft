/// @file CubeComms.cc
///
/// Class to provide extra MPI communicator functionality to manage the writing
/// of distributed spectral cubes.
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
/// @author Stephen Ord <Stephen.Ord@csiro.au>
///

#include <limits>
/// out own header first


#include "distributedimager/CubeComms.h"
#include "messages/IMessage.h"

///ASKAPsoft includes
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include "Blob/BlobIStream.h"
#include "Blob/BlobIBufVector.h"
#include "Blob/BlobOStream.h"
#include "Blob/BlobOBufVector.h"


#include "casacore/casa/OS/Timer.h"

// MPI includes
#ifdef HAVE_MPI
#include <mpi.h>
#endif

ASKAP_LOGGER(logger, ".CubeComms");

using namespace askap;
using namespace askap::cp;

CubeComms::CubeComms(int argc, const char** argv) : AskapParallel(argc, const_cast<const char **>(argv))
    {

    ASKAPLOG_DEBUG_STR(logger,"Constructor");
    this->writers.resize(0);
    this->channelsToWrite.resize(0);

}
CubeComms::~CubeComms() {
    ASKAPLOG_DEBUG_STR(logger,"Destructor");
}
bool CubeComms::isWriter() {
    ASKAPLOG_DEBUG_STR(logger,"Providing writer status");
    /// see if my rank is in the writers list
    for (int wr = 0; wr < this->writers.size(); wr++) {
        if (itsRank == this->writers[wr]) {
            return true;
        }
    }
    return false;
}
void CubeComms::addWriter(unsigned int writerRank) {
    this->writers.push_back(writerRank);
    unsigned int ch=0;
    this->channelsToWrite.push_back(ch);
}
void CubeComms::addChannelToWriter(unsigned int writerindex) {
    this->channelsToWrite[writerindex]++;
}

void CubeComms::sendMessage(const IMessage& msg, int dest)
{
    // Encode

    std::vector<int8_t> buf;
    LOFAR::BlobOBufVector<int8_t> bv(buf);
    LOFAR::BlobOStream out(bv);
    out.putStart("Message", 1);
    out << msg;
    out.putEnd();


    int messageType = msg.getMessageType();

    casa::Timer timer;
    timer.mark();
    /// askapparallel implements this
    send(&buf[0], buf.size(), dest, messageType);

    ASKAPLOG_DEBUG_STR(logger, "Sent Message of type " << messageType
                       << " to rank " << dest << " via MPI in " << timer.real()
                       << " seconds ");
}
#ifdef HAVE_MPI

void CubeComms::receiveMessage(IMessage& msg, int source)
{
    // This has to be overloaded because we need to
    // know how big the message is to size the blob
    // this is not exposed by the askapparallel method.

    const unsigned int c_maxint = std::numeric_limits<int>::max();
    int tag = msg.getMessageType();
    // First receive the size of the payload to be received,
    // remembering the size parameter passed to this function is
    // just the maximum size of the buffer, and hence the maximum
    // number of bytes that can be received.
    unsigned long payloadSize;
    MPI_Status status;
    int result = MPI_Recv(&payloadSize, 1, MPI_UNSIGNED_LONG,
                          source, tag, MPI_COMM_WORLD, &status);


    // The source parameter may be MPI_ANY_SOURCE, so the actual
    // source needs to be recorded for later use.
    const int actualSource = status.MPI_SOURCE;
    if (source != MPI_ANY_SOURCE) {
        ASKAPCHECK(actualSource == source,
                "Actual source of message differs from requested source");
    }
    // Receive the byte stream
    std::vector<int8_t> buf;
    buf.resize(payloadSize);
    unsigned long size = buf.size();

    // Receive the smaller of size or payloadSize
    size_t remaining = (payloadSize > size) ? size : payloadSize;
    void *base_addr = &buf[0];
    while (remaining > 0) {
        size_t offset = size - remaining;
        void* addr = addByteOffset(base_addr, offset);

        if (remaining >= c_maxint) {
            result = MPI_Recv(addr, c_maxint, MPI_BYTE, \
                              actualSource, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            remaining -= c_maxint;
        }

        else {
            result = MPI_Recv(addr, remaining, MPI_BYTE, \
                         actualSource, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            remaining = 0;
        }
    }

    // Decode

    LOFAR::BlobIBufVector<int8_t> bv(buf);
    LOFAR::BlobIStream in(bv);
    int version = in.getStart("Message");
    ASKAPASSERT(version == 1);
    in >> msg;
    in.getEnd();

}
void CubeComms::receiveMessageAnySrc(IMessage& msg, int& actualSource)
{
    // This has to be overloaded because we need to
    // know how big the message is to size the blob
    // this is not exposed by the askapparallel method.

    const unsigned int c_maxint = std::numeric_limits<int>::max();
    int tag = msg.getMessageType();
    // First receive the size of the payload to be received,
    // remembering the size parameter passed to this function is
    // just the maximum size of the buffer, and hence the maximum
    // number of bytes that can be received.
    unsigned long payloadSize;
    MPI_Status status;
    int result = MPI_Recv(&payloadSize, 1, MPI_UNSIGNED_LONG,
                          MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);


    // The source parameter may be MPI_ANY_SOURCE, so the actual
    // source needs to be recorded for later use.
    actualSource = status.MPI_SOURCE;

    // Receive the byte stream
    std::vector<int8_t> buf;
    buf.resize(payloadSize);
    unsigned long size = buf.size();
    void *base_addr = &buf[0];
    // Receive the smaller of size or payloadSize
    size_t remaining = (payloadSize > size) ? size : payloadSize;

    while (remaining > 0) {
        size_t offset = size - remaining;
        void* addr = addByteOffset(base_addr, offset);

        if (remaining >= c_maxint) {
            result = MPI_Recv(addr, c_maxint, MPI_BYTE, \
                              actualSource, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            remaining -= c_maxint;
        } else {
            result = MPI_Recv(addr, remaining, MPI_BYTE, \
                         actualSource, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            remaining = 0;
        }

    }

    // Decode

    LOFAR::BlobIBufVector<int8_t> bv(buf);
    LOFAR::BlobIStream in(bv);
    int version = in.getStart("Message");
    ASKAPASSERT(version == 1);
    in >> msg;
    in.getEnd();


}

void CubeComms::receiveMessageAnySrc(IMessage& msg)
{

    int id;
    receiveMessageAnySrc(msg, id);
    return;
}

#else

/// need some stubs here
void CubeComms::receiveMessage(IMessage& msg, int source)
{
}
void CubeComms::receiveMessageAnySrc(IMessage& msg, int& actualSource)
{
}
void CubeComms::receiveMessageAnySrc(IMessage& msg)
{

}

#endif

/// Dont like putting this here - would rather make the MPIComms version public
void* CubeComms::addByteOffset(const void *ptr, size_t offset) const
{
    char *cptr = static_cast<char*>(const_cast<void*>(ptr));
    cptr += offset;

    return cptr;
}

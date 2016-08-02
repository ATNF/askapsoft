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
#include <map>
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



ASKAP_LOGGER(logger, ".CubeComms");

using namespace askap;
using namespace askap::cp;



CubeComms::CubeComms(int argc, const char** argv) : AskapParallel(argc, const_cast<const char **>(argv))
{

    ASKAPLOG_DEBUG_STR(logger,"Constructor");

}
CubeComms::~CubeComms() {
    ASKAPLOG_DEBUG_STR(logger,"Destructor");
}

size_t CubeComms::buildCommIndex() {

    const int nWorkers = nProcs() - 1;
    std::vector<int> ranks(nWorkers, -1);

    for (size_t wrk = 0; wrk < nWorkers; ++wrk) {
          ranks[wrk] = 1 + wrk;
     }
     itsComrades = createComm(ranks);
     ASKAPLOG_INFO_STR(logger, "Interworker communicator index is "<< itsComrades);
     return itsComrades;
}
void CubeComms::initWriters(int nwriters, int nchanpercore) {

    unsigned int nWorkers = nProcs() - 1;
    unsigned int nWorkersPerGroup = nWorkers/nGroups();
    unsigned int nWorkersPerWriter = floor(nWorkers / nwriters);

    if (nWorkersPerWriter < 1) {
        nWorkersPerWriter = 1;
    }
    int channelCount = nWorkersPerWriter * nchanpercore;

    for (int wrk = 0; wrk < nWorkersPerGroup; wrk=wrk+nWorkersPerWriter) {
        int mywriter = floor(wrk/nWorkersPerWriter)*nWorkersPerWriter + 1;
        std::pair<std::map<int,int>::iterator,bool> ret;

        ret = writerMap.insert(std::pair<int,int> (mywriter,channelCount) );

        if (ret.second==false) {
            ASKAPLOG_WARN_STR(logger, "element '" << mywriter << "' already existed");
        }

    }

}
bool CubeComms::isWriter() {
    ASKAPLOG_DEBUG_STR(logger,"Providing writer status");
    /// see if my rank is in the writers list
    std::map<int,int>::iterator it = writerMap.begin();
    for (it=writerMap.begin(); it!=writerMap.end(); ++it) {
        if (itsRank == it->first) {
            return true;
        }
    }
    return false;
}
void CubeComms::addWriter(unsigned int writerRank) {
    int count = 0;
    std::pair<std::map<int,int>::iterator,bool> ret;
    ret = writerMap.insert(std::pair<int,int> (writerRank,count) );

    if (ret.second==false) {
        ASKAPLOG_WARN_STR(logger, "element '" << writerRank << "' already existed");
    }

}
void CubeComms::addChannelToWriter(unsigned int writerRank) {

    std::map<int,int>::iterator it;
    it = writerMap.find(writerRank);
    if (it != writerMap.end()) {
        int oldcount = it->second;
        int newcount = oldcount+1;
        writerMap.erase (it);
        std::pair<std::map<int,int>::iterator,bool> ret;
        ret = writerMap.insert(std::pair<int,int> (writerRank,newcount) );
    }
    else {
        ASKAPLOG_WARN_STR(logger,"Adding channel to non-existent writer");

    }

}
void CubeComms::removeChannelFromWriter(unsigned int writerRank) {

    std::map<int,int>::iterator it;
    it = writerMap.find(writerRank);
    if (it != writerMap.end()) {
        int oldcount = it->second;
        int newcount = oldcount-1;
        writerMap.erase (it);
        std::pair<std::map<int,int>::iterator,bool> ret;
        ret = writerMap.insert(std::pair<int,int> (writerRank,newcount) );
    }
    else {
        ASKAPLOG_WARN_STR(logger,"Removing channel from non-existent writer");

    }

}


/// Dont like putting this here - would rather make the MPIComms version public
void* CubeComms::addByteOffset(const void *ptr, size_t offset) const
{
    char *cptr = static_cast<char*>(const_cast<void*>(ptr));
    cptr += offset;

    return cptr;
}
int CubeComms::getOutstanding() {
    return writerMap[rank()];
}

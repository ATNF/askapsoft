/// @file CubeComms.h
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

#ifndef ASKAP_CP_IMAGER_CUBECOMMS_H
#define ASKAP_CP_IMAGER_CUBECOMMS_H

#include <map>
///ASKAP includes ...
#include <askapparallel/AskapParallel.h>
#include "messages/IMessage.h"

namespace askap {
namespace cp {

    class CubeComms: public askapparallel::AskapParallel
    {
        public:
            /// @brief Constructor
            /// @details The command line inputs are needed solely for MPI - currently no
            /// application specific information is passed on the command line.
            /// @param argc Number of command line inputs
            /// @param argv Command line inputs
            CubeComms(int argc, const char** argv);

            /// @brief isWriter
            /// @details This bool can be tested to find out whether the current
            /// rank is a writer
            /// returns writerCount 1->nwriters
            int isWriter();
            /// @brief adds a writer to the list by rank
            /// @details This adds a rank to a vector of ranks
            /// each is the rank of a writer
            void addWriter(unsigned int writer_rank);

            /// @brief initialises the writer list
            /// @details By evenly dividing the writing across
            /// the ranks

            void initWriters(int nwriters, int nchanpercore);
            /// @brief increments a counter (one for each rank)
            /// @details Takes the rank of the writer
            void addChannelToWriter(unsigned int writer_rank);

            void removeChannelFromWriter(unsigned int writer_rank);

            int getOutstanding();

            int anyWork();
            

            size_t buildCommIndex();
            /// @brief its communicator for its fellow workers
            size_t theWorkers() {return itsComrades;};


            ~CubeComms();

        private:
            // Add a byte offset to the  specified pointer, returning the result
            void* addByteOffset(const void *ptr, size_t offset) const;
            std::map<int,int> writerMap;
        int writerCount;
            size_t itsComrades;
    };
}
}
#endif

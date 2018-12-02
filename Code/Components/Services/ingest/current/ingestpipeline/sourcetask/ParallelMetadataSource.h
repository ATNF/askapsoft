/// @file ParallelMetadataSource.h
/// @brief An adapter to use MetadataSource in parallel environment
/// @details When ingest pipeline is running under TOS we want to align all streams to the
/// same metadata. This adapter wraps around a MetadataSource object instantiated on one
/// of the ranks and distributes the metadata to all other ranks by broadcast.
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#ifndef ASKAP_CP_INGEST_PARALLELMETADATASOURCE_H
#define ASKAP_CP_INGEST_PARALLELMETADATASOURCE_H

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "cpcommon/TosMetadata.h"

// Local package includes
#include "ingestpipeline/sourcetask/IMetadataSource.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief An adapter to use MetadataSource in parallel environment
/// @details When ingest pipeline is running under TOS we want to align all streams to the
/// same metadata. This adapter wraps around a MetadataSource object instantiated on one
/// of the ranks and distributes the metadata to all other ranks by broadcast.
class ParallelMetadataSource : virtual public IMetadataSource,
                               public boost::noncopyable {
public:

    /// @brief constructor
    /// @details The adapter is constructed in slave or master mode depending
    /// on the passed shared pointer with the actual metadata source object.
    /// If non-zero pointer is passed, this rank is considered to be master rank.
    /// Otherwise, it is a slave rank (i.e. null shared pointer) which will receive
    /// a copy of the metadata. This class uses MPI collective calls, therefore all
    /// ranks should call the constructor and 'next' method.
    /// @param[in] msrc metadata source doing the actual work
    ParallelMetadataSource(const boost::shared_ptr<IMetadataSource> &msrc);

    /// @brief Returns the next TosMetadata object.
    /// This call can be blocking, it will not return until an object is
    /// available to return.
    ///
    /// @param[in] timeout how long to wait for data before returning
    ///         a null pointer, in the case where the
    ///         buffer is empty. The timeout is in microseconds,
    ///         and anything less than zero will result in no
    ///         timeout (i.e. blocking functionality).
    ///
    /// @return a shared pointer to a TosMetadata object.
    virtual boost::shared_ptr<askap::cp::TosMetadata> next(const long timeout = -1);

private:

    /// @brief metadata source doing the actual work
    /// @details Uninitialised pointer in the slave mode
    boost::shared_ptr<IMetadataSource> itsMetadataSource;

    /// @brief master rank
    int itsMasterRank;
};

}
}
}

#endif // #ifndef ASKAP_CP_INGEST_PARALLELMETADATASOURCE_H


/// @file ITask.h
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

#ifndef ASKAP_CP_INGEST_ITASK_H
#define ASKAP_CP_INGEST_ITASK_H

// Std Includes
#include <string>

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "cpcommon/VisChunk.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Interface to which all pipeline tasks must conform to.
class ITask {
    public:
        /// Destructor.
        virtual ~ITask();

        /// @brief Process a VisChunk.
        /// @details
        ///
        /// This method is called once for each correlator integration.
        /// 
        /// @param[in,out] chunk    a shared pointer to a VisChunk object. The
        ///             VisChunk contains all the visibilities and associated
        ///             metadata for a single correlator integration. This method
        ///             is expected to take this VisChunk as input, perform any
        ///             transformations on it and return it as output. This
        ///             parameter is a pointer, so the method is free to change
        ///             the pointer to point to a new object. One of the special 
        ///             cases in the parallel mode is when a particular rank
        ///             ends or starts processing at some particular task 
        ///             (e.g. merging parallel streams together and continuing
        ///             reduction with a smaller number of parallel streams or
        ///             vice versa expanding the parallelism). The convention is
        ///             that this method should reset the shared pointer to
        ///             stop processing for the current rank. If the current
        ///             rank is inactive, this method will not be called unless
        ///             isAlwaysActive method returns true. In the latter case,
        ///             this method is called with an empty pointer.
        ///             
        virtual void process(askap::cp::common::VisChunk::ShPtr& chunk) = 0;

        /// @brief should this task be executed for inactive ranks?
        /// @details If a particular rank is inactive, process method is
        /// not called unless this method returns true. Possible use cases:
        ///   - Splitting the datastream expanding parallelism, i.e
        ///     inactive rank(s) become active after this task.
        ///   - Need for collective operations 
        /// @return true, if process method should be called even if
        /// this rank is inactive (i.e. uninitialised chunk pointer
        /// will be passed to process method).
        /// @note default action is to return false, i.e. process method
        /// is not called for inactive tasks.
        virtual bool isAlwaysActive() const;
        


        /// Gets the name/alias of this task.
        ///
        /// @return the name/alias of this task.
        virtual std::string getName(void) const;

        /// Sets a name/alias for this task.
        /// This is used for logging purposes.
        ///
        /// @param[in] the name/alias for this task.
        virtual void setName(const std::string& name);

        /// Shared pointer definition
        typedef boost::shared_ptr<ITask> ShPtr;

    private:

        /// The name/alias of this task
        std::string itsName;
};

}
}
}

#endif

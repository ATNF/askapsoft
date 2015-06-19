/// @file MergedSource.h
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

#ifndef ASKAP_CP_INGEST_MERGEDSOURCE_H
#define ASKAP_CP_INGEST_MERGEDSOURCE_H

// System includes
#include <set>
#include <string>
#include <stdint.h>

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "boost/system/error_code.hpp"
#include "boost/asio.hpp"
#include "boost/noncopyable.hpp"
#include "Common/ParameterSet.h"
#include "cpcommon/TosMetadata.h"
#include "cpcommon/VisDatagram.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "ingestpipeline/sourcetask/ISource.h"
#include "ingestpipeline/sourcetask/IVisSource.h"
#include "ingestpipeline/sourcetask/IMetadataSource.h"
#include "ingestpipeline/sourcetask/ScanManager.h"
#include "ingestpipeline/sourcetask/MonitoringPointManager.h"
#include "configuration/Configuration.h"
#include "ingestpipeline/sourcetask/VisConverter.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Ingest pipeline source tasks. The MergedSource task merges the TOS
/// metadata stream and the visibility stream creating a VISChunk object for
/// each correlator integration.
class MergedSource : public ISource,
                     public boost::noncopyable {
    public:
        /// @brief Constructor.
        ///
        /// @param[in] params           Parameters specific to this task
        /// @param[in] config           Configuration
        /// @param[in] metadataSource   Instance of a IMetadataSource from which the TOS
        ///                             TOS metadata will be sourced.
        /// @param[in] visSource    Instance of a IVisSource from which the visibilities
        ///                         will be sourced.
        /// @param[in] numTasks     Total number of ingest pipeline tasks. This enables
        ///                         the merged source to determine how many visibilities
        ///                         it is responsible for receiving.
        MergedSource(const LOFAR::ParameterSet& params,
                     const Configuration& config,
                     IMetadataSource::ShPtr metadataSource,
                     IVisSource::ShPtr visSource, int numTasks, int id);

        /// @brief Destructor.
        virtual ~MergedSource();

        /// @brief Called to obtain the next VisChunk from the merged stream.
        /// @return a shared pointer to a VisChunk.
        virtual askap::cp::common::VisChunk::ShPtr next(void);

    protected:

        /// @brief convert direction to J2000
        /// @details Helper method to convert given direction to 
        /// J2000 (some columns of the MS require fixed frame for
        /// all rows, it is handy to convert AzEl directions early
        /// in the processing chain).
        /// @param[in] epoch UTC time since MJD=0
        /// @param[in] ant antenna index (to get position on the ground)
        /// @param[in] dir direction measure to convert
        /// @return direction measure in J2000
        casa::MDirection convertToJ2000(const casa::MVEpoch &epoch, casa::uInt ant, 
                                        const casa::MDirection &dir) const;

    private:


        /// Initialises an "empty" VisChunk
        askap::cp::common::VisChunk::ShPtr createVisChunk(const TosMetadata& metadata);

        /// Handled the receipt of signals to "interrupt" the process
        void signalHandler(const boost::system::error_code& error,
                           int signalNumber);

        // Checks if a signal has been received requesting an interrupt.
        // If such a signal has been received, thorows an InterruptedException.
        void checkInterruptSignal();

        // The object that is the source of telescope metadata
        IMetadataSource::ShPtr itsMetadataSrc;
        
        // The object that is the source of visibilities
        IVisSource::ShPtr itsVisSrc;

        // Pointers to the two constituent datatypes
        boost::shared_ptr<TosMetadata> itsMetadata;
        boost::shared_ptr<VisDatagram> itsVis;

        // Scan Manager
        ScanManager itsScanManager;

        // Monitor point Manager
        const MonitoringPointManager itsMonitoringPointManager;

        // Interrupted by SIGTERM, SIGINT or SIGUSR1?
        bool itsInterrupted;

        // Boost io_service
        boost::asio::io_service itsIOService;

        // Interrupt signals
        boost::asio::signal_set itsSignals;

        /// @brief The last timestamp processed. This is stored to avoid the situation
        /// where we may produce two consecutive VisChunks with the same timestamp
        casa::uLong itsLastTimestamp;

        /// @brief visibility converter
        VisConverter<VisDatagram> itsVisConverter;

        /// For unit testing
        friend class MergedSourceTest;
};

}
}
}

#endif

/// @file NoMetadataSource.h
///
/// @copyright (c) 2013 CSIRO
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

#ifndef ASKAP_CP_INGEST_NOMETADATASOURCE_H
#define ASKAP_CP_INGEST_NOMETADATASOURCE_H

// System includes
#include <set>
#include <string>

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "boost/system/error_code.hpp"
#include "boost/asio.hpp"
#include "boost/noncopyable.hpp"
#include "boost/tuple/tuple.hpp"
#include "Common/ParameterSet.h"
#include "cpcommon/VisDatagram.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "ingestpipeline/sourcetask/ISource.h"
#include "ingestpipeline/sourcetask/IVisSource.h"
#include "ingestpipeline/sourcetask/MonitoringPointManager.h"
#include "configuration/Configuration.h"
#include "ingestpipeline/sourcetask/VisConverter.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Ingest pipeline source tasks. The NoMetadataSource task builds a VisChunk from
/// visibilities and configuration (in the parset) only, no TOs metadata is needed.
class NoMetadataSource : public ISource,
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
        NoMetadataSource(const LOFAR::ParameterSet& params,
                         const Configuration& config,
                         IVisSource::ShPtr visSource, int numTasks, int id);

        /// @brief Destructor.
        virtual ~NoMetadataSource();

        /// @brief Called to obtain the next VisChunk from the merged stream.
        /// @return a shared pointer to a VisChunk.
        virtual askap::cp::common::VisChunk::ShPtr next(void);

    private:

        /// @brief synchronise itsLastTimestamp across all ranks
        /// @details This method is probably only temporary. If
        /// ingest pipeline is used in parallel mode, this method
        /// ensures that all ranks have the same itsLastTimestamp
        /// corresponding to the latest value received. This 
        /// will help ingest pipeline to catch up if one of the
        /// cards missed an integration.
        /// @note Does nothing in the serial mode
        void syncrhoniseLastTimestamp();

        /// Initialises an "empty" VisChunk (inside the converter)
        askap::cp::common::VisChunk::ShPtr createVisChunk(const casa::uLong timestamp);

        /// Handled the receipt of signals to "interrupt" the process
        void signalHandler(const boost::system::error_code& error,
                           int signalNumber);

        // The object that is the source of visibilities
        IVisSource::ShPtr itsVisSrc;

        // Pointers to the two constituent datatypes
        boost::shared_ptr<VisDatagram> itsVis;

        // Interrupted by SIGTERM, SIGINT or SIGUSR1?
        bool itsInterrupted;

        // Boost io_service
        boost::asio::io_service itsIOService;

        // Interrupt signals
        boost::asio::signal_set itsSignals;

        /// @brief Centre frequency
        const casa::Quantity itsCentreFreq;

        /// @brief Target/field/source name
        const std::string itsTargetName;

        /// @brief Target direction
        const casa::MDirection itsTargetDirection;

        /// @brief Correlator Mode
        CorrelatorMode itsCorrelatorMode;

        // Monitor point Manager
        const MonitoringPointManager itsMonitoringPointManager;

        /// @brief The last timestamp processed. This is stored to avoid the situation
        /// where we may produce two consecutive VisChunks with the same timestamp
        casa::uLong itsLastTimestamp;

        /// @brief visibility converter
        VisConverter<VisDatagram> itsVisConverter;
};

}
}
}

#endif

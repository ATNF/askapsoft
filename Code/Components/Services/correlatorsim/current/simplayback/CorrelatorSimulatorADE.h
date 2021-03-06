/// @file CorrelatorSimulatorADE.h
///
/// @copyright (c) 2015 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>

#ifndef ASKAP_CP_SIMPLAYBACK_CORRELATORSIMULATORADE_H
#define ASKAP_CP_SIMPLAYBACK_CORRELATORSIMULATORADE_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/measures/Measures/Stokes.h"
#include "boost/scoped_ptr.hpp"

// Local package includes
#include "simplayback/ISimulator.h"
#include "simplayback/VisPortADE.h"
#include "simplayback/RandomReal.h"
#include "simplayback/CorrProdMap.h"
#include "simplayback/CorrBuffer.h"
#include "simplayback/ChannelMap.h"
#include "simplayback/CardFailMode.h"


namespace askap {
namespace cp {

/// @brief Simulates the visibility stream from the correlator.
class CorrelatorSimulatorADE : public ISimulator {
    public:
        /// Constructor
        ///
        /// @param[in] mode     Playback mode: normal or test
        /// @param[in] dataset  Filename for the measurement set which will be
        ///                     used to source the visibilities.
        /// @param[in] hostname Hostname or IP address of the host to which the
        ///                     UDP data stream will be sent.
        /// @param[in] port     UDP port number to which the UDP data stream 
        ///                     will be sent.
		/// @param[in] shelf	MPI rank
        /// @param[in] nAntenna         The number of antenna set by user
        /// @param[in] nCoarseChannel   The number of coarse channels
        /// @param[in] nChannelSub      The number of channel subdivision
        /// @param[in] coarseBandwidth  The bandwidth of coarse channel
        /// @param[in] delay            Transmission delay in microsecond
        CorrelatorSimulatorADE(
                const std::string& mode = "",
                const std::string& dataset ="",
				const std::string& hostname = "",
                const std::string& port = "",
                const uint32_t shelf = 0,
                const uint32_t nShelves = 0,
                const uint32_t nAntenna = 0,
                const uint32_t nCoarseChannel = 0,
				const uint32_t nFineChannel = 0,
                const uint32_t nChannelSub = 0,
                const double coarseBandwidth = 0.0,
                const uint32_t delay = 0,
				const CardFailMode& failMode = CardFailMode());

        /// Destructor
        virtual ~CorrelatorSimulatorADE();

        /// @brief Send the next correlator integration.
        /// @return True if there are more integrations in the dataset,
        ///         otherwise false. If false is returned, sendNext()
        ///         should not be called again.
        bool sendNext(void);

		/// @brief Reset current row, so we can read from the beginning
		void resetCurrentRow(void);

    private:

        // The mode of simulation. Possible values:
        // - coarse_channels: data needs to be expanded into fine channels
        // - fine_channels  : data is already in fine channels
        // - test           :
        string itsMode;

		// Correlation product map (this replaces baseline map)
		CorrProdMap itsCorrProdMap;

        // Channel mapping between measurement set (contiguous numbering)
        // and correlator simulator (non-contiguous numbering)
        ChannelMap itsChannelMap;

        // Shelf number [1..]
        const uint32_t itsShelf;

        // The number of shelves (= the number of MPI processes - 1)
        const uint32_t itsNShelves;

        // Number of antennas
        uint32_t itsNAntenna;

        // Number of correlation products (= baselines)
        uint32_t itsNCorrProd;

        // Number of slice
        uint32_t itsNSlice;

        // Number of coarse channels
        const uint32_t itsNCoarseChannel;

		// Number of fine channels
		const uint32_t itsNFineChannel;

        // Number of channel subdivision (coarse to fine)
        const uint32_t itsNChannelSub;

        // Coarse channel bandwidth
        const double itsCoarseBandwidth;

        // Fine channel bandwidth
        double itsFineBandwidth;

        // Current time stamp
        uint64_t itsCurrentTime;

        // Delay in microseconds
        uint32_t itsDelay;

		// Failure modes
		CardFailMode itsFailMode;

        // Cursor (index) for the main table of the measurement set
        uint32_t itsCurrentRow;

		// Count how many times data has been read from measurement set
		uint32_t itsDataReadCounter;

		// Count how many times data has been sent
		uint32_t itsDataSentCounter;

        // Measurement set
        boost::scoped_ptr<casa::MeasurementSet> itsMS;

        // Port for output of metadata
        boost::scoped_ptr<askap::cp::VisPortADE> itsPort;

        // Buffer data
        CorrBuffer itsBuffer;

        // Test buffer
        CorrBuffer itsTestBuffer;

        // Antenna indices
        vector<uint32_t> itsAntIndices;


        // Internal functions
        
        /// Given antenna pair and Stokes type, get correlation product
        /// @param[in] ant1 Antenna 1
        /// @param[in] ant2 Antenna 2
        /// @param[in] stokesType   Stokes type (XX, XY, YX, YY)
        uint32_t getCorrProdIndex
                (uint32_t ant1, uint32_t ant2,
                const casa::Stokes::StokesTypes& stokesType);

        /// Initialize buffer for intermediate storage
        void initBuffer();

        /// Get buffer data, which is a matrix of a beam (full set of
        /// correlation products, as set by user) and channels
        /// (from measurement set) 
        /// @return True if successful, false if not 
        /// (eg. no more data in measurement set)
#ifdef NEW_BUFFER
        bool getNewBufferData();
#else
        bool getBufferData();
#endif
   
        /// Fill empty correlation products by copying data from
        /// those originally filled with measurement set data
        void fillCorrProdInBuffer();

        /// Fill empty channels by copying data from
        /// those originally filled with measurement set data
        void fillChannelInBuffer();

        /// Renumber channels and cards to conform with datagram specification
        void renumberChannelAndCard();

        /// Send buffer data 
        /// @return True if successful, false if not 
        /// (eg. no more data in buffer to send)
        bool sendBufferData();

        /// Fill test buffer with data from payload.
        /// The test buffer simulates ingest.
        /// @param[in] payload Datagram containing visibility data
        void fillTestBuffer(askap::cp::VisDatagramADE& payload);

        /// Check the test buffer for bad data.
        /// At this moment it is used to check the correct association 
        /// between channel number and frequency.
        void checkTestBuffer();

		//
		//void getFailMode();
};

};
};
#endif

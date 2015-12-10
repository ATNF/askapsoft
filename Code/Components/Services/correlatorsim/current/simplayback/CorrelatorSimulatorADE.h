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
#include "ms/MeasurementSets/MeasurementSet.h"
#include "measures/Measures/Stokes.h"
#include "boost/scoped_ptr.hpp"

// Local package includes
#include "simplayback/ISimulator.h"
#include "simplayback/VisPortADE.h"
#include "simplayback/RandomReal.h"
#include "simplayback/CorrProdMap.h"
#include "simplayback/CorrBuffer.h"
#include "simplayback/ChannelMap.h"

namespace askap {
namespace cp {

/// @brief Simulates the visibility stream from the correlator.
class CorrelatorSimulatorADE : public ISimulator {
    public:
        /// Constructor
        ///
        /// @param[in] dataset  filename for the measurement set which will be
        ///                     used to source the visibilities.
        /// @param[in] hostname hostname or IP address of the host to which the
        ///                     UDP data stream will be sent.
        /// @param[in] port     UDP port number to which the UDP data stream 
        ///                     will be sent.
		/// @param[in] shelf	MPI rank
        /// @param[in] nAntenna The number of antenna set by user
        /// @param[in] nCoarseChannel   The number of coarse channels
        /// @param[in] nChannelSub      The number of channel subdivision
        /// @param[in] coarseBandwidth  The bandwidth of coarse channel
        /// @param[in] inputMode        Input mode of the simulator
        /// @param[in] delay            Transmission delay in microsecond
        CorrelatorSimulatorADE(const std::string& dataset,
				const std::string& hostname = "",
                const std::string& port = "",
                const int shelf = 0,
                const int nShelves = 0,
                const unsigned int nAntenna = 0,
                const unsigned int nCoarseChannel = 0,
                const unsigned int nChannelSub = 0,
                const double coarseBandwidth = 0.0,
                const std::string& inputMode = "",
                const unsigned int delay = 0);

        /// Destructor
        virtual ~CorrelatorSimulatorADE();

        /// @brief Send the next correlator integration.
        /// @return True if there are more integrations in the dataset,
        ///         otherwise false. If false is returned, sendNext()
        ///         should not be called again.
        bool sendNext(void);
		
    private:

		// Correlation product map (this replaces baseline map)
		CorrProdMap itsCorrProdMap;

        // Channel mapping
        ChannelMap itsChannelMap;

        // Shelf number [1..]
        const int itsShelf;

        // The number of shelves (= the number of MPI processes - 1)
        const int itsNShelves;

        // Number of antennas
        unsigned int itsNAntenna;

        // Number of correlation products (= baselines)
        unsigned int itsNCorrProd;

        // Number of slice
        unsigned int itsNSlice;

        // Number of coarse channels
        const unsigned int itsNCoarseChannel;

        // Number of channel subdivision (coarse to fine)
        const unsigned int itsNChannelSub;

        // Coarse channel bandwidth
        const double itsCoarseBandwidth;

        // Fine channel bandwidth
        double itsFineBandwidth;

        // Data input mode 
        const std::string itsInputMode;

        // Current time stamp
        uint64_t itsCurrentTime;

        // Delay in microseconds
        unsigned int itsDelay;

        // Cursor (index) for the main table of the measurement set
        unsigned int itsCurrentRow;

		const static unsigned int rowIncrement = 36;

        bool firstPayloadSent;

        // Measurement set
        boost::scoped_ptr<casa::MeasurementSet> itsMS;

        // Port for output of metadata
        boost::scoped_ptr<askap::cp::VisPortADE> itsPort;

        // Buffer data
        CorrBuffer buffer;

        // Antenna indices
        vector<unsigned int> antIndices;


        // Internal functions
        
        /// Given antenna pair and Stokes type, get correlation product
        /// @param[in] ant1 Antenna 1
        /// @param[in] ant2 Antenna 2
        /// @param[in] stokesType   Stokes type (XX, XY, YX, YY)
        uint32_t getCorrProdIndex
                (const uint32_t ant1, const uint32_t ant2,
                const casa::Stokes::StokesTypes stokesType);

        /// Initialize buffer for intermediate storage
        void initBuffer();

        /// Get buffer data, which is a matrix of a beam (full set of
        /// correlation products, as set by user) and channels
        /// (from measurement set) 
        /// @return True if successful, false if not 
        /// (eg. no more data in measurement set)
        bool getBufferData();
        
        /// Fill empty correlation products by copying data from
        /// those originally filled with measurement set data
        void fillCorrProdInBuffer();

        /// Fill empty channels by copying data from
        /// those originally filled with measurement set data
        void fillChannelInBuffer();

        /// Renumber channels and cards to conform with datagram specification
        void renumberChannelAndCard();

        /// Send the first payload
        /// @return True if successful, false if not 
        /// (eg. no more data in buffer to send)
        bool sendFirstPayload();

        /// Send buffer data 
        /// @return True if successful, false if not 
        /// (eg. no more data in buffer to send)
        bool sendBufferData();

        // To be deprecated
		// Send data of zero visibility for the whole baselines and channels 
		// for only 1 time period.
		bool sendNextZero();
	
        // To be deprecated
		// Extract one data point from measurement set and send the data 
		// for all baselines and channels for the time period given in 
        // the measurement set.
        // @return True if successful
		bool sendNextExpand();

};

};
};
#endif

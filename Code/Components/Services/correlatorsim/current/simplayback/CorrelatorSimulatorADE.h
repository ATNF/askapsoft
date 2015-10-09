/// @file CorrelatorSimulatorADE.h
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
//#include "simplayback/BaselineMap.h"
#include "simplayback/RandomReal.h"
#include "simplayback/CorrProdMap.h"

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
        /// @param[in] port     UDP port number to which the UDP data stream will
        ///                     be sent.
        /// @param[in] expansionFactor  the channel multiplication factor. A
        ///     non-unity expansion factor allows a small input dataset to be
        ///     used to produce a larger output data stream. For example
        ///     simulating a small 304 channel (1MHz channels) dataset and using
        ///     an expansion factor of 54 to get to a 16416 (18.5KHz channels)
        ///     data stream.
        /// @param[in] visSendFail  the chance a VisChunk will not be sent. A failure
        ///                         is simulated by simple not attempting the send.
        ///                         A value of of 0.0 results in no failures, while
        ///                         1.0 results in all sends failing.
		/// @param[in] shelf	MPI rank
        CorrelatorSimulatorADE(const std::string& dataset,
				const std::string& hostname = "",
                const std::string& port = "",
                const int shelf = 0,
                const unsigned int nAntenna = 0,
                const unsigned int nCoarseChannel = 0,
                const unsigned int nChannelSub = 0,
                const double coarseBandwidth = 0.0,
                const std::string& visSource = "",
                const unsigned int delay = 0);

        /// Destructor
        virtual ~CorrelatorSimulatorADE();

        /// @brief Send the next correlator integration.
        ///
        /// @return true if there are more integrations in the dataset,
        ///         otherwise false. If false is returned, sendNext()
        ///         should not be called again.
        bool sendNext(void);

    private:

		/// Total number of baselines
		//static const uint32_t NBASELINE = 2628;
		
		/// = 2628 / 657
		//static const uint32_t NSLICE = 4;	
		   
		// Correlation product map (this replaces baseline map)
		CorrProdMap itsCorrProdMap;
		
        // Channel multiplication factor
        //const unsigned int itsExpansionFactor;

        // The chance a VisChunk will not be sent.
        //const double itsVisSendFailChance;

        // Shelf number [1..]
        const int itsShelf;

        // Number of antennas
        const unsigned int itsNAntenna;

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

        // Visibility data source
        const std::string& itsVisSource;

        // Delay in microseconds
        unsigned int itsDelay;

        // Cursor (index) for the main table of the measurement set
        unsigned int itsCurrentRow;

        // Source of randomness (for simulating random failures)
        //RandomReal<double> itsRandom;

        // Measurement set
        boost::scoped_ptr<casa::MeasurementSet> itsMS;

        // Port for output of metadata
        boost::scoped_ptr<askap::cp::VisPortADE> itsPort;

        // Internal functions
        //
        // Given antenna pair and Stokes type, get correlation product
        uint32_t getCorrProdIndex
                (const uint32_t ant1, const uint32_t ant2,
                const casa::Stokes::StokesTypes stokesType);

};

};
};
#endif

/// @file VisConverterBase.cc
/// @brief base class for converter of the visibility data stream
/// @details Visibility converter class is responsible for populating
/// VisBuffer from the datagrams received from the correlator. It takes
/// care of integrity and the split between individual datagrams.
/// VisConverterBase is the base class which contains common methods. 
/// As we don't plan to use various distribution schemes in one system,
/// there is no much reason in making the methods of this class polymorphic,
/// nor derive from an abstract interface (although such change would be
/// straight forward). 
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

// Include own header file first
#include "VisConverterBase.h"

// Include package level header file
#include "askap_cpingest.h"


// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "utils/PolConverter.h"

// 3rd party includes
#include "measures/Measures.h"
#include "measures/Measures/MeasFrame.h"
#include "measures/Measures/MCEpoch.h"

ASKAP_LOGGER(logger, ".VisConverterBase");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

/// @param[in] params parameters specific to the associated source task
///                   used to set up mapping, etc
/// @param[in] config configuration
/// @param[in] id rank of the given ingest process
VisConverterBase::VisConverterBase(const LOFAR::ParameterSet& params,
                    const Configuration& config, int id) : 
       itsDatagramsExpected(0u),
       itsDatagramsCount(0u), itsDatagramsIgnored(0u), itsConfig(config),
       itsMaxNBeams(params.getUint32("maxbeams",0)),
       itsBeamsToReceive(params.getUint32("beams2receive",0)),
       itsChannelManager(params),
       itsBaselineMap(config.bmap()), itsId(id)
{
   // Trigger a dummy frame conversion with casa measures to ensure 
   // all caches are setup early on
   const casa::MVEpoch dummyEpoch(56000.);

   casa::MEpoch::Convert(casa::MEpoch(dummyEpoch, casa::MEpoch::Ref(casa::MEpoch::TAI)),
                         casa::MEpoch::Ref(casa::MEpoch::UTC))();

   // initialise beam mapping
   initBeamMap(params);
}


/// @brief current vis chunk
/// @return shared pointer to current vis chunk for further processing
/// @note An exception is thrown if one attempts to get an uninitialised
/// VisChunk.
const VisChunk::ShPtr& VisConverterBase::visChunk() const
{
   ASKAPCHECK(itsVisChunk, "VisChunk doesn't seem to be initialised");
   return itsVisChunk;
}

/// @brief initialise beam maps
/// @details Beams can be mapped and indices can be non-contiguous. This
/// method sets up the mapping based in the parset and also evaluates the
/// actual number of beams for the sizing of buffers.
/// @param[in] params parset with parameters (e.g. beammap)
void VisConverterBase::initBeamMap(const LOFAR::ParameterSet& params)
{                             
    const std::string beamidmap = params.getString("beammap","");
    if (beamidmap != "") {    
        ASKAPLOG_INFO_STR(logger, "Beam indices will be mapped according to <"<<beamidmap<<">");
        itsBeamIDMap.add(beamidmap);
    }   
    const casa::uInt nBeamsInConfig = itsConfig.feed().nFeeds();
    if (itsMaxNBeams == 0) {
        for (int beam = 0; beam < static_cast<int>(nBeamsInConfig) + 1; ++beam) {
             const int processedBeamIndex = itsBeamIDMap(beam);
             if (processedBeamIndex > static_cast<int>(itsMaxNBeams)) {
                 // negative values are automatically excluded by this condition
                 itsMaxNBeams = static_cast<casa::uInt>(processedBeamIndex);
             }
        }
        ++itsMaxNBeams;
    }   
    if (itsBeamsToReceive == 0) {
        itsBeamsToReceive = nBeamsInConfig;
    }        
    ASKAPLOG_INFO_STR(logger, "Number of beams: " << nBeamsInConfig << " (defined in configuration), "
            << itsBeamsToReceive << " (to be received), " << itsMaxNBeams << " (to be written into MS)");
    ASKAPDEBUGASSERT(itsMaxNBeams > 0);
    ASKAPDEBUGASSERT(itsBeamsToReceive > 0);
}


/// @brief sum of arithmetic series
/// @details helper method to obtain the sum of n elements of 
/// arithmetic series with the given first element and increment
/// @param[in] n number of elements in the series to sum
/// @param[in] a first element
/// @param[in] d increment
/// @return the sum of the series
uint32_t VisConverterBase::sumOfArithmeticSeries(uint32_t n, uint32_t a, 
                                                 uint32_t d)
{
   ASKAPDEBUGASSERT(n > 0);
   return (n / 2.0) * ((2 * a) + ((n - 1) * d));
}

/// @brief row for given baseline and beam
/// @details We have a fixed layout of data in the VisChunk/measurement set.
/// This helper method implements an analytical function mapping antenna
/// indices and beam index onto the row number.
/// @param[in] ant1 index of the first antenna
/// @param[in] ant2 index of the second antenna
/// @param[in] beam beam index
/// @return row number in the VisChunk
uint32_t VisConverterBase::calculateRow(uint32_t ant1, uint32_t ant2,
                                    uint32_t beam) const
{
    ASKAPDEBUGASSERT(ant1 <= ant2);
    const uint32_t nAntenna = itsConfig.antennas().size();
    ASKAPDEBUGASSERT(ant2 < nAntenna);
    ASKAPDEBUGASSERT(beam < itsMaxNBeams);
    return (beam * (nAntenna * (nAntenna + 1) / 2))
        + (ant1 * (nAntenna) - sumOfArithmeticSeries(ant1 + 1, 0, 1))
        + ant2;
}


/// @brief create a new VisChunk
/// @details This method initialises itsVisChunk with a new buffer.
/// It is intended to be used when the first datagram of a new 
/// integration is processed.
/// @param[in] timestamp BAT corresponding to this new chunk
/// @param[in] corrMode correlator mode parameters (determines shape, etc)
void VisConverterBase::initVisChunk(const casa::uLong timestamp, 
                                    const CorrelatorMode &corrMode)
{
    const casa::uInt nAntenna = itsConfig.antennas().size();
    ASKAPCHECK(nAntenna > 0, "Must have at least one antenna defined");
    const casa::uInt nChannels = itsChannelManager.localNChannels(itsId);
    // number of polarisation products is determined by the correlator mode
    const casa::uInt nPol = corrMode.stokes().size();
    const casa::uInt nBaselines = nAntenna * (nAntenna + 1) / 2;
    const casa::uInt nRow = nBaselines * itsMaxNBeams;
    // correlator dump time is determined by the correlator mode
    const casa::uInt period = corrMode.interval(); // in microseconds

    // now shape is determined, can create a new chunk
    itsVisChunk.reset(new VisChunk(nRow, nChannels, nPol, nAntenna));

    // Convert the time from integration start in microseconds to an
    // integration mid-point in seconds
    const uint64_t midpointBAT = static_cast<uint64_t>(timestamp + (period / 2ull));
    itsVisChunk->time() = bat2epoch(midpointBAT).getValue();
    // Convert the interval from microseconds (long) to seconds (double)
    const casa::Double interval = period / 1000.0 / 1000.0;
    itsVisChunk->interval() = interval;

    // All visibilities get flagged as bad, then as the visibility data
    // arrives they are unflagged
    itsVisChunk->flag() = true;
    itsVisChunk->visibility() = 0.0;

    ASKAPCHECK(nPol <= 4, "Only supporting a maximum of 4 polarisation products");
    ASKAPCHECK(nPol > 0, "The number of polarisations need to be positive");

    // this way of creating the Stokes vectors ensures the 
    // canonical order of polarisation products
    // the last parameter of stokesFromIndex just defines the 
    // frame (i.e. linear, circular) and can be
    // any product from the chosen frame. 
    const casa::Stokes::StokesTypes stokesTemplate = corrMode.stokes()[0];
    casa::uInt outPolIndex = 0;
    for (casa::uInt polIndex = 0; polIndex < 4; ++polIndex) {
         casa::Stokes::StokesTypes thisStokes = casa::Stokes::Undefined;
         const casa::Stokes::StokesTypes testedStokes = 
               scimath::PolConverter::stokesFromIndex(polIndex, stokesTemplate);
         // search via for-loop rather than via std::find to do further checks
         for (std::vector<casa::Stokes::StokesTypes>::const_iterator 
                ci = corrMode.stokes().begin(); ci != corrMode.stokes().end(); 
                ++ci) {
              if (*ci == testedStokes) {
                  // this product is present in the correlator output
                  ASKAPCHECK(thisStokes == casa::Stokes::Undefined, 
                      "Duplicate Stokes products found in the polarisation setup: "<<
                       scimath::PolConverter::toString(corrMode.stokes()));
                  thisStokes = testedStokes;
              }
         }
         if (thisStokes != casa::Stokes::Undefined) {
             ASKAPASSERT(outPolIndex < nPol);
             itsVisChunk->stokes()(outPolIndex++) = thisStokes;
         }
    }
    ASKAPCHECK(outPolIndex == nPol, 
     "Mixed polarisation products are not supported. Correlator output has: "<<
      scimath::PolConverter::toString(corrMode.stokes())<<
      ", successfully matched only "<<outPolIndex<<" products: "<<scimath::PolConverter::toString(itsVisChunk->stokes()));

    // channel width is determined by the correlator configuration
    itsVisChunk->channelWidth() = corrMode.chanWidth().getValue("Hz");

    for (casa::uInt beam = 0, row = 0; beam < itsMaxNBeams; ++beam) {
        for (casa::uInt ant1 = 0; ant1 < nAntenna; ++ant1) {
            for (casa::uInt ant2 = ant1; ant2 < nAntenna; ++ant2,++row) {
                ASKAPCHECK(row < nRow, "Row index (" << row <<
                           ") should be less than nRow (" << nRow << ")");

                // nothing is done to phase centre and pointing
                // centre here. These fields are filled by
                // the methods of the source task
                itsVisChunk->antenna1()(row) = ant1;
                itsVisChunk->antenna2()(row) = ant2;
                itsVisChunk->beam1()(row) = beam;
                itsVisChunk->beam2()(row) = beam;
                itsVisChunk->beam1PA()(row) = 0;
                itsVisChunk->beam2PA()(row) = 0;
                itsVisChunk->uvw()(row) = 0.0;
            }
        }
    }
    
    // initialise stats, expected number of datagrams is configured
    // in derived classes just reset it here
    itsDatagramsExpected = 0; 
    itsDatagramsIgnored = 0;
    itsDatagramsCount = 0;
}



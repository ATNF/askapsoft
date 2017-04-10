/// @file VisConverterBase.h
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

#ifndef ASKAP_CP_INGEST_VISCONVERTERBASE_H
#define ASKAP_CP_INGEST_VISCONVERTERBASE_H

// 3rd party includes
#include "boost/shared_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "boost/optional.hpp"
#include "Common/ParameterSet.h"
#include "casacore/casa/aipstype.h"

// standard includes
#include <stdint.h>
#include <utility>
#include <set>

// ASKAPsoft includes
#include "cpcommon/VisDatagram.h"
#include "cpcommon/VisChunk.h"
#include "askap/IndexConverter.h"

// local includes
#include "configuration/Configuration.h"
#include "configuration/BaselineMap.h"
#include "configuration/CorrelatorMode.h"
#include "ingestpipeline/sourcetask/ChannelManager.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief base class for converter of the visibility data stream
/// @details Visibility converter class is responsible for populating
/// VisBuffer from the datagrams received from the correlator. It takes
/// care of integrity and the split between individual datagrams.
/// VisConverterBase is the base class which contains common methods. 
/// As we don't plan to use various distribution schemes in one system,
/// there is no much reason in making the methods of this class polymorphic,
/// nor derive from an abstract interface (although such change would be
/// straight forward). 
class VisConverterBase : public boost::noncopyable {
public:
   /// @param[in] params parameters specific to the associated source task
   ///                   used to set up mapping, etc
   /// @param[in] config configuration
   VisConverterBase(const LOFAR::ParameterSet& params,
                    const Configuration& config);

   /// @brief get expected number of datagrams
   /// @return the number of datagrams required to complete VisChunk
   /// @note it is initialised in derived classes together with VisChunk
   inline casa::uInt datagramsExpected() const { return itsDatagramsExpected; }

   /// @brief number of datagrams used
   /// @return the number of datagrams contributing to the current VisChunk
   inline casa::uInt datagramsCount() const { return itsDatagramsCount; }

   /// @brief number of datagrams ignored
   /// @return the number of successfully received datagrams which are
   /// ignored for some reason (e.g. mapping) and didn't contribute to
   /// the current VisChunk.
   inline casa::uInt datagramsIgnored() const { return itsDatagramsIgnored; }

   /// @brief check that all expected datagrams received
   /// @return true, if datagramsCount() + datagramsIgnored() == datagramsExpected()
   inline bool gotAllExpectedDatagrams() const
         { return datagramsCount() + datagramsIgnored() == datagramsExpected(); }

   /// @brief current vis chunk
   /// @return shared pointer to current vis chunk for further processing
   /// @note An exception is thrown if one attempts to get an uninitialised
   /// VisChunk.
   const common::VisChunk::ShPtr& visChunk() const;

   /// @brief access to configuration
   /// @return const reference to configuration class
   inline const Configuration& config() const { return itsConfig;}

   /// @return maximum number of beams
   inline casa::uInt maxNumberOfBeams() const { return itsMaxNBeams; } 

   /// @return number of beams to receive
   inline casa::uInt nBeamsToReceive() const { return itsBeamsToReceive; }

   /// @brief obtain channel manager
   /// @return const reference to the channel manager
   inline const ChannelManager& channelManager() const 
           { return itsChannelManager; }

   /// @brief flag a particular antenna for the whole chunk
   /// @details We want to avoid expensive per-channel operations
   /// where possible. This method allows to set antenna-based flags
   /// which remain valid until the next initVisChunk call and can
   /// be quieried in derived classes to bypass unflagging.
   /// By default (and after every call to initVisChunk) all antennas
   /// are unflagged.
   /// @param[in] antenna index for the antenna to flag as bad
   void flagAntenna(casa::uInt antenna);

   /// @brief query whether given antenna produce good data
   /// @param[in] antenna index of antenna to check
   /// @return true if a given antenna is unflagged
   bool isAntennaGood(casa::uInt antenna) const;

protected:

   /// @return number of correlator products in the map
   inline uint32_t nCorrProducts() const { return itsBaselineMap.size(); }

   // note, there is no much reason making this class polymorphic as
   // we don't intend to use different implementations simultaneously.
   // Therefore, derived classes will contain the actual entry point and
   // this base class has a number of methods made protected, so they are
   // not used directly. It can be converted to interface/factory +
   // polymorphic behavior later on, if needed.
   
   /// @brief increment number of ignored datagrams
   inline void countDatagramAsIgnored() { ++itsDatagramsIgnored; }

   /// @brief increment number of useful datagrams
   inline void countDatagramAsUseful() { ++itsDatagramsCount; }

   /// @brief set the number of expected datagrams
   /// @details This method is intended to be used in derived classes
   /// @param[in] number number of expected datagrams
   inline void setNumberOfExpectedDatagrams(casa::uInt number) 
          { itsDatagramsExpected = number; }

   /// @brief map correlation product to the visibility chunk
   /// @details This method maps baseline and beam ids to
   /// the VisChunk row and polarisation index. The remaining
   /// dimension of the cube (channel) has to be taken care of
   /// separately. The return of undefined value means that
   /// given IDs are not mapped (quite possibly intentionally,
   /// e.g. if we don't want to write all data received from the IOC).
   /// @param[in] baseline baseline ID to map (defined by the IOC)
   /// @param[in] beam beam ID to map (defined by the IOC)
   /// @return a pair of row and polarisation indices (guaranteed to
   /// be within VisChunk shape). Undefined value for unmapped products.
   boost::optional<std::pair<casa::uInt, casa::uInt> > mapCorrProduct(uint32_t baseline, uint32_t beam) const;

   /// @brief create a new VisChunk
   /// @details This method initialises itsVisChunk with a new buffer.
   /// It is intended to be used when the first datagram of a new 
   /// integration is processed.
   /// @param[in] timestamp BAT corresponding to this new chunk
   /// @param[in] corrMode correlator mode parameters (determines shape, etc)
   void initVisChunk(const casa::uLong timestamp, const CorrelatorMode &corrMode);

private:

   /// @brief row for given baseline and beam
   /// @details We have a fixed layout of data in the VisChunk/measurement set.
   /// This helper method implements an analytical function mapping antenna
   /// indices and beam index onto the row number.
   /// @param[in] ant1 index of the first antenna
   /// @param[in] ant2 index of the second antenna
   /// @param[in] beam beam index
   /// @return row number in the VisChunk
   uint32_t calculateRow(uint32_t ant1, uint32_t ant2, uint32_t beam) const;
  
   /// @brief helper method to map polarisation product
   /// @details This method obtains polarisation dimension index
   /// for the given Stokes parameter. Undefined value means VisChunk
   /// does not contain selected product.
   /// param[in] stokes input Stokes parameter
   /// @return polarisation index. Undefined value for unmapped polarisation.
   boost::optional<casa::uInt> mapStokes(casa::Stokes::StokesTypes stokes) const;

   /// @brief initialise beam maps
   /// @details Beams can be mapped and indices can be non-contiguous. This
   /// method sets up the mapping based on the parset and also evaluates the
   /// actual number of beams for the sizing of buffers.
   /// @param[in] params parset with parameters (e.g. beammap)
   void initBeamMap(const LOFAR::ParameterSet& params);

   /// @brief sum of arithmetic series
   /// @details helper method to obtain the sum of n elements of 
   /// arithmetic series with the given first element and increment
   /// @param[in] n number of elements in the series to sum
   /// @param[in] a first element
   /// @param[in] d increment
   /// @return the sum of the series
   static uint32_t sumOfArithmeticSeries(uint32_t n, uint32_t a, uint32_t d);

   /// @brief shared pointer to visibility chunk being filled
   common::VisChunk::ShPtr itsVisChunk;

   /// @brief expected number of datagrams
   /// @details This field is initialised at the time a new VisChunk 
   /// is created and contains the number of datagrams required to
   /// complete VisChunk. Exact value is determined in derived classes.
   casa::uInt itsDatagramsExpected;

   /// @brief number of datagrams used
   /// @details This counter is reset each time a new VisChunk is created.
   /// It is intended to count the number of datagrams contributing to the
   /// given chunk (i.e. excludes the datagrams which are ignored for some
   /// reason)
   casa::uInt itsDatagramsCount;

   /// @brief This counter is reset each time a new VisChunk is created.
   /// It is intended to count the number of successfully received datagrams
   /// which are ignored for some reason.
   casa::uInt itsDatagramsIgnored;

   /// @brief configuration
   const Configuration itsConfig;

   // for beam configuration/selection/mapping

   /// @brief beam id map
   /// @details It is possible to filter the beams received by the source 
   /// and map the indices. This map provides translation (by default, 
   /// any index is passed as is)
   utility::IndexConverter  itsBeamIDMap;

   /// @brief largest supported number of beams
   /// @details The space is reserved for the given number of beams 
   /// (set via the parset). This value is always less than or equal 
   /// to the number of beams specified via the configuration (the 
   /// latter is the default). The visibility cube is resized to match
   /// this parameter (allowing to drop unnecessary beams if used 
   /// together with itsBeamIDMap)
   ///
   /// @note it is zero by default, which triggers the initBeamMap 
   /// method called from the constructor to set it equal to the
   /// configuration (i.e. to write everything, unless a specific mapping
   /// is configured in the parset)
   casa::uInt itsMaxNBeams;

   /// @brief number of beams to expect in the data stream
   /// @details A larger number of beams can be received from the 
   /// datastream than is stored into MS. To avoid unnecessary bloat of 
   /// the MS size, only itsMaxNBeams beams are stored. This field 
   /// controls the data stream unpacking.
   ///
   /// @note it is zero by default, which triggers initBeamMap
   /// method called from the constructor to set it from
   /// configuration (i.e. to write everything, unless a specific mapping
   /// is configured via the parset)
   casa::uInt itsBeamsToReceive;

   /// @brief Channel Manager
   ChannelManager itsChannelManager;

   /// @brief Baseline Map
   const BaselineMap itsBaselineMap;

   /// @brief warning flag per unknown polarisation
   /// @details to avoid spitting out too much messages
   mutable std::set<casa::Stokes::StokesTypes> itsIgnoredStokesWarned;

   /// @brief antenna-based flags
   /// @detail Zero length means all antennas are unflagged. Otherwise,
   /// it should always be of an appropriate size to handle all indices
   /// available in the chunk.
   std::vector<casa::uInt> itsAntWithValidData;

   /// For unit testing
   friend class VisConverterBaseTest;
};

} // namespace ingest

} // namespace cp

} // namespace askap

#endif // #ifndef ASKAP_CP_INGEST_VISCONVERTERBASE_H



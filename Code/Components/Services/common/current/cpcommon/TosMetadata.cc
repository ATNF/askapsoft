/// @file TosMetadata.cc
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

// Include own header file first
#include "TosMetadata.h"

// System includes
#include <vector>
#include <map>
#include <utility>

// ASKAPsoft includes
#include "askap/askap/AskapError.h"
#include "askap/askap/AskapUtil.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"
#include "cpcommon/CasaBlobUtils.h"

// LOFAR
#include <Blob/BlobArray.h>
// Using
using namespace std;
using namespace askap::cp;

TosMetadata::TosMetadata() : itsTime(0), itsScanId(-1), itsFlagged(false)
{
}

/// @brief copy constructor
/// @details It is needed due to reference semantics of casa arrays
/// @param[in] other an object to copy from
TosMetadata::TosMetadata(const TosMetadata &other) : itsTime(other.itsTime), itsScanId(other.itsScanId),
    itsFlagged(other.itsFlagged), itsCentreFreq(other.itsCentreFreq), itsTargetName(other.itsTargetName),
    itsTargetDirection(other.itsTargetDirection), itsPhaseDirection(other.itsPhaseDirection),
    itsCorrMode(other.itsCorrMode), itsBeamOffsets(other.itsBeamOffsets.copy()),
    itsAntennas(other.itsAntennas) {}

/// @brief assignment operator
/// @details It is needed due to reference semantics of casa arrays
/// @param[in] other an object to copy from
TosMetadata& TosMetadata::operator=(const TosMetadata &other)
{
  if (this != &other) {
      itsTime = other.itsTime;
      itsScanId = other.itsScanId;
      itsFlagged = other.itsFlagged;
      itsCentreFreq = other.itsCentreFreq;
      itsTargetName = other.itsTargetName;
      itsTargetDirection = other.itsTargetDirection;
      itsPhaseDirection = other.itsPhaseDirection;
      itsCorrMode = other.itsCorrMode;
      itsBeamOffsets.assign(other.itsBeamOffsets.copy());
      itsAntennas = other.itsAntennas;
  }
  return *this;
}

casacore::uLong TosMetadata::time(void) const
{
    return itsTime;
}

void TosMetadata::time(const casacore::uLong time)
{
    itsTime = time;
}

casacore::Int TosMetadata::scanId(void) const
{
    return itsScanId;
}

void TosMetadata::scanId(const casacore::Int id)
{
    itsScanId = id;
}

casacore::Bool TosMetadata::flagged(void) const
{
    return itsFlagged;
}

void TosMetadata::flagged(const casacore::Bool flag)
{
    itsFlagged = flag;
}

casacore::Quantity TosMetadata::centreFreq(void) const
{
    return itsCentreFreq;
}

void TosMetadata::centreFreq(const casacore::Quantity& freq)
{
    itsCentreFreq = freq;
}

std::string TosMetadata::targetName(void) const
{
    return itsTargetName;
}

void TosMetadata::targetName(const std::string& name)
{
    itsTargetName = name;
}

casacore::MDirection TosMetadata::targetDirection(void) const
{
    return itsTargetDirection;
}

void TosMetadata::targetDirection(const casacore::MDirection& dir)
{
    itsTargetDirection = dir;
}

casacore::MDirection TosMetadata::phaseDirection(void) const
{
    return itsPhaseDirection;
}

void TosMetadata::phaseDirection(const casacore::MDirection& dir)
{
    itsPhaseDirection = dir;
}

void TosMetadata::corrMode(const std::string& mode)
{
    itsCorrMode = mode;
}

std::string TosMetadata::corrMode(void) const
{
    return itsCorrMode;
}

/// @return the reference to 2xnBeam beam offset matrix
/// @note the current design/metadata datagram assumes same beam offsets
/// for all antennas. Also in some special modes we set these offsets to zero
/// regardless of the actual beam pointings. Values are in radians (although this
/// class doesn't rely on particular units and passes whatever value was set.
const casacore::Matrix<casacore::Double>& TosMetadata::beamOffsets() const
{
    return itsBeamOffsets;
}

/// @brief Set beam offsets
/// @param[in] offsets 2xnBeam beam offsets matrix 
void TosMetadata::beamOffsets(const casacore::Matrix<casacore::Double> &offsets)
{
    if (offsets.nelements() != 0) {
        ASKAPCHECK(offsets.nrow() == 2, "Beam offset matrix is expected to have 2 x nBeam shape, you have: "<<offsets.shape());
        // make a copy due to reference semantics of casa arrays
        itsBeamOffsets.assign(offsets.copy());
    } else {
        itsBeamOffsets.resize(0,0);
    }
}

void TosMetadata::addAntenna(const TosMetadataAntenna& ant)
{
    // Ensure an antenna of this name does not already exist
    const map<string, TosMetadataAntenna>::const_iterator it = itsAntennas.find(ant.name());
    if (it != itsAntennas.end()) {
        ASKAPTHROW(AskapError, "An antenna with this name (" << ant.name()
                << ") already exists");
    }

    itsAntennas.insert(make_pair(ant.name(), ant));
}

casacore::uInt TosMetadata::nAntenna() const
{
    return static_cast<casacore::uInt>(itsAntennas.size());
}

std::vector<std::string> TosMetadata::antennaNames(void) const
{
    vector<string> names;
    for (map<string, TosMetadataAntenna>::const_iterator it = itsAntennas.begin();
            it != itsAntennas.end(); ++it) {
        names.push_back(it->first);
    }
    return names;
}

const TosMetadataAntenna& TosMetadata::antenna(const std::string& name) const
{
    const map<string, TosMetadataAntenna>::const_iterator it = itsAntennas.find(name);
    if (it == itsAntennas.end()) {
        ASKAPTHROW(AskapError, "Antenna " << name << " not found in metadata");
    }
    return it->second;
}

/// serialise TosMetadata
/// @param[in] os output stream
/// @param[in] obj object to serialise
/// @return output stream
LOFAR::BlobOStream& LOFAR::operator<<(LOFAR::BlobOStream& os, const askap::cp::TosMetadata& obj)
{
   os.putStart("TosMetadata", 2);
   os << static_cast<uint64>(obj.time()) << obj.scanId() << obj.flagged() << obj.centreFreq() << obj.targetName() << 
         obj.targetDirection() << obj.phaseDirection() << obj.corrMode() << obj.beamOffsets();
   const std::vector<std::string> antNames = obj.antennaNames();
   os << static_cast<uint64>(antNames.size());
   for (std::vector<std::string>::const_iterator ci = antNames.begin(); ci != antNames.end(); ++ci) {
        os << *ci<< obj.antenna(*ci);
   }
   os.putEnd();
   return os;
}

/// deserialise TosMetadata
/// @param[in] is input stream
/// @param[out] obj object to deserialise
/// @return input stream
LOFAR::BlobIStream& LOFAR::operator>>(LOFAR::BlobIStream& is, askap::cp::TosMetadata& obj)
{
   using namespace askap;
   const int version = is.getStart("TosMetadata");
   ASKAPASSERT(version == 2);
   obj = TosMetadata();
   uint64 time;
   is >> time;
   obj.time(static_cast<casacore::uLong>(time));
   int scanId;
   is >> scanId;
   obj.scanId(scanId);
   bool flag;
   is >> flag;
   obj.flagged(flag);
   
   casacore::Quantity q;
   is >> q;
   obj.centreFreq(q);
   std::string bufStr;
   is >> bufStr;
   obj.targetName(bufStr);

   casacore::MDirection dir;
   is >> dir;
   obj.targetDirection(dir);
   is >> dir;
   obj.phaseDirection(dir);
   is >> bufStr;
   obj.corrMode(bufStr);

   casacore::Matrix<casacore::Double> offsetBuf;
   is >> offsetBuf;
   // technically this does an unnecessary copy, we could've benefited from reference semantics
   // of casa arrays and access data fields directly, but it breaks encapsulation - so will do it
   // only if it causes performance problems in the future
   obj.beamOffsets(offsetBuf);

   // now load antenna metadata
   uint64 nAntennas = 0;
   is >> nAntennas;
   TosMetadataAntenna mdataBuf("buffer");
   for (uint64 ant = 0; ant<nAntennas; ++ant) {
        is >> bufStr >> mdataBuf;
        ASKAPCHECK(bufStr == std::string(mdataBuf.name()), "Inconsistency in the serialised antenna metadata: name key = "<<
              bufStr<<" antenna name = "<<mdataBuf.name());
        obj.addAntenna(mdataBuf);
   }
 
   is.getEnd();
   
   return is;
}

/// @file ChannelManager.cc
///
/// @copyright (c) 2011 CSIRO
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
#include "ChannelManager.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <map>
#include <iterator>
#include <limits>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "Common/ParameterSet.h"
#include "casacore/casa/Arrays/Vector.h"
#include "askap/AskapUtil.h"
#include "casacore/casa/Quanta.h"


ASKAP_LOGGER(logger, ".ChannelManager");

using namespace std;
using namespace askap;
using namespace askap::cp::ingest;

ChannelManager::ChannelManager(const LOFAR::ParameterSet& params) :
    itsFreqOffset(asQuantity(params.getString("freq_offset","0.0Hz")).getValue("Hz"))
{
    const LOFAR::ParameterSet subset = params.makeSubset("n_channels.");

    for (LOFAR::ParameterSet::const_iterator ci = subset.begin(); ci != subset.end(); ++ci) {
         const std::string& key = ci->first;
         const size_t pos = key.find("..");
         if (pos == std::string::npos) {
             const int rank = utility::fromString<int>(key);
             ASKAPCHECK(itsChannelMap.find(rank) == itsChannelMap.end(), "Number of channels has already been defined for rank="<<rank);

             itsChannelMap[rank] = subset.getUint32(key);
             ASKAPLOG_DEBUG_STR(logger, "Channel Mappings - receiver " << rank
                               << " will handle " << itsChannelMap[rank] << " channels");
         } else {
             ASKAPCHECK(key.size() > pos + 2, "Unable to parse rank space "<<key<<" in channel manager");
             const int startRank = utility::fromString<int>(key.substr(0,pos));
             const int endRank = utility::fromString<int>(key.substr(pos+2));
             const unsigned int nchan = subset.getUint32(key);
             ASKAPLOG_DEBUG_STR(logger, "Channel Mappings - receivers from " << startRank<<" to "<<endRank<<", inclusive,"
                               << " will handle " << nchan << " channels");
             for (int rank = startRank; rank <= endRank; ++rank) {
                  ASKAPCHECK(itsChannelMap.find(rank) == itsChannelMap.end(), "Number of channels has already been defined for rank="<<rank);
                  itsChannelMap[rank] = subset.getUint32(key);
             }
         }
    }
    ASKAPLOG_INFO_STR(logger, "Frequency offset of "<<itsFreqOffset/1e6<<" MHz will be applied to the whole spectral axis");
}

unsigned int ChannelManager::localNChannels(const int rank) const
{
    map<int, unsigned int>::const_iterator it = itsChannelMap.find(rank);

    if (it == itsChannelMap.end()) {
        ASKAPTHROW(AskapError, "No channel mapping for this rank");
    }

    return it->second;

}

casa::Vector<casa::Double> ChannelManager::localFrequencies(const int rank,
        const casa::Double centreFreq,
        const casa::Double chanWidth,
        const casa::uInt totalNChan) const
{
    casa::Vector<casa::Double> frequencies(localNChannels(rank));;

    // 1: Find the first frequency (freq of lowest channel) for this rank
    casa::Double firstFreq = centreFreqToStartFreq(centreFreq, chanWidth, totalNChan) + itsFreqOffset;

    for (int i = 0; i < rank; ++i) {
        firstFreq += localNChannels(i) * chanWidth;
    }

    // 2: Populate the vector with the frequencies the process specified
    // by the "rank" parameter handles
    for (unsigned int i = 0; i < frequencies.size(); ++i) {
        frequencies(i) = firstFreq + (i * chanWidth);
    }

    return frequencies;
}

casa::Double ChannelManager::centreFreqToStartFreq(const casa::Double centreFreq,
                                                   const casa::Double chanWidth,
                                                   const casa::uInt totalNChan)
{
    const casa::Double total = static_cast<casa::Double>(totalNChan);
    return centreFreq - (chanWidth * (total / 2.0)) + (chanWidth / 2.0);
}

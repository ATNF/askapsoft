/// @file MetadataConverter.cc
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
#include "MetadataConverter.h"

// System includes
#include <vector>
#include <string>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "askap/AskapLogging.h"
#include "boost/shared_ptr.hpp"
#include "cpcommon/TosMetadata.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"

// CP Ice interfaces
#include "TypedValues.h"

// Local package includes
#include "TypedValueMapMapper.h"
#include "TypedValueMapConstMapper.h"

// Using
using namespace askap::cp::icewrapper;
using namespace askap::interfaces;
using namespace casa;

ASKAP_LOGGER(logger, ".MetadataConverter");

askap::cp::TosMetadata MetadataConverter::convert(const askap::interfaces::TimeTaggedTypedValueMap& source)
{
    // Use a mapper to easily get access to the elements and map them
    // to native (or casa) types
    TypedValueMapConstMapper srcMapper(source.data);

    // A copy of this object will be returned from this method
    TosMetadata dest;

    // time
    dest.time(source.timestamp);

    // scan_id
    dest.scanId(srcMapper.getInt("scan_id"));

    if (dest.scanId() < 0) {
        // Additional metadata is not guaranteed to be present when scanid < 0
        return dest;
    }

    // Global flag
    dest.flagged(srcMapper.getBool("flagged"));

    // Centre frequency
    const double centreFreqInMHz = srcMapper.getDouble("sky_frequency");
    dest.centreFreq(casa::Quantity(centreFreqInMHz, "MHz"));

    // Target name
    dest.targetName(srcMapper.getString("target_name"));

    // Target direction
    dest.targetDirection(srcMapper.getDirection("target_direction"));

    // Phase centre
    dest.phaseDirection(srcMapper.getDirection("phase_direction"));

    // Correlator mode
    dest.corrMode(srcMapper.getString("corrmode"));

    // beam offsets - treat it as an optional field
    if (srcMapper.has("beams_offsets")) {
        // we probably need to change the type as the offsets dealt with here are not the directions!
        const std::vector<casa::MDirection> offsetDirs = srcMapper.getDirectionSeq("beams_offsets");
        ASKAPLOG_DEBUG_STR(logger, "Received beams_offsets with "<<offsetDirs.size()<<" elements");
        if (offsetDirs.size() > 0) {
            casa::Matrix<casa::Double> beamOffsets(2, offsetDirs.size());
            for (casa::uInt beam = 0; beam < beamOffsets.ncolumn(); ++beam) {
                 beamOffsets(0, beam) = offsetDirs[beam].getValue().getLong();
                 beamOffsets(1, beam) = offsetDirs[beam].getValue().getLat();
                 if (beam < 1) {
                     ASKAPLOG_DEBUG_STR(logger, "Beam "<<beam<<": "<<offsetDirs[beam]<<" or ( "<<beamOffsets(0,beam)*180./casa::C::pi<<" "<<beamOffsets(1,beam)*180./casa::C::pi<<" degrees )");
                 }
            }
            dest.beamOffsets(beamOffsets);
        }
    }

    // antenna_names
    const std::vector<casa::String> antennaNames = srcMapper.getStringSeq("antennas");

    /////////////////////////
    // Metadata per antenna
    /////////////////////////
    ASKAPLOG_DEBUG_STR(logger, "About to convert metadata for "<<antennaNames.size()<<" antennas");
    for (size_t i = 0; i < antennaNames.size(); ++i) {
        convertAntenna(antennaNames[i], source, dest);
    }

    return dest;
}

askap::interfaces::TimeTaggedTypedValueMap MetadataConverter::convert(const askap::cp::TosMetadata& source)
{
    TimeTaggedTypedValueMap dest;
    dest.timestamp = source.time();

    // Use a mapper to easily convert native (or casa types) to TypedValues
    TypedValueMapMapper destMapper(dest.data);

    // scan_id
    destMapper.setInt("scan_id", source.scanId());

    // Global flag
    destMapper.setBool("flagged", source.flagged());

    // Centre frequency
    destMapper.setDouble("sky_frequency", source.centreFreq().getValue("MHz"));

    // Target name
    destMapper.setString("target_name", source.targetName());

    // Target direction
    destMapper.setDirection("target_direction", source.targetDirection());

    // Phase centre
    destMapper.setDirection("phase_direction", source.phaseDirection());

    // Correlator mode
    destMapper.setString("corrmode", source.corrMode());

    // Beam offsets - treat it as an optional field - only convert if matrix is not empty
    const casa::Matrix<casa::Double>& beamOffsets = source.beamOffsets();
    if (beamOffsets.ncolumn() > 0) {
        ASKAPASSERT(beamOffsets.nrow() == 2u);
        // we probably need a different type for beam offsets as conceptually they are not directions
        std::vector<casa::MDirection> beamOffsetsAsDir(beamOffsets.ncolumn());
        for (casa::uInt beam = 0; beam < beamOffsets.ncolumn(); ++beam) {
             const casa::MVDirection mvDir(casa::Quantity(beamOffsets(0, beam), "rad"), casa::Quantity(beamOffsets(1,beam), "rad"));
             // note, frame doesn't make sense here at all, use J200 as it is supported by the sequence converter
             beamOffsetsAsDir[beam] = casa::MDirection(mvDir, casa::MDirection::J2000);
        }
        destMapper.setDirectionSeq("beams_offsets", beamOffsetsAsDir);
    }

    // antenna_names
    const std::vector<std::string> stdnames =  source.antennaNames();
    std::vector<casa::String> antennaNames;
    for (size_t i = 0; i < stdnames.size(); ++i) {
        antennaNames.push_back(stdnames[i]);
    }
    destMapper.setStringSeq("antennas", antennaNames);

    /////////////////////////
    // Metadata per antenna
    /////////////////////////
    for (unsigned int i = 0; i < stdnames.size(); ++i) {
        convertAntenna(stdnames[i], source, dest);
    }

    return dest;
}

// Convert antenna portion of the Tos Metadata from
// askap::cp::TosMetadata to askap::interfaces::TimeTaggedTypedValueMap
void MetadataConverter::convertAntenna(const std::string& name,
        const askap::cp::TosMetadata& source, 
        askap::interfaces::TimeTaggedTypedValueMap& dest)
{
    TypedValueMapMapper destMapper(dest.data);

    // Obtain the instance of TosMetadataAntenna to convert
    const TosMetadataAntenna& antenna = source.antenna(name);
    const std::string antennaName = antenna.name();

    // <antenna name>.actual_radec
    destMapper.setDirection(makeMapKey(antennaName, "actual_radec"),
            antenna.actualRaDec());

    // <antenna name>.actual_azel
    destMapper.setDirection(makeMapKey(antennaName, "actual_azel"),
            antenna.actualAzEl());

    // <antenna name>.actual_pol
    destMapper.setDouble(makeMapKey(antennaName, "actual_pol"),
            antenna.actualPolAngle().getValue("deg"));

    // <antenna name>.on_source
    destMapper.setBool(makeMapKey(antennaName, "on_source"),
            antenna.onSource());

    // <antenna name>.flagged
    destMapper.setBool(makeMapKey(antennaName, "flagged"),
            antenna.flagged());
    
    std::vector<double> uvwBuf;
    antenna.uvw().tovector(uvwBuf);
    destMapper.setDoubleSeq(makeMapKey(antennaName, "uvw"), uvwBuf);
}

// Convert antenna portion of the Tos Metadata from
// askap::interfaces::TimeTaggedTypedValueMap to askap::cp::TosMetadata
void MetadataConverter::convertAntenna(const std::string& antennaName,
        const askap::interfaces::TimeTaggedTypedValueMap& source,
        askap::cp::TosMetadata& dest)
{
    // Use a mapper to easily get access to the elements and map them
    // to native (or casa) types
    TypedValueMapConstMapper srcMapper(source.data);

    TosMetadataAntenna ant(antennaName);

    // hw_error
    ant.flagged(srcMapper.getBool(makeMapKey(antennaName,
                    "flagged")));

    // at this stage assume that uvw values are always present, even for flagged antenna
    // although may not be valid. Therefore, uvw's are accessed outside if-statement.
    // This can easily be changed by moving the following code into the if-statement, but
    // the consequence would be of having junk uvw's in the MS for flagged antennas (which
    // I (MV) haven't seen for any other telescope).
    ant.uvw(srcMapper.getDoubleSeq(makeMapKey(antennaName, "uvw")));

    // If the antenna is flagged (other than for being !on_source then the other
    // metadata may not be present
    if (!ant.flagged()) {
        // actual_radec
        ant.actualRaDec(srcMapper.getDirection(makeMapKey(antennaName,
                        "actual_radec")));
        // actual_azel
        ant.actualAzEl(srcMapper.getDirection(makeMapKey(antennaName,
                        "actual_azel")));
        // actual_pol
        ant.actualPolAngle(Quantity(srcMapper.getDouble(makeMapKey(antennaName,
                        "actual_pol")), "deg"));
        // on_source
        ant.onSource(srcMapper.getBool(makeMapKey(antennaName,
                        "on_source")));
    }
    dest.addAntenna(ant);
}

std::string MetadataConverter::makeMapKey(const std::string& prefix,
        const std::string& suffix)
{
    std::ostringstream ss;
    ss << prefix << "." << suffix;
    return ss.str();
}

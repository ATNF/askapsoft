/// @file MSSink.cc
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
#include "MSSink.h"

// Include package level header file
#include "askap_cpingest.h"

// System includes
#include <string>
#include <sstream>
#include <limits>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include <askap/AskapUtil.h>
#include "cpcommon/VisChunk.h"

// Casecore includes
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Cube.h"
#include "casacore/casa/Arrays/MatrixMath.h"
#include "casacore/casa/OS/Time.h"
#include "casacore/casa/OS/Timer.h"
#include "casacore/tables/Tables/TableDesc.h"
#include "casacore/tables/Tables/SetupNewTab.h"
#include "casacore/tables/DataMan/IncrementalStMan.h"
#include "casacore/tables/DataMan/StandardStMan.h"
#include "casacore/tables/DataMan/TiledShapeStMan.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"
#include "casacore/tables/Tables/ScaColDesc.h"

// Local package includes
#include "configuration/Configuration.h" // Includes all configuration attributes too
#include "monitoring/MonitoringSingleton.h"

// name substitution should get the same name for all ranks, we need MPI for that
// it would be nicer to get all MPI-related stuff to a single (top-level?) file eventually
#include <mpi.h>


ASKAP_LOGGER(logger, ".MSSink");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;
using namespace casa;

//////////////////////////////////
// Public methods
//////////////////////////////////

MSSink::MSSink(const LOFAR::ParameterSet& parset,
        const Configuration& config) :
    itsParset(parset), itsConfig(config),
    itsPointingTableEnabled(parset.getBool("pointingtable.enable", false)),
    itsPreviousScanIndex(-1),
    itsFieldRow(-1), itsDataDescRow(-1), itsStreamNumber(0)
{
    if (itsConfig.nprocs() == 1) {
        ASKAPLOG_DEBUG_STR(logger, "Constructor - serial mode, initialising");
        itsFileName = substituteFileName(itsParset.getString("filename"));
        initialise();
    } else {
        ASKAPLOG_DEBUG_STR(logger, "Constructor - parallel mode, initialisation postponed until data arrive");
    }
}

MSSink::~MSSink()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
    itsMs.reset();
}

/// @brief should this task be executed for inactive ranks?
/// @details If a particular rank is inactive, process method is
/// not called unless this method returns true. This class has the
/// following behavior.
///   - Returns true initially to allow collective operations if
///     number of ranks is more than 1.
///   - After the first call to process method, inactive ranks are
///     identified and false is returned for them.
/// @return true, if process method should be called even if
/// this rank is inactive (i.e. uninitialised chunk pointer
/// will be passed to process method).
bool MSSink::isAlwaysActive() const
{
   // initially: itsStreamNumber is 0, itsMs is empty for all ranks -> make them always active
   // after first call to process:
   //          -  inactive ranks have itsStreamNumber of -1 and empty itsMs (don't want to create
   //             junk files).
   //          -  active ranks have non-empty itsMs and non-negative itsStreamNumber

   return !itsMs && (itsStreamNumber >= 0);
}


void MSSink::process(VisChunk::ShPtr& chunk)
{
    casa::Timer timer;
    timer.mark();
    if (!itsMs) {
        // this is delayed initialisation in the parallel mode
        ASKAPDEBUGASSERT(itsConfig.nprocs() > 1);
        // active ranks receive non-zero shared pointer, can cast it to bool
        itsStreamNumber = countActiveRanks(chunk);

        // collective MPI calls are still possible here (required for substitution)
        // this is the reason the file name is initialised outside initialise
        // We could also achieve the same by setting up a specialised MPI group here
        // instead of using MPI_WORLD.
        itsFileName = substituteFileName(itsParset.getString("filename"));
        ASKAPCHECK(itsFileName.size() > 0, "Substituted file name appears to be an empty string");

        if (itsStreamNumber < 0) {
            ASKAPLOG_DEBUG_STR(logger, "This rank is not active");
            return;
        }

        // no collective MPI calls are possible below this point
        ASKAPLOG_DEBUG_STR(logger, "Initialising MS, stream number "<<itsStreamNumber);
        initialise();
    }
    ASKAPASSERT(chunk);
    // Calculate monitoring points and submit them
    submitMonitoringPoints(chunk);

    // Handle the details for when a new scan starts
    if (itsPreviousScanIndex != static_cast<casa::Int>(chunk->scan())) {
        itsFieldRow = findOrAddField(chunk);
        itsDataDescRow = findOrAddDataDesc(chunk);
        itsPreviousScanIndex = chunk->scan();
    }

    MSColumns msc(*itsMs);
    const casa::uInt baseRow = msc.nrow();
    const casa::uInt newRows = chunk->nRow();
    itsMs->addRow(newRows);

    // First set the constant things outside the loop,
    // as they apply to all rows
    msc.scanNumber().put(baseRow, chunk->scan());
    msc.fieldId().put(baseRow, itsFieldRow);
    msc.dataDescId().put(baseRow, itsDataDescRow);

    const casa::Quantity chunkMidpoint = chunk->time().getTime();
    msc.time().put(baseRow, chunkMidpoint.getValue("s"));
    msc.time().put(baseRow, chunkMidpoint.getValue("s"));
    msc.timeCentroid().put(baseRow, chunkMidpoint.getValue("s"));

    msc.arrayId().put(baseRow, 0);
    msc.processorId().put(baseRow, 0);
    msc.exposure().put(baseRow, chunk->interval());
    msc.interval().put(baseRow, chunk->interval());
    msc.observationId().put(baseRow, 0);
    msc.stateId().put(baseRow, -1);

    for (casa::uInt i = 0; i < newRows; ++i) {
        const casa::uInt row = i + baseRow;
        msc.antenna1().put(row, chunk->antenna1()(i));
        msc.antenna2().put(row, chunk->antenna2()(i));
        msc.feed1().put(row, chunk->beam1()(i));
        msc.feed2().put(row, chunk->beam2()(i));
        msc.uvw().put(row, chunk->uvw()(i).vector());

        //msc.data().put(row, casa::transpose(chunk->visibility().yzPlane(i)));
        //msc.flag().put(row, casa::transpose(chunk->flag().yzPlane(i)));
        msc.flagRow().put(row, False);

        // TODO: Need to get this data from somewhere
        const Vector<Float> tmp(chunk->nPol(), 1.0);
        msc.weight().put(row, tmp);
        msc.sigma().put(row, tmp);
    }

    //
    // Update the observation table
    //
    // If this is the first integration cycle update the start time,
    // otherwise just update the end time.
    const casa::Double Tmid = chunkMidpoint.getValue("s");
    const casa::Double Tint = chunk->interval();

    MSObservationColumns& obsc = msc.observation();
    casa::Vector<casa::Double> timeRange = obsc.timeRange()(0);
    if (timeRange(0) == 0) {
        const casa::Double Tstart = Tmid - (Tint / 2);
        timeRange(0) = Tstart;
    }

    const casa::Double Tend = Tmid + (Tint / 2);
    timeRange(1) = Tend;
    obsc.timeRange().put(0, timeRange);

    //
    // Update the pointing table
    //
    addPointingRows(*chunk);

    itsMs->flush();
    // update monitoring point showing required time to write this chunk
    MonitoringSingleton::update<float>("MSWritingDuration", timer.real());
}

//////////////////////////////////
// Private methods
//////////////////////////////////

/// @brief make substitution in the file name
/// @details To simplify configuring the pipeline for different purposes certain
/// expressions are recognised and substituted by this methiod
/// %w is replaced by the rank, %d is replaced by the date (in YYYY-MM-DD format),
/// %t is replaced by the time (in HHMMSS format). Note, both date and time are
/// obtained on the rank zero and then broadcast to other ranks, unless in the standalone
/// mode.
/// @param[in] in input file name (may contain patterns to substitute)
/// @return file name with patterns substituted
std::string MSSink::substituteFileName(const std::string &in) const
{
   // special structure to have full control over it, otherwise could've used casa::Time
   struct TimeBuf {
     casa::uInt year;
     casa::uInt month;
     casa::uInt day;
     casa::uInt hour;
     casa::uInt min;
     casa::uInt sec;

     // technically we don't need a constructor, but it is neater to have it
     TimeBuf() : year(0), month(0), day(0), hour(0), min(0), sec(0) {}
   };
   // first just a sanity check
   if (itsStreamNumber > 0) {
       ASKAPCHECK((in.find("%w") != std::string::npos) || (in.find("%s") != std::string::npos), "File name should contain %w or %s in the MPI case to provide different names for different ranks");
   }

   // this instance is used only if a date or time substitution is required
   TimeBuf tbuf;
   if ((in.find("%d") != std::string::npos) || (in.find("%t") != std::string::npos)) {
       // need date/time later on
       if ( (itsConfig.nprocs() == 1) || (itsConfig.rank() == 0) ) {
            casa::Time tm;
            tm.now();
            tbuf.year = tm.year();
            tbuf.month = tm.month();
            tbuf.day = tm.dayOfMonth();
            tbuf.hour = tm.hours();
            tbuf.min = tm.minutes();
            tbuf.sec = tm.seconds();
       }
       if (itsConfig.nprocs() > 1) {
           // broadcast the value to all ranks
           // of course, it would be nicer to have all MPI-related stuff at the top level only, but keep it as it is for now
           const int response = MPI_Bcast((void*)&tbuf, sizeof(tbuf), MPI_INTEGER, 0, MPI_COMM_WORLD);
           ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Bcast = "<<response);
       }
   }
   std::string result;
   for (size_t cursor = 0; cursor < in.size(); ++cursor) {
       size_t pos = in.find("%",cursor);
       if (pos == std::string::npos) {
           result += in.substr(cursor);
           break;
       }
       result += in.substr(cursor, pos - cursor);
       if (++pos == in.size()) {
           result += in[pos-1];
           break;
       }
       if ((in[pos] == 's') && (itsStreamNumber >= 0)) {
           result += utility::toString(itsStreamNumber);
       } else if (in[pos] == 'w') {
          result += utility::toString(itsConfig.rank());
       } else if (in[pos] == 'd') {
          result += utility::toString(tbuf.year)+"-"+makeTwoElementString(tbuf.month)+"-"+makeTwoElementString(tbuf.day);
       } else if (in[pos] == 't') {
          result += makeTwoElementString(tbuf.hour)+makeTwoElementString(tbuf.min)+
                    makeTwoElementString(tbuf.sec);
       } else {
          // unrecognised symbol, pass it as is
          result += in.substr(pos-1,2);
       }
       cursor = pos;
   }
   return result;
}

/// @brief helper method to obtain stream sequence number
/// @details It does counting of active ranks across the whole rank space.
/// @param[in] isActive true if this rank is active, false otherwise
/// @return sequence number of the stream handled by this rank or -1 if it is
/// not active.
/// @note The method uses MPI collective calls and should be executed by all ranks,
/// including inactive ones.
int MSSink::countActiveRanks(const bool isActive) const
{
   ASKAPDEBUGASSERT(itsConfig.rank() < itsConfig.nprocs());
   std::vector<int> activityFlags(itsConfig.nprocs(), 0);
   if (isActive) {
       activityFlags[itsConfig.rank()] = 1;
   }
   const int response = MPI_Allreduce(MPI_IN_PLACE, (void*)activityFlags.data(),
        activityFlags.size(), MPI_INT, MPI_SUM, MPI_COMM_WORLD);
   ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Allreduce = "<<response);

   // integrate
   int totalNumber = 0, streamNumber = 0;
   for (size_t rank = 0; rank < activityFlags.size(); ++rank) {
        const int currentFlag = activityFlags[rank];
        // could be either 0 or 1
        ASKAPASSERT(currentFlag < 2);
        ASKAPASSERT(currentFlag >= 0);
        if (static_cast<int>(rank) < itsConfig.rank()) {
            streamNumber += currentFlag;
        } else {
            totalNumber += currentFlag;
        }
   }
   totalNumber += streamNumber;
   ASKAPCHECK(totalNumber > 0, "MSSink has no active ranks!");
   if (!isActive) {
       return -1;
   }
   // consistency checks
   ASKAPDEBUGASSERT(streamNumber < totalNumber);
   ASKAPDEBUGASSERT(totalNumber <= itsConfig.nprocs());
   if (totalNumber == itsConfig.nprocs()) {
       ASKAPASSERT(streamNumber == itsConfig.rank());
   }
   return streamNumber;
}

/// @brief make two-character string
/// @details Helper method to convert unsigned integer into a 2-character string.
/// It is used to form the file name with date and time.
/// @param[in] in input number
/// @return two-element string
std::string MSSink::makeTwoElementString(const casa::uInt in)
{
  ASKAPASSERT(in<100);
  std::string result;
  if (in<10) {
      result += "0";
  }
  result += utility::toString(in);
  return result;
}

void MSSink::create(void)
{
    // Get configuration first to ensure all parameters are present
    casa::uInt bucketSize = itsParset.getUint32("stman.bucketsize", 128 * 1024);
    casa::uInt tileNcorr = itsParset.getUint32("stman.tilencorr", 4);
    casa::uInt tileNchan = itsParset.getUint32("stman.tilenchan", 1);

    if (bucketSize < 8192) {
        bucketSize = 8192;
    }
    if (tileNcorr < 1) {
        tileNcorr = 1;
    }
    if (tileNchan < 1) {
        tileNchan = 1;
    }

    ASKAPLOG_DEBUG_STR(logger, "Creating dataset " << itsFileName);

    // Make MS with standard columns
    TableDesc msDesc(MS::requiredTableDesc());

    // Add the DATA column.
    MS::addColumnToDesc(msDesc, MS::DATA, 2);

    SetupNewTable newMS(itsFileName, msDesc, Table::New);

    // Set the default Storage Manager to be the Incr one
    {
        IncrementalStMan incrStMan("ismdata", bucketSize);
        newMS.bindAll(incrStMan, True);
    }

    // Bind ANTENNA1, and ANTENNA2 to the standardStMan
    // as they may change sufficiently frequently to make the
    // incremental storage manager inefficient for these columns.

    {
        StandardStMan ssm("ssmdata", bucketSize);
        newMS.bindColumn(MS::columnName(MS::ANTENNA1), ssm);
        newMS.bindColumn(MS::columnName(MS::ANTENNA2), ssm);
        newMS.bindColumn(MS::columnName(MS::UVW), ssm);
    }

    // These columns contain the bulk of the data so save them in a tiled way
    {
        // Get nr of rows in a tile.
        const int nrowTile = std::max(1u, bucketSize / (8*tileNcorr*tileNchan));
        ASKAPLOG_INFO_STR(logger, "Number of rows in the tile = "<<nrowTile);
        TiledShapeStMan dataMan("TiledData",
                IPosition(3, tileNcorr, tileNchan, nrowTile));
        newMS.bindColumn(MeasurementSet::columnName(MeasurementSet::DATA),
                dataMan);
        newMS.bindColumn(MeasurementSet::columnName(MeasurementSet::FLAG),
                dataMan);
    }
    {
        const int nrowTile = std::max(1u, bucketSize / (4*8));
        TiledShapeStMan dataMan("TiledWeight",
                IPosition(2, 4, nrowTile));
        newMS.bindColumn(MeasurementSet::columnName(MeasurementSet::SIGMA),
                dataMan);
        newMS.bindColumn(MeasurementSet::columnName(MeasurementSet::WEIGHT),
                dataMan);
    }

    // Now we can create the MeasurementSet and add the (empty) subtables
    itsMs.reset(new MeasurementSet(newMS, 0));
    itsMs->createDefaultSubtables(Table::New);
    itsMs->flush();

    // Add non-standard columns to the pointing table if the pointing table is written
    if (itsPointingTableEnabled) {
        addNonStandardPointingColumn("AZIMUTH", "Actual azimuth angle (in degrees)");
        addNonStandardPointingColumn("ELEVATION", "Actual azimuth angle (in degrees)");
        addNonStandardPointingColumn("POLANGLE", "Actual polarisation angle (in degrees) of the third-axis");
    }

    // Set the TableInfo
    {
        TableInfo& info(itsMs->tableInfo());
        info.setType(TableInfo::type(TableInfo::MEASUREMENTSET));
        info.setSubType(String(""));
        info.readmeAddLine("This is a MeasurementSet Table holding simulated astronomical observations");
    }

    // Set Epoch Reference to UTC
    MSColumns msc(*itsMs);
    msc.setEpochRef(casa::MEpoch::UTC);
}

/// @brief add non-standard column to POINTING table
/// @details We use 3 non-standard columns to capture
/// actual pointing on all three axes. This method creates one such
/// column.
/// @param[in] name column name
/// @param[in] description text description
void MSSink::addNonStandardPointingColumn(const std::string &name,
                  const std::string &description)
{
   ASKAPASSERT(itsMs);
   MSPointing& pointing = itsMs->pointing();
   casa::ScalarColumnDesc<casa::Float> colDesc(name, description);
   colDesc.rwKeywordSet().define("unit","deg");
   pointing.addColumn(colDesc);
}

void MSSink::initAntennas(void)
{
    const std::vector<Antenna> antennas = itsConfig.antennas();
    std::vector<Antenna>::const_iterator it;
    for (it = antennas.begin(); it != antennas.end(); ++it) {
        casa::Int id = addAntenna(itsConfig.arrayName(),
                it->position(),
                it->name(),
                it->mount(),
                it->diameter().getValue("m"));

        // For each antenna one or more feed entries must be created
        const FeedConfig& feed = itsConfig.feed();
        initFeeds(feed, id);
    }
}

void MSSink::initFeeds(const FeedConfig& feeds, const casa::Int antennaID)
{
    const uInt nFeeds = feeds.nFeeds();

    casa::Vector<double> x(nFeeds);
    casa::Vector<double> y(nFeeds);
    casa::Vector<casa::String> pol(nFeeds);

    for (size_t i = 0; i < nFeeds; ++i) {
        x(i) = feeds.offsetX(i).getValue("rad");
        y(i) = feeds.offsetY(i).getValue("rad");
        pol(i) = "X Y";
    }

    addFeeds(antennaID, x, y, pol);
}

void MSSink::initObs(void)
{
    addObs("ASKAP", "", 0, 0);
}

casa::Int MSSink::addObs(const casa::String& telescope,
        const casa::String& observer,
        const double obsStartTime,
        const double obsEndTime)
{
    MSColumns msc(*itsMs);
    MSObservation& obs = itsMs->observation();
    MSObservationColumns& obsc = msc.observation();
    const uInt row = obsc.nrow();
    obs.addRow();
    obsc.telescopeName().put(row, telescope);
    Vector<double> timeRange(2);
    timeRange(0) = obsStartTime;
    timeRange(1) = obsEndTime;
    obsc.timeRange().put(row, timeRange);
    obsc.observer().put(row, observer);

    // Post-conditions
    ASKAPCHECK(obsc.nrow() == (row + 1), "Unexpected observation row count");

    return row;
}

void MSSink::addPointingRows(const VisChunk& chunk)
{
    if (!itsPointingTableEnabled) return;

    MSColumns msc(*itsMs);
    MSPointingColumns& pointingc = msc.pointing();
    casa::ScalarColumn<casa::Float> polAngleCol(itsMs->pointing(), "POLANGLE");
    casa::ScalarColumn<casa::Float> azimuthCol(itsMs->pointing(), "AZIMUTH");
    casa::ScalarColumn<casa::Float> elevationCol(itsMs->pointing(), "ELEVATION");

    uInt row = pointingc.nrow();

    // Initialise if this is the first cycle. All standard direction-type columns are
    // always in J2000.
    if (row == 0) {
        pointingc.setDirectionRef(casa::MDirection::J2000);
       // pointingc.setDirectionRef(casa::MDirection::castType(
         //           chunk.actualPointingCentre()(0).getRef().getType()));
    }

    const casa::uInt nAntenna = chunk.nAntenna();
    itsMs->pointing().addRow(nAntenna);

    const double t = chunk.time().getTime().getValue("s");
    const double interval = chunk.interval();

    for (casa::uInt i = 0; i < nAntenna; ++i) {
        pointingc.antennaId().put(row, i);
        pointingc.time().put(row, t);
        pointingc.interval().put(row, interval);

        pointingc.name().put(row, "");
        pointingc.numPoly().put(row, 0);
        pointingc.timeOrigin().put(row, 0);

        const Vector<MDirection> actual(1, chunk.actualPointingCentre()(i));
        pointingc.directionMeasCol().put(row, actual);

        const Vector<MDirection> target(1, chunk.targetPointingCentre()(i).getValue());
        pointingc.targetMeasCol().put(row, target);

        pointingc.tracking().put(row, chunk.onSourceFlag()(i));

        // Non-standard-columns
        polAngleCol.put(row,
                static_cast<float>(chunk.actualPolAngle()(i).getValue("deg")));
        azimuthCol.put(row,
                static_cast<float>(chunk.actualAzimuth()(i).getValue("deg")));
        elevationCol.put(row,
                static_cast<float>(chunk.actualElevation()(i).getValue("deg")));

        ++row;
    }
}

casa::Int MSSink::addField(const casa::String& fieldName,
        const casa::MDirection& fieldDirection,
        const casa::String& calCode)
{
    MSColumns msc(*itsMs);
    MSFieldColumns& fieldc = msc.field();
    const uInt row = fieldc.nrow();

    ASKAPLOG_INFO_STR(logger, "Creating new field " << fieldName << ", ID "
            << row);

    itsMs->field().addRow();
    fieldc.name().put(row, fieldName);
    fieldc.code().put(row, calCode);
    fieldc.time().put(row, 0.0);
    fieldc.numPoly().put(row, 0);
    fieldc.sourceId().put(row, 0);
    Vector<MDirection> direction(1);
    direction(0) = fieldDirection;
    fieldc.delayDirMeasCol().put(row, direction);
    fieldc.phaseDirMeasCol().put(row, direction);
    fieldc.referenceDirMeasCol().put(row, direction);

    // Post-conditions
    ASKAPCHECK(fieldc.nrow() == (row + 1), "Unexpected field row count");

    return row;
}

void MSSink::addFeeds(const casa::Int antennaID,
        const casa::Vector<double>& x,
        const casa::Vector<double>& y,
        const casa::Vector<casa::String>& polType)
{
    // Pre-conditions
    const uInt nFeeds = x.size();
    ASKAPCHECK(nFeeds == y.size(), "X and Y vectors must be of equal length");
    ASKAPCHECK(nFeeds == polType.size(),
            "Pol type vector must have the same length as X and Y");

    // Add to the Feed table
    MSColumns msc(*itsMs);
    MSFeedColumns& feedc = msc.feed();
    const uInt startRow = feedc.nrow();
    itsMs->feed().addRow(nFeeds);

    for (casa::uInt i = 0; i < nFeeds; ++i) {
        casa::uInt row = startRow + i;
        feedc.antennaId().put(row, antennaID);
        feedc.feedId().put(row, i);
        feedc.spectralWindowId().put(row, -1);
        feedc.beamId().put(row, 0);
        feedc.numReceptors().put(row, 2);

        // Feed position
        Vector<double> feedXYZ(3, 0.0);
        feedc.position().put(row, feedXYZ);

        // Beam offset
        Matrix<double> beamOffset(2, 2);
        beamOffset(0, 0) = x(i);
        beamOffset(1, 0) = y(i);
        beamOffset(0, 1) = x(i);
        beamOffset(1, 1) = y(i);
        feedc.beamOffset().put(row, beamOffset);

        // Polarisation type
        Vector<String> feedPol(2);
        if (polType(i).contains("X", 0)) {
            feedPol(0) = "X";
            feedPol(1) = "Y";
        } else {
            feedPol(0) = "L";
            feedPol(1) = "R";
        }
        feedc.polarizationType().put(row, feedPol);

        // Polarisation response
        Matrix<Complex> polResp(2, 2);
        polResp = Complex(0.0, 0.0);
        polResp(1, 1) = Complex(1.0, 0.0);
        polResp(0, 0) = Complex(1.0, 0.0);
        feedc.polResponse().put(row, polResp);

        // Receptor angle
        Vector<double> feedAngle(2, 0.0);
        feedc.receptorAngle().put(row, feedAngle);

        // Time
        feedc.time().put(row, 0.0);

        // Interval - 1.e30 is effectivly forever
        feedc.interval().put(row, 1.e30);
    };

    // Post-conditions
    ASKAPCHECK(feedc.nrow() == (startRow + nFeeds), "Unexpected feed row count");
}

casa::Int MSSink::addAntenna(const casa::String& station,
        const casa::Vector<double>& antXYZ,
        const casa::String& name,
        const casa::String& mount,
        const casa::Double& dishDiameter)
{
    // Pre-conditions
    ASKAPCHECK(antXYZ.size() == 3, "Antenna position vector must contain 3 elements");

    // Write the rows to the measurement set
    MSColumns msc(*itsMs);
    MSAntennaColumns& antc = msc.antenna();
    const uInt row = antc.nrow();

    MSAntenna& ant = itsMs->antenna();
    ant.addRow();

    antc.name().put(row, name);
    antc.station().put(row,station);
    antc.type().put(row, "GROUND-BASED");
    antc.mount().put(row, mount);
    antc.position().put(row, antXYZ);
    antc.dishDiameter().put(row, dishDiameter);
    antc.flagRow().put(row, false);

    // Post-conditions
    ASKAPCHECK(antc.nrow() == (row + 1), "Unexpected antenna row count");

    return row;
}

casa::Int MSSink::addDataDesc(const casa::Int spwId, const casa::Int polId)
{
    // 1: Add new row and determine its offset
    MSColumns msc(*itsMs);
    MSDataDescColumns& ddc = msc.dataDescription();
    const uInt row = ddc.nrow();
    itsMs->dataDescription().addRow();

    // 2: Populate DATA DESCRIPTION table
    ddc.flagRow().put(row, False);
    ddc.spectralWindowId().put(row, spwId);
    ddc.polarizationId().put(row, polId);

    return row;
}

/// @note The implementation of method isSpectralWindowRowEqual() is tightly
/// coupled to the implementation of this method. If this method is changed
/// it is likely isSpectralWindowRowEqual() should be too.
casa::Int MSSink::addSpectralWindow(const casa::String& spwName,
            const int nChan,
            const casa::Quantity& startFreq,
            const casa::Quantity& freqInc)
{
    MSColumns msc(*itsMs);
    MSSpWindowColumns& spwc = msc.spectralWindow();
    const uInt row = spwc.nrow();
    ASKAPLOG_INFO_STR(logger, "Creating new spectral window " << spwName
            << ", ID " << row);

    itsMs->spectralWindow().addRow();

    spwc.numChan().put(row, nChan);
    spwc.name().put(row, spwName);
    spwc.netSideband().put(row, 1);
    spwc.ifConvChain().put(row, 0);
    spwc.freqGroup().put(row, 0);
    spwc.freqGroupName().put(row, "Group 1");
    spwc.flagRow().put(row, False);
    spwc.measFreqRef().put(row, MFrequency::TOPO);

    Vector<double> freqs(nChan);
    Vector<double> bandwidth(nChan, freqInc.getValue("Hz"));

    double vStartFreq(startFreq.getValue("Hz"));
    double vFreqInc(freqInc.getValue("Hz"));

    for (Int chan = 0; chan < nChan; chan++) {
        freqs(chan) = vStartFreq + chan * vFreqInc;
    }

    spwc.refFrequency().put(row, vStartFreq);
    spwc.chanFreq().put(row, freqs);
    spwc.chanWidth().put(row, bandwidth);
    spwc.effectiveBW().put(row, bandwidth);
    spwc.resolution().put(row, bandwidth);
    spwc.totalBandwidth().put(row, nChan * vFreqInc);

    return row;
}

/// @note The implementation of method isPolarisationRowEqual() is tightly
/// coupled to the implementation of this method. If this method is changed
/// it is likely isPolarisationRowEqual() should be too.
casa::Int MSSink::addPolarisation(const casa::Vector<casa::Stokes::StokesTypes>& stokesTypes)
{
    const Int nCorr = stokesTypes.size();

    MSColumns msc(*itsMs);
    MSPolarizationColumns& polc = msc.polarization();
    const uInt row = polc.nrow();
    itsMs->polarization().addRow();

    polc.flagRow().put(row, False);
    polc.numCorr().put(row, nCorr);

    // Translate stokesTypes into receptor products, catch invalid
    // fallibles.
    Matrix<Int> corrProduct(uInt(2), uInt(nCorr));
    Fallible<Int> fi;

    casa::Vector<casa::Int> stokesTypesInt(nCorr);
    for (Int i = 0; i < nCorr; i++) {
        fi = Stokes::receptor1(stokesTypes(i));
        corrProduct(0, i) = (fi.isValid() ? fi.value() : 0);
        fi = Stokes::receptor2(stokesTypes(i));
        corrProduct(1, i) = (fi.isValid() ? fi.value() : 0);
        stokesTypesInt(i) = stokesTypes(i);
    }

    polc.corrType().put(row, stokesTypesInt);
    polc.corrProduct().put(row, corrProduct);

    return row;
}

casa::Int MSSink::findOrAddField(const askap::cp::common::VisChunk::ShPtr chunk)
{
    const casa::String fieldName = chunk->targetName();
    const casa::MDirection fieldDirection = chunk->phaseCentre()[0];
    const casa::String& calCode = "";

    MSColumns msc(*itsMs);
    ROMSFieldColumns& fieldc = msc.field();
    const uInt nRows = fieldc.nrow();

    for (uInt i = 0; i < nRows; ++i) {
        const Vector<MDirection> dirVec = fieldc.referenceDirMeasCol()(i);
        if ((fieldName.compare(fieldc.name()(i)) == 0)
                && (calCode.compare(fieldc.code()(i)) == 0)
                && equal(dirVec[0],  fieldDirection)) {
            return i;
        }
    }

    return addField(fieldName, fieldDirection, calCode);
}

casa::Int MSSink::findOrAddDataDesc(askap::cp::common::VisChunk::ShPtr chunk)
{
   casa::Int spwId;
   casa::Int polId;

   // 1: Try to find a data description that matches the scan
   MSColumns msc(*itsMs);
   ROMSDataDescColumns& ddc = msc.dataDescription();
   uInt nRows = ddc.nrow();
   for (uInt row = 0; row < nRows; ++row) {
       spwId = ddc.spectralWindowId()(row);
       polId = ddc.polarizationId()(row);
       if (isSpectralWindowRowEqual(chunk, spwId) &&
               isPolarisationRowEqual(chunk, polId)) {
           return row;
       }
   }

   // The value -1 indicates an entry that matches the scan has not
   // been found.
   spwId = -1;
   polId = -1;

   // 2: Try to find a spectral window row that matches
   nRows = msc.spectralWindow().nrow();
   for (uInt row = 0; row < nRows; ++row) {
       if (isSpectralWindowRowEqual(chunk, row)) {
           spwId = row;
           break;
       }
   }


   // 3: Try to find a polarisation row that matches
   nRows = msc.polarization().nrow();
   for (uInt row = 0; row < nRows; ++row) {
       if (isPolarisationRowEqual(chunk, row)) {
           polId = row;
           break;
       }
   }

   // 4: Create the missing entry and a data desc
   if (spwId == -1) {
       const casa::String spWindowName("NO_NAME"); // TODO: Add name
       spwId = addSpectralWindow(spWindowName,
               chunk->nChannel(),
               casa::Quantity(chunk->frequency()(0), "Hz"),
               casa::Quantity(chunk->channelWidth(), "Hz"));
   }
   if (polId == -1) {
       polId = addPolarisation(chunk->stokes());
   }

   return addDataDesc(spwId, polId);
}

// Compares the given row in the spectral window table with the spectral window
// setup as defined in the Scan.
//
// @note This is not an apples to apples comparison, and depends somewhat on
// how the infomration in the "Scan" object was translated to a spectral
// window setup. For this reason, the implementation of this method is
// tightly coupled to the addSpectralWindow() method in this class. If that
// method is modified, so should this.
//
// @return true if the two are effectivly equal, otherwise false.
bool MSSink::isSpectralWindowRowEqual(askap::cp::common::VisChunk::ShPtr chunk,
        const casa::uInt row) const
{
    MSColumns msc(*itsMs);
    ROMSSpWindowColumns& spwc = msc.spectralWindow();
    ASKAPCHECK(row < spwc.nrow(), "Row index out of bounds");

    if (spwc.numChan()(row) != static_cast<casa::Int>(chunk->nChannel())) {
        return false;
    }
    if (spwc.flagRow()(row) != false) {
        return false;
    }
    const casa::Vector<double> freqs = spwc.chanFreq()(row);
    const double dblEpsilon = std::numeric_limits<double>::epsilon();
    if (fabs(freqs(0) - chunk->frequency()(0)) > dblEpsilon) {
        return false;
    }
    const casa::Vector<double> bandwidth = spwc.chanWidth()(row);
    if (fabs(bandwidth(0) - chunk->channelWidth()) > dblEpsilon) {
        return false;
    }

    return true;
}

// Compares the given row in the polarisation table with the polarisation
// setup as defined in the Scan.
//
// @note This is not an apples to apples comparison, and depends somewhat on
// how the infomration in the "Scan" object was translated to a spectral
// window setup. For this reason, the implementation of this method is
// tightly coupled to the addPolarisation() method in this class. If that
// method is modified, so should this.
//
// @return true if the two are effectivly equal, otherwise false.
bool MSSink::isPolarisationRowEqual(askap::cp::common::VisChunk::ShPtr chunk,
        const casa::uInt row) const
{
    MSColumns msc(*itsMs);
    ROMSPolarizationColumns& polc = msc.polarization();
    ASKAPCHECK(row < polc.nrow(), "Row index out of bounds");

    if (polc.numCorr()(row) != static_cast<casa::Int>(chunk->stokes().size())) {
        return false;
    }
    if (polc.flagRow()(row) != false) {
        return false;
    }
    casa::Vector<casa::Int> stokesTypesInt = polc.corrType()(row);
    for (casa::uInt i = 0; i < stokesTypesInt.size(); ++i) {
        if (stokesTypesInt(i) != chunk->stokes()(i)) {
            return false;
        }
    }

    return true;
}

void MSSink::submitMonitoringPoints(askap::cp::common::VisChunk::ShPtr chunk)
{
    ASKAPDEBUGASSERT(chunk);
    // Calculate flagged visibility counts
    int32_t flagCount = 0;
    const casa::Cube<casa::Bool>& flags = chunk->flag();
    for (Array<casa::Bool>::const_contiter it = flags.cbegin();
            it != flags.cend(); ++it) {
        if (*it) ++flagCount;
    }

    ASKAPLOG_DEBUG_STR(logger, "  " << flagCount << " of " << flags.size()
            << " visibilities flagged");

    // Submit monitoring data
    MonitoringSingleton::update("VisFlagCount", flagCount);
    if (flags.size()) {
        MonitoringSingleton::update("VisFlagPercent",
                flagCount / static_cast<float>(flags.size()) * 100.0);
    } else {
        MonitoringSingleton::invalidatePoint("VisFlagPercent");
    }

    // note, this has been moved from source task. We can have separate monitoring
    // points here and there with different names
    if (chunk->interval() > 0) {
        const float nVis = chunk->nChannel() * chunk->nRow() * chunk->nPol();
        // data estimated as 8byte vis, 4byte sigma + nRow * 100 bytes (mdata)
        const float nDataInMB = (12. * nVis + 100. * chunk->nRow()) / 1048576.;
        MonitoringSingleton::update("obs.DataRate", nDataInMB / chunk->interval());
    } else {
       MonitoringSingleton::invalidatePoint("obs.DataRate");
    }

    MonitoringSingleton::update<float>("obs.StartFreq", chunk->frequency()[0]/ 1000 / 1000);
    MonitoringSingleton::update<int32_t>("obs.nChan", chunk->nChannel());
    MonitoringSingleton::update<float>("obs.ChanWidth", chunk->channelWidth() / 1000);
}

bool MSSink::equal(const casa::MDirection &dir1, const casa::MDirection &dir2)
{
    if (dir1.getRef().getType() != dir2.getRef().getType()) {
        return false;
    }
    return dir1.getValue().separation(dir2.getValue()) < std::numeric_limits<double>::epsilon();
}


/// @brief initialise the measurement set
/// @details In the serial mode we run initialisation in the constructor.
/// However, in the parallel mode it is handy to initialise the measurement set
/// upon the first call to process method (as this is the only way to automatically
/// deduce which ranks are active and which are not; the alternative design would be
/// to specify this information in parset, but this seems to be unnecessary).
/// This method encapsulates all required initialisation actions.
void MSSink::initialise()
{
    create();
    initAntennas(); // Includes FEED table
    initObs();
}

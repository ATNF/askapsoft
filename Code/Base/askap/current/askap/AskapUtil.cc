/// @file AskapUtil.cc
/// @brief Common ASKAP utility functions and classes
///
/// @copyright (c) 2007 CSIRO
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

// Include own header file first
#include "askap/AskapUtil.h"

// System includes
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <unistd.h>

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Quanta/MVDirection.h"
#include "casacore/casa/Quanta/MVAngle.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/casa/Quanta/MVTime.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/MPosition.h"
#include "casacore/measures/Measures/MEpoch.h"
#include "casacore/measures/Measures/MCEpoch.h"
#include "casacore/measures/Measures/MeasConvert.h"
#include "casacore/measures/Measures/MeasIERS.h"
#include "casacore/tables/Tables/Table.h"
#include "casacore/tables/Tables/TableRecord.h"

namespace askap {

std::string printDirection(const casa::MVDirection &dir)
{
    std::ostringstream os;
    os << std::setprecision(8) << casa::MVAngle::Format(casa::MVAngle::TIME)
        << casa::MVAngle(dir.getLong("deg"))
        << " " << std::setprecision(8) << casa::MVAngle::Format(casa::MVAngle::ANGLE) <<
    casa::MVAngle(dir.getLat("deg"));
    return os.str();
}

std::string printLon(const casa::MDirection& dir)
{
    std::ostringstream os;
    os << std::setprecision(8) << casa::MVAngle::Format(casa::MVAngle::TIME)
        << casa::MVAngle(dir.getValue().getLong("deg"));
    return os.str();
}

std::string printLat(const casa::MDirection& dir)
{
    std::ostringstream os;
    os << std::setprecision(8) << casa::MVAngle::Format(casa::MVAngle::ANGLE)
        << casa::MVAngle(dir.getValue().getLat("deg"));
    return os.str();
}

std::string getHostName(bool full)
{
    char hname[256];

    if (gethostname(hname, 256) != 0) {
        return std::string("localhost");
    }

    std::string hostname(hname);

    if (!full) {
        std::string::size_type dotloc = hostname.find_first_of(".");

        if (dotloc != hostname.npos) {
            return hostname.substr(0, dotloc);
        }
    }

    return hostname;
}

std::string toUpper(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), toupper);
    return str;
}

std::string toLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), tolower);
    return str;
}

casa::Quantity asQuantity(const string& s, const std::string& unit)
{
    casa::Quantity q;
    casa::Quantity::read(q, s);

    if ((!unit.empty()) && (!q.isConform(casa::Unit(unit)))) {
        ASKAPTHROW(AskapError, "Quantity: " << s << " does not conform to unit " << unit);
    }

    return q;
}

casa::MEpoch asMEpoch(const vector<string>& epoch)
{
    ASKAPCHECK(epoch.size() == 2, "Not a valid epoch");

    casa::Quantity datetime;
    casa::Quantity::read(datetime, epoch[0]);
    casa::MEpoch::Types type;
    casa::MEpoch::getType(type, epoch[1]);
    casa::MEpoch ep(datetime, type);
    return ep;
}

/// @brief Converts a colon separated latitude coordinate,
/// a format not supported by casacore, to a format that is
/// supported.
/// For example:
/// 45:00:0.00 -> 45h00m0.00
static std::string convertLatitude(const std::string& s)
{
    std::string out(s);
    size_t i = out.find_first_of(":");
    if (i != string::npos) {
        out[i] = 'd';
        i = out.find_first_of(":");
        if (i != string::npos) {
            out[i] = 'm';
        }
    }
    return out;
}

casa::MDirection asMDirection(const vector<string>& direction)
{
    ASKAPCHECK(direction.size() == 3, "Not a valid direction");

    casa::Quantity lng;
    casa::Quantity::read(lng, direction[0]);

    casa::Quantity lat;
    casa::Quantity::read(lat, convertLatitude(direction[1]));

    casa::MDirection::Types type;
    casa::MDirection::getType(type, direction[2]);
    casa::MDirection dir(lng, lat, type);
    return dir;
}

casa::MPosition asMPosition(const vector<string>& position)
{
    ASKAPCHECK(position.size() == 4, "Not a valid position");

    casa::Quantity lng;
    casa::Quantity::read(lng, position[0]);
    casa::Quantity lat;
    casa::Quantity::read(lat, position[1]);
    casa::Quantity height;
    casa::Quantity::read(height, position[2]);
    casa::MPosition::Types type;
    casa::MPosition::getType(type, position[3]);
    casa::MVPosition mvPos(height, lng, lat);
    casa::MPosition pos(mvPos, type);
    return pos;
}



/// @brief convert BAT to UTC Epoch via casa
/// @param[in] bat BAT as 64-bit integer
/// @return casa epoch measure in the UTC frame
casa::MEpoch bat2epoch(const uint64_t &bat)
{
  const casa::MVEpoch timeTAI(static_cast<casa::Double>(bat / microsecondsPerDay),
                              static_cast<casa::Double>(bat % microsecondsPerDay) /
                              static_cast<casa::Double>(microsecondsPerDay));
  const casa::MEpoch epoch = casa::MEpoch::Convert(casa::MEpoch(timeTAI, casa::MEpoch::Ref(casa::MEpoch::TAI)),
                             casa::MEpoch::Ref(casa::MEpoch::UTC))();
  return epoch;
}

/// @brief convert casa Epoch to BAT
/// @param[in] epoch casa epoch measure, typically in UTC frame, but can be anything supported by casa
/// @return BAT as 64-bit integer
uint64_t epoch2bat(const casa::MEpoch &epoch)
{
   const casa::MVEpoch epochTAI= casa::MEpoch::Convert(epoch,
                  casa::MEpoch::Ref(casa::MEpoch::TAI))().getValue();
   const uint64_t startOfDayBAT = static_cast<uint64_t>(epochTAI.getDay()*microsecondsPerDay);
   return startOfDayBAT + static_cast<uint64_t>(epochTAI.getDayFraction()*microsecondsPerDay);
}

/// @brief helper method to check the TAI_UTC measures table version
/// @details casacore measures data need to be updated regularly. The TAI_UTC
/// table seems to be the one most frequently updated. However, its version and
/// date, although checked by internal measures routines, are not accessible
/// directly using casacore methods. This method does this to allow these details
/// to be monitored.
/// @note No caching has been done, but this information is expected to be
/// accessed very infrequently (i.e. once per scheduling block). The code
/// could, in principle, be pushed into casacore. An exception is thrown if
/// the code is unable to access the appropriate measures database table
/// @return a pair with table date MJD (first) and the version string (second)
std::pair<double, std::string> measuresTableVersion()
{
  casa::Table tab;
  const bool ok = casa::MeasIERS::findTab(tab, 0, "measures.tai_utc.directory", "geodetic", "TAI_UTC");
  ASKAPCHECK(ok, "Unable to open TAI_UTC measures table via casa::MeasIERS");

  const casa::TableRecord kw(tab.keywordSet());

  ASKAPCHECK(kw.isDefined("VS_DATE") && kw.isDefined("VS_VERSION"),
      "The measures table is incomplete, no date or version stored. type="<<tab.tableInfo().type());
  casa::Quantity qDate;
  ASKAPCHECK(casa::MVTime::read(qDate, kw.asString("VS_DATE")), "Unable to parse VS_DATE: "<<kw.asString("VS_DATE"));

  return std::pair<double, std::string>(casa::MVTime(qDate), kw.asString("VS_VERSION"));
}

/// @brief helper method to check the validity of measures data
/// @details casacore measures data need to be updated regulargly. Although
/// different data are updated at different cadence, looking for dUT1 seems to be
/// the fastest way to catch the issue. This method attemts to get dUT1 through
/// low-level get method of the casacore, same as for dUT1 method of MeasTable
/// class, but check the validity flag and ignores the result. The casacore's
/// dUT1 method only uses the validity flag to give the warning and doesn't allow
/// the user of the library to access it. The code could, in principle, be pushed
/// into casacore.
/// @note No caching has been done, but this method is expected to be accessed
/// very infrequently (i.e. once per scheduling block).
/// @param[in] mjd Modified Julian Date to check
bool measuresValid(double mjd)
{
   double res;
   const bool ok = casa::MeasIERS::get(res, casa::MeasIERS::MEASURED, casa::MeasIERS::dUT1, mjd);
   return ok;
}



} // end namespace askap

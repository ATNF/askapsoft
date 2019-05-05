/// @file
/// @brief log4cxx LogSink for Casa log messages

/// @copyright (c) 2008 CSIRO
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
/// @author Ger van Diepen (gvd AT astron DOT nl)

#include <askap/Log4cxxLogSink.h>

#include <askap/AskapLogging.h>
ASKAP_LOGGER(logger, ".CASA");


namespace askap {

  Log4cxxLogSink::Log4cxxLogSink()
    : casacore::LogSinkInterface (casacore::LogFilter())
  {}

  Log4cxxLogSink::Log4cxxLogSink (casacore::LogMessage::Priority filter)
    : casacore::LogSinkInterface (casacore::LogFilter(filter))
  {}

  Log4cxxLogSink::Log4cxxLogSink (const casacore::LogFilterInterface& filter)
    : casacore::LogSinkInterface (filter)
  {}

  Log4cxxLogSink::~Log4cxxLogSink()
  {}

  casacore::Bool Log4cxxLogSink::postLocally (const casacore::LogMessage& message)
  {
    casacore::Bool posted = casacore::False;
    if (filter().pass(message)) {
      std::string msg (message.origin().location() + ": " + message.message());
      posted = casacore::True;
      switch (message.priority()) {
      case casacore::LogMessage::DEBUGGING:
      case casacore::LogMessage::DEBUG2:
      case casacore::LogMessage::DEBUG1:
	{
	  ASKAPLOG_DEBUG (logger, msg);
	  break;
	}
      case casacore::LogMessage::NORMAL5:
      case casacore::LogMessage::NORMAL4:
      case casacore::LogMessage::NORMAL3:
      case casacore::LogMessage::NORMAL2:
      case casacore::LogMessage::NORMAL1:
      case casacore::LogMessage::NORMAL:
	{
	  ASKAPLOG_INFO (logger, msg);
	  break;
	}
      case casacore::LogMessage::WARN:
	{
	  ASKAPLOG_WARN (logger, msg);
	  break;
	}
      case casacore::LogMessage::SEVERE:
	{
	  ASKAPLOG_ERROR (logger, msg);
	  break;
	}
      }
    }
    return posted;
  }

  void Log4cxxLogSink::clearLocally()
  {}

  casacore::String Log4cxxLogSink::localId()
  {
    return casacore::String("Log4cxxLogSink");
  }

  casacore::String Log4cxxLogSink::id() const
  {
    return casacore::String("Log4cxxLogSink");
  }

} // end namespaces

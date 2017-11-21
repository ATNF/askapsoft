/// @file
/// @brief Service based implementation of the calibration solution accessor
/// @details This implementation is to be used with the Calibration Data Service
/// it implements both a source and sink depending upon the context.
//
/// @copyright (c) 2017 CSIRO
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
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>
/// @author Stephen Ord <Stephen.Ord@csiro.au>


#ifndef ASKAP_CP_CALDATASERVICE_SOLUTION_ACCESSOR_H
#define ASKAP_CP_CALDATASERVICE_SOLUTION_ACCESSOR_H

// own includes
#include <calibaccess/ICalSolutionAccessor.h>

// std includes
#include <string>

namespace askap {

namespace cp {

namespace caldataservice {

/// @brief Service based implementation of the calibration solution accessor
/// @details This implementation is to be used with the Calibration Data Service
/// it implements both a source and sink depending upon the context.

/// @ingroup calibaccess
class ServiceCalSolutionAccessor : virtual public accessors::ICalSolutionAccessor

  {
public:
  /// @brief constructor
  /// @details It reads the given parset file, configures the service client
  /// @param[in] parset parset file name
  /// @param[in] readonly if true, additional checks are done that file exists

  explicit ServiceCalSolutionAccessor(const std::string &parset, casa::Long iD = 0, bool readonly = false);

  /// @brief destructor
  /// @details Not yet sure what functionality that needs to be here

  virtual ~ServiceCalSolutionAccessor();

  // override write methods to handle service access

  /// @brief obtain gains (J-Jones)
  /// @details This method retrieves parallel-hand gains for both
  /// polarisations (corresponding to XX and YY). If no gains are defined
  /// for a particular index, gains of 1. with invalid flags set are
  /// returned.
  /// @param[in] index ant/beam index
  /// @return JonesJTerm object with gains and validity flags
  virtual accessors::JonesJTerm gain(const accessors::JonesIndex &index) const;

  /// @brief obtain leakage (D-Jones)
  /// @details This method retrieves cross-hand elements of the
  /// Jones matrix (polarisation leakages). There are two values
  /// (corresponding to XY and YX) returned (as members of JonesDTerm
  /// class). If no leakages are defined for a particular index,
  /// zero leakages are returned with invalid flags set.
  /// @param[in] index ant/beam index
  /// @return JonesDTerm object with leakages and validity flags
  virtual accessors::JonesDTerm leakage(const accessors::JonesIndex &index) const;

  /// @brief obtain bandpass (frequency dependent J-Jones)
  /// @details This method retrieves parallel-hand spectral
  /// channel-dependent gain (also known as bandpass) for a
  /// given channel and antenna/beam. The actual implementation
  /// does not necessarily store these channel-dependent gains
  /// in an array. It could also implement interpolation or
  /// sample a polynomial fit at the given channel (and
  /// parameters of the polynomial could be in the database). If
  /// no bandpass is defined (at all or for this particular channel),
  /// gains of 1.0 are returned (with invalid flag is set).
  /// @param[in] index ant/beam index
  /// @param[in] chan spectral channel of interest
  /// @return JonesJTerm object with gains and validity flags
  virtual accessors::JonesJTerm bandpass(const accessors::JonesIndex &index, const casa::uInt chan) const;

  /// @brief set gains (J-Jones)
  /// @details This method writes parallel-hand gains for both
  /// polarisations (corresponding to XX and YY)
  /// @param[in] index ant/beam index
  /// @param[in] gains JonesJTerm object with gains and validity flags
  virtual void setGain(const accessors::JonesIndex &index, const accessors::JonesJTerm &gains);

  /// @brief set leakages (D-Jones)
  /// @details This method writes cross-pol leakages
  /// (corresponding to XY and YX)
  /// @param[in] index ant/beam index
  /// @param[in] leakages JonesDTerm object with leakages and validity flags
  virtual void setLeakage(const accessors::JonesIndex &index, const accessors::JonesDTerm &leakages);

  /// @brief set gains for a single bandpass channel
  /// @details This method writes parallel-hand gains corresponding to a single
  /// spectral channel (i.e. one bandpass element).
  /// @param[in] index ant/beam index
  /// @param[in] bp JonesJTerm object with gains for the given channel and validity flags
  /// @param[in] chan spectral channel
  /// @note We may add later variants of this method assuming that the bandpass is
  /// approximated somehow, e.g. by a polynomial. For simplicity, for now we deal with
  /// gains set explicitly for each channel.
  virtual void setBandpass(const accessors::JonesIndex &index, const accessors::JonesJTerm &bp, const casa::uInt chan);

  /// @brief shared pointer definition
  typedef boost::shared_ptr<ServiceCalSolutionAccessor> ShPtr;


protected:



private:
  /// @brief parset file name for reading or writing
  std::string itsParsetFileName;


};
} // namespace caldataservice

} // namespace accessors

} // namespace askap

#endif

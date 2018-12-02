/// @file
/// @brief Service based implementation of the calibration solution source
/// @details This implementation is to be used with the Calibration Data Service, one of
/// the ASKAP realtime services.
/// Main functionality is implemented in the corresponding ServiceCalSolutionAccessor class.
/// This class just creates an instance of the accessor and manages it.
///
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


#ifndef ASKAP_ACCESSORS_SERVICE_SOLUTION_SOURCESTUB_H
#define ASKAP_ACCESSORS_SERVICE_SOLUTION_SOURCESTUB_H

#include <Common/ParameterSet.h>

#include <calibaccess/ICalSolutionSource.h>


namespace askap {

namespace accessors {

/// @brief Service based implementation of the calibration solution source
/// @details This implementation is to be used with the Calibration Data Service, one of
/// the ASKAP realtime services.
/// Main functionality is implemented in the corresponding ServiceCalSolutionAccessor class.
/// This class just creates an instance of the accessor and manages it.
/// @ingroup calibaccess
struct ServiceCalSolutionSourceStub : public accessors::ICalSolutionSource {


  /// @brief constructor
  /// @details Creates solution source object for a given parset
  /// (whether it is for writing or reading depends on the actual methods
  /// used).
  /// @param[in] parset parset file name
  explicit ServiceCalSolutionSourceStub(const LOFAR::ParameterSet &parset);

  /// @brief obtain ID for the most recent solution
  /// @return ID for the most recent solution
  /// @note This particular implementation doesn't support multiple
  /// solutions and, therefore, always returns the same ID.
  virtual long mostRecentSolution() const;

  /// @brief obtain solution ID for a given time
  /// @details This method looks for a solution valid at the given time
  /// and returns its ID. It is equivalent to mostRecentSolution() if
  /// called with a time sufficiently into the future.
  /// @param[in] time time stamp in seconds since MJD of 0.
  /// @return solution ID
  /// @note This particular implementation doesn't support multiple
  /// solutions and, therefore, always returns the same ID.
  virtual long solutionID(const double time) const;

  /// @brief obtain read-only accessor for a given solution ID
  /// @details This method returns a shared pointer to the solution accessor, which
  /// can be used to read the parameters. If a solution with the given ID doesn't
  /// exist, an exception is thrown. Existing solutions with undefined parameters
  /// are managed via validity flags of gains, leakages and bandpasses
  /// @param[in] id solution ID to read
  /// @return shared pointer to an accessor object
  /// @note This particular implementation doesn't support multiple solutions and
  /// always returns the same accessor (for both reading and writing)
  virtual boost::shared_ptr<ICalSolutionConstAccessor> roSolution(const long id) const;


  /// @brief obtain a solution ID to store new solution
  /// @details This method provides a solution ID for a new solution. It must
  /// be called before any write operation (one needs a writable accessor to
  /// write the actual solution and to get this accessor one needs an ID).
  /// @param[in] time time stamp of the new solution in seconds since MJD of 0.
  /// @return solution ID
  /// @note This particular implementation always returns the same ID as it
  /// doesn't hangle multiple solution. Use table-based implementation to handle
  /// multiple (e.g. time-dependent) solutions
  virtual long newSolutionID(const double time);

  /// @brief obtain a writeable accessor for a given solution ID
  /// @details This method returns a shared pointer to the solution accessor, which
  /// can be used to both read the parameters and write them back. If a solution with
  /// the given ID doesn't exist, an exception is thrown. Existing solutions with undefined
  /// parameters are managed via validity flags of gains, leakages and bandpasses
  /// @param[in] id solution ID to access
  /// @return shared pointer to an accessor object
  /// @note This particular implementation returns the same accessor regardless of the
  /// chosen ID (for both reading and writing)
  virtual boost::shared_ptr<ICalSolutionAccessor> rwSolution(const long id) const;

  /// @brief shared pointer definition
  typedef boost::shared_ptr<ServiceCalSolutionSourceStub> ShPtr;

protected:
  /// @brief get shared pointer to accessor
  /// @return shared pointer to the accessor
  inline boost::shared_ptr<ICalSolutionAccessor> accessor() const { return itsAccessor;}
  const LOFAR::ParameterSet& itsParset;

private:
  /// @brief accessor doing actual work
  boost::shared_ptr<ICalSolutionAccessor> itsAccessor;
};



} // namespace accessors

} // namespace askap

#endif

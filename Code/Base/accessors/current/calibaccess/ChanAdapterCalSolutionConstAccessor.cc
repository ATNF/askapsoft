/// @file
/// @brief An adapter to adjust channel number                           
/// @details This adapter is handy if one needs to add a fixed offset to
/// channel numbers in the requested bandpass solution. It is not clear 
/// whether we want this class to stay long term (it is largely intended
/// for situations where the design was not very good and ideally we need
/// to redesign the code rather than do it quick and dirty way via the adapter).
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
/// @author Max Voronkov <Maxim.Voronkov@csiro.au>

// own includes
#include <calibaccess/ChanAdapterCalSolutionConstAccessor.h>
#include <askap/AskapError.h>

namespace askap {
namespace accessors {

/// @brief set up the adapter
/// @details The constructor sets the shared pointer to the accessor which
/// is wrapped around and the channel offset
/// @param[in] acc shared pointer to the original accessor to wrap
/// @param[in] offset channel offset to add to bandpass request
ChanAdapterCalSolutionConstAccessor::ChanAdapterCalSolutionConstAccessor(const boost::shared_ptr<ICalSolutionConstAccessor> &acc, const casacore::uInt offset) :
        itsAccessor(acc), itsOffset(offset) 
{
   ASKAPASSERT(acc);
}
   
/// @brief obtain gains (J-Jones)
/// @details This method retrieves parallel-hand gains for both 
/// polarisations (corresponding to XX and YY). If no gains are defined
/// for a particular index, gains of 1. with invalid flags set are
/// returned.
/// @param[in] index ant/beam index 
/// @return JonesJTerm object with gains and validity flags
JonesJTerm ChanAdapterCalSolutionConstAccessor::gain(const JonesIndex &index) const
{
   return itsAccessor->gain(index);
}
   
/// @brief obtain leakage (D-Jones)
/// @details This method retrieves cross-hand elements of the 
/// Jones matrix (polarisation leakages). There are two values
/// (corresponding to XY and YX) returned (as members of JonesDTerm 
/// class). If no leakages are defined for a particular index,
/// zero leakages are returned with invalid flags set. 
/// @param[in] index ant/beam index
/// @return JonesDTerm object with leakages and validity flags
JonesDTerm ChanAdapterCalSolutionConstAccessor::leakage(const JonesIndex &index) const
{
   return itsAccessor->leakage(index);
}
   
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
JonesJTerm ChanAdapterCalSolutionConstAccessor::bandpass(const JonesIndex &index, const casacore::uInt chan) const
{
   return itsAccessor->bandpass(index, chan + itsOffset);
}
   
} // namespace accessors
} // namespace askap


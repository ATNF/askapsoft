/// @file TableScalarFieldSelector.h
/// @brief An implementation of ITableDataSelectorImpl for simple (scalar) fields, like feed ID.
/// @details This class represents a selection of visibility
///         data according to some criterion. This is an
///         implementation of the part of the IDataSelector
///         interface, which can be done with the table selection
///         mechanism in the table based case. Only simple
///         (scalar) fields are included in this selection.
///         Epoch-based selection is done via a separate class
///         because a fully defined converter is required to
///         perform such selection.
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>
///
#ifndef ASKAP_ACCESSORS_TABLE_SCALAR_FIELD_SELECTOR_H
#define ASKAP_ACCESSORS_TABLE_SCALAR_FIELD_SELECTOR_H

// casa includes
#include <casacore/tables/TaQL/ExprNode.h>
#include <casacore/tables/Tables/Table.h>

// own includes
#include <dataaccess/ITableDataSelectorImpl.h>
#include <dataaccess/IDataConverter.h>
#include <dataaccess/ITableHolder.h>
#include <dataaccess/ITableInfoAccessor.h>

// std includes
#include <string>

namespace askap {

namespace accessors {
	
/// @brief An implementation of ITableDataSelectorImpl for simple (scalar) fields, like feed ID.
/// @details This class represents a selection of visibility
///         data according to some criterion. This is an
///         implementation of the part of the IDataSelector
///         interface, which can be done with the table selection
///         mechanism in the table based case. Only simple
///         (scalar) fields are included in this selection.
///         Epoch-based selection is done via a separate class
///         because a fully defined converter is required to
///         perform such selection.
///
/// A derivative from this class is passed to a DataSource object in the
/// request for an iterator. The iterator obtained that way runs through
/// the selected part of the dataset.
/// @ingroup dataaccess_tab
class TableScalarFieldSelector : virtual public ITableDataSelectorImpl,
				 virtual protected ITableInfoAccessor
{
public:
  
  /// Choose a single feed, the same for both antennae
  /// @param[in] feedID the sequence number of feed to choose
  virtual void chooseFeed(casa::uInt feedID);

  /// Choose a single baseline
  /// @param[in] ant1 the sequence number of the first antenna
  /// @param[in] ant2 the sequence number of the second antenna
  /// Which one is the first and which is the second is not important
  virtual void chooseBaseline(casa::uInt ant1, casa::uInt ant2);

  /// Choose all baselines to given antenna
  /// @param[in] ant the sequence number of antenna
  virtual void chooseAntenna(casa::uInt ant);

  /// Choose a single spectral window (also known as IF).
  /// @param[in] spWinID the ID of the spectral window to choose
  virtual void chooseSpectralWindow(casa::uInt spWinID);

  /// @brief choose user-defined index
  /// @param[in] column column name in the measurement set for a user-defined index
  /// @param[in] value index value
  virtual void chooseUserDefinedIndex(const std::string &column, const casa::uInt value);
  
  /// @brief Choose autocorrelations only
  virtual void chooseAutoCorrelations();
  
  /// @brief Choose crosscorrelations only
  virtual void chooseCrossCorrelations();
  
  /// @brief Choose samples corresponding to a uv-distance larger than threshold
  /// @details This effectively rejects the baselines giving a smaller
  /// uv-distance than the specified threshold
  /// @param[in] uvDist threshold (in metres)
  virtual void chooseMinUVDistance(casa::Double uvDist);

  /// @brief Choose samples corresponding to a uv-distance smaller than threshold
  /// @details This effectively rejects the baselines giving a larger
  /// uv-distance than the specified threshold
  /// @param[in] uvDist threshold (in metres)
  virtual void chooseMaxUVDistance(casa::Double uvDist);

  /// Choose a single scan number
  /// @param[in] scanNumber the scan number to choose
  virtual void chooseScanNumber(casa::uInt scanNumber);

  /// @brief Obtain a table expression node for selection. 
  /// @details This method is
  /// used in the implementation of the iterator to form a subtable
  /// obeying the selection criteria specified by the user via
  /// IDataSelector interface
  ///
  /// @param conv  a shared pointer to the converter, which is used to sort
  ///              out epochs and other measures used in the selection
  /// @return a const reference to table expression node object
  virtual const casa::TableExprNode& getTableSelector(const
               boost::shared_ptr<IDataConverterImpl const> &conv) const;
      
protected:
  /// @brief get read-write access to expression node
  /// @return a reference to the cached table expression node
  ///
  casa::TableExprNode& rwTableSelector() const;

private:
  /// a current table selection expression (cache)
  mutable casa::TableExprNode  itsTableSelector;  
};
  
} // namespace accessors
  
} // namespace askap
  
#endif // #ifndef TABLE_SCALAR_FIELD_SELECTOR_H

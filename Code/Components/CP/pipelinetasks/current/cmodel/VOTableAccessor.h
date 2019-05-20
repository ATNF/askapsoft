/// @file VOTableAccessor.h
///
/// @copyright (c) 2012 CSIRO
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

#ifndef ASKAP_CP_PIPELINETASKS_VOTABLEACCESSOR_H
#define ASKAP_CP_PIPELINETASKS_VOTABLEACCESSOR_H

// System includes

// ASKAPsoft includes
#include "askap/votable/VOTableField.h"
#include "askap/votable/VOTableRow.h"

// Local package includes
#include "cmodel/Common.h"
#include "cmodel/IGlobalSkyModel.h"

namespace askap {
namespace cp {
namespace pipelinetasks {

/// @brief
class VOTableAccessor : public IGlobalSkyModel {
    public:
        /// Constructor
        /// @param[in] filename name of the VOTable containing the source catalog
        VOTableAccessor(const std::string& filename);

        /// Constructor
        /// Used for testing only, so a stringstream can be
        /// passed in.
        /// @param[in] stream   istream from which data will be read.
        VOTableAccessor(const std::stringstream& sstream);

        /// Destructor
        virtual ~VOTableAccessor();

        /// @see askap::cp::pipelinetasks::IGlobalSkyModel::coneSearch
        virtual askap::cp::sms::client::ComponentListPtr coneSearch(
                const casacore::Quantity& ra,
                const casacore::Quantity& dec,
                const casacore::Quantity& searchRadius,
                const casacore::Quantity& fluxLimit);

    private:

        /// Enumerates the required and optional fields
        enum FieldEnum {
            RA,
            DEC,
            FLUX,
            MAJOR_AXIS,
            MINOR_AXIS,
            POSITION_ANGLE,
            SPECTRAL_INDEX,
            SPECTRAL_CURVATURE
        };

        /// Reads the field information out of the VOTable and populates the two maps.
        /// Only those fields found in FieldEnum will be actioned. All other fields
        /// found in the "fields" input will be ignored.
        ///
        /// @param[in] field    vector of fields.
        /// @param[out] posMap  maps FieldEnum to a zero-based index in the row
        /// @param[out] unitMap maps FieldEnum to units
        static void initFieldInfo(const std::vector<askap::accessors::VOTableField>& fields,
                std::map<VOTableAccessor::FieldEnum, size_t>& posMap,
                std::map<VOTableAccessor::FieldEnum, casacore::Unit>& unitMap);

        /// Check if the given UCD is found in the UCD attribute of the field.
        ///
        /// @param[in] field    the field to be searched.
        /// @param[in] ucd      the ucd to be searched for.
        /// @return true if the ucd is present in the ucd attribute of
        ///  the VOTable field, otherwise false.
        static bool hasUCD(const askap::accessors::VOTableField& field, const std::string& ucd);

        /// Process a row from the VOTable, creating a Component object and
        /// adding to the the "list".
        void processRow(const std::vector<std::string>& cells,
                const casacore::Quantity& searchRA,
                const casacore::Quantity& searchDec,
                const casacore::Quantity& searchRadius,
                const casacore::Quantity& fluxLimit,
                std::map<VOTableAccessor::FieldEnum, size_t>& posMap,
                std::map<VOTableAccessor::FieldEnum, casacore::Unit>& unitMap,
                std::list<askap::cp::sms::client::Component>& list);

        /// Filename of the VOTable
        const std::string itsFilename;

        /// Count of components below the flux limit
        casacore::uLong itsBelowFluxLimit;

        /// Count of components outside of the search radius
        casacore::uLong itsOutsideSearchCone;
};

}
}
}

#endif

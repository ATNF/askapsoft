/// @file TypedValueMapConstMapper.h
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

#ifndef ASKAP_CP_ICEWRAPPER_TYPEDVALUEMAPCONSTMAPPER_H
#define ASKAP_CP_ICEWRAPPER_TYPEDVALUEMAPCONSTMAPPER_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "casacore/casa/aips.h"
#include "casacore/measures/Measures/MDirection.h"

// CP ice interfaces
#include "TypedValues.h"

namespace askap {
namespace cp {
namespace icewrapper {

/// @brief Used to map between an askap::interfaces::TypedValueMap instance
/// and native Casa types.
///
/// This class provides read-only access to the TypedValueMap. If read/write
/// access is required the TypedValueMapMapper may be used.
/// 
/// @ingroup tosmetadata
class TypedValueMapConstMapper {
    public:
        /// @brief Constructor
        /// @param[in] map  the TypedValueMap this mapper maps from/to
        TypedValueMapConstMapper(const askap::interfaces::TypedValueMap& map);

        /// @brief Destructor.
        virtual ~TypedValueMapConstMapper() {};
        
        virtual casacore::Int getInt(const std::string& key) const;
        virtual casacore::Long getLong(const std::string& key) const;
        virtual casacore::String getString(const std::string& key) const;
        virtual casacore::Bool getBool(const std::string& key) const;
        virtual casacore::Float getFloat(const std::string& key) const;
        virtual casacore::Double getDouble(const std::string& key) const;
        virtual casacore::Complex getFloatComplex(const std::string& key) const;
        virtual casacore::DComplex getDoubleComplex(const std::string& key) const;
        virtual casacore::MDirection getDirection(const std::string& key) const;

        virtual std::vector<casacore::Int> getIntSeq(const std::string& key) const;
        virtual std::vector<casacore::Long> getLongSeq(const std::string& key) const;
        virtual std::vector<casacore::String> getStringSeq(const std::string& key) const;
        virtual std::vector<casacore::Bool> getBoolSeq(const std::string& key) const;
        virtual std::vector<casacore::Float> getFloatSeq(const std::string& key) const;
        virtual std::vector<casacore::Double> getDoubleSeq(const std::string& key) const;
        virtual std::vector<casacore::Complex> getFloatComplexSeq(const std::string& key) const;
        virtual std::vector<casacore::DComplex> getDoubleComplexSeq(const std::string& key) const;
        virtual std::vector<casacore::MDirection> getDirectionSeq(const std::string& key) const;

        /// @brief test that the particular key exists in the metadata
        /// @details
        /// @param[in] key key to query
        /// @return true, if the particular key exists
        virtual bool has(const std::string &key) const;

    private:
        // Template params are:
        /// @brief Parse the value of element identified by "key" and return in
        /// the appropriate type.
        ///
        /// @tparam T        Native (or casa) type
        /// @tparam TVType   Enum value from TypedValueType enum
        /// @tparam TVPtr    TypedValue pointer type
        ///
        /// @return the value for map element identified by the parameter "key".
        template <class T, askap::interfaces::TypedValueType TVType, class TVPtr>
        T get(const std::string& key) const;

        /// @brief Convert a TypedValue (Slice) direction type to a
        /// casacore::MDirection
        casacore::MDirection convertDirection(const askap::interfaces::Direction& dir) const;

        /// @brief The TypedValueMap this mapper maps from/to.
        const askap::interfaces::TypedValueMap itsConstMap;
};

}
}
}

#endif

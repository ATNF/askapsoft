/// @file TypedValueMapMapper.h
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

#ifndef ASKAP_CP_ICEWRAPPER_TYPEDVALUEMAPMAPPER_H
#define ASKAP_CP_ICEWRAPPER_TYPEDVALUEMAPMAPPER_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include "casacore/casa/aips.h"
#include "casacore/measures/Measures/MDirection.h"

// CP ice interfaces
#include "TypedValues.h"

// Local package includes
#include "TypedValueMapConstMapper.h"

namespace askap {
namespace cp {
namespace icewrapper {

/// @brief Used to map between an askap::interfaces::TypedValueMap instance
/// and native Casa types.
///
/// This class provides read/write access to the TypedValueMap. If read-only
/// is required, the TypedValueMapConstMapper may be used.
///
/// @ingroup tosmetadata
class TypedValueMapMapper : public TypedValueMapConstMapper {
    public:
        /// @brief Constructor
        /// @param[in] map  the TypedValueMap this mapper maps from/to
        TypedValueMapMapper(askap::interfaces::TypedValueMap& map);

        /// @brief Destructor
        virtual ~TypedValueMapMapper() {};
        
        void setInt(const std::string& key, const casacore::Int& val);
        void setLong(const std::string& key, const casacore::Long& val);
        void setString(const std::string& key, const casacore::String& val);
        void setBool(const std::string& key, const casacore::Bool& val);
        void setFloat(const std::string& key, const casacore::Float& val);
        void setDouble(const std::string& key, const casacore::Double& val);
        void setFloatComplex(const std::string& key, const casacore::Complex& val);
        void setDoubleComplex(const std::string& key, const casacore::DComplex& val);
        void setDirection(const std::string& key, const casacore::MDirection& val);

        void setIntSeq(const std::string& key, const std::vector<casacore::Int>& val);
        void setLongSeq(const std::string& key, const std::vector<casacore::Long>& val);
        void setStringSeq(const std::string& key, const std::vector<casacore::String>& val);
        void setBoolSeq(const std::string& key, const std::vector<casacore::Bool>& val);
        void setFloatSeq(const std::string& key, const std::vector<casacore::Float>& val);
        void setDoubleSeq(const std::string& key, const std::vector<casacore::Double>& val);
        void setFloatComplexSeq(const std::string& key, const std::vector<casacore::Complex>& val);
        void setDoubleComplexSeq(const std::string& key, const std::vector<casacore::DComplex>& val);
        void setDirectionSeq(const std::string& key, const std::vector<casacore::MDirection>& val);

    private:
        /// @brief Add or modify the value of element identified by "key".
        ///
        /// @tparam T           Native (or casa) type
        /// @tparam TVType      Enum value from TypedValueType enum
        /// @tparam TVClass     TypedValue type
        /// @param[in]key       the key via which the element is identified
        /// @param[in]val       the value to set
        template <class T, askap::interfaces::TypedValueType TVType, class TVClass>
        void set(const std::string& key, const T& val);

        /// @brief Convert a casacore::MDirection to a TypedValue (Slice) direction
        /// type.
        askap::interfaces::Direction convertDirection(const casacore::MDirection& dir) const;

        /// @brief The TypedValueMap this mapper maps from/to.
        askap::interfaces::TypedValueMap& itsMap;
};

}
}
}

#endif

/// @file TypedValueMapConstMapper.cc
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
#include "TypedValueMapConstMapper.h"

// ASKAPsoft includes
#include "askap/askap/AskapError.h"
#include "casacore/casa/aips.h"

#include "casacore/measures/Measures/MDirection.h"

// CP Ice interfaces
#include "TypedValues.h"

// Using
using namespace askap;
using namespace askap::cp::icewrapper;
using namespace askap::interfaces;

TypedValueMapConstMapper::TypedValueMapConstMapper(const TypedValueMap& map) :
    itsConstMap(map)
{
}

/// @brief test that the particular key exists in the metadata
/// @details
/// @param[in] key key to query
/// @return true, if the particular key exists
bool TypedValueMapConstMapper::has(const std::string &key) const
{
    return itsConstMap.count(key) != 0;
}

int TypedValueMapConstMapper::getInt(const std::string& key) const
{
    return get<int, TypeInt, TypedValueIntPtr>(key);
}

long TypedValueMapConstMapper::getLong(const std::string& key) const
{
    // ::Ice::Long is 64-bit (even on 32-bit x86) whereas casacore::Long will be 
    // 32-bit. Using this mapper on such a system will likely lead to grief.
#ifndef __LP64__
    ASKAPTHROW(AskapError, "This platform does not support 64-bit long");
#else
    return get<long, TypeLong, TypedValueLongPtr>(key);
#endif
}

casacore::String TypedValueMapConstMapper::getString(const std::string& key) const
{
    return get<casacore::String, TypeString, TypedValueStringPtr>(key);
}

casacore::Bool TypedValueMapConstMapper::getBool(const std::string& key) const
{
    return get<casacore::Bool, TypeBool, TypedValueBoolPtr>(key);
}

casacore::Float TypedValueMapConstMapper::getFloat(const std::string& key) const
{
    return get<casacore::Float, TypeFloat, TypedValueFloatPtr>(key);
}

casacore::Double TypedValueMapConstMapper::getDouble(const std::string& key) const
{
    return get<casacore::Double, TypeDouble, TypedValueDoublePtr>(key);
}

casacore::Complex TypedValueMapConstMapper::getFloatComplex(const std::string& key) const
{
    askap::interfaces::FloatComplex val =
        get<askap::interfaces::FloatComplex, TypeFloatComplex,
        TypedValueFloatComplexPtr>(key);

    return casacore::Complex(val.real, val.imag);
}

casacore::DComplex TypedValueMapConstMapper::getDoubleComplex(const std::string& key) const
{
    askap::interfaces::FloatComplex val =
        get<askap::interfaces::FloatComplex, TypeFloatComplex,
        TypedValueFloatComplexPtr>(key);

    return casacore::DComplex(val.real, val.imag);
}

casacore::MDirection TypedValueMapConstMapper::getDirection(const std::string& key) const
{
    askap::interfaces::Direction val =
        get<askap::interfaces::Direction, TypeDirection,
        TypedValueDirectionPtr>(key);

    return convertDirection(val);
}

std::vector<casacore::Int> TypedValueMapConstMapper::getIntSeq(const std::string& key) const
{
    return get<std::vector<casacore::Int>, TypeIntSeq, TypedValueIntSeqPtr>(key);
}

std::vector<casacore::Long> TypedValueMapConstMapper::getLongSeq(const std::string& key) const
{
    // ::Ice::Long is 64-bit (even on 32-bit x86) whereas casacore::Long will be 
    // 32-bit. Using this mapper on such a system will likely lead to grief.
#ifndef __LP64__
    ASKAPTHROW(AskapError, "This platform does not support 64-bit long");
#else
    //return get<std::vector<casacore::Long>, TypeLongSeq, TypedValueLongSeqPtr>(key);
    LongSeq seq = get<LongSeq, TypeLongSeq, TypedValueLongSeqPtr>(key);
    return std::vector<casacore::Long>(seq.begin(), seq.end());
#endif
}

std::vector<casacore::String> TypedValueMapConstMapper::getStringSeq(const std::string& key) const
{
    askap::interfaces::StringSeq val =
        get<askap::interfaces::StringSeq, TypeStringSeq,
        TypedValueStringSeqPtr>(key);

    return std::vector<casacore::String>(val.begin(), val.end());
}

std::vector<casacore::Bool> TypedValueMapConstMapper::getBoolSeq(const std::string& key) const
{
    return get<std::vector<casacore::Bool>, TypeBoolSeq, TypedValueBoolSeqPtr>(key);
}

std::vector<casacore::Float> TypedValueMapConstMapper::getFloatSeq(const std::string& key) const
{
    return get<std::vector<casacore::Float>, TypeFloatSeq, TypedValueFloatSeqPtr>(key);
}

std::vector<casacore::Double> TypedValueMapConstMapper::getDoubleSeq(const std::string& key) const
{
    return get<std::vector<casacore::Double>, TypeDoubleSeq, TypedValueDoubleSeqPtr>(key);
}

std::vector<casacore::Complex> TypedValueMapConstMapper::getFloatComplexSeq(const std::string& key) const
{
    askap::interfaces::FloatComplexSeq val =
        get<askap::interfaces::FloatComplexSeq, TypeFloatComplexSeq,
        TypedValueFloatComplexSeqPtr>(key);

    // Populate this vector before returning it
    std::vector<casacore::Complex> container;

    askap::interfaces::FloatComplexSeq::const_iterator it = val.begin();
    for (it = val.begin(); it != val.end(); ++it) {
        container.push_back(casacore::Complex(it->real, it->imag));
    }

    return container;
}

std::vector<casacore::DComplex> TypedValueMapConstMapper::getDoubleComplexSeq(const std::string& key) const
{
    askap::interfaces::DoubleComplexSeq val =
        get<askap::interfaces::DoubleComplexSeq, TypeDoubleComplexSeq,
        TypedValueDoubleComplexSeqPtr>(key);

    // Populate this vector before returning it
    std::vector<casacore::DComplex> container;

    askap::interfaces::DoubleComplexSeq::const_iterator it = val.begin();
    for (it = val.begin(); it != val.end(); ++it) {
        container.push_back(casacore::DComplex(it->real, it->imag));
    }

    return container;
}

std::vector<casacore::MDirection> TypedValueMapConstMapper::getDirectionSeq(const std::string& key) const
{
    askap::interfaces::DirectionSeq val =
        get<askap::interfaces::DirectionSeq, TypeDirectionSeq,
        TypedValueDirectionSeqPtr>(key);

    // Populate this vector before returning it
    std::vector<casacore::MDirection> container;

    askap::interfaces::DirectionSeq::const_iterator it = val.begin();
    for (it = val.begin(); it != val.end(); ++it) {
        container.push_back(convertDirection(*it));
    }

    return container;
}

template <class T, askap::interfaces::TypedValueType TVType, class TVPtr>
T TypedValueMapConstMapper::get(const std::string& key) const
{
    if (itsConstMap.count(key) == 0) {
        ASKAPTHROW(AskapError, "Specified key (" << key << ") does not exist");
    }
    const TypedValuePtr tv = itsConstMap.find(key)->second;
    if (tv->type != TVType) {
        ASKAPTHROW(AskapError, "Specified key (" << key << ") not of the requested type");
    }

    return TVPtr::dynamicCast(tv)->value;
}

casacore::MDirection TypedValueMapConstMapper::convertDirection(const askap::interfaces::Direction& dir) const
{
    switch (dir.sys) {
        case J2000 :
            return casacore::MDirection(casacore::Quantity(dir.coord1, "deg"),
                    casacore::Quantity(dir.coord2, "deg"),
                    casacore::MDirection::Ref(casacore::MDirection::J2000));
            break;
        case AZEL :
            return casacore::MDirection(casacore::Quantity(dir.coord1, "deg"),
                    casacore::Quantity(dir.coord2, "deg"),
                    casacore::MDirection::Ref(casacore::MDirection::AZEL));
            break;
    }

    // This is the default case
   ASKAPTHROW(AskapError, "Coordinate system not supported");
}

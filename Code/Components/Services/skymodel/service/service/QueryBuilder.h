/// ----------------------------------------------------------------------------
/// This file is generated by schema_definitions/generate.py.
/// Do not edit directly or your changes will be lost!
/// ----------------------------------------------------------------------------
///
/// @copyright (c) 2016 CSIRO
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#pragma once

// System includes
#include <string>
#include <boost/shared_ptr.hpp>

// ASKAPsoft includes
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>

// Ice interfaces
#include <SkyModelServiceCriteria.h>

// Local package includes
#include "datamodel/ContinuumComponent.h"

namespace askap {
namespace cp {
namespace sms {

// Alias for the Ice type namespace
namespace ice_interfaces = askap::interfaces::skymodelservice;

/// @brief Builds an ODB ContinuumComponent query from the Ice criteria struct
///
/// @param[in] The Ice SearchCriteria struct
/// @return The ContinuumComponent query
odb::query<datamodel::ContinuumComponent> queryBuilder(const sms_interface::SearchCriteria& criteria)
{
    typedef odb::query<datamodel::ContinuumComponent> query;
    query q;

    if (criteria.minRaErr >= 0)
        q = q && query::ra_err >= criteria.minRaErr;
    if (criteria.maxRaErr >= 0)
        q = q && query::ra_err <= criteria.maxRaErr;
    if (criteria.minDecErr >= 0)
        q = q && query::dec_err >= criteria.minDecErr;
    if (criteria.maxDecErr >= 0)
        q = q && query::dec_err <= criteria.maxDecErr;
    if (criteria.minFreq >= 0)
        q = q && query::freq >= criteria.minFreq;
    if (criteria.maxFreq >= 0)
        q = q && query::freq <= criteria.maxFreq;
    if (criteria.minFluxPeak >= 0)
        q = q && query::flux_peak >= criteria.minFluxPeak;
    if (criteria.maxFluxPeak >= 0)
        q = q && query::flux_peak <= criteria.maxFluxPeak;
    if (criteria.minFluxPeakErr >= 0)
        q = q && query::flux_peak_err >= criteria.minFluxPeakErr;
    if (criteria.maxFluxPeakErr >= 0)
        q = q && query::flux_peak_err <= criteria.maxFluxPeakErr;
    if (criteria.minFluxInt >= 0)
        q = q && query::flux_int >= criteria.minFluxInt;
    if (criteria.maxFluxInt >= 0)
        q = q && query::flux_int <= criteria.maxFluxInt;
    if (criteria.minFluxIntErr >= 0)
        q = q && query::flux_int_err >= criteria.minFluxIntErr;
    if (criteria.maxFluxIntErr >= 0)
        q = q && query::flux_int_err <= criteria.maxFluxIntErr;
    if (criteria.minMajAxis >= 0)
        q = q && query::maj_axis >= criteria.minMajAxis;
    if (criteria.maxMajAxis >= 0)
        q = q && query::maj_axis <= criteria.maxMajAxis;
    if (criteria.minMinAxis >= 0)
        q = q && query::min_axis >= criteria.minMinAxis;
    if (criteria.maxMinAxis >= 0)
        q = q && query::min_axis <= criteria.maxMinAxis;
    if (criteria.useMinPosAng)
        q = q && query::pos_ang >= criteria.minPosAng;
    if (criteria.useMaxPosAng)
        q = q && query::pos_ang <= criteria.maxPosAng;
    if (criteria.minMajAxisErr >= 0)
        q = q && query::maj_axis_err >= criteria.minMajAxisErr;
    if (criteria.maxMajAxisErr >= 0)
        q = q && query::maj_axis_err <= criteria.maxMajAxisErr;
    if (criteria.minMinAxisErr >= 0)
        q = q && query::min_axis_err >= criteria.minMinAxisErr;
    if (criteria.maxMinAxisErr >= 0)
        q = q && query::min_axis_err <= criteria.maxMinAxisErr;
    if (criteria.useMinSpectralIndex)
        q = q && query::spectral_index >= criteria.minSpectralIndex;
    if (criteria.useMaxSpectralIndex)
        q = q && query::spectral_index <= criteria.maxSpectralIndex;
    if (criteria.useMinSpectralCurvature)
        q = q && query::spectral_curvature >= criteria.minSpectralCurvature;
    if (criteria.useMaxSpectralCurvature)
        q = q && query::spectral_curvature <= criteria.maxSpectralCurvature;
    if (criteria.minMajAxisDeconvErr >= 0)
        q = q && query::maj_axis_deconv_err >= criteria.minMajAxisDeconvErr;
    if (criteria.maxMajAxisDeconvErr >= 0)
        q = q && query::maj_axis_deconv_err <= criteria.maxMajAxisDeconvErr;
    if (criteria.minMinAxisDeconvErr >= 0)
        q = q && query::min_axis_deconv_err >= criteria.minMinAxisDeconvErr;
    if (criteria.maxMinAxisDeconvErr >= 0)
        q = q && query::min_axis_deconv_err <= criteria.maxMinAxisDeconvErr;
    if (criteria.minPosAngDeconvErr >= 0)
        q = q && query::pos_ang_deconv_err >= criteria.minPosAngDeconvErr;
    if (criteria.maxPosAngDeconvErr >= 0)
        q = q && query::pos_ang_deconv_err <= criteria.maxPosAngDeconvErr;
    if (criteria.minSpectralIndexErr >= 0)
        q = q && query::spectral_index_err >= criteria.minSpectralIndexErr;
    if (criteria.maxSpectralIndexErr >= 0)
        q = q && query::spectral_index_err <= criteria.maxSpectralIndexErr;

    return q;
}

}
}
}
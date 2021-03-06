/// @file IceMapper.h
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

#ifndef ASKAP_CP_CALDATASERVICE_ICEMAPPER_H
#define ASKAP_CP_CALDATASERVICE_ICEMAPPER_H

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"
#include "CalibrationDataService.h" // Ice generated interface

// Local package includes
#include "calibaccess/JonesIndex.h"
#include "calibaccess/JonesJTerm.h"
#include "calibaccess/JonesDTerm.h"
#include "GenericSolution.h"

namespace askap {
namespace cp {
namespace caldataservice {

/// Utility class that provides conversions between the Ice and Native (i.e. wrapper)
/// classes.
class IceMapper {

    public:

        static askap::interfaces::calparams::TimeTaggedGainSolution toIce(const askap::cp::caldataservice::GainSolution& sol);
        static askap::interfaces::calparams::TimeTaggedLeakageSolution toIce(const askap::cp::caldataservice::LeakageSolution& sol);
        static askap::interfaces::calparams::TimeTaggedBandpassSolution toIce(const askap::cp::caldataservice::BandpassSolution& sol);

        static askap::cp::caldataservice::GainSolution fromIce(const askap::interfaces::calparams::TimeTaggedGainSolution& ice_sol);
        static askap::cp::caldataservice::LeakageSolution fromIce(const askap::interfaces::calparams::TimeTaggedLeakageSolution& ice_sol);
        static askap::cp::caldataservice::BandpassSolution fromIce(const askap::interfaces::calparams::TimeTaggedBandpassSolution& ice_sol);

    private:
        static askap::interfaces::FloatComplex toIce(const casa::Complex& val);
        static askap::interfaces::calparams::JonesIndex toIce(const askap::accessors::JonesIndex& jindex);
        static askap::interfaces::calparams::JonesJTerm toIce(const askap::accessors::JonesJTerm& jterm);
        static askap::interfaces::calparams::JonesDTerm toIce(const askap::accessors::JonesDTerm& dterm);

        static casa::Complex fromIce(const askap::interfaces::FloatComplex& ice_val);
        static askap::accessors::JonesIndex fromIce(const askap::interfaces::calparams::JonesIndex& ice_jindex);
        static askap::accessors::JonesJTerm fromIce(const askap::interfaces::calparams::JonesJTerm& ice_jterm);
        static askap::accessors::JonesDTerm fromIce(const askap::interfaces::calparams::JonesDTerm& ice_dterm);
};

};
};
};

#endif

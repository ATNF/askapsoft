/// @file VisConverter.cc
/// @brief generic converter of visibility stream to vis chunks
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>


// for some reason I have troubles using autoinstantiation
// for now use manual instantiation, but it would be good to
// come back to this question when we have time.

#include "ingestpipeline/sourcetask/VisConverter.h"
#include "cpcommon/VisDatagram.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"

ASKAP_LOGGER(logger, ".VisConverter");

#include "ingestpipeline/sourcetask/VisConverterBETA.tcc"
#include "ingestpipeline/sourcetask/VisConverterADE.tcc"

namespace askap {
namespace cp {
namespace ingest {

class VisConverter<VisDatagramBETA>;

class VisConverter<VisDatagramADE>;

} // namespace ingest
} // namespace cp
} // namespace askap



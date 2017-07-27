/// @file HealPixFacade.cc
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

// Include own header file first
#include "HealPixFacade.h"

// Include package level header file
#include "askap_skymodel.h"

// System includes
#include <string>

// ASKAPsoft includes
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>
#include <Common/ParameterSet.h>
#include <boost/scoped_ptr.hpp>
#include <healpix_tables.h>
#include <rangeset.h>

// Local includes
//#include "SkyModelServiceImpl.h"

ASKAP_LOGGER(logger, ".HealPixFacade");

using namespace std;
using namespace boost;
using namespace askap::cp::sms;

namespace askap {
namespace cp {
namespace sms {


HealPixFacade::HealPixFacade(Index order)
    :
    itsHealPixBase(2 << order, NEST, SET_NSIDE),
    itsNSide(2 << order)
{
}

HealPixFacade::Index HealPixFacade::calcHealPixIndex(Coordinate coordinate) const
{
    // Note: this initial implementation is not likely to be the most efficient,
    // but it does give me enough to sort out the basics.
    // A more efficient option is likely to be threaded processing of
    // contiguous arrays of ra and dec coordinates, with the T_Healpix_Base
    // object being reused if possible, or thread-local if it is not
    // thread-safe.
    return itsHealPixBase.ang2pix(J2000ToPointing(coordinate));
}

HealPixFacade::IndexListPtr HealPixFacade::queryDisk(Coordinate centre, double radius, int fact) const
{
    rangeset<Index> pixels;
    itsHealPixBase.query_disc_inclusive(
        J2000ToPointing(centre),
        utility::degreesToRadians(radius),
        pixels,
        fact);

    return IndexListPtr(new IndexList(pixels.toVector()));
}

HealPixFacade::IndexListPtr HealPixFacade::queryRect(
    Rect rect,
    int fact) const
{
    // munge the inputs into a polygon, moving clockwise from the top-left
    std::vector<pointing> vertex;
    vertex.push_back(J2000ToPointing(rect.topLeft()));
    vertex.push_back(J2000ToPointing(rect.topRight()));
    vertex.push_back(J2000ToPointing(rect.bottomRight()));
    vertex.push_back(J2000ToPointing(rect.bottomLeft()));
    
    // intersect with HEALPix
    rangeset<Index> pixels;
    itsHealPixBase.query_polygon_inclusive(vertex, pixels, fact);

    // return pixels as an IndexList
    return IndexListPtr(new IndexList(pixels.toVector()));
}

};
};
};

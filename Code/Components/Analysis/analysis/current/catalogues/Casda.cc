/// @file
///
/// Constants needed for CASDA catalogues
///
/// @copyright (c) 2014 CSIRO
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#include <catalogues/Casda.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
#include <sourcefitting/RadioSource.h>
#include <outputs/CataloguePreparation.h>

namespace askap {

namespace analysis {

namespace casda {

std::string getIslandID(sourcefitting::RadioSource &obj)
{
    std::stringstream id;
    id << "island_" << obj.getID();
    return id.str();
}

std::string getComponentID(sourcefitting::RadioSource &obj, const unsigned int fitNumber)
{
    std::stringstream id;
    id << "component_" << obj.getID() << getSuffix(fitNumber);
    return id.str();
}


ValueError::ValueError():
    itsValue(0.),
    itsError(0.)
{
}

ValueError::ValueError(double val, double err):
    itsValue(val),
    itsError(err)
{
}

ValueError::~ValueError()
{
}

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &blob, ValueError& src)
{
    blob << src.itsValue;
    blob << src.itsError;
    return blob;
}

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &blob, ValueError& src)
{
    blob >> src.itsValue;
    blob >> src.itsError;
    return blob;
}

}
}
}


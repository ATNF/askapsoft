/// @file
///
/// Implementation of base class CasdaObject
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
#include <catalogues/CasdaObject.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>

ASKAP_LOGGER(logger, ".casdaobject");

namespace askap {

namespace analysis {

CasdaObject::CasdaObject()
{
}

CasdaObject::CasdaObject(const LOFAR::ParameterSet &parset):
    itsParset(parset),
    itsSBid(parset.getString("sbid", "null")),
    itsIDbase(parset.getString("sourceIdBase", ""))
{
    std::stringstream id;
    if (itsIDbase != "") {
        id << itsIDbase << "_";
    } else if (itsSBid != "null") {
        id << "SB" << itsSBid << "_";
    }
    itsIDbase = id.str();

}


void CasdaObject::printTableRow(std::ostream &stream,
                                duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
}

void CasdaObject::printTableEntry(std::ostream &stream,
                                  duchamp::Catalogues::Column &column)
{
    ASKAPLOG_WARN_STR(logger, "No printTableEntry defined for base class");
}

void CasdaObject::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle)
{
    for (size_t i = 0; i < spec.size(); i++) {
        this->checkCol(spec.column(i), checkTitle);
    }
}
/// Allow the Column provided to check its width against that
/// required by the value for this Component, and increase its
/// width if need be. The correct value is chose according to
/// the COLNAME key. If a key is given that was not expected,
/// an Askap Error is thrown. Column must be non-const as it
/// could change.
void CasdaObject::checkCol(duchamp::Catalogues::Column &column, bool checkTitle)
{
    ASKAPLOG_WARN_STR(logger, "No checkCol defined for base class");

}




}

}

/// @file
///
/// Abstract Base Class for catalogue entries
///
/// @copyright (c) 2015 CSIRO
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
#ifndef ASKAP_ANALYSIS_CASDA_OBJECT_H_
#define ASKAP_ANALYSIS_CASDA_OBJECT_H_
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <string>

namespace askap {

namespace analysis {

/// The base class for an entry in a catalogue. Primary functionality
/// is to get the Scheduling Block (SB) ID from the parset, and create
/// a base for a full component/island/whatever ID combining the SB_ID
/// and the image name. The class also provides for methods to get the
/// RA and Dec of the entry.
class CasdaObject {
    public:

        /// Default constructor that does nothing
        CasdaObject();

        /// Constructor from a parset, getting the SB ID and making a
        /// base ID with it and the image name.
        CasdaObject(const LOFAR::ParameterSet &parset);

        /// Default destructor
        virtual ~CasdaObject() {};

        /// Accessor for the RA (not defined in base class)
        virtual const float ra() = 0;

        /// Accessor for the Declination (not defined in base class)
        virtual const float dec() = 0;

        virtual void printTableRow(std::ostream &stream, duchamp::Catalogues::CatalogueSpecification &columns);

        virtual void printTableEntry(std::ostream &stream, duchamp::Catalogues::Column &column);

        virtual void checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle);
        /// Allow the Column provided to check its width against that
        /// required by the value for this Component, and increase its
        /// width if need be. The correct value is chose according to
        /// the COLNAME key. If a key is given that was not expected,
        /// an Askap Error is thrown. Column must be non-const as it
        /// could change.
        virtual void checkCol(duchamp::Catalogues::Column &column, bool checkTitle);


    protected:
        /// @brief Parset - can add things here
        LOFAR::ParameterSet            itsParset;

        /// The Scheduling Block ID
        std::string itsSBid;

        /// The base ID that ties an entry to a unique observation &
        /// image combination.
        std::string itsIDbase;

};


}

}



#endif

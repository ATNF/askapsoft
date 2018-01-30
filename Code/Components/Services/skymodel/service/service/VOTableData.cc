/// @file VOTableData.cc
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
#include "VOTableData.h"

// Include package level header file
#include "askap_skymodel.h"

// System includes
#include <string>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// ASKAPsoft includes
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>

// Local package includes
#include "HealPixFacade.h"
#include "Utility.h"
#include "VOTableParser.h"

ASKAP_LOGGER(logger, ".VOTableData");

using namespace std;
using namespace askap::cp::sms;
using namespace askap::accessors;


boost::shared_ptr<VOTableData> VOTableData::create(
    string components_file,
    string polarisation_file,
    boost::int64_t healpix_order)
{
    // open components file
    VOTable components = VOTable::fromXML(components_file);
    ASKAPASSERT(components.getResource().size() == 1ul);
    ASKAPASSERT(components.getResource()[0].getTables().size() == 1ul);
    const VOTableTable components_table = components.getResource()[0].getTables()[0];
    vector<VOTableField> fields = components_table.getFields();
    vector<VOTableRow> rows = components_table.getRows();
    const unsigned long num_components = rows.size();

    boost::shared_ptr<VOTableData> data(new VOTableData(num_components));
    ASKAPASSERT(data.get());

    map<string, size_t> componentsById;

    // TODO: will it be better to reverse the order of field and row iteration?
    // Typically there will be ~30 fields and ~1000 rows
    size_t row_index = 0;
    vector<VOTableRow>::iterator rit;
    // for each component row ...
    for (rit = rows.begin(), row_index = 0;
         rit != rows.end();
         rit++, row_index++) {
        const vector<std::string> rowData = rit->getCells();
        long field_index = 0;
        vector<VOTableField>::iterator fit;
        // for each field in the current row ...
        for (fit = fields.begin(), field_index = 0;
             fit != fields.end();
             fit++, field_index++) {
            parseComponentRowField(
                row_index,
                fit->getUCD(),
                fit->getName(),
                fit->getDatatype(),
                fit->getUnit(),
                rowData[field_index],
                data->itsComponents);
        }

        componentsById.insert(pair<string, size_t>(
            data->itsComponents[row_index].component_id,
            row_index));
    }

    // load polarisation file if it exists
    if (boost::filesystem::exists(polarisation_file))
    {
        // parse polarisation file
        VOTable polarisation = VOTable::fromXML(polarisation_file);
        ASKAPASSERT(polarisation.getResource().size() == 1ul);
        ASKAPASSERT(polarisation.getResource()[0].getTables().size() == 1ul);
        const VOTableTable polarisation_table = polarisation.getResource()[0].getTables()[0];
        vector<VOTableField> polfields = polarisation_table.getFields();
        vector<VOTableRow> polrows = polarisation_table.getRows();
        vector<VOTableRow>::iterator rit;
        // for each polarisation row ...
        for (rit = polrows.begin(); rit != polrows.end(); rit++) {
            const vector<std::string> rowData = rit->getCells();
            long field_index = 0;
            vector<VOTableField>::iterator fit;

            // create a new polarisation object
            boost::shared_ptr<datamodel::Polarisation> pPol(
                new datamodel::Polarisation());

            // parse each field in the current row ...
            for (fit = polfields.begin(), field_index = 0;
                fit != polfields.end();
                fit++, field_index++) {
                parsePolarisationRowField(
                    fit->getUCD(),
                    fit->getName(),
                    fit->getDatatype(),
                    fit->getUnit(),
                    rowData[field_index],
                    pPol);
            }

            // Now we have a complete polarisation object. Set it into the matching component
            data->itsComponents[componentsById[pPol->component_id]].polarisation = pPol;
        }
    }

    data->calcHealpixIndicies(healpix_order);

    return data;
}

VOTableData::VOTableData(unsigned long num_components) :
    itsComponents(num_components)
{
}

VOTableData::~VOTableData()
{
    ASKAPLOG_DEBUG_STR(logger, "dtor");
}

void VOTableData::calcHealpixIndicies(boost::int64_t healpix_order) {
    ASKAPLOG_DEBUG_STR(logger, "Starting HEALPix indexation");
    HealPixFacade hp(healpix_order);
    for (ComponentList::iterator it = itsComponents.begin(); it != itsComponents.end(); it++) {
        it->healpix_index = hp.calcHealPixIndex(Coordinate(it->ra, it->dec));
    }
    ASKAPLOG_DEBUG_STR(logger, "HEALPix indexation complete");
}

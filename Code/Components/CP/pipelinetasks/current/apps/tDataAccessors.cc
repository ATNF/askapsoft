/// @file tDataAccessors.cc
///
/// @copyright (c) 2018 CSIRO
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

// System includes
#include <iostream>
#include <string>

// ASKAPsoft includes
#include "askap/Application.h"
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"

// Local package includes
#include "cmodel/Common.h"
#include "cmodel/IGlobalSkyModel.h"
#include "cmodel/VOTableAccessor.h"
#include "cmodel/DataserviceAccessor.h"

// Using
//using namespace askap::cp;
//using namespace askap::cp::icewrapper;
using namespace askap;
using namespace askap::cp::pipelinetasks;
using namespace askap::cp::sms::client;
using namespace casa;

ASKAP_LOGGER(logger, ".tDataAccessors");


// custom sort predicate for sorting Components by flux
struct flux_less_than
{
    inline bool operator() (const Component& lhs, const Component& rhs)
    {
        return lhs.i1400() < rhs.i1400();
    }
};

void printComponent(const Component& c)
{
    cout << "RA, Dec: " << c.rightAscension() << ", " << c.declination() 
        << "   flux: " << c.i1400() 
        << "   major axis: " << c.majorAxis()
        << "   minor axis: " << c.minorAxis()
        << "   position angle: " << c.positionAngle()
        << "   spectral index: " << c.spectralIndex()
        << "   spectral curvature: " << c.spectralCurvature()
        << endl;
}

class TestDataAccessorsApp : public askap::Application
{
    public:
        virtual int run(int argc, char* argv[])
        {
            const std::string locatorHost = config().getString("ice.locator_host");
            const std::string locatorPort = config().getString("ice.locator_port");
            const std::string serviceName = config().getString("ice.service_name");
            const std::string filename = config().getString("Cmodel.gsm.file");

            //std::cout << "locator host: " << locatorHost << "\n"
                //<< "locator port: " << locatorPort << "\n"
                //<< "service name: " << serviceName << "\n"
                //<< "catalog file: " << filename << "\n"
                //<< std::endl;

            // common search params
            casacore::Quantity ra(79.8, "deg");
            casacore::Quantity dec(-71.8, "deg");
            casacore::Quantity radius(2.0, "deg");
            casacore::Quantity minFlux(80, "mJy");
            casacore::Unit deg("deg");
            casacore::Unit rad("rad");
            casacore::Unit arcsec("arcsec");
            casacore::Unit Jy("Jy");
            const MVDirection searchVector(ra, dec);

            // create the VOTableAccessor and query for some components
            boost::scoped_ptr<VOTableAccessor> votable(new VOTableAccessor(filename));
            ComponentListPtr votableResults = votable->coneSearch(ra, dec, radius, minFlux);

            // create the DataserviceAccessor
            boost::scoped_ptr<DataserviceAccessor> sms(new DataserviceAccessor(locatorHost, locatorPort, serviceName));
            ComponentListPtr smsResults = sms->coneSearch(ra, dec, radius, minFlux);

            // check that the SMS results are all brighter than the flux limit
            for (ComponentList::const_iterator it = smsResults->begin();
                 it != smsResults->end();
                 it++) {
                ASKAPASSERT(it->i1400().getValue(Jy) >= minFlux.getValue(Jy));
            }

            // The SMS implements spatial queries via HEALPix pixels without an
            // additional spatial refinement. This means that all components in
            // the pixels intersecting the search boundary will be
            // returned, even if those components are outside the search region.
            // The chance is pretty small, but just in case we filter the data service results with a precise spatial test
            ComponentList smsFilteredResults;
            for (ComponentList::const_iterator it = smsResults->begin();
                 it != smsResults->end();
                 it++) {
                MVDirection(it->rightAscension(), it->declination());
                const Quantity separation = searchVector.separation(
                        MVDirection(it->rightAscension(), it->declination()),
                        deg);
                if (separation.getValue(deg) <= radius.getValue(deg)) {
                    smsFilteredResults.push_back(*it);
                }
            }

            // Test that we have the same result counts in each list
            ASKAPASSERT(smsFilteredResults.size() == votableResults->size());

            // sort the two vectors so we can compare element by element
            sort(smsFilteredResults.begin(), smsFilteredResults.end(), flux_less_than());
            sort(votableResults->begin(), votableResults->end(), flux_less_than());
            const double tolerance = 0.000005;
            for (size_t i = 0; i < smsFilteredResults.size(); i++) {
                Component& a = (*votableResults)[i];
                Component& b = smsFilteredResults[i];
                cout << "votable - ";
                printComponent(a);
                cout << "SMS -     ";
                printComponent(b);
                ASKAPASSERT(std::abs(a.rightAscension().getValue(deg) - b.rightAscension().getValue(deg)) < tolerance);
                ASKAPASSERT(std::abs(a.declination().getValue(deg) - b.declination().getValue(deg)) < tolerance);
                ASKAPASSERT(std::abs(a.i1400().getValue(Jy) - b.i1400().getValue(Jy)) < tolerance);
                ASKAPASSERT(std::abs(a.positionAngle().getValue(deg) - b.positionAngle().getValue(deg)) < tolerance);
                ASKAPASSERT(std::abs(a.majorAxis().getValue(arcsec) - b.majorAxis().getValue(arcsec)) < tolerance);
                ASKAPASSERT(std::abs(a.minorAxis().getValue(arcsec) - b.minorAxis().getValue(arcsec)) < tolerance);
                ASKAPASSERT(std::abs(a.spectralIndex() - b.spectralIndex()) < tolerance);
                ASKAPASSERT(std::abs(a.spectralCurvature() - b.spectralCurvature()) < tolerance);
            }

            return 0;
        }

    private:

        //ComponentListPtr coneSearch(
                //IGlobalSkyModel* pGSM,
                //const casacore::Quantity& ra,
                //const casacore::Quantity& dec,
                //const casacore::Quantity& searchRadius,
                //const casacore::Quantity& fluxLimit)
};

int main(int argc, char *argv[])
{
    TestDataAccessorsApp app;
    return app.main(argc, argv);
}

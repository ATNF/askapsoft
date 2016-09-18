/// @file
///
/// Utility function to convert a CASA image to a FITS image. Provides
/// a parset interface to allow more flexibility than the casacore
/// image2fits function.
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

// Package level header file
#include <askap_accessors.h>

// ASKAPsoft includes
#include <askap/Application.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askap/StatReporter.h>

#include <Common/ParameterSet.h>
#include <casacore/images/Images/ImageFITSConverter.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/tables/Tables/TableRecord.h>
#include <casacore/casa/Logging/LogIO.h>

using namespace askap;

ASKAP_LOGGER(logger, "imageToFITS.log");

class ConvertApp : public askap::Application {
    public:
        virtual int run(int argc, char* argv[])
        {
            try {
                StatReporter stats;

                ASKAPLOG_INFO_STR(logger, "ASKAP source finder " << ASKAP_PACKAGE_VERSION);

                // Create a new parset with a nocase key compare policy, then
                // adopt the contents of the real parset
                LOFAR::ParameterSet parset;
                parset.adoptCollection(config());
                LOFAR::ParameterSet subset(parset.makeSubset("ImageToFITS."));

                std::string casaimage = subset.getString("casaimage","");
                std::string fitsimage = subset.getString("fitsimage","");
                unsigned int memoryInMB = subset.getUint("memoryInMB",64);
                bool preferVelocity = subset.getBool("preferVelocity",false);
                bool opticalVelocity = subset.getBool("opticalVelocity",true);
                int bitpix = subset.getInt("bitpix",-32);
                float minpix = subset.getFloat("minpix", 1.0);
                float maxpix = subset.getFloat("maxpix", -1.0);
                bool allowOverwrite = subset.getBool("allowOverwrite",false);
                bool degenerateLast = subset.getBool("degenerateLast",false);
                bool verbose = subset.getBool("verbose",true);
                bool stokesLast = subset.getBool("stokesLast",false);
                bool preferWavelength = subset.getBool("preferWavelength",false);
                bool airWavelength = subset.getBool("airWavelength",false);
                bool copyHistory = subset.getBool("copyHistory", true);

                std::string origin = ASKAP_PACKAGE_VERSION;

                if (bitpix!=-32 && bitpix!=16){
                    ASKAPTHROW(AskapError, "BITPIX can only be -32 or 16.");
                }

                casa::String errorMsg;
                bool returnVal;

                casa::PagedImage<float> casaim(casaimage);
                casa::TableRecord miscinfo = casaim.miscInfo();
                
                std::vector<std::string> headersToUpdate = subset.getStringVector("headers","");
                if (headersToUpdate.size()>0){
                    // There are headers we want to update
                    for(std::vector<std::string>::iterator head=headersToUpdate.begin();
                        head<headersToUpdate.end();head++){
                        std::string val = subset.getString("headers."+*head,"");
                        if (val!=""){
                            miscinfo.define(*head,val);
                        }
                    }
                    casaim.setMiscInfo(miscinfo);
                }

                std::vector<std::string> historyMessages = subset.getStringVector("history","");
                if(historyMessages.size()>0){
                    casa::LogIO log=casaim.logSink();
                    for(std::vector<std::string>::iterator history=historyMessages.begin();
                        history<historyMessages.end();history++){
                        log << *history<< casa::LogIO::POST;
                    }
                }
                        
                
                returnVal = casacore::ImageFITSConverter::ImageToFITS(errorMsg, casaim, fitsimage,
                                                                      memoryInMB, preferVelocity, opticalVelocity,
                                                                      bitpix, minpix, maxpix, allowOverwrite,
                                                                      degenerateLast, verbose, stokesLast,
                                                                      preferWavelength, airWavelength,
                                                                      origin, copyHistory);

                if(!returnVal){
                    ASKAPTHROW(AskapError, errorMsg);
                }

                
                 stats.logSummary();
               ///==============================================================================
            } catch (const askap::AskapError& x) {
                ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
                std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
                exit(1);
            } catch (const std::exception& x) {
                ASKAPLOG_FATAL_STR(logger,
                                   "Unexpected exception in " << argv[0] << ": " << x.what());
                std::cerr << "Unexpected exception in " << argv[0] << ": " <<
                          x.what() << std::endl;
                exit(1);
            }

            return 0;
        }
};

int main(int argc, char *argv[])
{
    ConvertApp app;
    return app.main(argc, argv);
}

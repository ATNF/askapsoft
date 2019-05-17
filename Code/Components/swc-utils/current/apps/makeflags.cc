/// @file 
///
/// @brief tool to extract flagging information from "waterfall" image
/// @details This application builds flagging information. We may evolve it to
/// something more flexible, but at this stage we expect to flag anything which is 
/// bad in any plane.
///
/// @copyright (c) 2007 CSIRO
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

// own includes
#include <askap/askap/AskapError.h>
#include <askap_swcorrelator.h>
#include <askap/askap/AskapLogging.h>
#include <askap/scimath/utils/MultiDimArrayPlaneIter.h>

// casa includes
#include <casacore/casa/OS/Timer.h>
#include <casacore/tables/Tables/Table.h>
#include <casacore/tables/Tables/TableError.h>
#include <casacore/tables/Tables/TableRecord.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/Cube.h>
#include <casacore/casa/Arrays/MatrixMath.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <casacore/images/Images/PagedImage.h>

// other 3rd party
#include <askap/askapparallel/AskapParallel.h>
#include <CommandLineParser.h>


#include <set>

ASKAP_LOGGER(logger, ".makeflags");

using namespace askap;
//using namespace askap::swcorrelator;

void process(const std::string &fname) 
{
  const float threshold = 0.3;
  ASKAPLOG_INFO_STR(logger,  "Extracting flags "<<fname<<" threshold: "<<threshold);
  casacore::PagedImage<casacore::Float> img(fname);
  casacore::Array<casacore::Float> pixels;
  img.get(pixels);
  const casacore::IPosition shape = pixels.shape();
  ASKAPLOG_INFO_STR(logger, "Input shape: "<<shape);
  ASKAPDEBUGASSERT(shape.nelements()>=2);
  std::set<casacore::uInt> bad_channels;
  casacore::Matrix<casacore::Float> peaks(shape[0],shape.nelements()>2 ? shape[2] : 1,0.);
  
  for (scimath::MultiDimArrayPlaneIter iter(shape); iter.hasMore(); iter.next()) {
       casacore::Array<casacore::Float> thisPlane = iter.getPlane(pixels).nonDegenerate();
       ASKAPDEBUGASSERT(thisPlane.shape().nelements() == 2);
       for (casacore::uInt ch = 0; ch < casacore::uInt(shape[0]); ++ch) {
            casacore::Vector<casacore::Float> thisChan = casacore::Matrix<casacore::Float>(thisPlane).row(ch);
            ASKAPDEBUGASSERT(ch < peaks.nrow());
            ASKAPDEBUGASSERT(iter.sequenceNumber() < peaks.ncolumn());
            for (casacore::uInt tm = 0; tm<thisChan.nelements(); ++tm) {
                 if (thisChan[tm] > threshold) {
                     bad_channels.insert(ch);
                 }
                 if (thisChan[tm] > peaks(ch,iter.sequenceNumber())) {
                     peaks(ch,iter.sequenceNumber()) = thisChan[tm];
                 }
            }
       }    
  }

  std::ofstream os("flags.dat"); 
  for (std::set<casacore::uInt>::const_iterator ci = bad_channels.begin(); ci != bad_channels.end(); ++ci) {
       os<<*ci<<std::endl;
  }
  ASKAPLOG_INFO_STR(logger, "Total number of channels to be flagged: "<<bad_channels.size()<<" out of "<<shape[0]<<" present");
  std::ofstream os2("peaks.dat");
  for (casacore::uInt ch=0; ch<peaks.nrow(); ++ch) {
       os2<<ch;
       for (casacore::uInt bsln = 0; bsln < peaks.ncolumn(); ++bsln) {
            os2<<" "<<peaks(ch,bsln);
       }
       os2<<std::endl;
  }
}


// Main function
int main(int argc, const char** argv)
{
    // This class must have scope outside the main try/catch block
    askap::askapparallel::AskapParallel comms(argc, argv);
    
    try {
       casacore::Timer timer;
       timer.mark();
       
       cmdlineparser::Parser parser; // a command line parser
       // command line parameter
       cmdlineparser::GenericParameter<std::string> imgFileName;
       // this parameter is optional
       parser.add(imgFileName, cmdlineparser::Parser::throw_exception);

       parser.process(argc, argv);
       
       process(imgFileName.getValue());       
       
       ASKAPLOG_INFO_STR(logger,  "Total times - user:   " << timer.user() << " system: " << timer.system()
                          << " real:   " << timer.real());
        ///==============================================================================
    } catch (const cmdlineparser::XParser &ex) {
        ASKAPLOG_FATAL_STR(logger, "Command line parser error, wrong arguments " << argv[0]);
        ASKAPLOG_FATAL_STR(logger, "Usage: " << argv[0] << " waterfall_plot.img");
        return 1;
    } catch (const askap::AskapError& x) {
        ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
        return 1;
    } catch (const std::exception& x) {
        ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
        return 1;
    }

    return 0;
    
}


       

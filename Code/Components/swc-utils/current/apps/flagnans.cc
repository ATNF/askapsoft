/// @file 
///
/// @brief flags visibilities which contain NaN
/// @details This application is intended to fix flag and data column. Some datasets were
/// found to contain NaNs for some reason which complicates processing. This application
/// replaces NaNs with zeros and flags the appropriate point.
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



// other 3rd party
#include <askap/askapparallel/AskapParallel.h>
#include <CommandLineParser.h>

ASKAP_LOGGER(logger, ".flagnans");

using namespace askap;
//using namespace askap::swcorrelator;

void process(const std::string &fname) 
{
  ASKAPLOG_INFO_STR(logger,  "Searching "<<fname<<" for NaNs and flagging appropriate points");
  casacore::Table ms(fname, casacore::Table::Update);
  
  casacore::ArrayColumn<casacore::Bool> flagCol(ms, "FLAG");
  casacore::ArrayColumn<casacore::Complex> visCol(ms, "DATA");
  
  ASKAPLOG_INFO_STR(logger,"Total number of rows in the measurement set: "<<ms.nrow());
  size_t nFlagged = 0;
  size_t nAlreadyFlagged = 0;

  for (casacore::uInt row = 0; row<ms.nrow(); ++row) {
       casacore::Array<casacore::Bool> flagBuf;
       flagCol.get(row,flagBuf);

       casacore::Array<casacore::Complex> visBuf;
       visCol.get(row, visBuf);

       bool changed = false;

       ASKAPDEBUGASSERT(flagBuf.shape().nelements() == 2);
       ASKAPDEBUGASSERT(visBuf.shape().nelements() == 2);
       ASKAPDEBUGASSERT(visBuf.shape() == flagBuf.shape());

       casacore::Matrix<casacore::Complex> vis(visBuf);
       casacore::Matrix<casacore::Bool> flag(flagBuf);
       for (casacore::uInt ch=0; ch<vis.nrow(); ++ch) {
            for (casacore::uInt pol=0; pol<vis.ncolumn(); ++pol) {
                 if (std::isnan(real(vis(ch,pol))) || std::isnan(imag(vis(ch,pol)))) {
                     changed = true;
                     vis(ch,pol) = casacore::Complex(0.,0.);
                     if (flag(ch,pol)) {
                         ++nAlreadyFlagged;
                     } else {
                         flag(ch,pol) = true;
                         ++nFlagged;
                     }
                 }
            }
       }
            
       if (changed) {
           flagCol.put(row,flagBuf);
           visCol.put(row,visBuf);
       }
  }
  ASKAPLOG_INFO_STR(logger,"Total number of NaNs found: "<<(nFlagged + nAlreadyFlagged));
  ASKAPLOG_INFO_STR(logger,"  Already flagged: "<<nAlreadyFlagged);
  ASKAPLOG_INFO_STR(logger,"  Newly flagged: "<<nFlagged);
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
       cmdlineparser::GenericParameter<std::string> msFileName;
       // this parameter is optional
       parser.add(msFileName, cmdlineparser::Parser::throw_exception);

       parser.process(argc, argv);
       
       process(msFileName.getValue());       
       
       ASKAPLOG_INFO_STR(logger,  "Total times - user:   " << timer.user() << " system: " << timer.system()
                          << " real:   " << timer.real());
        ///==============================================================================
    } catch (const cmdlineparser::XParser &ex) {
        ASKAPLOG_FATAL_STR(logger, "Command line parser error, wrong arguments " << argv[0]);
        ASKAPLOG_FATAL_STR(logger, "Usage: " << argv[0] << " measurement_set_to_change");
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


       

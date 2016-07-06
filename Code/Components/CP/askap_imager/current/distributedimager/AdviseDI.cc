/// @file
///
/// Support for parallel statistics accumulation to advise on imaging parameters
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
/// @author Stephen Ord <stephen.ord@csiro.au>
///

#include <distributedimager/AdviseDI.h>
#include <askap/AskapError.h>
#include <measurementequation/SynthesisParamsHelper.h>
#include <dataaccess/TableDataSource.h>
#include <dataaccess/ParsetInterface.h>
#include <dataaccess/SharedIter.h>
#include <askap_synthesis.h>
#include <askap/AskapLogging.h>


ASKAP_LOGGER(logger, ".adviseDI");

#include <profile/AskapProfiler.h>


#include <fitting/INormalEquations.h>
#include <fitting/Solver.h>

#include <casacore/casa/aips.h>
#include <casacore/casa/OS/Timer.h>
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"

#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>


#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

namespace askap {

namespace synthesis {


// actual AdviseDI implementation

/// @brief Constructor from ParameterSet
/// @details The parset is used to construct the internal state. We could
/// also support construction from a python dictionary (for example).
/// This is needed becuase the default AdviseParallel assumes a master/worker
/// distribution that may not be the case.

/// The command line inputs are needed solely for MPI - currently no
/// application specific information is passed on the command line.
/// @param comms communication object
/// @param parset ParameterSet for inputs
AdviseDI::AdviseDI(askap::askapparallel::AskapParallel& comms, const LOFAR::ParameterSet& parset) :
    AdviseParallel(comms,parset),itsParset(parset)
{

}

void AdviseDI::addMissingParameters()
{
   // this assumes only a sinlge spectral window - must generalise

   // Read from the configruation the list of datasets to process
   const vector<string> ms = getDatasets();
   casa::uInt srow = 0;

   vector<double> chanFreq;
   vector<double> chanWidth;
   vector<double> effectiveBW;
   vector<double> resolution;
   vector<double> centre;

   // Not really sure what to do for multiple ms or what that means in this
   // context... but i'm doing it any way - probably laying a trap for myself

   // Iterate over all measurement sets

   ASKAPCHECK(ms.size() == 1,"More than one measurement set not supported in adviseDI");


   for (unsigned int n = 0; n < ms.size(); ++n) {
   // Open the input measurement set
      const casa::MeasurementSet in(ms[n]);
      const casa::ROMSColumns srcCols(in);
      const casa::ROMSSpWindowColumns& sc = srcCols.spectralWindow();
      const casa::ROMSFieldColumns& fc = srcCols.field();

      const casa::uInt totChanIn = casa::ROScalarColumn<casa::Int>(in.spectralWindow(),"NUM_CHAN")(0);

      srow = sc.nrow()-1;

      ASKAPCHECK(srow==0,"More than one spectral window not currently supported in adviseDI");

      ASKAPLOG_INFO_STR(logger,
                        "Number of channels in " << ms << " is " << totChanIn);

      chanFreq.resize(totChanIn);
      chanWidth.resize(totChanIn);
      effectiveBW.resize(totChanIn);
      resolution.resize(totChanIn);

      for (uint i = 0; i < totChanIn; ++i) {
         chanFreq[i] = sc.chanFreq()(srow)(casa::IPosition(1, i));
         chanWidth[i] = sc.chanWidth()(srow)(casa::IPosition(1, i));
         effectiveBW[i] = sc.effectiveBW()(srow)(casa::IPosition(1, i));
         resolution[i] = sc.resolution()(srow)(casa::IPosition(1, i));

      }
      if (n == 0) {
         const casa::Vector<casa::MDirection> dirVec = fc.phaseDirMeasCol()(0);
         itsTangent = dirVec(0).getValue();
      }




   }

   ASKAPLOG_INFO_STR(logger, "Assumed tangent point: "<<printDirection(itsTangent)<<" (J2000)");
   ASKAPCHECK(itsParset.isDefined("Channels"),"Channels keyword not supplied in parset");

   std::vector<LOFAR::uint32> chans = itsParset.getUint32Vector("Channels");

   ASKAPLOG_INFO_STR(logger,"Channel list" << chanFreq);
   ASKAPLOG_INFO_STR(logger,"Channel selection " << chans);

   ASKAPCHECK(chans[0] == 1,"More than one channel wide not supported");

   casa::uInt ChanIn = chanFreq.size();


   channel = chans[1]-1; // FIXME: check this offset - I hope ...
   if (channel < 0) {
      // this is a master - give it the average frequency
      if (chanFreq[0] < chanFreq[ChanIn-1]) {
         minFrequency = chanFreq[0] - (chanWidth[0]/2.);
         maxFrequency = chanFreq[ChanIn-1] + (chanWidth[0]/2.);
      }
      else {
         minFrequency = chanFreq[ChanIn-1];
         maxFrequency = chanFreq[0];
      }
   }
   else {
      minFrequency = chanFreq[channel] - (chanWidth[channel]/2.);
      maxFrequency = chanFreq[channel] + (chanWidth[channel]/2.);
   }
   double refFreq = 0.5*(chanFreq[0] + chanFreq[ChanIn-1]);


   // test for missing image-specific parameters:

   // these parameters can be set globally or individually
   bool cellsizeNeeded = false;
   bool shapeNeeded = false;
   int nTerms = 1;

   string param;


   const vector<string> imageNames = itsParset.getStringVector("Images.Names", false);

   for (size_t img = 0; img < imageNames.size(); ++img) {

      param = "Images."+imageNames[img]+".cellsize";
      if ( !itsParset.isDefined(param) ) cellsizeNeeded = true;

      param = "Images."+imageNames[img]+".shape";
      if ( !itsParset.isDefined(param) ) shapeNeeded = true;

      param = "Images."+imageNames[img]+".frequency";
      if ( !itsParset.isDefined(param) ) {
         const string key="Images."+imageNames[img]+".frequency";
         char tmp[64];
         // changing this to match adviseParallel
         const double aveFreq = 0.5*(minFrequency+maxFrequency);
         sprintf(tmp,"[%f,%f]",aveFreq,aveFreq);
         string val = string(tmp);
         itsParset.add(key,val);

      }
      param ="Images."+imageNames[img]+".direction";
      if ( !itsParset.isDefined(param) ) {
         std::ostringstream pstr;
         // Only J2000 is implemented at the moment.
         pstr<<"["<<printLon(itsTangent)<<", "<<printLat(itsTangent)<<", J2000]";
         ASKAPLOG_INFO_STR(logger, "  Advising on parameter " << param << ": " << pstr.str().c_str());
         itsParset.add(param, pstr.str().c_str());
      }
      param = "Images."+imageNames[img]+".nterms"; // if nterms is set, store it for later
      if (itsParset.isDefined(param)) {
         if ((nTerms>1) && (nTerms!=itsParset.getInt(param))) {
            ASKAPLOG_WARN_STR(logger, "  Imaging with different nterms may not work");
         }
         nTerms = itsParset.getInt(param);
      }

      if ( !itsParset.isDefined("Images."+imageNames[img]+".nchan") ) {

      }
   }

   if (nTerms > 1) { // check required MFS parameters
      param = "visweights"; // set to "MFS" if unset and nTerms > 1
      if (!itsParset.isDefined(param)) {
         std::ostringstream pstr;
         pstr<<"MFS";
         ASKAPLOG_INFO_STR(logger, "  Advising on parameter " << param <<": " << pstr.str().c_str());
         itsParset.add(param, pstr.str().c_str());
      }

      param = "visweights.MFS.reffreq"; // set to average frequency if unset and nTerms > 1
      if ((itsParset.getString("visweights")=="MFS") && !itsParset.isDefined(param)) {
         std::ostringstream pstr;

         pstr<<refFreq;
         ASKAPLOG_INFO_STR(logger, "  Advising on parameter " << param <<": " << pstr.str().c_str());
         itsParset.add(param, pstr.str().c_str());
      }
   }

   // test for general missing parameters:
   if ( cellsizeNeeded && !itsParset.isDefined("nUVWMachines") ) {

   } else if ( cellsizeNeeded && !itsParset.isDefined("Images.cellsize") ) {

   } else if ( shapeNeeded && !itsParset.isDefined("Images.shape") ) {

   }

}
   // Utility function to get dataset names from parset.
std::vector<std::string> AdviseDI::getDatasets()
{
   if (itsParset.isDefined("dataset") && itsParset.isDefined("dataset0")) {
      ASKAPTHROW(std::runtime_error,
                  "Both dataset and dataset0 are specified in the parset");
   }

   // First look for "dataset" and if that does not exist try "dataset0"
   vector<string> ms;
   if (itsParset.isDefined("dataset")) {
      ms = itsParset.getStringVector("dataset", true);
   } else {
      string key = "dataset0";   // First key to look for
      long idx = 0;
      while (itsParset.isDefined(key)) {
         const string value = itsParset.getString(key);
         ms.push_back(value);

         LOFAR::ostringstream ss;
         ss << "dataset" << idx + 1;
         key = ss.str();
         ++idx;
      }
   }

   return ms;
}

} // namespace synthesis

} // namespace askap

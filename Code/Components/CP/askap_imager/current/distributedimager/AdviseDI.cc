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

#include <casacore/casa/BasicSL.h>
#include <casacore/casa/aips.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/ms/MeasurementSets/MSColumns.h>
#include <casacore/ms/MSOper/MSReader.h>
#include <casacore/casa/Arrays/ArrayIO.h>
#include <casacore/casa/iostream.h>
#include <casacore/casa/namespace.h>
#include <casacore/casa/Quanta/MVTime.h>

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

void AdviseDI::prepare() {
    // this assumes only a sinlge spectral window - must generalise

    // Read from the configruation the list of datasets to process
    const vector<string> ms = getDatasets();
    casa::uInt srow = 0;

    chanFreq.resize(0);
    chanWidth.resize(0);
    effectiveBW.resize(0);
    resolution.resize(0);
    centre.resize(0);

    // Not really sure what to do for multiple ms or what that means in this
    // context... but i'm doing it any way - probably laying a trap for myself

    // Iterate over all measurement sets
    // combine all the channels into a list...

    casa::uInt totChanIn = 0;

    for (unsigned int n = 0; n < ms.size(); ++n) {
    // Open the input measurement set
       const casa::MeasurementSet in(ms[n]);
       const casa::ROMSColumns srcCols(in);
       const casa::ROMSSpWindowColumns& sc = srcCols.spectralWindow();
       const casa::ROMSFieldColumns& fc = srcCols.field();
       const casa::ROMSObservationColumns& oc = srcCols.observation();
       const casa::ROMSAntennaColumns& ac = srcCols.antenna();
       const casa::ROArrayColumn<casa::Double> times = casa::ROArrayColumn<casa::Double>(oc.timeRange());
       const casa::ROArrayColumn<casa::Double> ants = casa::ROArrayColumn<casa::Double>(ac.position());
       const casa::uInt thisRef = casa::ROScalarColumn<casa::Int>(in.spectralWindow(),"MEAS_FREQ_REF")(0);
       const  casa::uInt thisChanIn = casa::ROScalarColumn<casa::Int>(in.spectralWindow(),"NUM_CHAN")(0);
       srow = sc.nrow()-1;

       ASKAPCHECK(srow==0,"More than one spectral window not currently supported in adviseDI");

       for (uint i = 0; i < thisChanIn; ++i) {
          chanFreq.push_back(sc.chanFreq()(srow)(casa::IPosition(1, i)));
          chanWidth.push_back(sc.chanWidth()(srow)(casa::IPosition(1, i)));
          effectiveBW.push_back(sc.effectiveBW()(srow)(casa::IPosition(1, i)));
          resolution.push_back(sc.resolution()(srow)(casa::IPosition(1, i)));
       }

       totChanIn = totChanIn + thisChanIn;


       if (n == 0) {
           itsDirVec = fc.phaseDirMeasCol()(0);
           itsTangent = itsDirVec(0).getValue();

           // Read the position on Antenna 0
           Array<casa::Double> posval;
           ants.get(0,posval,true);
           vector<double> pval = posval.tovector();

           MVPosition mvobs(Quantity(pval[0], "m").getBaseValue(),
           Quantity(pval[1], "m").getBaseValue(),
           Quantity(pval[2], "m").getBaseValue());

           itsPosition = MPosition(mvobs,casa::MPosition::ITRF);

           // Get the Epoch
           Array<casa::Double> tval;
           vector<double> tvals;

           times.get(0,tval,true);
           tvals = tval.tovector();
           double mjd = tvals[0]/(86400.);
           casa::MVTime dat(mjd);

           itsEpoch = MVEpoch(dat.day());

           itsRef = thisRef;
       }
       else {
           ASKAPLOG_WARN_STR(logger,"Assuming subsequent measurement sets share Epoch,Position and Direction");
       }

    }


    ASKAPLOG_INFO_STR(logger, "Assumed tangent point: "<<printDirection(itsTangent)<<" (J2000)");



    // Lets build a barycentric channel list
    MeasFrame itsFrame(MEpoch(itsEpoch),itsPosition,itsDirVec[0]);
    MFrequency::Ref refin(MFrequency::castType(itsRef),itsFrame);
    MFrequency::Ref refout(MFrequency::BARY,itsFrame);

    MFrequency::Convert forw(refin,refout);
    MFrequency::Convert backw(refout,refin);

    itsBaryFrequencies.resize(0);
    itsTopoFrequencies.resize(0);
    bool barycentre = itsParset.getBool("barycentre",false);

    for (unsigned int ch = 0; ch < chanFreq.size(); ++ch) {
        itsBaryFrequencies.push_back(forw(chanFreq[ch]).getValue());
        itsTopoFrequencies.push_back(MFrequency(MVFrequency(chanFreq[ch]),refin));


        if (barycentre) {
            // correct the internal arrays
            const MVFrequency botThisChan = chanFreq[ch]-chanWidth[ch]/2.0;
            const MVFrequency topThisChan = chanFreq[ch]+chanWidth[ch]/2.0;
            casa::MFrequency botThisMF(botThisChan,refin);
            casa::MFrequency topThisMF(topThisChan,refin);
            casa::MFrequency botBary = forw(botThisMF).getValue();
            casa::MFrequency topBary = forw(topThisMF).getValue();
            casa::MFrequency centreBary = forw(chanFreq[ch]).getValue();
            chanFreq[ch] = centreBary.getValue();
            chanWidth[ch] = abs(topBary.getValue() - botBary.getValue());

        }
        ASKAPLOG_INFO_STR(logger,"Topocentric Channel " << ch << ":" << itsTopoFrequencies[ch]);
        ASKAPLOG_INFO_STR(logger,"Barycentric Channel " << ch << ":" << itsBaryFrequencies[ch]);
    }




}
void AdviseDI::addMissingParameters()
{

    this->prepare();


    ASKAPCHECK(itsParset.isDefined("Channels"),"Channels keyword not supplied in parset");

    std::vector<LOFAR::uint32> chans = itsParset.getUint32Vector("Channels");


    ASKAPLOG_INFO_STR(logger,"Channel selection " << chans);

    ASKAPCHECK(chans[0] == 1,"More than one channel wide not supported");


    int ChanIn = chanFreq.size();

    int channel = chans[1]-1; // FIXME: check this offset - I hope ...

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

    refFreq = 0.5*(chanFreq[0] + chanFreq[ChanIn-1]);

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

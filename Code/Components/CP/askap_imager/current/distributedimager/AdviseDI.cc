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

bool custom_compare (const casa::MFrequency& X, const casa::MFrequency& Y) {
    return (X.getValue() == Y.getValue());
}
bool custom_lessthan (const casa::MFrequency& X, const casa::MFrequency& Y) {
    return (X.getValue() < Y.getValue());
}
bool in_range(double end1, double end2, double test) {


    if (test >= end1 && test <= end2) {
        ASKAPLOG_INFO_STR(logger,"Test frequency " << test \
        << " between " << end1 << " and " << end2);
        return true;
    }
    if (test <= end1 && test >= end2) {
        ASKAPLOG_INFO_STR(logger,"Test frequency " << test \
        << " between " << end1 << " and " << end2);
        return true;
    }
    ASKAPLOG_INFO_STR(logger,"Test frequency " << test \
    << " NOT between " << end1 << " and " << end2);
    return false;
}
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
AdviseDI::AdviseDI(askap::cp::CubeComms& comms, const LOFAR::ParameterSet& parset) :
    AdviseParallel(comms,parset),itsParset(parset)
{
    isPrepared = false;
    barycentre = false;
    itsWorkUnitCount=0;
}

void AdviseDI::prepare() {
    // this assumes only a sinlge spectral window - must generalise

    // Read from the configruation the list of datasets to process
    const vector<string> ms = getDatasets();

    unsigned int nWorkers = itsComms.nProcs() - 1;
    unsigned int nWorkersPerGroup = nWorkers/itsComms.nGroups();
    unsigned int nGroups = itsComms.nGroups();
    const int nchanpercore = itsParset.getInt32("nchanpercore", 1);
    const int nwriters = itsParset.getInt32("nwriters", 1);
    unsigned int nWorkersPerWriter = floor(nWorkers / nwriters);

    if (nWorkersPerWriter < 1) {
        nWorkersPerWriter = 1;
    }

    casa::uInt srow = 0;
    chanFreq.resize(ms.size());
    chanWidth.resize(ms.size());
    effectiveBW.resize(ms.size());
    resolution.resize(ms.size());
    centre.resize(ms.size());

    // Not really sure what to do for multiple ms or what that means in this
    // context... but i'm doing it any way - probably laying a trap for myself

    // Iterate over all measurement sets
    // combine all the channels into a list...
    // these measurement sets may now be from different epochs they should not
    // have different channel ranges - but it is possible that the channel range
    // may have been broken up into chunks.

    // need to calculate the allocations too.

    casa::uInt totChanIn = 0;

    for (unsigned int n = 0; n < ms.size(); ++n) {
        chanFreq[n].resize(0);
        chanWidth[n].resize(0);
        effectiveBW[n].resize(0);
        resolution[n].resize(0);
        centre[n].resize(0);
    // Open the input measurement set
        ASKAPLOG_INFO_STR(logger, "Opening " << ms[n] << " filecount " << n );
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
          chanFreq[n].push_back(sc.chanFreq()(srow)(casa::IPosition(1, i)));
          chanWidth[n].push_back(sc.chanWidth()(srow)(casa::IPosition(1, i)));
          effectiveBW[n].push_back(sc.effectiveBW()(srow)(casa::IPosition(1, i)));
          resolution[n].push_back(sc.resolution()(srow)(casa::IPosition(1, i)));
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
        ASKAPLOG_INFO_STR(logger, "Completed filecount " << n);
    }


    ASKAPLOG_INFO_STR(logger, "Assuming tangent point: "<<printDirection(itsTangent)<<" (J2000)");



    // Lets build a barycentric channel list
    MeasFrame itsFrame(MEpoch(itsEpoch),itsPosition,itsDirVec[0]);
    MFrequency::Ref refin(MFrequency::castType(itsRef),itsFrame);
    MFrequency::Ref refout(MFrequency::BARY,itsFrame);

    MFrequency::Convert forw(refin,refout);
    MFrequency::Convert backw(refout,refin);

    itsBaryFrequencies.resize(0);
    itsTopoFrequencies.resize(0);
    barycentre = itsParset.getBool("barycentre",false);

    // we now have each topocentric channel from each MS
    // in a unique array.
    // first we need to sort and uniqify the list
    // then resize the list to get the channel range.
    // This is required becuase we are trying to form a unique
    // reference channel list from the input measurement sets

    // This first loop just appends all the frequencies into 2 single arrays
    // the list of TOPO and BARY frequencies.


    itsAllocatedFrequencies.resize(nWorkersPerGroup);
    itsAllocatedWork.resize(nWorkers);

    for (unsigned int n = 0; n < ms.size(); ++n) {
        for (unsigned int ch = 0; ch < chanFreq[n].size(); ++ch) {
            itsBaryFrequencies.push_back(forw(chanFreq[n][ch]).getValue());
            itsTopoFrequencies.push_back(MFrequency(MVFrequency(chanFreq[n][ch]),refin));


            if (barycentre) {
                // correct the internal arrays
                const MVFrequency botThisChan = chanFreq[n][ch]-chanWidth[n][ch]/2.0;
                const MVFrequency topThisChan = chanFreq[n][ch]+chanWidth[n][ch]/2.0;
                casa::MFrequency botThisMF(botThisChan,refin);
                casa::MFrequency topThisMF(topThisChan,refin);
                casa::MFrequency botBary = forw(botThisMF).getValue();
                casa::MFrequency topBary = forw(topThisMF).getValue();
                casa::MFrequency centreBary = forw(chanFreq[n][ch]).getValue();
                chanFreq[n][ch] = centreBary.getValue();
                if (chanFreq[n].size() > 1)
                    chanWidth[n][ch] = abs(topBary.getValue() - botBary.getValue());

            }
        }
    }

    ///uniquifying the lists

    std::sort(itsBaryFrequencies.begin(),itsBaryFrequencies.end(), custom_lessthan);
    std::vector<casa::MFrequency>::iterator bary_it;
    bary_it = std::unique(itsBaryFrequencies.begin(),itsBaryFrequencies.end(),custom_compare);
    itsBaryFrequencies.resize(std::distance(itsBaryFrequencies.begin(),bary_it));

    std::sort(itsTopoFrequencies.begin(),itsTopoFrequencies.end(), custom_lessthan);
    std::vector<casa::MFrequency>::iterator topo_it;
    topo_it = std::unique(itsTopoFrequencies.begin(),itsTopoFrequencies.end(),custom_compare);
    itsTopoFrequencies.resize(std::distance(itsTopoFrequencies.begin(),topo_it));

    for (unsigned int ch = 0; ch < itsTopoFrequencies.size(); ++ch) {

        ASKAPLOG_INFO_STR(logger,"Topocentric Channel " << ch << ":" << itsTopoFrequencies[ch]);
        ASKAPLOG_INFO_STR(logger,"Barycentric Channel " << ch << ":" << itsBaryFrequencies[ch]);
        unsigned int allocation_index = floor(ch / nchanpercore);
        /// We allocate the frequencies based upon the topocentric range.
        /// We do this becuase it is easier for the user to understand.
        /// Plus - all beams will have the same allocation. Which will produce cubes/images
        /// that will easily merge.

        /// Beware the syntactic confusion here - we are allocating a frequency that is from
        /// the Topocentric list. But will match a channel based upon the barycentric frequency
        ASKAPLOG_INFO_STR(logger,"Allocating frequency "<< itsTopoFrequencies[ch].getValue() \
        << " to worker " << allocation_index+1);

        itsAllocatedFrequencies[allocation_index].push_back(itsTopoFrequencies[ch].getValue());
    }


    // Now for each allocated workunit we need to fill in the rest of the workunit
    // we now have a workUnit for each channel in the allocation - but not
    // for each Epoch.

    cp::CubeComms& itsCubeComms = dynamic_cast<cp::CubeComms&> (itsComms);

    for (int wrk = 0; wrk < nWorkersPerGroup; wrk=wrk+nWorkersPerWriter) {
        int mywriter = floor(wrk/nWorkersPerWriter)*nWorkersPerWriter + 1;
        itsCubeComms.addWriter(mywriter);
    }



    for (unsigned int work = 0; work < itsAllocatedFrequencies.size(); ++work) {
        ASKAPLOG_INFO_STR(logger,"Allocating frequency channels for worker " << work);
        // loop over the measurement sets and find the local channel number
        // associated with the barycentric channel

        vector<double>& thisAllocation = itsAllocatedFrequencies[work];

        for (unsigned int frequency=0;frequency < thisAllocation.size();++frequency) {

            // need to allocate the measurement sets for this channel to this allocation
            // this may require appending new work units.
            ASKAPLOG_INFO_STR(logger,"Allocating " << thisAllocation[frequency]);
            bool allocated = false;
            for (unsigned int set=0;set < ms.size();++set){
                int lc = 0;

                lc = match(set,thisAllocation[frequency]);
                if (lc >= 0) {
                    // there is a channel of this frequency in the measurement set


                    cp::ContinuumWorkUnit wu;

                    int mywriter = floor(work/nWorkersPerWriter)*nWorkersPerWriter + 1;

                    itsCubeComms.addChannelToWriter(mywriter);

                    wu.set_writer(mywriter);
                    wu.set_payloadType(cp::ContinuumWorkUnit::WORK);
                    wu.set_channelFrequency(thisAllocation[frequency]);

                    if (itsTopoFrequencies.size() > 1)
                        wu.set_channelWidth(fabs(itsTopoFrequencies[1].getValue() - itsTopoFrequencies[0].getValue()));
                    else
                        wu.set_channelWidth(fabs(chanWidth[0][0]));

                    wu.set_localChannel(lc);
                    wu.set_globalChannel(work);
                    wu.set_dataset(ms[set]);
                    itsAllocatedWork[work].push(wu);
                    itsWorkUnitCount++;
                    ASKAPLOG_INFO_STR(logger,"Allocating " << thisAllocation[frequency] \
                    << " with local channel " << lc << " of width " << wu.get_channelWidth() << " in set: " << ms[set] \
                    << " to worker " << work << "Count " << itsWorkUnitCount );
                    allocated = true;
                }
            }
            if (allocated == false)
            {
                ASKAPLOG_WARN_STR(logger,"Allocating FAIL Cannot match " << thisAllocation[frequency] \
                << " in any set: ");
                // warn it does not match ....
            }

        }

    }

    // expand the channels by the number of groups - this is cheap on memory and
    // allows easier indexing
    // But this is only really needed by the master
    if (itsComms.isMaster()) {
        for (int grp = 1; grp < itsComms.nGroups(); grp++) {
            for (int wrk = 0; wrk < nWorkersPerGroup; wrk++) {
                itsAllocatedWork[grp*nWorkersPerGroup+wrk] = itsAllocatedWork[wrk];

                itsWorkUnitCount=itsWorkUnitCount + itsAllocatedWork[wrk].size();

                ASKAPLOG_INFO_STR(logger,"Allocating worker " << grp*nWorkersPerGroup+wrk \
                << " the same units as worker " << wrk << " Count " << itsWorkUnitCount);
            }
        }
    }

    /// Now if required we need to allocate the writers for a parallel writers
    /// The writers do not need to be dedicated cores - they can write in addition
    /// to their other duties.



    isPrepared = true;
    ASKAPLOG_INFO_STR(logger, "Prepared the advice");
}
cp::ContinuumWorkUnit AdviseDI::getAllocation(int id) {
    cp::ContinuumWorkUnit rtn;
    if (itsAllocatedWork[id].empty() == true) {
        ASKAPLOG_INFO_STR(logger, "Stack is empty for " << id);
        rtn.set_payloadType(cp::ContinuumWorkUnit::DONE);
    }
    else {
        rtn = itsAllocatedWork[id].top();
        itsAllocatedWork[id].pop();
        itsWorkUnitCount--;
    }
    if (itsAllocatedWork[id].empty() == true) {
        // this is the last unitParset
        rtn.set_payloadType(cp::ContinuumWorkUnit::LAST);
    }
    return rtn;
}

int AdviseDI::match(int ms_number, casa::MVFrequency testFreq) {
    /// Which channel does the frequency correspond to.
    /// IF the barycentr flag has been set then this will match
    /// the barycentred channel to it.
    vector<double>::iterator it_current = chanFreq[ms_number].begin();
    vector<double>::iterator it_end = chanFreq[ms_number].end()-1;
    double testVal = testFreq.getValue();

    if (in_range(*it_current,*it_end,testVal)) {
        int ch = 0;
        it_current=chanFreq[ms_number].begin();
        for (ch=0 ; ch < chanFreq[ms_number].size(); ++ch) {
            ASKAPLOG_INFO_STR(logger, "looking for " << testVal << \
            " in test frequency channel " << *it_current << \
                " width " << chanWidth[ms_number][ch]);
            double one_edge = (*it_current) - chanWidth[ms_number][ch]/2.;
            double other_edge = (*it_current) + chanWidth[ms_number][ch]/2.;

            if (in_range(one_edge,other_edge,testVal)) {

                return ch;
            }
            it_current++;

        }
    }

    return -1;


}
void AdviseDI::addMissingParameters()
{
    if (isPrepared == false) {
        ASKAPLOG_INFO_STR(logger,"Running prepare from addMissingParameters");
        this->prepare();
    }

    ASKAPLOG_INFO_STR(logger,"Adding missing params ");

    std::vector<casa::MFrequency>::iterator begin_it;
    std::vector<casa::MFrequency>::iterator end_it;
    if (barycentre) {
        begin_it = itsBaryFrequencies.begin();
        end_it = itsBaryFrequencies.end();
    }
    else {
        begin_it = itsTopoFrequencies.begin();
        end_it = itsTopoFrequencies.end();

    }
    minFrequency = (*begin_it).getValue();
    maxFrequency = (*end_it).getValue();

    // FIXME problem .... this is probably the wrong refFreq. It needs to be for the whole
    // observation not just this allocation....
    // Currently I fix this by forcing it to be set in the Parset - not optimal

    refFreq = 0.5*(minFrequency + maxFrequency);

   // test for missing image-specific parameters:

   // these parameters can be set globally or individually
   bool cellsizeNeeded = false;
   bool shapeNeeded = false;
   int nTerms = 1;

   string param;


   const vector<string> imageNames = itsParset.getStringVector("Images.Names", false);

   param = "Images.direction";
   if ( !itsParset.isDefined(param) ) {
       std::ostringstream pstr;
       // Only J2000 is implemented at the moment.
       pstr<<"["<<printLon(itsTangent)<<", "<<printLat(itsTangent)<<", J2000]";
       ASKAPLOG_INFO_STR(logger, "  Advising on parameter " << param << ": " << pstr.str().c_str());
       itsParset.add(param, pstr.str().c_str());
   }
   param = "Images.restFrequency";

   if ( !itsParset.isDefined(param) ) {
       std::ostringstream pstr;
       // Only J2000 is implemented at the moment.
       pstr<<"HI";
       ASKAPLOG_INFO_STR(logger, "  Advising on parameter " << param << ": " << pstr.str().c_str());
       itsParset.add(param, pstr.str().c_str());
   }

   for (size_t img = 0; img < imageNames.size(); ++img) {

       param = "Images."+imageNames[img]+".cellsize";
       if ( !itsParset.isDefined(param) ) {
           cellsizeNeeded = true;
       }
       else {
            param = "Images.cellsize";
            if (!itsParset.isDefined(param) ) {
                const vector<string> cellSizeVector = itsParset.getStringVector("Images.cellsize");
                std::ostringstream pstr;
                pstr<<"["<< cellSizeVector[0].c_str() <<"arcsec,"<<cellSizeVector[1].c_str() <<"arcsec]";
                ASKAPLOG_INFO_STR(logger, "  Advising on parameter " << param <<": " << pstr.str().c_str());
                itsParset.add(param, pstr.str().c_str());
            }
       }
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
       if ((itsParset.getString("visweights")=="MFS")) {
           ASKAPCHECK(itsParset.isDefined(param),"Reference Frequency MUST be defined for MFS \
in distributed mode");

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

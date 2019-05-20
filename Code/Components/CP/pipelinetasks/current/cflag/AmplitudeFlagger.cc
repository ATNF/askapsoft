/// @file AmplitudeFlagger.cc
///
/// @copyright (c) 2013,2014 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "AmplitudeFlagger.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <limits>
#include <set>
#include <vector>
#include <cmath>

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "Common/ParameterSet.h"
#include "casacore/casa/aipstype.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/casa/Arrays/Cube.h"
#include "casacore/casa/Utilities/GenSort.h"

// Local package includes
#include "cflag/FlaggingStats.h"

ASKAP_LOGGER(logger, ".AmplitudeFlagger");

using namespace askap;
using namespace casacore;
using namespace askap::cp::pipelinetasks;

vector< boost::shared_ptr<IFlagger> > AmplitudeFlagger::build(
        const LOFAR::ParameterSet& parset,
        const casacore::MeasurementSet& /*ms*/)
{
    vector< boost::shared_ptr<IFlagger> > flaggers;
    const string key = "amplitude_flagger.enable";
    if (parset.isDefined(key) && parset.getBool(key)) {
        const LOFAR::ParameterSet subset = parset.makeSubset("amplitude_flagger.");
        flaggers.push_back(boost::shared_ptr<IFlagger>(new AmplitudeFlagger(subset)));
    }
    return flaggers;
}

AmplitudeFlagger::AmplitudeFlagger(const LOFAR::ParameterSet& parset)
        : itsStats("AmplitudeFlagger"),
          itsHasHighLimit(false), itsHasLowLimit(false),
          itsAutoThresholds(false), itsThresholdFactor(5.0),
          itsIntegrateSpectra(false), itsSpectraFactor(5.0),
          itsIntegrateTimes(false), itsTimesFactor(5.0),
          itsAveAll(false), itsAveAllButPol(false), itsAveAllButBeam(false),
          itsAverageFlagsAreReady(true)
{
    // check parset
    AmplitudeFlagger::loadParset(parset);
    // log parameter summary
    AmplitudeFlagger::logParsetSummary(parset);
}

FlaggingStats AmplitudeFlagger::stats(void) const
{
    return itsStats;
}

casacore::Bool AmplitudeFlagger::processingRequired(const casacore::uInt pass)
{
    if (itsIntegrateSpectra || itsIntegrateTimes) {
        return (pass<2);
    } else {
        return (pass<1);
    }
}

casacore::Vector<casacore::Int> AmplitudeFlagger::getStokesType(
        casacore::MSColumns& msc, const casacore::uInt row)
{
    const casacore::ROMSDataDescColumns& ddc = msc.dataDescription();
    const int dataDescId = msc.dataDescId()(row);
    const casacore::ROMSPolarizationColumns& polc = msc.polarization();
    const unsigned int descPolId = ddc.polarizationId()(dataDescId);
    return polc.corrType()(descPolId);
}

void AmplitudeFlagger::processRow(casacore::MSColumns& msc, const casacore::uInt pass,
                                  const casacore::uInt row, const bool dryRun)
{
    const Matrix<casacore::Complex> data = msc.data()(row);
    Matrix<casacore::Bool> flags = msc.flag()(row);

    // Only need to write out the flag matrix if it was updated
    bool wasUpdated = false;
    // Only set flagRow if all corr are flagged
    // Only looking for row flags in "itsAveTimes" data. Could generalise.
    bool leaveRowFlag = false;

    const casacore::Vector<casacore::Int> stokesTypesInt = getStokesType(msc, row);

    // normalise averages and search them for peaks to flag
    if ( !itsAverageFlagsAreReady && (pass==1) ) {
        ASKAPLOG_INFO_STR(logger, "Finalising averages at the start of pass "
            <<pass+1);
        setFlagsFromIntegrations();
    }

    // Iterate over rows (one row is one correlation product)
    for (size_t corr = 0; corr < data.nrow(); ++corr) {

        // If this row doesn't contain a product we are meant to be flagging,
        // then ignore it
        if (!itsStokes.empty() &&
            (itsStokes.find(Stokes::type(stokesTypesInt(corr))) ==
                itsStokes.end())) {
            leaveRowFlag = true;
            continue;
        }

        // return a tuple that indicate which integration this row is in
        rowKey key = getRowKey(msc, row, corr);

        // update a counter for this row and the storage vectors
        // do it before any processing that is dependent on "pass"
        if ( itsIntegrateTimes ) {
            updateTimeVectors(key, pass);
        }

        // if this is the first instance of this key, initialise storage vectors
        if ( itsIntegrateSpectra && (pass==0) &&
               (itsAveSpectra.find(key) == itsAveSpectra.end()) ) {
            initSpectrumVectors(key, data.row(0).shape());
        }

        // need temporary indicators that can be updated if necessary
        bool hasLowLimit = itsHasLowLimit;
        bool hasHighLimit = itsHasHighLimit;

        // get the spectrum
        casacore::Vector<casacore::Float>
            spectrumAmplitudes = casacore::amplitude(data.row(corr));

        // set a mask (only needed when averaging, so move this if need be)
        casacore::Vector<casacore::Bool>
            unflaggedMask = (flags.row(corr)==casacore::False);

        if ( itsAutoThresholds ) {
            // check that there is something to flag and continue if there isn't
            if (std::find(unflaggedMask.begin(),
                     unflaggedMask.end(), casacore::True) == unflaggedMask.end()) {
                itsStats.visAlreadyFlagged += data.ncolumn();
                if ( itsIntegrateTimes ) {
                   itsMaskTimes[key][itsCountTimes[key]] = casacore::False;
                }
                continue;
            }
        }

        // individual flagging and averages are only done during the first pass
        // could change this
        if ( pass==0 ) {

            if ( itsAutoThresholds ) {

                // combine amplitudes with mask and get the median-based statistics
                casacore::MaskedArray<casacore::Float>
                    maskedAmplitudes(spectrumAmplitudes, unflaggedMask);
                casacore::Vector<casacore::Float>
                    statsVector = getRobustStats(maskedAmplitudes);
                casacore::Float median = statsVector[0];
                casacore::Float sigma_IQR = statsVector[1];

                // set cutoffs
                if ( !hasLowLimit ) {
                    itsLowLimit = median-itsThresholdFactor*sigma_IQR;
                    hasLowLimit = casacore::True;
                }
                if ( !hasHighLimit ) {
                    itsHighLimit = median+itsThresholdFactor*sigma_IQR;
                    hasHighLimit = casacore::True;
                }

                // check min and max relative to thresholds, and do not loop over
                // data again if they are good. If indicies were also sorted, could
                // just test where the sorted amplitudes break the threshold...
                // ** cannot do this when averages are needed, or they'll be skipped **
                if (!itsIntegrateSpectra && !itsIntegrateTimes &&
                        (statsVector[2] >= itsLowLimit) &&
                        (statsVector[3] <= itsHighLimit)) {
                    continue;
                }

            }

            // only need these if itsIntegrateTimes
            casacore::Double aveTime = 0.0;
            casacore::uInt countTime = 0;

            // look for individual peaks and do any integrations
            for (size_t chan = 0; chan < data.ncolumn(); ++chan) {
                if (flags(corr, chan)) {
                    itsStats.visAlreadyFlagged++;
                    continue;
                }

                // look for individual peaks
                const float amp = spectrumAmplitudes(chan);
                if ((hasLowLimit && (amp < itsLowLimit)) ||
                    (hasHighLimit && (amp > itsHighLimit))) {
                    flags(corr, chan) = true;
                    wasUpdated = true;
                    itsStats.visFlagged++;
                }
                else if ( itsIntegrateSpectra || itsIntegrateTimes ) {
                    if ( itsIntegrateSpectra ) {
                        // do spectra integration
                        itsAveSpectra[key][chan] += amp;
                        itsCountSpectra[key][chan]++;
                        itsAverageFlagsAreReady = casacore::False;
                    }
                    if ( itsIntegrateTimes ) {
                        // do time-series integration
                        aveTime += amp;
                        countTime++;
                    }
                }

            }
            if ( itsIntegrateTimes ) {
                // normalise this average
                if ( countTime>0 ) {
                    itsAveTimes[key][itsCountTimes[key]] =
                        aveTime/casacore::Double(countTime);
                    itsMaskTimes[key][itsCountTimes[key]] = casacore::True;
                    itsAverageFlagsAreReady = casacore::False;
                }
                else {
                    itsMaskTimes[key][itsCountTimes[key]] = casacore::False;
                }
            }

        }
        else if ( (pass==1) &&  ( itsIntegrateSpectra || itsIntegrateTimes ) ) {
            // only flag unflagged data, so that new flags can be counted.
            // "flags" is true for flags, "mask*" are false for flags
            if ( itsIntegrateTimes ) {
                // apply itsMaskTimes flags. Could just use flagRow,
                // but not sure that all applications support flagRow
                if ( !itsMaskTimes[key][itsCountTimes[key]] ) {
                    for (size_t chan = 0; chan < data.ncolumn(); ++chan) {
                        if (flags(corr, chan)) continue;
                            flags(corr, chan) = true;
                            wasUpdated = true;
                            itsStats.visFlagged++;
                    }
                    // everything is flagged, so move to the next "corr"
                    continue;
                }
                // need this to be false for all "corr" to warrent flagRow
                else leaveRowFlag = true;
            }
            // apply itsIntegrateSpectra flags
            if ( itsIntegrateSpectra ) {
                for (size_t chan = 0; chan < data.ncolumn(); ++chan) {
                    if ( !flags(corr, chan) && !itsMaskSpectra[key][chan] ) {
                        flags(corr, chan) = true;
                        wasUpdated = true;
                        itsStats.visFlagged++;
                    }
                }
            }
        }

    }

    if (wasUpdated && itsIntegrateTimes && !leaveRowFlag && (pass==1)) {
        itsStats.rowsFlagged++;
        if (!dryRun) {
            msc.flagRow().put(row, true);
        }
    }
    if (wasUpdated && !dryRun) {
        msc.flag().put(row, flags);
    }
}


void AmplitudeFlagger::processRows(casacore::MSColumns& msc, const casacore::uInt pass,
                                  const casacore::uInt row, const casacore::uInt nrow,
                                  const bool dryRun)
{
    Slicer rowSlicer(Slice(row,nrow));
    const Cube<casacore::Complex> data = msc.data().getColumnRange(rowSlicer);
    Cube<casacore::Bool> flags = msc.flag().getColumnRange(rowSlicer);
    casacore::uInt nPol = flags.shape()(0);
    casacore::uInt nChan = flags.shape()(1);

    // Only need to write out the flag matrix if it was updated
    bool wasUpdated = false;

    const casacore::Vector<casacore::Int> stokesTypesInt = getStokesType(msc, row);

    for (casacore::uInt k = 0; k < nrow; k++) {

        // Only set flagRow if all corr are flagged
        // Only looking for row flags in "itsAveTimes" data. Could generalise.
        bool leaveRowFlag = false;
        bool wasUpdatedRow = false;
        // normalise averages and search them for peaks to flag
        if ( !itsAverageFlagsAreReady && (pass==1) ) {
            ASKAPLOG_INFO_STR(logger, "Finalising averages at the start of pass "
                <<pass+1);
            setFlagsFromIntegrations();
        }

        // Iterate over rows (one row is one correlation product)
        for (size_t corr = 0; corr < nPol; ++corr) {

            // If this row doesn't contain a product we are meant to be flagging,
            // then ignore it
            if (!itsStokes.empty() &&
                (itsStokes.find(Stokes::type(stokesTypesInt(corr))) ==
                    itsStokes.end())) {
                leaveRowFlag = true;
                continue;
            }

            // return a tuple that indicate which integration this row is in
            rowKey key = getRowKey(msc, row + k, corr);

            // update a counter for this row and the storage vectors
            // do it before any processing that is dependent on "pass"
            if ( itsIntegrateTimes ) {
                updateTimeVectors(key, pass);
            }

            // if this is the first instance of this key, initialise storage vectors
            if ( itsIntegrateSpectra && (pass==0) &&
                   (itsAveSpectra.find(key) == itsAveSpectra.end()) ) {
                initSpectrumVectors(key, IPosition(1,nChan));
            }

            // need temporary indicators that can be updated if necessary
            bool hasLowLimit = itsHasLowLimit;
            bool hasHighLimit = itsHasHighLimit;


            // set a mask (only needed when averaging, so move this if need be)
            casacore::Vector<casacore::Bool> unflaggedMask(nChan,casacore::False);
            bool allFlagged = true;
            for (uInt chan = 0; chan < nChan; chan++) {
                if (!flags(corr,chan,k)) {
                    unflaggedMask(chan) = casacore::True;
                    allFlagged = false;
                }
            }
            if ( itsAutoThresholds ) {
                // check that there is something to flag and continue if there isn't
                if (allFlagged) {
                    itsStats.visAlreadyFlagged += nChan;
                    if ( itsIntegrateTimes ) {
                       itsMaskTimes[key][itsCountTimes[key]] = casacore::False;
                    }
                    continue;
                }
            }

            // individual flagging and averages are only done during the first pass
            // could change this
            if ( pass==0 ) {
                // get the spectrum
                casacore::Vector<casacore::Float> spectrumAmplitudes(nChan);
                for (uInt chan = 0; chan < nChan; chan++)
                    spectrumAmplitudes(chan) = abs(data(corr,chan,k));

                if ( itsAutoThresholds ) {

                    // combine amplitudes with mask and get the median-based statistics
                    casacore::MaskedArray<casacore::Float>
                        maskedAmplitudes(spectrumAmplitudes, unflaggedMask);
                    casacore::Vector<casacore::Float>
                        statsVector = getRobustStats(maskedAmplitudes);
                    casacore::Float median = statsVector[0];
                    casacore::Float sigma_IQR = statsVector[1];

                    // set cutoffs
                    if ( !hasLowLimit ) {
                        itsLowLimit = median-itsThresholdFactor*sigma_IQR;
                        hasLowLimit = casacore::True;
                    }
                    if ( !hasHighLimit ) {
                        itsHighLimit = median+itsThresholdFactor*sigma_IQR;
                        hasHighLimit = casacore::True;
                    }

                    // check min and max relative to thresholds, and do not loop over
                    // data again if they are good. If indicies were also sorted, could
                    // just test where the sorted amplitudes break the threshold...
                    // ** cannot do this when averages are needed, or they'll be skipped **
                    if (!itsIntegrateSpectra && !itsIntegrateTimes &&
                            (statsVector[2] >= itsLowLimit) &&
                            (statsVector[3] <= itsHighLimit)) {
                        continue;
                    }

                }

                // only need these if itsIntegrateTimes
                casacore::Double aveTime = 0.0;
                casacore::uInt countTime = 0;

                // look for individual peaks and do any integrations
                for (size_t chan = 0; chan < nChan; ++chan) {
                    if (flags(corr, chan, k)) {
                        itsStats.visAlreadyFlagged++;
                        continue;
                    }

                    // look for individual peaks
                    const float amp = spectrumAmplitudes(chan);
                    if ((hasLowLimit && (amp < itsLowLimit)) ||
                        (hasHighLimit && (amp > itsHighLimit))) {
                        flags(corr, chan, k) = true;
                        wasUpdatedRow = true;
                        itsStats.visFlagged++;
                    }
                    else if ( itsIntegrateSpectra || itsIntegrateTimes ) {
                        if ( itsIntegrateSpectra ) {
                            // do spectra integration
                            itsAveSpectra[key][chan] += amp;
                            itsCountSpectra[key][chan]++;
                            itsAverageFlagsAreReady = casacore::False;
                        }
                        if ( itsIntegrateTimes ) {
                            // do time-series integration
                            aveTime += amp;
                            countTime++;
                        }
                    }

                }
                if ( itsIntegrateTimes ) {
                    // normalise this average
                    if ( countTime>0 ) {
                        itsAveTimes[key][itsCountTimes[key]] =
                            aveTime/casacore::Double(countTime);
                        itsMaskTimes[key][itsCountTimes[key]] = casacore::True;
                        itsAverageFlagsAreReady = casacore::False;
                    }
                    else {
                        itsMaskTimes[key][itsCountTimes[key]] = casacore::False;
                    }
                }

            }
            else if ( (pass==1) &&  ( itsIntegrateSpectra || itsIntegrateTimes ) ) {
                // only flag unflagged data, so that new flags can be counted.
                // "flags" is true for flags, "mask*" are false for flags
                if ( itsIntegrateTimes ) {
                    // apply itsMaskTimes flags. Could just use flagRow,
                    // but not sure that all applications support flagRow
                    if ( !itsMaskTimes[key][itsCountTimes[key]] ) {
                        for (size_t chan = 0; chan < nChan; ++chan) {
                            if (flags(corr, chan, k)) continue;
                                flags(corr, chan, k) = true;
                                wasUpdatedRow = true;
                                itsStats.visFlagged++;
                        }
                        // everything is flagged, so move to the next "corr"
                        continue;
                    }
                    // need this to be false for all "corr" to warrent flagRow
                    else leaveRowFlag = true;
                }
                // apply itsIntegrateSpectra flags
                if ( itsIntegrateSpectra ) {
                    for (size_t chan = 0; chan < nChan; ++chan) {
                        if ( !flags(corr, chan, k) && !itsMaskSpectra[key][chan] ) {
                            flags(corr, chan, k) = true;
                            wasUpdatedRow = true;
                            itsStats.visFlagged++;
                        }
                    }
                }
            }
        }

        if (wasUpdatedRow && itsIntegrateTimes && !leaveRowFlag && (pass==1)) {
            itsStats.rowsFlagged++;
            if (!dryRun) {
                msc.flagRow().put(row + k, true);
            }
        }
        if (wasUpdatedRow) wasUpdated = true;
    }
    if (wasUpdated && !dryRun) {
        msc.flag().putColumnRange(rowSlicer, flags);
    }
}


// return the median, the interquartile range, and the min/max of a masked array
casacore::Vector<casacore::Float>AmplitudeFlagger::getRobustStats(
    casacore::MaskedArray<casacore::Float> maskedAmplitudes)
{
    casacore::Vector<casacore::Float> statsVector(4,0.0);

    // Grab unflagged frequency channels and sort their amplitudes.
    // extract all of the unflagged amplitudes
    casacore::Vector<casacore::Float>
        amplitudes = maskedAmplitudes.getCompressedArray();

    // return with zeros if all of the data are flagged
    casacore::uInt n = amplitudes.nelements();
    if (n == 0) return(statsVector);

    casacore::Float minVal, maxVal;
    casacore::minMax(minVal,maxVal,amplitudes);

    // Now find median and IQR
    // Use the fact that nth_element does a partial sort:
    // all elements before the selected element will be smaller
    std::vector<casacore::Float> vamp = amplitudes.tovector();
    const casacore::uInt Q1 = n / 4;
    const casacore::uInt Q2 = n / 2;
    const casacore::uInt Q3 = 3 * n /4;
    std::nth_element(vamp.begin(),        vamp.begin() + Q2, vamp.end());
    std::nth_element(vamp.begin(),        vamp.begin() + Q1, vamp.begin() + Q2);
    std::nth_element(vamp.begin() + Q2+1, vamp.begin() + Q3, vamp.end());

    statsVector[0] = vamp[Q2]; // median
    statsVector[1] = (vamp[Q3] - vamp[Q1]) / 1.34896; // sigma from IQR
    statsVector[2] = minVal; // min
    statsVector[3] = maxVal; // max
    return(statsVector);
}

// Generate a key for a given row and polarisation
rowKey AmplitudeFlagger::getRowKey(
    casacore::MSColumns& msc,
    const casacore::uInt row,
    const casacore::uInt corr)
{

    // specify which fields to keep separate and which to average over
    // any set to zero will be averaged over
    casacore::Int field = 0;
    casacore::Int feed1 = 0;
    casacore::Int feed2 = 0;
    casacore::Int ant1  = 0;
    casacore::Int ant2  = 0;
    casacore::Int pol   = 0;
    if (itsAveAll) {
        if (itsAveAllButPol) {
            pol = corr;
        }
        if (itsAveAllButBeam) {
            feed1 = msc.feed1()(row);
            feed2 = msc.feed2()(row);
        }
    } else {
        field = msc.fieldId()(row);
        feed1 = msc.feed1()(row);
        feed2 = msc.feed2()(row);
        ant1  = msc.antenna1()(row);
        ant2  = msc.antenna2()(row);
        pol   = corr;
    }
#ifdef TUPLE_INDEX
    return boost::make_tuple(field,feed1,feed2,ant1,ant2,pol);
#else
    // replace tuple with integer to speed things up, but this can run out of range
    // feed().nrow is nant*nfeed for askap - we really want the number of beams (usually 36)
    casacore::Int nant = msc.antenna().nrow();
    casacore::Int nfeed = msc.feed().nrow();
    if (nant > 0 && nfeed >= nant) nfeed /= nant;
    return ((((field*4+pol)*nfeed+feed1)*nant+ant2)*nant+ant1);
#endif
}

void AmplitudeFlagger::updateTimeVectors(const rowKey &key, const casacore::uInt pass)
{
    if (itsCountTimes.find(key) == itsCountTimes.end()) {
        itsCountTimes[key] = 0; // init counter for this key
    }
    else {
        itsCountTimes[key]++;
    }
    if ( pass==0 ) {
        itsAveTimes[key].resize(itsCountTimes[key]+1,casacore::True);
        itsMaskTimes[key].resize(itsCountTimes[key]+1,casacore::True);
        itsMaskTimes[key][itsCountTimes[key]] = casacore::True;
    }
}

void AmplitudeFlagger::initSpectrumVectors(const rowKey &key, const casacore::IPosition &shape)
{
    itsAveSpectra[key].resize(shape);
    itsAveSpectra[key].set(0.0);
    itsCountSpectra[key].resize(shape);
    itsCountSpectra[key].set(0);
    itsMaskSpectra[key].resize(shape);
    itsMaskSpectra[key].set(casacore::True);
}

// Set flags based on integrated quantities
void AmplitudeFlagger::setFlagsFromIntegrations(void)
{

    if ( itsIntegrateSpectra ) {

        for (std::map<rowKey, casacore::Vector<casacore::Double> >::iterator
             it=itsAveSpectra.begin(); it!=itsAveSpectra.end(); ++it) {

            // get the spectra
            casacore::Vector<casacore::Float> aveSpectrum(it->second.shape());
            casacore::Vector<casacore::Int> countSpectrum = itsCountSpectra[it->first];
            casacore::Vector<casacore::Bool> maskSpectrum = itsMaskSpectra[it->first];

            for (size_t chan = 0; chan < aveSpectrum.size(); ++chan) {
                if (countSpectrum[chan]>0) {
                    aveSpectrum[chan] = it->second[chan] /
                                        casacore::Double(countSpectrum[chan]);
                    countSpectrum[chan] = 1;
                    maskSpectrum[chan] = casacore::True;
                }
                else {
                    maskSpectrum[chan] = casacore::False;
                }
            }

            // generate the flagging stats. could fill the unflagged spectrum
            // directly in the preceding loop, but the full vector is needed below
            casacore::MaskedArray<casacore::Float>
                maskedAmplitudes(aveSpectrum, maskSpectrum);
            casacore::Vector<casacore::Float>
                statsVector = getRobustStats(maskedAmplitudes);
            casacore::Float median = statsVector[0];
            casacore::Float sigma_IQR = statsVector[1];
            casacore::Float lowerLim = median-itsSpectraFactor*sigma_IQR;
            casacore::Float upperLim = median+itsSpectraFactor*sigma_IQR;

            // check min and max relative to thresholds.
            // do not loop over data again if all unflagged channels are good
            if ((statsVector[2] < lowerLim) || (statsVector[3] > upperLim)) {

                for (size_t chan = 0; chan < aveSpectrum.size(); ++chan) {
                    if (maskSpectrum[chan]==casacore::False) continue;
                    if ((aveSpectrum[chan]<lowerLim) ||
                        (aveSpectrum[chan]>upperLim)) {
                        maskSpectrum[chan]=casacore::False;
                    }
                }

            }

        }

    }

    if ( itsIntegrateTimes ) {

        for (std::map<rowKey, casacore::Vector<casacore::Float> >::iterator
             it=itsAveTimes.begin(); it!=itsAveTimes.end(); ++it) {

            // reset the counter for this key
            itsCountTimes[it->first] = -1;

            // get the spectra
            casacore::Vector<casacore::Float> aveTime = it->second;
            casacore::Vector<casacore::Bool> maskTime = itsMaskTimes[it->first];

            // generate the flagging stats
            casacore::MaskedArray<casacore::Float> maskedAmplitudes(aveTime, maskTime);
            casacore::Vector<casacore::Float>
                statsVector = getRobustStats(maskedAmplitudes);
            casacore::Float median = statsVector[0];
            casacore::Float sigma_IQR = statsVector[1];
            casacore::Float lowerLim = median-itsTimesFactor*sigma_IQR;
            casacore::Float upperLim = median+itsTimesFactor*sigma_IQR;

            // check min and max relative to thresholds.
            // do not loop over data again if all unflagged times are good
            if ((statsVector[2] < lowerLim) || (statsVector[3] > upperLim)) {

                for (size_t t = 0; t < aveTime.size(); ++t) {
                    if (maskTime[t] == casacore::False) continue;
                    if ((aveTime[t] < lowerLim) ||
                        (aveTime[t] > upperLim)) {
                        maskTime[t] = casacore::False;
                    }
                }

            }

        }

    }

    itsAverageFlagsAreReady = casacore::True;

}

// load relevant parset parameters
void AmplitudeFlagger::loadParset(const LOFAR::ParameterSet& parset)
{

    if (parset.isDefined("high")) {
        itsHasHighLimit = true;
        itsHighLimit = parset.getFloat("high");
    }
    if (parset.isDefined("low")) {
        itsHasLowLimit = true;
        itsLowLimit = parset.getFloat("low");
    }
    if (parset.isDefined("dynamicBounds")) {
        itsAutoThresholds = parset.getBool("dynamicBounds");
    }
    if (parset.isDefined("threshold")) {
        itsThresholdFactor = parset.getFloat("threshold");
    }
    if (parset.isDefined("integrateSpectra")) {
        itsIntegrateSpectra = parset.getBool("integrateSpectra");
        if (parset.isDefined("integrateSpectra.threshold")) {
            itsSpectraFactor = parset.getFloat("integrateSpectra.threshold");
        }
    }
    if (parset.isDefined("integrateTimes")) {
        itsIntegrateTimes = parset.getBool("integrateTimes");
        if (parset.isDefined("integrateTimes.threshold")) {
            itsTimesFactor = parset.getFloat("integrateTimes.threshold");
        }
    }
    if (parset.isDefined("aveAll")) {
        itsAveAll = parset.getBool("aveAll");
        if (parset.isDefined("aveAll.noPol")) {
            itsAveAllButPol = parset.getBool("aveAll.noPol");
        }
        if (parset.isDefined("aveAll.noBeam")) {
            itsAveAllButBeam = parset.getBool("aveAll.noBeam");
        }
    }

    // Converts Stokes vector string to StokesType
    if (parset.isDefined("stokes")) {
        vector<string> strvec = parset.getStringVector("stokes");

        for (size_t i = 0; i < strvec.size(); ++i) {
            itsStokes.insert(Stokes::type(strvec[i]));
        }
    }
}

// add a summary of the relevant parset parameters to the log
void AmplitudeFlagger::logParsetSummary(const LOFAR::ParameterSet& parset)
{

    ASKAPLOG_INFO_STR(logger, "Parameter Summary:");

    if (!itsHasHighLimit && !itsHasLowLimit && !itsAutoThresholds &&
        !itsIntegrateSpectra && !itsIntegrateTimes) {
        ASKAPTHROW(AskapError, "No amplitude flagging has been defined");
    }
    if (itsAutoThresholds) {
        if (itsHasHighLimit && itsHasLowLimit) {
            ASKAPLOG_WARN_STR(logger,
                "Amplitude thresholds defined. No auto-threshold");
        }
        if (itsHasHighLimit) {
            ASKAPLOG_INFO_STR(logger, "High threshold set to "<<itsHighLimit);
        } else {
            ASKAPLOG_INFO_STR(logger,
                "High threshold set automatically with threshold factor of "<<
                itsThresholdFactor);
        }
        if (itsHasLowLimit) {
            ASKAPLOG_INFO_STR(logger, "Low threshold set to "<<itsLowLimit);
        } else {
            ASKAPLOG_INFO_STR(logger,
                "Low threshold set automatically with threshold factor of "<<
                itsThresholdFactor);
        }
    }
    if (itsIntegrateSpectra) {
        ASKAPLOG_INFO_STR(logger,
            "Searching for outliers in integrated spectra with a "
            <<itsSpectraFactor<<"-sigma cutoff");
    }
    if (itsIntegrateTimes) {
        ASKAPLOG_INFO_STR(logger,
            "Searching for outliers in integrated time series with a "
            <<itsTimesFactor<<"-sigma cutoff");
    }
    if (itsAveAll && (itsIntegrateSpectra || itsIntegrateTimes)) {
        if (itsAveAllButPol || itsAveAllButBeam) {
            ASKAPLOG_INFO_STR(logger,
                " - except for the following, will ignore properites when integrating");
            if (itsAveAllButPol) {
                ASKAPLOG_INFO_STR(logger,
                    "   * keeping polarisations separate");
            }
            if (itsAveAllButBeam) {
                ASKAPLOG_INFO_STR(logger,
                    "   * keeping beams separate");
            }
        }
        else {
            ASKAPLOG_INFO_STR(logger,
                " - ignoring visibility properites when integrating");
        }
    }

}

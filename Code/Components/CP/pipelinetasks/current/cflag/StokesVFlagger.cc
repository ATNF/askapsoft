/// @file StokesVFlagger.cc
///
/// @copyright (c) 2012-2014 CSIRO
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
#include "StokesVFlagger.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <map>
#include <limits>
#include <vector>
#include <algorithm>
#include <cmath>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "boost/shared_ptr.hpp"
#include "Common/ParameterSet.h"
#include "casacore/casa/aipstype.h"
#include "casacore/casa/Arrays/ArrayMath.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/VectorIter.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/MatrixIter.h"
#include "casacore/casa/Arrays/Cube.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"
#include "casacore/ms/MeasurementSets/MSPolColumns.h"
#include "casacore/ms/MeasurementSets/StokesConverter.h"

// Local package includes
#include "cflag/FlaggingStats.h"

ASKAP_LOGGER(logger, ".StokesVFlagger");

using namespace std;
using namespace askap;
using namespace casa;
using namespace askap::cp::pipelinetasks;

vector< boost::shared_ptr<IFlagger> > StokesVFlagger::build(
        const LOFAR::ParameterSet& parset,
        const casa::MeasurementSet& /*ms*/)
{
    vector< boost::shared_ptr<IFlagger> > flaggers;
    const string key = "stokesv_flagger.enable";
    if (parset.isDefined(key) && parset.getBool(key)) {
        const LOFAR::ParameterSet subset = parset.makeSubset("stokesv_flagger.");

        const float threshold = subset.getFloat("threshold", 5.0);
        const bool robustStatistics = subset.getBool("useRobustStatistics", false);
        const bool quickRobust = subset.getBool("useQuickRobust", false);
        const bool integrateSpectra = subset.getBool("integrateSpectra", false);
        const float spectraThreshold = subset.getFloat("integrateSpectra.threshold", 5.0);
        const bool integrateTimes = subset.getBool("integrateTimes", false);
        const float timesThreshold = subset.getFloat("integrateTimes.threshold", 5.0);

        ASKAPLOG_INFO_STR(logger, "Parameter Summary:");
        ASKAPLOG_INFO_STR(logger, "Searching for outliers with a "<<threshold<<"-sigma cutoff");
        if (robustStatistics) {
            if (quickRobust) {
                ASKAPLOG_INFO_STR(logger, "Using approximate robust statistics");
            } else {
                ASKAPLOG_INFO_STR(logger, "Using robust statistics");
            }
        }
        if (integrateSpectra) {
            ASKAPLOG_INFO_STR(logger,
                "Searching for outliers in integrated spectra with a "
                <<spectraThreshold<<"-sigma cutoff");
        }
        if (integrateTimes) {
            ASKAPLOG_INFO_STR(logger,
                "Searching for outliers in integrated time series with a "
                <<timesThreshold<<"-sigma cutoff");
        }

        flaggers.push_back(boost::shared_ptr<IFlagger>
            (new StokesVFlagger(threshold,robustStatistics,integrateSpectra,
                                spectraThreshold,integrateTimes,timesThreshold,
                                quickRobust)));
    }
    return flaggers;
}

StokesVFlagger:: StokesVFlagger(float threshold, bool robustStatistics,
                                bool integrateSpectra, float spectraThreshold,
                                bool integrateTimes, float timesThreshold,
                                bool quickRobust)
    : itsStats("StokesVFlagger"),
      itsThreshold(threshold), itsRobustStatistics(robustStatistics),
      itsIntegrateSpectra(integrateSpectra), itsSpectraThreshold(spectraThreshold),
      itsIntegrateTimes(integrateTimes), itsTimesThreshold(timesThreshold),
      itsAverageFlagsAreReady(true)
{
    ASKAPCHECK(itsThreshold > 0.0, "Threshold must be greater than zero");
}

FlaggingStats StokesVFlagger::stats(void) const
{
    return itsStats;
}

casa::Bool StokesVFlagger::processingRequired(const casa::uInt pass)
{
    if (itsIntegrateSpectra || itsIntegrateTimes) {
        return (pass<2);
    } else {
        return (pass<1);
    }
}

casa::StokesConverter& StokesVFlagger::getStokesConverter(
    const casa::ROMSPolarizationColumns& polc, const casa::Int polId)
{
    const casa::Vector<Int> corrType = polc.corrType()(polId);
    std::map<casa::Int, casa::StokesConverter>::iterator it = itsConverterCache.find(polId);
    if (it == itsConverterCache.end()) {
        //ASKAPLOG_DEBUG_STR(logger, "Creating StokesConverter for pol table entry " << polId);
        const casa::Vector<Int> target(1, Stokes::V);
        itsConverterCache.insert(pair<casa::Int, casa::StokesConverter>(polId,
                                 casa::StokesConverter(target, corrType)));
    }

    return itsConverterCache[polId];
}

void StokesVFlagger::processRow(casa::MSColumns& msc, const casa::uInt pass,
                                const casa::uInt row, const bool dryRun)
{
    // Get a description of what correlation products are in the data table.
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::Int dataDescId = msc.dataDescId()(row);
    const casa::Int polId = ddc.polarizationId()(dataDescId);

    // Get the (potentially cached) stokes converter
    const StokesConverter& stokesconv = getStokesConverter(msc.polarization(), polId);

    // Convert data to Stokes V (imag(data(2,i))-imag(data(3,i)))
    const Matrix<casa::Complex> data = msc.data()(row);
    casa::Matrix<casa::Complex> vmatrix(1, data.ncolumn());
    stokesconv.convert(vmatrix, data);
    casa::Vector<casa::Complex> vdata = vmatrix.row(0);

    // Build a vector with the amplitudes
    Matrix<casa::Bool> flags = msc.flag()(row);
    std::vector<casa::Float> tmpamps;
    for (size_t i = 0; i < vdata.size(); ++i) {
        bool anyFlagged = anyEQ(flags.column(i), true);
        if (!anyFlagged) {
            tmpamps.push_back(abs(vdata(i)));
        }
    }

    // normalise averages and search them for peaks to flag
    if ( !itsAverageFlagsAreReady && (pass==1) ) {
        ASKAPLOG_INFO_STR(logger, "Finalising averages at the start of pass "
            <<pass+1);
        setFlagsFromIntegrations();
    }

    // return a tuple that indicate which integration this row is in
    rowKey key = getRowKey(msc, row);

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

    // If all visibilities are flagged, nothing to do
    if (tmpamps.empty()) return;

    bool wasUpdated = false;

    if ( pass==0 ) {

        // Convert to a casa::Vector so we can use ArrayMath functions
        // to determine the mean and stddev
        casa::Vector<casa::Float> amps(tmpamps);

        // Flag all correlations where the Stokes V product
        // is greater than the threshold
        casa::Float sigma, avg;
        if (itsRobustStatistics) {
            casa::Vector<casa::Float> statsVector = getRobustStats(amps);
            avg = statsVector[0];
            sigma = statsVector[1];
            // if min and max are bounded, they all are.
            // so skip if there is no other reason to loop over frequencies
            if ((statsVector[2] >= (avg - (sigma * itsThreshold))) &&
                (statsVector[3] <= (avg + (sigma * itsThreshold))) &&
                !itsIntegrateSpectra && !itsIntegrateTimes) {
                return;
            }
        }
        else {
            avg = mean(amps);
            sigma = stddev(amps);
        }

        // If stokes-v can't be formed due to lack of the necessary input products
        // then vdata will contain all zeros. In this case, no flagging can be done.
        const casa::Float epsilon = std::numeric_limits<casa::Float>::epsilon();
        if (near(sigma, 0.0, epsilon) && near(avg, 0.0, epsilon)) {
            return;
        }

        // Apply threshold based flagging and accumulate any averages
        // only need these if itsIntegrateTimes
        casa::Double aveTime = 0.0;
        casa::uInt countTime = 0;
        for (size_t i = 0; i < vdata.size(); ++i) {
            const casa::Float amp = abs(vdata(i));
            // Apply threshold based flagging
            if (amp > (avg + (sigma * itsThreshold))) {
                for (casa::uInt pol = 0; pol < flags.nrow(); ++pol) {
                    if (flags(pol, i)) {
                        itsStats.visAlreadyFlagged++;
                        continue;
                    }
                    flags(pol, i) = true;
                    wasUpdated = true;
                    itsStats.visFlagged++;
                }
            }
            // Accumulate any averages
            else if ( itsIntegrateSpectra || itsIntegrateTimes ) {
                if ( itsIntegrateSpectra ) {
                    // do spectra integration
                    itsAveSpectra[key][i] += amp;
                    itsCountSpectra[key][i]++;
                    itsAverageFlagsAreReady = casa::False;
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
                    aveTime/casa::Double(countTime);
                itsMaskTimes[key][itsCountTimes[key]] = casa::True;
                itsAverageFlagsAreReady = casa::False;
            }
            else {
                itsMaskTimes[key][itsCountTimes[key]] = casa::False;
            }
        }

    }
    else if ( (pass==1) &&  ( itsIntegrateSpectra || itsIntegrateTimes ) ) {
        // only flag unflagged data, so that new flags can be counted.
        // "flags" is true for flags, "mask*" are false for flags
        bool rowFlagged = false;
        if ( itsIntegrateTimes ) {
            // apply itsMaskTimes flags. Could just use flagRow,
            // but not sure that all applications support flagRow
            if ( !itsMaskTimes[key][itsCountTimes[key]] ) {
                rowFlagged = true;
                itsStats.rowsFlagged++;
                for (size_t i = 0; i < vdata.size(); ++i) {
                    for (casa::uInt pol = 0; pol < flags.nrow(); ++pol) {
                        if (flags(pol, i)) continue;
                        flags(pol, i) = true;
                        wasUpdated = true;
                        itsStats.visFlagged++;
                    }

                }
            }
        }
        // apply itsIntegrateSpectra flags
        if ( itsIntegrateSpectra && !rowFlagged ) {
            for (size_t i = 0; i < vdata.size(); ++i) {
                if ( !itsMaskSpectra[key][i] ) {
                    for (casa::uInt pol = 0; pol < flags.nrow(); ++pol) {
                        if ( flags(pol, i) ) continue;
                        flags(pol, i) = true;
                        wasUpdated = true;
                        itsStats.visFlagged++;
                    }
                }
            }
        }
    }

    if (wasUpdated && !dryRun) {
        if (itsIntegrateTimes && !itsMaskTimes[key][itsCountTimes[key]] && (pass==1)) {
            msc.flagRow().put(row, true);
        }
        msc.flag().put(row, flags);
    }
}


void StokesVFlagger::processRows(casa::MSColumns& msc, const casa::uInt pass,
                                 const casa::uInt row, const casa::uInt nrow,
                                 const bool dryRun)
{
    // Get a description of what correlation products are in the data table.
    const casa::ROMSDataDescColumns& ddc = msc.dataDescription();
    const casa::Int dataDescId = msc.dataDescId()(row);
    const casa::Int polId = ddc.polarizationId()(dataDescId);

    // Get the (potentially cached) stokes converter
    const StokesConverter& stokesconv = getStokesConverter(msc.polarization(), polId);

    // Convert data to Stokes V (imag(data(2,i))-imag(data(3,i)))
    Slicer rowSlicer(Slice(row,nrow));
    const Cube<casa::Complex> data = msc.data().getColumnRange(rowSlicer);
    // does it help to keep this around?
    static casa::Cube<casa::Complex> vcube;
    vcube.resize(1, data.shape()[1], data.shape()[2]);
    stokesconv.convert(vcube, data);
    casa::Matrix<casa::Complex> vdata = vcube.yzPlane(0);
    Cube<casa::Bool> flags = msc.flag().getColumnRange(rowSlicer);
    casa::uInt nPol = flags.shape()(0);
    casa::uInt nChan = flags.shape()(1);
    bool wasUpdated = false;

    for (casa::uInt k = 0; k < nrow;  k++) {

        // Build a vector with the amplitudes
        bool allFlagged = true;
        std::vector<casa::Float> tmpamps;
        for (size_t i = 0; i < nChan; ++i) {
            bool anyFlagged = false;
            for (casa::uInt pol=0; pol < nPol; pol++)
                if (flags(pol,i,k)) anyFlagged = true;
            if (!anyFlagged) {
                if (pass == 0) tmpamps.push_back(abs(vdata(i,k)));
                allFlagged = false;
            }
        }

        // normalise averages and search them for peaks to flag
        if ( !itsAverageFlagsAreReady && (pass==1) ) {
            ASKAPLOG_INFO_STR(logger, "Finalising averages at the start of pass "
                <<pass+1);
            setFlagsFromIntegrations();
        }

        // return a key that indicates which integration this row is in
        rowKey key = getRowKey(msc, row + k);

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

        // If all visibilities are flagged, nothing to do
        if (allFlagged) continue;

        bool wasUpdatedRow = false;

        if ( pass==0 ) {

            // Convert to a casa::Vector so we can use ArrayMath functions
            // to determine the mean and stddev
            casa::Vector<casa::Float> amps(tmpamps);

            // Flag all correlations where the Stokes V product
            // is greater than the threshold
            casa::Float sigma, avg;
            if (itsRobustStatistics) {
                casa::Vector<casa::Float> statsVector = getRobustStats(amps);
                avg = statsVector[0];
                sigma = statsVector[1];
                // if min and max are bounded, they all are.
                // so skip if there is no other reason to loop over frequencies
                if ((statsVector[2] >= (avg - (sigma * itsThreshold))) &&
                    (statsVector[3] <= (avg + (sigma * itsThreshold))) &&
                    !itsIntegrateSpectra && !itsIntegrateTimes) {
                    continue;
                }
            }
            else {
                avg = mean(amps);
                sigma = stddev(amps);
            }

            // If stokes-v can't be formed due to lack of the necessary input products
            // then vdata will contain all zeros. In this case, no flagging can be done.
            const casa::Float epsilon = std::numeric_limits<casa::Float>::epsilon();
            if (near(sigma, 0.0, epsilon) && near(avg, 0.0, epsilon)) {
                continue;
            }

            // Apply threshold based flagging and accumulate any averages
            // only need these if itsIntegrateTimes
            casa::Double aveTime = 0.0;
            casa::uInt countTime = 0;
            for (size_t i = 0; i < nChan; ++i) {
                const casa::Float amp = abs(vdata(i,k));
                // Apply threshold based flagging
                if (amp > (avg + (sigma * itsThreshold))) {
                    for (casa::uInt pol = 0; pol < nPol; ++pol) {
                        if (flags(pol, i, k)) {
                            itsStats.visAlreadyFlagged++;
                            continue;
                        }
                        flags(pol, i, k) = true;
                        wasUpdatedRow = true;
                        itsStats.visFlagged++;
                    }
                }
                // Accumulate any averages
                else if ( itsIntegrateSpectra || itsIntegrateTimes ) {
                    if ( itsIntegrateSpectra ) {
                        // do spectra integration
                        itsAveSpectra[key][i] += amp;
                        itsCountSpectra[key][i]++;
                        itsAverageFlagsAreReady = casa::False;
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
                        aveTime/casa::Double(countTime);
                    itsMaskTimes[key][itsCountTimes[key]] = casa::True;
                    itsAverageFlagsAreReady = casa::False;
                }
                else {
                    itsMaskTimes[key][itsCountTimes[key]] = casa::False;
                }
            }

        }
        else if ( (pass==1) &&  ( itsIntegrateSpectra || itsIntegrateTimes ) ) {
            // only flag unflagged data, so that new flags can be counted.
            // "flags" is true for flags, "mask*" are false for flags
            bool rowFlagged = false;
            if ( itsIntegrateTimes ) {
                // apply itsMaskTimes flags. Could just use flagRow,
                // but not sure that all applications support flagRow
                if ( !itsMaskTimes[key][itsCountTimes[key]] ) {
                    rowFlagged = true;
                    itsStats.rowsFlagged++;
                    for (size_t i = 0; i < nChan; ++i) {
                        for (casa::uInt pol = 0; pol < nPol; ++pol) {
                            if (flags(pol, i, k)) continue;
                            flags(pol, i, k) = true;
                            wasUpdatedRow = true;
                            itsStats.visFlagged++;
                        }
                    }
                }
            }
            // apply itsIntegrateSpectra flags
            if ( itsIntegrateSpectra && !rowFlagged ) {
                for (size_t i = 0; i < nChan; ++i) {
                    if ( !itsMaskSpectra[key][i] ) {
                        for (casa::uInt pol = 0; pol < nPol; ++pol) {
                            if ( flags(pol, i, k) ) continue;
                            flags(pol, i, k) = true;
                            wasUpdatedRow = true;
                            itsStats.visFlagged++;
                        }
                    }
                }
            }
            if (wasUpdatedRow && !dryRun) {
                if (itsIntegrateTimes && !itsMaskTimes[key][itsCountTimes[key]]) {
                    msc.flagRow().put(row, true);
                }
            }
        }
        if (wasUpdatedRow) wasUpdated = true;
    }
    if (wasUpdated && !dryRun) {
        msc.flag().putColumnRange(rowSlicer, flags);
    }
}


// return the median, the interquartile range, and the min/max of a masked array
casa::Vector<casa::Float>StokesVFlagger::getRobustStats(
    casa::MaskedArray<casa::Float> maskedAmplitudes)
{
    // extract all of the unflagged amplitudes
    casa::Vector<casa::Float> amplitudes = maskedAmplitudes.getCompressedArray();

    // return with zeros if all of the data are flagged
    if (amplitudes.nelements() == 0) {
        casa::Vector<casa::Float> statsVector(4,0.0);
        return(statsVector);
    }
    return getRobustStats(amplitudes);
}

// return the median, the interquartile range, and the min/max of a non-masked array
casa::Vector<casa::Float>StokesVFlagger::getRobustStats(
    casa::Vector<casa::Float> amplitudes)
{
    casa::Float minVal, maxVal;
    casa::minMax(minVal,maxVal,amplitudes);

    // Now find median and IQR
    // Use the fact that nth_element does a partial sort:
    // all elements before the selected element will be smaller
    casa::uInt n = amplitudes.nelements();
    std::vector<casa::Float> vamp = amplitudes.tovector();
    const casa::uInt Q1 = n / 4;
    const casa::uInt Q2 = n / 2;
    const casa::uInt Q3 = 3 * n /4;
    std::nth_element(vamp.begin(),        vamp.begin() + Q2, vamp.end());
    std::nth_element(vamp.begin(),        vamp.begin() + Q1, vamp.begin() + Q2);
    std::nth_element(vamp.begin() + Q2+1, vamp.begin() + Q3, vamp.end());

    casa::Vector<casa::Float> statsVector(4);
    statsVector[0] = vamp[Q2]; // median
    statsVector[1] = (vamp[Q3] - vamp[Q1]) / 1.34896; // sigma from IQR
    statsVector[2] = minVal; // min
    statsVector[3] = maxVal; // max
    return(statsVector);
}

// Generate a key for a given row and polarisation
rowKey StokesVFlagger::getRowKey(
    const casa::MSColumns& msc,
    const casa::uInt row)
{

    // looking for outliers in a single polarisation, so set the corr key to zero
#ifdef TUPLE_INDEX
    return boost::make_tuple(msc.fieldId()(row),
                             msc.feed1()(row),
                             msc.feed2()(row),
                             msc.antenna1()(row),
                             msc.antenna2()(row),
                             0); // corr
#else
    casa::Int field = msc.fieldId()(row);
    casa::Int feed1 = msc.feed1()(row);
    //feed2 = msc.feed2()(row);
    casa::Int ant1  = msc.antenna1()(row);
    casa::Int ant2  = msc.antenna2()(row);
    casa::Int nant = msc.antenna().nrow();
    casa::Int nfeed = msc.feed().nrow();
    if (nant > 0 && nfeed >= nant) nfeed /= nant;
    return (((field*nfeed+feed1)*nant+ant2)*nant+ant1);
#endif
}

void StokesVFlagger::updateTimeVectors(const rowKey &key, const casa::uInt pass)
{
    if (itsCountTimes.find(key) == itsCountTimes.end()) {
        itsCountTimes[key] = 0; // init counter for this key
    }
    else {
        itsCountTimes[key]++;
    }
    if ( pass==0 ) {
        itsAveTimes[key].resize(itsCountTimes[key]+1,casa::True);
        itsMaskTimes[key].resize(itsCountTimes[key]+1,casa::True);
        itsMaskTimes[key][itsCountTimes[key]] = casa::True;
    }
}

void StokesVFlagger::initSpectrumVectors(const rowKey &key, const casa::IPosition &shape)
{
    itsAveSpectra[key].resize(shape);
    itsAveSpectra[key].set(0.0);
    itsCountSpectra[key].resize(shape);
    itsCountSpectra[key].set(0);
    itsMaskSpectra[key].resize(shape);
    itsMaskSpectra[key].set(casa::True);
}

// Set flags based on integrated quantities
void StokesVFlagger::setFlagsFromIntegrations(void)
{

    if ( itsIntegrateSpectra ) {

        for (std::map<rowKey, casa::Vector<casa::Double> >::iterator
             it=itsAveSpectra.begin(); it!=itsAveSpectra.end(); ++it) {

            // get the spectra
            casa::Vector<casa::Float> aveSpectrum(it->second.shape());
            casa::Vector<casa::Int> countSpectrum = itsCountSpectra[it->first];
            casa::Vector<casa::Bool> maskSpectrum = itsMaskSpectra[it->first];
            //std::vector<casa::Float> tmpamps; // use instead of MaskedArray?

            for (size_t chan = 0; chan < aveSpectrum.size(); ++chan) {
                if (countSpectrum[chan]>0) {
                    aveSpectrum[chan] = it->second[chan] /
                                        casa::Double(countSpectrum[chan]);
                    //tmpamps.push_back(aveSpectrum[chan]);
                    countSpectrum[chan] = 1;
                    maskSpectrum[chan] = casa::True;
                }
                else {
                    maskSpectrum[chan] = casa::False;
                }
            }

            // generate the flagging stats. could fill the unflagged spectrum
            // directly in the preceding loop, but the full vector is needed below
            casa::MaskedArray<casa::Float>
                maskedAmplitudes(aveSpectrum, maskSpectrum);
            casa::Vector<casa::Float> statsVector =
                getRobustStats(maskedAmplitudes);
            casa::Float median = statsVector[0];
            casa::Float sigma_IQR = statsVector[1];

            // check min and max relative to thresholds.
            // do not loop over data again if all unflagged channels are good
            if ((statsVector[2] < median-itsSpectraThreshold*sigma_IQR) ||
                (statsVector[3] > median+itsSpectraThreshold*sigma_IQR)) {

                for (size_t chan = 0; chan < aveSpectrum.size(); ++chan) {
                    if (maskSpectrum[chan]==casa::False) continue;
                    if ((aveSpectrum[chan]<median-itsSpectraThreshold*sigma_IQR) ||
                        (aveSpectrum[chan]>median+itsSpectraThreshold*sigma_IQR)) {
                        maskSpectrum[chan]=casa::False;
                    }
                }

            }

        }

    }

    if ( itsIntegrateTimes ) {

        for (std::map<rowKey, casa::Vector<casa::Float> >::iterator
             it=itsAveTimes.begin(); it!=itsAveTimes.end(); ++it) {

            // reset the counter for this key
            itsCountTimes[it->first] = -1;

            // get the spectra
            casa::Vector<casa::Float> aveTime = it->second;
            casa::Vector<casa::Bool> maskTime = itsMaskTimes[it->first];

            // generate the flagging stats
            casa::MaskedArray<casa::Float> maskedAmplitudes(aveTime, maskTime);
            casa::Vector<casa::Float>
                statsVector = getRobustStats(maskedAmplitudes);
            casa::Float median = statsVector[0];
            casa::Float sigma_IQR = statsVector[1];

            // check min and max relative to thresholds.
            // do not loop over data again if all unflagged times are good
            if ((statsVector[2] < median-itsTimesThreshold*sigma_IQR) ||
                (statsVector[3] > median+itsTimesThreshold*sigma_IQR)) {

                for (size_t t = 0; t < aveTime.size(); ++t) {
                    if (maskTime[t] == casa::False) continue;
                    if ((aveTime[t] < median-itsTimesThreshold*sigma_IQR) ||
                        (aveTime[t] > median+itsTimesThreshold*sigma_IQR)) {
                        maskTime[t] = casa::False;
                    }
                }

            }

        }

    }

    itsAverageFlagsAreReady = casa::True;

}

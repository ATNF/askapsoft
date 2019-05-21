/// @file StokesVFlagger.h
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

#ifndef ASKAP_CP_PIPELINETASKS_STOKESVFLAGGER_H
#define ASKAP_CP_PIPELINETASKS_STOKESVFLAGGER_H

// System includes
#include <vector>
#include <map>

// ASKAPsoft includes
#include "Common/ParameterSet.h"
#include "boost/shared_ptr.hpp"
#include "casacore/casa/aipstype.h"
#include "casacore/ms/MeasurementSets/MeasurementSet.h"
#include "casacore/ms/MeasurementSets/MSColumns.h"
#include "casacore/ms/MeasurementSets/MSPolColumns.h"
#include "casacore/ms/MeasurementSets/StokesConverter.h"

// Local package includes
#include "cflag/IFlagger.h"
#include "cflag/FlaggingStats.h"

namespace askap {
namespace cp {
namespace pipelinetasks {

/// @brief Performs flagging based on Stokes-V thresholding. For each row
/// the mean and standard deviation for all Stokes-V correlations (i.e. all
/// channels within a given row). Then, where the stokes-V correlation exceeds
/// the average plus (stddev * threshold) all correlations for that channel in
/// that row will be flagged.
///
/// The one parameter that is read from the parset passed to the constructor is
/// "threshold". To flag at the five-sigma point specify a valud of "5.0".
class StokesVFlagger : public IFlagger {

    public:

        /// @brief Constructs zero or more instances of the StokesVFlagger.
        /// The flagger is responsible for reading the "parset" and constructing
        /// zero or more instances of itself, depending on the configuration.
        static std::vector< boost::shared_ptr<IFlagger> > build(
                const LOFAR::ParameterSet& parset,
                const casacore::MeasurementSet& ms);

        /// @brief Constructor
        StokesVFlagger(float threshold, bool robustStatistics,
                       bool integrateSpectra, float spectraThreshold,
                       bool integrateTimes, float timesThreshold,
                       bool quickRobust);

        /// @see IFlagger::processRow()
        virtual void processRow(casacore::MSColumns& msc, const casacore::uInt pass,
                                const casacore::uInt row, const bool dryRun);

        /// @see IFlagger::processRows()
        virtual void processRows(casacore::MSColumns& msc, const casacore::uInt pass,
                                 const casacore::uInt row, const casacore::uInt nrow,
                                 const bool dryRun);

        /// @see IFlagger::stats()
        virtual FlaggingStats stats(void) const;

        /// @see IFlagger::stats()
        virtual casacore::Bool processingRequired(const casacore::uInt pass);

    private:

        /// Returns an instance of a stokes converter that will convert to Stokes-V.
        /// The converter is cached, and as such a reference to the
        /// appropriate converter in the cache is returned. The reference is valid
        /// as long as the instance of this StokesVFlagger class exists.
        ///
        /// @param[in] polc a reference to the polarisation table data. This data
        ///                 describes which polarisation products exist in a
        ///                 given measurement set row.
        /// @param[in] polId    the index number for the polarisation description
        ///                     of interest.
        /// @return a reference to n instance of a stokes converter that will
        ///         convert to Stokes-V given the input products described by
        ///         the polc and polId parameters.
        casacore::StokesConverter& getStokesConverter(const casacore::ROMSPolarizationColumns& polc,
                const casacore::Int polId);

        // Flagging statistics
        FlaggingStats itsStats;

        // Flagging threshold (in standard deviations)
        float itsThreshold;

        // Use the median and interquartile range to estimate the mean and stddev
        bool itsRobustStatistics;

        // Generate averaged spectra and search these for peaks to flag
        bool itsIntegrateSpectra;
        // Flagging threshold
        casacore::Float itsSpectraThreshold;

        // Generate averaged time series and search these for peaks to flag
        bool itsIntegrateTimes;
        // Flagging threshold
        casacore::Float itsTimesThreshold;

        // When integrating, used to limit flag generation to a single call to
        // "processRow"
        bool itsAverageFlagsAreReady;

        // StokesConverter cache
        std::map<casacore::Int, casacore::StokesConverter> itsConverterCache;

        // Calculate the median, the interquartile range, the min and the max
        // of a simple array without masking
        casacore::Vector<casacore::Float> getRobustStats(casacore::Vector<casacore::Float> amplitudes);

        // Calculate the median, the interquartile range, the min and the max
        // of a masked array
        casacore::Vector<casacore::Float>getRobustStats(casacore::MaskedArray<casacore::Float> maskedAmplitudes);

        // Generate a key for a given row and polarisation
        rowKey getRowKey(const casacore::MSColumns& msc, const casacore::uInt row);

        // Maps of accumulation std::vectors for averaging spectra and generating flags
        std::map<rowKey, casacore::Vector<casacore::Double> > itsAveSpectra;
        std::map<rowKey, casacore::Vector<casacore::Bool> > itsMaskSpectra;
        std::map<rowKey, casacore::Vector<casacore::Int> > itsCountSpectra;

        // Maps of accumulation std::vectors for averaging time series and generating flags
        std::map<rowKey, casacore::Vector<casacore::Float> > itsAveTimes;
        std::map<rowKey, casacore::Vector<casacore::Bool> > itsMaskTimes;
        std::map<rowKey, casacore::Int> itsCountTimes;

        // Functions to handle accumulation std::vectors and indices
        void updateTimeVectors(const rowKey &key, const casacore::uInt pass);
        void initSpectrumVectors(const rowKey &key, const casacore::IPosition &shape);

        // Set flags based on integrated quantities
        void setFlagsFromIntegrations(void);

};

}
}
}

#endif

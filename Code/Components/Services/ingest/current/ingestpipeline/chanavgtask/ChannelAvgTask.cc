/// @file ChannelAvgTask.cc
///
/// @copyright (c) 2010 CSIRO
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
#include "ChannelAvgTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/askap/AskapLogging.h"
#include "askap/askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Cube.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "configuration/Configuration.h"

ASKAP_LOGGER(logger, ".ChannelAvgTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;

ChannelAvgTask::ChannelAvgTask(const LOFAR::ParameterSet& parset,
        const Configuration& /* config */) :
    itsParset(parset)
{
    ASKAPLOG_DEBUG_STR(logger, "Constructor");
    itsAveraging = parset.getUint32("averaging");
}

ChannelAvgTask::~ChannelAvgTask()
{
    ASKAPLOG_DEBUG_STR(logger, "Destructor");
}

void ChannelAvgTask::process(VisChunk::ShPtr& chunk)
{
    if (itsAveraging < 2) {
        // No averaging required for 0 or 1
        return;
    }

    const casacore::uInt nChanOriginal = chunk->nChannel();
    if (nChanOriginal % itsAveraging != 0) {
        ASKAPTHROW(AskapError, "Number of channels not a multiple of averaging number");
    }
    const casacore::uInt nChanNew = nChanOriginal / itsAveraging;

    // Average frequencies vector
    const casacore::Vector<casacore::Double>& origFreq = chunk->frequency();
    casacore::Vector<casacore::Double> newFreq(nChanNew);

    for (casacore::uInt newIdx = 0; newIdx < nChanNew; ++newIdx) {
        const casacore::uInt origIdx = itsAveraging * newIdx;
        casacore::Double sum = 0.0;
        for (casacore::uInt i = 0; i < itsAveraging; ++i) {
            sum += origFreq(origIdx + i);
        }
        newFreq(newIdx) = sum / itsAveraging;
    }

    // Update the channel width
    chunk->channelWidth() = chunk->channelWidth() * itsAveraging;

    // Average vis and flag cubes
    const casacore::uInt nRow = chunk->nRow();
    const casacore::uInt nPol = chunk->nPol();
    const casacore::Cube<casacore::Complex>& origVis = chunk->visibility();
    const casacore::Cube<casacore::Bool>& origFlag = chunk->flag();
    casacore::Cube<casacore::Complex> newVis(nRow, nChanNew, nPol);
    casacore::Cube<casacore::Bool> newFlag(nRow, nChanNew, nPol);

    for (casacore::uInt row = 0; row < nRow; ++row) {
        for (casacore::uInt newIdx = 0; newIdx < nChanNew; ++newIdx) {
            for (casacore::uInt pol = 0; pol < nPol; ++pol) {

                // Track the samples added, since those flagged are not
                casacore::uInt numGoodSamples = 0;

                // Calculate the average over the number of samples to
                // be averaged together (itsAveraging)
                const casacore::uInt origIdx = itsAveraging * newIdx;
                casacore::Complex sum(0.0, 0.0);
                for (casacore::uInt i = 0; i < itsAveraging; ++i) {
                    // Only sum if not flagged
                    if (!origFlag(row, origIdx + i, pol)) {
                        sum += origVis(row, origIdx + i, pol);
                        numGoodSamples++;
                    }
                }

                if (numGoodSamples > 0) {
                    newVis(row, newIdx, pol) = casacore::Complex(sum.real() / numGoodSamples,
                                                             sum.imag() / numGoodSamples);
                    newFlag(row, newIdx, pol) = false;
                } else {
                    newVis(row, newIdx, pol) = casacore::Complex(0.0, 0.0);
                    newFlag(row, newIdx, pol) = true;
                }

            }
        }
    }

    chunk->resize(newVis, newFlag, newFreq);
}

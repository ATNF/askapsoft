/// @file VariableThresholdingHelpers.cc
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

#include <preprocessing/VariableThresholdingHelpers.h>

#include <casacore/casa/aipstype.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/ArrayPartMath.h>
#include <casacore/casa/Arrays/MaskArrMath.h>
#include <casacore/casa/namespace.h>

#include <duchamp/Utils/Statistics.hh>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".varthreshhelp");

namespace askap {

namespace analysis {

void slidingBoxStats(casacore::Array<Float> &input,
                     casacore::Array<Float> &middle,
                     casacore::Array<Float> &spread,
                     casacore::IPosition &box,
                     bool useRobust)
{
    ASKAPASSERT(input.shape() == middle.shape());
    ASKAPASSERT(input.shape() == spread.shape());

    if (useRobust) {
        middle = casacore::slidingArrayMath(input, box, MedianFunc<Float>());
        spread = casacore::slidingArrayMath(input, box, MadfmFunc<Float>()) /
                 Statistics::correctionFactor;
    } else {
        middle = casacore::slidingArrayMath(input, box, MeanFunc<Float>());
	// now has an argument == dof
        spread = casacore::slidingArrayMath(input, box, StddevFunc<Float>(1));
    }
}

casacore::Array<Float> calcSNR(casacore::Array<Float> &input,
                           casacore::Array<Float> &middle,
                           casacore::Array<Float> &spread)
{
    ASKAPASSERT(input.shape() == middle.shape());
    ASKAPASSERT(input.shape() == spread.shape());
    casacore::Array<Float> snr(input.shape(), 0.);
    // Make sure we don't divide by the zeros around the edge of
    // madfm. Need to set those values to S/N=0.
    Array<Float>::iterator inputEnd(input.end());
    Array<Float>::iterator iterInput(input.begin()), iterMiddle(middle.begin()), iterSpread(spread.begin()), iterSnr(snr.begin());
    while (iterInput != inputEnd) {
        *iterSnr = (*iterSpread > 0) ? (*iterInput - *iterMiddle) / (*iterSpread) : 0.;
        iterInput++;
        iterMiddle++;
        iterSpread++;
        iterSnr++;
    }
    return snr;
}

void slidingBoxMaskedStats(casacore::MaskedArray<Float> &input,
                           casacore::Array<Float> &middle,
                           casacore::Array<Float> &spread,
                           casacore::IPosition &box,
                           bool useRobust)
{
    ASKAPASSERT(input.shape() == middle.shape());
    ASKAPASSERT(input.shape() == spread.shape());

    if (useRobust) {
        middle = casacore::slidingArrayMath(input, box, MaskedMedianFunc<Float>());
        spread = casacore::slidingArrayMath(input, box, MaskedMadfmFunc<Float>()) /
                 Statistics::correctionFactor;
    } else {
        middle = casacore::slidingArrayMath(input, box, MaskedMeanFunc<Float>());
        spread = casacore::slidingArrayMath(input, box, MaskedStddevFunc<Float>());
    }
}

casacore::Array<Float> calcMaskedSNR(casacore::MaskedArray<Float> &input,
                                 casacore::Array<Float> &middle,
                                 casacore::Array<Float> &spread)
{
    ASKAPASSERT(input.shape() == middle.shape());
    ASKAPASSERT(input.shape() == spread.shape());
    casacore::Array<Float> snr(input.shape(), 0.);
    // Make sure we don't divide by the zeros around the edge of
    // madfm. Need to set those values to S/N=0.

    const Array<Float> &inputArr = input.getArray();
    const LogicalArray &inputMask = input.getMask();

    Array<Float>::const_iterator inputEnd(inputArr.end());
    Array<Float>::const_iterator iterInput(inputArr.begin());
    LogicalArray::const_iterator iterMask(inputMask.begin());
    Array<Float>::iterator iterMiddle(middle.begin());
    Array<Float>::iterator iterSpread(spread.begin());
    Array<Float>::iterator iterSnr(snr.begin());
    while (iterInput != inputEnd) {
        *iterSnr = (*iterSpread > 0 && (*iterMask)) ?
                   (*iterInput - *iterMiddle) / (*iterSpread) : 0.;
        iterInput++;
        iterMask++;
        iterMiddle++;
        iterSpread++;
        iterSnr++;
    }
    return snr;
}


}

}

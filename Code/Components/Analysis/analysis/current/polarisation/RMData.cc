/// @file
///
/// Implementation of RM data measurement
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
///
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#include <polarisation/RMData.h>
#include <askap_analysis.h>
#include <polarisation/RMSynthesis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <complex>
#include <casacore/casa/BasicSL/Complex.h>

#include <Common/ParameterSet.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".rmdata");

namespace askap {

namespace analysis {

static const float defaultSNRthreshold = 8.0;
static const float defaultDebiasThreshold = 5.0;

RMData::RMData(const LOFAR::ParameterSet &parset):
    itsDetectionThreshold(parset.getFloat("polThresholdSNR", defaultSNRthreshold)),
    itsDebiasThreshold(parset.getFloat("polThresholdDebias", defaultDebiasThreshold))
{

}

void RMData::calculate(RMSynthesis *rmsynth)
{
    const casa::Vector<casa::Complex> fdf = rmsynth->fdf();
    casa::Vector<float> fdf_p = casa::amplitude(fdf);
    const casa::Vector<float> phi_rmsynth = rmsynth->phi();
    const float noise = rmsynth->fdf_noise();
    const float RMSF_FWHM = rmsynth->rmsf_width();
    const float lsqzero = rmsynth->refLambdaSq();
    const unsigned int numChan = rmsynth->numFreqChan();

    float minFDF, maxFDF;
    casa::IPosition locMin, locMax;
    casa::minMax<float>(minFDF, maxFDF, locMin, locMax, fdf_p);

    itsSNR = maxFDF / noise;
    itsFlagDetection = (itsSNR > itsDetectionThreshold);

    if (itsFlagDetection) {

        itsPintPeak = maxFDF;
        itsPintPeak_err = M_SQRT2 * fabs(itsPintPeak) / noise;
        itsPintPeakEff = sqrt(maxFDF * maxFDF - 2.3 * noise);

        itsPhiPeak = phi_rmsynth(locMax);
        itsPhiPeak_err = RMSF_FWHM * noise / (2. * itsPintPeak);


//      itsPolAngleRef = 0.5 * casa::phase(fdf(locMax));  // THIS MAY NOT WORK - NEED TO CHECK
        itsPolAngleRef_err = 0.5 * noise / fabs(itsPintPeak);

        itsPolAngleZero = itsPolAngleRef - itsPhiPeak * lsqzero;
        itsPolAngleZero_err = (1. / (4.*(numChan - 2) * itsPintPeak * itsPintPeak)) *
                              (float((numChan - 1) / numChan) + pow(lsqzero, 4) /
                               rmsynth->lsqVariance());

//      itsFracPol
        // Need to get the Imodel into the RMSynth object somehow.

    } else {

        //itsPintPeak = maxFDF * itsDetectionThreshold;
        itsPintPeak = noise * itsDetectionThreshold;
        itsPintPeakEff = -1.;




    }

}

//----------------------------------------//

void RMData::printSummary()
{
    if (itsFlagDetection) {

        std::cout << "Detected!" << std::endl;
        std::cout << "Peak Polarised intensity = "
                  << itsPintPeak << std::endl;
        std::cout << "Peak Polarised intensity (error) = "
                  << itsPintPeak_err << std::endl;
        std::cout << "RM of Peak Polarised intensity = "
                  << itsPhiPeak << std::endl;
        std::cout << "RM of Peak Polarised intensity (error) = "
                  << itsPhiPeak_err << std::endl;
        std::cout << "Peak Polarised intensity (effective) = "
                  << itsPintPeakEff << std::endl;

    } else {

        std::cout << "Not detected." << std::endl;
        std::cout << "Limit on peak polarised intensity = "
                  << itsPintPeak << std::endl;

    }




}


}
}

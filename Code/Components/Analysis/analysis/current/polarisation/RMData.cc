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

#include <mathsutils/MathsUtils.h>

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
    itsPintPeak=0.;
    itsPintPeak_err=0.;
    itsPintPeakEff=0.;
    itsPhiPeak=0.;
    itsPhiPeak_err=0.;
    itsPintPeakFit=0.;
    itsPintPeakFit_err=0.;
    itsPintPeakFitEff=0.;
    itsPhiPeakFit=0.;
    itsPhiPeakFit_err=0.;

    itsFlagDetection=false;
    itsFlagEdge=false;

    itsPolAngleRef=0.;
    itsPolAngleRef_err=0.;
    itsPolAngleZero=0.;
    itsPolAngleZero_err=0.;

    itsFracPol=0.;
    itsFracPol_err=0.;

    itsSNR=0.;
    itsSNR_err=0.;
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

    ASKAPLOG_DEBUG_STR(logger, fdf_p);
    
    float minFDF, maxFDF;
    casa::IPosition locMin, locMax;
    casa::minMax<float>(minFDF, maxFDF, locMin, locMax, fdf_p);
    ASKAPLOG_DEBUG_STR(logger, "minFDF="<<minFDF<< ", maxFDF="<<maxFDF);

    itsSNR = maxFDF / noise;
    itsFlagDetection = (itsSNR > itsDetectionThreshold);

    if (itsFlagDetection) {

        itsPintPeak = maxFDF;
        itsPintPeak_err = noise;
        itsPintPeakEff = sqrt(maxFDF * maxFDF - 2.3 * noise);

        itsPhiPeak = phi_rmsynth(locMax);
        itsPhiPeak_err = RMSF_FWHM * noise / (2. * itsPintPeak);

        // Fitting
        if (locMax>0 && locMax<(fdf_p.size()-1) ){

            std::vector<float> fit =
                analysisutilities::fit3ptParabola(phi_rmsynth(locMax-1),fdf_p(locMax-1),
                                                  phi_rmsynth(locMax),fdf_p(locMax),
                                                  phi_rmsynth(locMax+1),fdf_p(locMax+1));
            
            itsPintPeakFit = fit[2]-(fit[1]*fit[1])/(4. * fit[0]);
            itsPhiPeakFit = -1. * fit[1] / (2. * fit[0]);
            // @todo The following is wrong, I think - should be normalised by the SNR, not the noise.
            // Check Condon+98
            //itsPintPeakFit_err = M_SQRT2 * fabs(itsPintPeakFit) / noise;
            itsPintPeakFit_err = noise;
            itsPhiPeakFit_err = RMSF_FWHM * noise / (2. * itsPintPeakFit);

        }
        

        itsPolAngleRef = 0.5 * casa::arg(fdf(locMax)) * 180./M_PI;  
        itsPolAngleRef_err = 0.5 * noise / fabs(itsPintPeak) * 180./M_PI;

        itsPolAngleZero = itsPolAngleRef - itsPhiPeak * lsqzero;
        itsPolAngleZero_err = (1. / (4.*(numChan - 2) * itsPintPeak * itsPintPeak)) *
            (float((numChan - 1) / numChan) + pow(lsqzero, 4) /
             rmsynth->lsqVariance());


        
        itsFracPol = itsPintPeak / rmsynth->Imodel_refLambdaSq();
        itsFracPol_err = sqrt(itsPintPeak_err*itsPintPeak_err + noise*noise);

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
        std::cout << "Fitted Peak Polarised intensity = "
                  << itsPintPeakFit << std::endl;
        std::cout << "Fitted Peak Polarised intensity (error) = "
                  << itsPintPeakFit_err << std::endl;
        std::cout << "RM of Fitted Peak Polarised intensity = "
                  << itsPhiPeakFit << std::endl;
        std::cout << "RM of Fitted Peak Polarised intensity (error) = "
                  << itsPhiPeakFit_err << std::endl;
        std::cout << "Pol. angle reference = "
                  << itsPolAngleRef << std::endl;
        std::cout << "Pol. angle reference (error) = "
                  << itsPolAngleRef_err << std::endl;

    } else {

        std::cout << "Not detected." << std::endl;
        std::cout << "Limit on peak polarised intensity = "
                  << itsPintPeak << std::endl;

    }




}


}
}

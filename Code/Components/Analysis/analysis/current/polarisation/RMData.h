/// @file
///
/// Holds the measured parameters taken from the results of RM Synthesis
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
#ifndef ASKAP_ANALYSIS_RM_DATA_H_
#define ASKAP_ANALYSIS_RM_DATA_H_
#include <polarisation/RMSynthesis.h>
#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

/// @brief A class to handle the parameterisation of RM Synthesis
/// results.
/// @details This class encapsulates many of the measurements required
/// to produce the polarisation catalogue entry. It taks the results
/// of RM Synthesis on a single component, and measures various
/// parameters of the peak of the FDF, as well as properties of the
/// RMSF.
class RMData {
    public:
        /// @brief Constructor
        /// @details Initialises certain parameters from the parset, and
        /// sets all others to zero prior to their measurement.
        /// @param Input parset
        RMData(const LOFAR::ParameterSet &parset);
        virtual ~RMData() {};

        /// @brief Calculate RM parameters
        /// @details Uses the RM Synthesis results to evaluate all
        /// parameters. Whether or not most are calculated depends on
        /// whether the SNR of the peak of the Faraday Dispersion Function
        /// is above the requested threshold.
        /// @param rmsynth The RM Synthesis results. The RM Synthesis
        /// should have been performed so that the arrays are available.
        void calculate(RMSynthesis *rmsynth);

        /// @brief Prints a summary of the results to stdout.
        void printSummary();

        /// @brief Returns the requested detection threshold
        const float detectionThreshold() { return itsDetectionThreshold;};
        /// @brief Returns the requested threshold for debiasing
        const float debiasThreshold() { return itsDebiasThreshold;};
        /// @brief Return the peak polarised intensity from the FDF
        const float pintPeak() {return itsPintPeak;};
        /// @brief Return the uncertainty in pintPeak()
        const float pintPeak_err() {return itsPintPeak_err;};
        /// @brief Return the effective peak polarised intensity (after debiasing)
        const float pintPeakEff() {return itsPintPeakEff;};
        /// @brief Return the Faraday depth at the peak channel of the FDF
        const float phiPeak() {return itsPhiPeak;};
        /// @brief Return the uncertainty in phiPeak()
        const float phiPeak_err() {return itsPhiPeak_err;};
        /// @brief Return the fitted peak polarised intensity
        const float pintPeakFit() {return itsPintPeakFit;};
        /// @brief Return the uncertainty in pintPeakFit()
        const float pintPeakFit_err() {return itsPintPeakFit_err;};
        /// @brief Return the debiased value of pintPeakFit()
        const float pintPeakFitEff() {return itsPintPeakFitEff;};
        /// @brief Return the Faraday Depth of the fitted peak of the FDF
        const float phiPeakFit() {return itsPhiPeakFit;};
        /// @brief Return the uncertainty in phiPeakFit()
        const float phiPeakFit_err() {return itsPhiPeakFit_err;};
        /// @brief Return true if there was a significant detection in the FDF
        const bool flagDetection() {return itsFlagDetection;};
        /// @brief Return true if the peak lies at the edge of the FDF
        const bool flagEdge() {return itsFlagEdge;};
        /// @brief Return the polarisation position angle at the reference frequency (or wavelength=lambda_0)
        const float polAngleRef() {return itsPolAngleRef;};
        /// @brief Return the uncertainty in polAngleRef()
        const float polAngleRef_err() {return itsPolAngleRef_err;};
        /// @brief Return the polarisation position angle de-rotated to lambda=0
        const float polAngleZero() {return itsPolAngleZero;};
        /// @brief Return the uncertainty in polAngleZero()
        const float polAngleZero_err() {return itsPolAngleZero_err;};
        /// @brief Return the fractional polarised intensity (at the FDF peak)
        const float fracPol() {return itsFracPol;};
        /// @brief Return the uncertainty in fracPol()
        const float fracPol_err() {return itsFracPol_err;};
        /// @brief Return the signal-to-noise ratio of the FDF peak
        const float SNR() {return itsSNR;};
        /// @brief Return the uncertainty in SNR()
        const float SNR_err() {return itsSNR_err;};

    private:

        /// @brief The user-defined SNR threshold for the peak to be declared a detection
        float itsDetectionThreshold;
        /// @brief The user-defined SNR threshold for debiasing to be done
        float itsDebiasThreshold;

        /// @brief The peak polarised intensity channel value of the FDF
        float itsPintPeak;
        /// @brief Uncertainty in itsPintPeak
        float itsPintPeak_err;
        /// @brief De-biased value of peak polarised intensity
        float itsPintPeakEff;
        /// @brief Faraday depth of the peak channel in the FDF
        float itsPhiPeak;
        /// @brief Uncertainty in itsPhiPeak
        float itsPhiPeak_err;
        /// @brief Peak polarised intensity of the FDF from a fit to the peak
        float itsPintPeakFit;
        /// @brief Uncertainty in itsPintPeakFit
        float itsPintPeakFit_err;
        /// @brief De-biased value of the fitted peak polarised intensity
        float itsPintPeakFitEff;
        /// @brief Faraday depth of the fitted peak polarised intensity
        float itsPhiPeakFit;
        /// @brief Uncertainty in itsPhiPeakFit
        float itsPhiPeakFit_err;

        /// @brief True if the peak meets the detection threshold itsDetectionThreshold
        bool  itsFlagDetection;
        /// @brief True if the peak lies close to the edge of the FDF
        bool  itsFlagEdge;

        /// @brief Polarisation angle at the reference frequency/wavelength
        float itsPolAngleRef;
        /// @brief Uncertainty in itsPolAngleRef
        float itsPolAngleRef_err;
        /// @brief Polarisation angle de-rotated to lambda=0
        float itsPolAngleZero;
        /// @brief Uncertainty in itsPolAngleZero
        float itsPolAngleZero_err;

        /// @brief Fractional polarised intensity at the FDF peak
        float itsFracPol;
        /// @brief Uncertainty in itsFracPol
        float itsFracPol_err;

        /// @brief Signal-to-noise Ratio at the FDF peak
        float itsSNR;
        /// @brief Uncertainty in itsSNR
        float itsSNR_err;



};

}

}

#endif

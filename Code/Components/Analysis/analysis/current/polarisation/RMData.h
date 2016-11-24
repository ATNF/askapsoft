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

class RMData {
    public:
        RMData(const LOFAR::ParameterSet &parset);
        virtual ~RMData() {};

        void calculate(RMSynthesis *rmsynth);

        void printSummary();

        const float detectionThreshold() { return itsDetectionThreshold;};
        const float debiasThreshold() { return itsDebiasThreshold;};
        const float pintPeak() {return itsPintPeak;};
        const float pintPeak_err() {return itsPintPeak_err;};
        const float pintPeakEff() {return itsPintPeakEff;};
        const float phiPeak() {return itsPhiPeak;};
        const float phiPeak_err() {return itsPhiPeak_err;};
        const float pintPeakFit() {return itsPintPeakFit;};
        const float pintPeakFit_err() {return itsPintPeakFit_err;};
        const float pintPeakFitEff() {return itsPintPeakFitEff;};
        const float phiPeakFit() {return itsPhiPeakFit;};
        const float phiPeakFit_err() {return itsPhiPeakFit_err;};
        const bool flagDetection() {return itsFlagDetection;};
        const bool flagEdge() {return itsFlagEdge;};
        const float polAngleRef() {return itsPolAngleRef;};
        const float polAngleRef_err() {return itsPolAngleRef_err;};
        const float polAngleZero() {return itsPolAngleZero;};
        const float polAngleZero_err() {return itsPolAngleZero_err;};
        const float fracPol() {return itsFracPol;};
        const float fracPol_err() {return itsFracPol_err;};
        const float SNR() {return itsSNR;};
        const float SNR_err() {return itsSNR_err;};

    private:

        float itsDetectionThreshold;
        float itsDebiasThreshold;

        float itsPintPeak;
        float itsPintPeak_err;
        float itsPintPeakEff;
        float itsPhiPeak;
        float itsPhiPeak_err;
        float itsPintPeakFit;
        float itsPintPeakFit_err;
        float itsPintPeakFitEff;
        float itsPhiPeakFit;
        float itsPhiPeakFit_err;

        bool itsFlagDetection;
        bool itsFlagEdge;

        float itsPolAngleRef;
        float itsPolAngleRef_err;
        float itsPolAngleZero;
        float itsPolAngleZero_err;

        float itsFracPol;
        float itsFracPol_err;

        float itsSNR;
        float itsSNR_err;



};

}

}

#endif

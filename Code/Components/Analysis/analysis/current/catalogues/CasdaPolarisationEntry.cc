/// @file
///
/// Implementation of the polarisation components for CASDA
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
#include <catalogues/CasdaPolarisationEntry.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaIsland.h>
#include <catalogues/casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <polarisation/RMSynthesis.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>

#include <vector>

ASKAP_LOGGER(logger, ".casdapolarisation");

namespace askap {

namespace analysis {

static const float defaultSNRthreshold = 8.0;
static const float defaultDebiasThreshold = 5.0;

CasdaPolarisationEntry::CasdaPolarisationEntry(const CasdaComponent &comp,
        RMSynthesis &rmsynth,
        const LOFAR::ParameterSet &parset):
    CatalogueEntry(parset),
    itsDetectionThreshold(parset.getFloat("polThresholdSNR", defaultSNRthreshold)),
    itsDebiasThreshold(parset.getFloat("polThresholdDebias", defaultDebiasThreshold))
{
    const casa::Vector<casa::Complex> fdf = rmsynth.fdf();
    casa::Vector<float> fdf_p = casa::amplitude(fdf);
    const casa::Vector<float> phi_rmsynth = rmsynth.phi();
    const float noise = rmsynth.fdf_noise();
    const float RMSF_FWHM = rmsynth.rmsf_width();
    const float lsqzero = rmsynth.refLambdaSq();
    const unsigned int numChan = rmsynth.numFreqChan();

    float minFDF, maxFDF;
    casa::IPosition locMin, locMax;
    casa::minMax<float>(minFDF, maxFDF, locMin, locMax, fdf_p);

    itsPintFitSNR = maxFDF / noise;
    itsFlagDetection = (itsPintFitSNR > itsDetectionThreshold);

    if (itsFlagDetection) {

        itsPintPeak = maxFDF;
        itsPintPeak_err = M_SQRT2 * fabs(itsPintPeak) / noise;
        itsPintPeakDebias = sqrt(maxFDF * maxFDF - 2.3 * noise);

        itsPhiPeak = phi_rmsynth(locMax);
        itsPhiPeak_err = RMSF_FWHM * noise / (2. * itsPintPeak);


//      itsPolAngleRef = 0.5 * casa::phase(fdf(locMax));  // THIS MAY NOT WORK - NEED TO CHECK
        itsPolAngleRef_err = 0.5 * noise / fabs(itsPintPeak);

        itsPolAngleZero = itsPolAngleRef - itsPhiPeak * lsqzero;
        itsPolAngleZero_err = (1. / (4.*(numChan - 2) * itsPintPeak * itsPintPeak)) *
                              (float((numChan - 1) / numChan) + pow(lsqzero, 4) /
                               rmsynth.lsqVariance());

//      itsFracPol
        // Need to get the Imodel into the RMSynth object somehow.

    } else {

        //itsPintPeak = maxFDF * itsDetectionThreshold;
        itsPintPeak = noise * itsDetectionThreshold;
        itsPintPeakDebias = -1.;




    }

}

const float CasdaPolarisationEntry::ra()
{
    return itsRA;
}

const float CasdaPolarisationEntry::dec()
{
    return itsDEC;
}


void CasdaPolarisationEntry::printTableRow(std::ostream &stream,
        duchamp::Catalogues::CatalogueSpecification &columns)
{
    stream.setf(std::ios::fixed);
    for (size_t i = 0; i < columns.size(); i++) {
        this->printTableEntry(stream, columns.column(i));
    }
    stream << "\n";
}


void CasdaPolarisationEntry::printTableEntry(std::ostream &stream,
        duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.printEntry(stream, itsComponentID);
    } else if (type == "NAME") {
        column.printEntry(stream, itsName);
    } else if (type == "RAJD") {
        column.printEntry(stream, itsRA);
    } else if (type == "DECJD") {
        column.printEntry(stream, itsDEC);
    } else if (type == "IFLUX") {
        column.printEntry(stream, itsFluxImedian);
    } else if (type == "QFLUX") {
        column.printEntry(stream, itsFluxQmedian);
    } else if (type == "UFLUX") {
        column.printEntry(stream, itsFluxUmedian);
    } else if (type == "VFLUX") {
        column.printEntry(stream, itsFluxImedian);
    } else if (type == "RMS_I") {
        column.printEntry(stream, itsRmsI);
    } else if (type == "RMS_Q") {
        column.printEntry(stream, itsRmsQ);
    } else if (type == "RMS_U") {
        column.printEntry(stream, itsRmsU);
    } else if (type == "RMS_V") {
        column.printEntry(stream, itsRmsV);
    } else if (type == "CO1") {
        column.printEntry(stream, itsPolyCoeff0);
    } else if (type == "CO2") {
        column.printEntry(stream, itsPolyCoeff1);
    } else if (type == "CO3") {
        column.printEntry(stream, itsPolyCoeff2);
    } else if (type == "CO4") {
        column.printEntry(stream, itsPolyCoeff3);
    } else if (type == "CO5") {
        column.printEntry(stream, itsPolyCoeff4);
    } else if (type == "LAMSQ") {
        column.printEntry(stream, itsLambdaSqRef);
    } else if (type == "RMSF") {
        column.printEntry(stream, itsRmsfFwhm);
    } else if (type == "POLPEAK") {
        column.printEntry(stream, itsPintPeak);
    } else if (type == "POLPEAKDB") {
        column.printEntry(stream, itsPintPeakDebias);
    } else if (type == "POLPEAKERR") {
        column.printEntry(stream, itsPintPeak_err);
    } else if (type == "POLPEAKFIT") {
        column.printEntry(stream, itsPintPeakFit);
    } else if (type == "POLPEAKFITDB") {
        column.printEntry(stream, itsPintPeakFitDebias);
    } else if (type == "POLPEAKFITERR") {
        column.printEntry(stream, itsPintPeakFit_err);
    } else if (type == "POLPEAKFITSNR") {
        column.printEntry(stream, itsPintFitSNR);
    } else if (type == "POLPEAKFITSNRERR") {
        column.printEntry(stream, itsPintFitSNR_err);
    } else if (type == "FDPEAK") {
        column.printEntry(stream, itsPhiPeak);
    } else if (type == "FDPEAKERR") {
        column.printEntry(stream, itsPhiPeak_err);
    } else if (type == "FDPEAKFIT") {
        column.printEntry(stream, itsPhiPeakFit);
    } else if (type == "FDPEAKFITERR") {
        column.printEntry(stream, itsPhiPeakFit_err);
    } else if (type == "POLANG") {
        column.printEntry(stream, itsPolAngleRef);
    } else if (type == "POLANGERR") {
        column.printEntry(stream, itsPolAngleRef_err);
    } else if (type == "POLANG0") {
        column.printEntry(stream, itsPolAngleZero);
    } else if (type == "POLANG0ERR") {
        column.printEntry(stream, itsPolAngleZero_err);
    } else if (type == "POLFRAC") {
        column.printEntry(stream, itsFracPol);
    } else if (type == "POLFRACERR") {
        column.printEntry(stream, itsFracPol_err);
    } else if (type == "COMPLEX1") {
        column.printEntry(stream, itsComplexity);
    } else if (type == "COMPLEX2") {
        column.printEntry(stream, itsComplexity_screen);
    } else if (type == "FLAG1") {
        column.printEntry(stream, itsFlagDetection);
    } else if (type == "FLAG2") {
        column.printEntry(stream, itsFlagEdge);
    } else if (type == "FLAG3") {
        column.printEntry(stream, itsFlag3);
    } else if (type == "FLAG4") {
        column.printEntry(stream, itsFlag4);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }
}

void CasdaPolarisationEntry::checkCol(duchamp::Catalogues::Column &column)
{
    std::string type = column.type();
    if (type == "ID") {
        column.check(itsComponentID);
    } else if (type == "NAME") {
        column.check(itsName);
    } else if (type == "RAJD") {
        column.check(itsRA);
    } else if (type == "DECJD") {
        column.check(itsDEC);
    } else if (type == "IFLUX") {
        column.check(itsFluxImedian);
    } else if (type == "QFLUX") {
        column.check(itsFluxQmedian);
    } else if (type == "UFLUX") {
        column.check(itsFluxUmedian);
    } else if (type == "VFLUX") {
        column.check(itsFluxImedian);
    } else if (type == "RMS_I") {
        column.check(itsRmsI);
    } else if (type == "RMS_Q") {
        column.check(itsRmsQ);
    } else if (type == "RMS_U") {
        column.check(itsRmsU);
    } else if (type == "RMS_V") {
        column.check(itsRmsV);
    } else if (type == "CO1") {
        column.check(itsPolyCoeff0);
    } else if (type == "CO2") {
        column.check(itsPolyCoeff1);
    } else if (type == "CO3") {
        column.check(itsPolyCoeff2);
    } else if (type == "CO4") {
        column.check(itsPolyCoeff3);
    } else if (type == "CO5") {
        column.check(itsPolyCoeff4);
    } else if (type == "LAMSQ") {
        column.check(itsLambdaSqRef);
    } else if (type == "RMSF") {
        column.check(itsRmsfFwhm);
    } else if (type == "POLPEAK") {
        column.check(itsPintPeak);
    } else if (type == "POLPEAKDB") {
        column.check(itsPintPeakDebias);
    } else if (type == "POLPEAKERR") {
        column.check(itsPintPeak_err);
    } else if (type == "POLPEAKFIT") {
        column.check(itsPintPeakFit);
    } else if (type == "POLPEAKFITDB") {
        column.check(itsPintPeakFitDebias);
    } else if (type == "POLPEAKFITERR") {
        column.check(itsPintPeakFit_err);
    } else if (type == "POLPEAKFITSNR") {
        column.check(itsPintFitSNR);
    } else if (type == "POLPEAKFITSNRERR") {
        column.check(itsPintFitSNR_err);
    } else if (type == "FDPEAK") {
        column.check(itsPhiPeak);
    } else if (type == "FDPEAKERR") {
        column.check(itsPhiPeak_err);
    } else if (type == "FDPEAKFIT") {
        column.check(itsPhiPeakFit);
    } else if (type == "FDPEAKFITERR") {
        column.check(itsPhiPeakFit_err);
    } else if (type == "POLANG") {
        column.check(itsPolAngleRef);
    } else if (type == "POLANGERR") {
        column.check(itsPolAngleRef_err);
    } else if (type == "POLANG0") {
        column.check(itsPolAngleZero);
    } else if (type == "POLANG0ERR") {
        column.check(itsPolAngleZero_err);
    } else if (type == "POLFRAC") {
        column.check(itsFracPol);
    } else if (type == "POLFRACERR") {
        column.check(itsFracPol_err);
    } else if (type == "COMPLEX1") {
        column.check(itsComplexity);
    } else if (type == "COMPLEX2") {
        column.check(itsComplexity_screen);
    } else if (type == "FLAG1") {
        column.check(itsFlagDetection);
    } else if (type == "FLAG2") {
        column.check(itsFlagEdge);
    } else if (type == "FLAG3") {
        column.check(itsFlag3);
    } else if (type == "FLAG4") {
        column.check(itsFlag4);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaPolarisationEntry::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec)
{
    for (size_t i = 0; i < spec.size(); i++) {
        this->checkCol(spec.column(i));
    }
}



}

}

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
#include <catalogues/Casda.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <polarisation/RMSynthesis.h>
#include <polarisation/RMData.h>
#include <polarisation/FDFwriter.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>

#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>

#include <vector>

ASKAP_LOGGER(logger, ".casdapolarisation");

namespace askap {

namespace analysis {

CasdaPolarisationEntry::CasdaPolarisationEntry():
    CatalogueEntry()
{
}

CasdaPolarisationEntry::CasdaPolarisationEntry(CasdaComponent *comp,
        const LOFAR::ParameterSet &parset):
    CatalogueEntry(parset)
{

    itsRA = comp->ra();
    itsDec = comp->dec();
    itsName = comp->name();
    itsComponentID = comp->componentID();

    LOFAR::ParameterSet polParset = parset.makeSubset("RMSynthesis.");
    if(! polParset.isDefined("imagetype")){
        polParset.add("imagetype","fits");
    }

    PolarisationData poldata(polParset);
    poldata.initialise(comp);

    // Do the RM Synthesis, and calculate all parameters.
    RMSynthesis rmsynth(polParset);
    rmsynth.calculate(poldata);

    if (polParset.getBool("writeSpectra", "true")) {
        // write out the FDF array to image file on disk
        FDFwriter writer(polParset, poldata, rmsynth);
        writer.write();
    }

    // Parameterise the RM Synthesis results
    RMData rmdata(polParset);
    rmdata.calculate(&rmsynth);

    // Now assign the parameters

    itsDetectionThreshold = rmdata.detectionThreshold();
    itsDebiasThreshold = rmdata.debiasThreshold();

    casa::Unit cubeBunit = poldata.I().bunit();
    const double intFluxScale =
        casa::Quantum<float>(1.0, cubeBunit).getValue(casda::intFluxUnitContinuum);

    itsFluxImedian = poldata.I().median() * intFluxScale;
    itsFluxQmedian = poldata.Q().median() * intFluxScale;
    itsFluxUmedian = poldata.U().median() * intFluxScale;
    itsFluxVmedian = poldata.V().median() * intFluxScale;
    itsRmsI = poldata.I().medianNoise() * intFluxScale;
    itsRmsQ = poldata.Q().medianNoise() * intFluxScale;
    itsRmsU = poldata.U().medianNoise() * intFluxScale;
    itsRmsV = poldata.V().medianNoise() * intFluxScale;

    // Correct the scale for the first coefficient, as this is purely flux
    itsPolyCoeff0 = poldata.model().coeff(0) * intFluxScale;
    itsPolyCoeff1 = poldata.model().coeff(1);
    itsPolyCoeff2 = poldata.model().coeff(2);
    itsPolyCoeff3 = poldata.model().coeff(3);
    itsPolyCoeff4 = poldata.model().coeff(4);

    itsLambdaSqRef = rmsynth.refLambdaSq();
    itsRmsfFwhm = rmsynth.rmsf_width();

    itsPintPeak.value() = rmdata.pintPeak() * intFluxScale;
    itsPintPeak.error() = rmdata.pintPeak_err() * intFluxScale;
    itsPintPeakDebias = rmdata.pintPeakEff() * intFluxScale;
    itsPintPeakFit.value() = rmdata.pintPeakFit() * intFluxScale;
    itsPintPeakFit.error() = rmdata.pintPeakFit_err() * intFluxScale;
    itsPintPeakFitDebias = rmdata.pintPeakFitEff() * intFluxScale;

    itsPintFitSNR.value() = rmdata.SNR();
    itsPintFitSNR.error() = rmdata.SNR_err();

    itsPhiPeak.value() = rmdata.phiPeak();
    itsPhiPeak.error() = rmdata.phiPeak_err();
    itsPhiPeakFit.value() = rmdata.phiPeakFit();
    itsPhiPeakFit.error() = rmdata.phiPeakFit_err();

    itsPolAngleRef.value() = rmdata.polAngleRef();
    itsPolAngleRef.error() = rmdata.polAngleRef_err();
    itsPolAngleZero.value() = rmdata.polAngleZero();
    itsPolAngleZero.error() = rmdata.polAngleZero_err();

    itsFracPol.value() = rmdata.fracPol();
    itsFracPol.error() = rmdata.fracPol_err();

    itsComplexity = rmdata.complexityConstant();
    itsComplexity_screen = rmdata.complexityResidual();

    itsFlagDetection = rmdata.flagDetection() ? 1 : 0;
    itsFlagEdge = rmdata.flagEdge() ? 1 : 0;
    itsFlag3 = 0;
    itsFlag4 = 0;
}

const float CasdaPolarisationEntry::ra()
{
    return itsRA;
}

const float CasdaPolarisationEntry::dec()
{
    return itsDec;
}

const std::string CasdaPolarisationEntry::id()
{
    return itsComponentID;
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
        column.printEntry(stream, itsDec);
    } else if (type == "IFLUX") {
        column.printEntry(stream, itsFluxImedian);
    } else if (type == "QFLUX") {
        column.printEntry(stream, itsFluxQmedian);
    } else if (type == "UFLUX") {
        column.printEntry(stream, itsFluxUmedian);
    } else if (type == "VFLUX") {
        column.printEntry(stream, itsFluxVmedian);
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
        column.printEntry(stream, itsPintPeak.value());
    } else if (type == "POLPEAKDB") {
        column.printEntry(stream, itsPintPeakDebias);
    } else if (type == "POLPEAKERR") {
        column.printEntry(stream, itsPintPeak.error());
    } else if (type == "POLPEAKFIT") {
        column.printEntry(stream, itsPintPeakFit.value());
    } else if (type == "POLPEAKFITDB") {
        column.printEntry(stream, itsPintPeakFitDebias);
    } else if (type == "POLPEAKFITERR") {
        column.printEntry(stream, itsPintPeakFit.error());
    } else if (type == "POLPEAKFITSNR") {
        column.printEntry(stream, itsPintFitSNR.value());
    } else if (type == "POLPEAKFITSNRERR") {
        column.printEntry(stream, itsPintFitSNR.error());
    } else if (type == "FDPEAK") {
        column.printEntry(stream, itsPhiPeak.value());
    } else if (type == "FDPEAKERR") {
        column.printEntry(stream, itsPhiPeak.error());
    } else if (type == "FDPEAKFIT") {
        column.printEntry(stream, itsPhiPeakFit.value());
    } else if (type == "FDPEAKFITERR") {
        column.printEntry(stream, itsPhiPeakFit.error());
    } else if (type == "POLANG") {
        column.printEntry(stream, itsPolAngleRef.value());
    } else if (type == "POLANGERR") {
        column.printEntry(stream, itsPolAngleRef.error());
    } else if (type == "POLANG0") {
        column.printEntry(stream, itsPolAngleZero.value());
    } else if (type == "POLANG0ERR") {
        column.printEntry(stream, itsPolAngleZero.error());
    } else if (type == "POLFRAC") {
        column.printEntry(stream, itsFracPol.value());
    } else if (type == "POLFRACERR") {
        column.printEntry(stream, itsFracPol.error());
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

void CasdaPolarisationEntry::checkCol(duchamp::Catalogues::Column &column, bool checkTitle)
{
    std::string type = column.type();
    if (type == "ID") {
        column.check(itsComponentID, checkTitle);
    } else if (type == "NAME") {
        column.check(itsName, checkTitle);
    } else if (type == "RAJD") {
        column.check(itsRA, checkTitle);
    } else if (type == "DECJD") {
        column.check(itsDec, checkTitle);
    } else if (type == "IFLUX") {
        column.check(itsFluxImedian, checkTitle);
    } else if (type == "QFLUX") {
        column.check(itsFluxQmedian, checkTitle);
    } else if (type == "UFLUX") {
        column.check(itsFluxUmedian, checkTitle);
    } else if (type == "VFLUX") {
        column.check(itsFluxImedian, checkTitle);
    } else if (type == "RMS_I") {
        column.check(itsRmsI, checkTitle);
    } else if (type == "RMS_Q") {
        column.check(itsRmsQ, checkTitle);
    } else if (type == "RMS_U") {
        column.check(itsRmsU, checkTitle);
    } else if (type == "RMS_V") {
        column.check(itsRmsV, checkTitle);
    } else if (type == "CO1") {
        column.check(itsPolyCoeff0, checkTitle);
    } else if (type == "CO2") {
        column.check(itsPolyCoeff1, checkTitle);
    } else if (type == "CO3") {
        column.check(itsPolyCoeff2, checkTitle);
    } else if (type == "CO4") {
        column.check(itsPolyCoeff3, checkTitle);
    } else if (type == "CO5") {
        column.check(itsPolyCoeff4, checkTitle);
    } else if (type == "LAMSQ") {
        column.check(itsLambdaSqRef, checkTitle);
    } else if (type == "RMSF") {
        column.check(itsRmsfFwhm, checkTitle);
    } else if (type == "POLPEAK") {
        column.check(itsPintPeak.value(), checkTitle);
    } else if (type == "POLPEAKDB") {
        column.check(itsPintPeakDebias, checkTitle);
    } else if (type == "POLPEAKERR") {
        column.check(itsPintPeak.error(), checkTitle);
    } else if (type == "POLPEAKFIT") {
        column.check(itsPintPeakFit.value(), checkTitle);
    } else if (type == "POLPEAKFITDB") {
        column.check(itsPintPeakFitDebias, checkTitle);
    } else if (type == "POLPEAKFITERR") {
        column.check(itsPintPeakFit.error(), checkTitle);
    } else if (type == "POLPEAKFITSNR") {
        column.check(itsPintFitSNR.value(), checkTitle);
    } else if (type == "POLPEAKFITSNRERR") {
        column.check(itsPintFitSNR.error(), checkTitle);
    } else if (type == "FDPEAK") {
        column.check(itsPhiPeak.value(), checkTitle);
    } else if (type == "FDPEAKERR") {
        column.check(itsPhiPeak.error(), checkTitle);
    } else if (type == "FDPEAKFIT") {
        column.check(itsPhiPeakFit.value(), checkTitle);
    } else if (type == "FDPEAKFITERR") {
        column.check(itsPhiPeakFit.error(), checkTitle);
    } else if (type == "POLANG") {
        column.check(itsPolAngleRef.value(), checkTitle);
    } else if (type == "POLANGERR") {
        column.check(itsPolAngleRef.error(), checkTitle);
    } else if (type == "POLANG0") {
        column.check(itsPolAngleZero.value(), checkTitle);
    } else if (type == "POLANG0ERR") {
        column.check(itsPolAngleZero.error(), checkTitle);
    } else if (type == "POLFRAC") {
        column.check(itsFracPol.value(), checkTitle);
    } else if (type == "POLFRACERR") {
        column.check(itsFracPol.error(), checkTitle);
    } else if (type == "COMPLEX1") {
        column.check(itsComplexity, checkTitle);
    } else if (type == "COMPLEX2") {
        column.check(itsComplexity_screen, checkTitle);
    } else if (type == "FLAG1") {
        column.check(itsFlagDetection, checkTitle);
    } else if (type == "FLAG2") {
        column.check(itsFlagEdge, checkTitle);
    } else if (type == "FLAG3") {
        column.check(itsFlag3, checkTitle);
    } else if (type == "FLAG4") {
        column.check(itsFlag4, checkTitle);
    } else {
        ASKAPTHROW(AskapError,
                   "Unknown column type " << type);
    }

}

void CasdaPolarisationEntry::checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool checkTitle)
{
    for (size_t i = 0; i < spec.size(); i++) {
        this->checkCol(spec.column(i), checkTitle);
    }
}

//**************************************************************//

LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& blob, CasdaPolarisationEntry& src)
{
    std::string s;
    double d;
    casda::ValueError v;
    unsigned int u;

    // from CatalogueEntry.h
    s = src.itsSBid; blob << s;
    s = src.itsIDbase; blob << s;
    // from CasdaPolarisationEntry.h
    s = src.itsComponentID; blob << s;
    s = src.itsName; blob << s;
    d = src.itsRA; blob << d;
    d = src.itsDec; blob << d;
    d = src.itsFluxImedian; blob << d;
    d = src.itsFluxQmedian; blob << d;
    d = src.itsFluxUmedian; blob << d;
    d = src.itsFluxVmedian; blob << d;
    d = src.itsRmsI; blob << d;
    d = src.itsRmsQ; blob << d;
    d = src.itsRmsU; blob << d;
    d = src.itsRmsV; blob << d;
    d = src.itsPolyCoeff0; blob << d;
    d = src.itsPolyCoeff1; blob << d;
    d = src.itsPolyCoeff2; blob << d;
    d = src.itsPolyCoeff3; blob << d;
    d = src.itsPolyCoeff4; blob << d;
    d = src.itsLambdaSqRef; blob << d;
    d = src.itsRmsfFwhm; blob << d;
    d = src.itsDetectionThreshold; blob << d;
    d = src.itsDebiasThreshold; blob << d;
    v = src.itsPintPeak; blob << v;
    d = src.itsPintPeakDebias; blob << d;
    v = src.itsPintPeakFit; blob << v;
    d = src.itsPintPeakFitDebias; blob << d;
    v = src.itsPintFitSNR; blob << v;
    v = src.itsPhiPeak; blob << v;
    v = src.itsPhiPeakFit; blob << v;
    v = src.itsPolAngleRef; blob << v;
    v = src.itsPolAngleZero; blob << v;
    v = src.itsFracPol; blob << v;
    d = src.itsComplexity; blob << d;
    d = src.itsComplexity_screen; blob << d;
    u = src.itsFlagDetection; blob << u;
    u = src.itsFlagEdge; blob << u;
    u = src.itsFlag3; blob << u;
    u = src.itsFlag4; blob << u;

    return blob;

}

LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& blob, CasdaPolarisationEntry& src)
{
    std::string s;
    double d;
    casda::ValueError v;
    unsigned int u;

    // from CatalogueEntry.h
    blob >> s; src.itsSBid = s;
    blob >> s; src.itsIDbase = s;
    // from CasdaPolarisationEntry.h
    blob >> s; src.itsComponentID = s;
    blob >> s; src.itsName = s;
    blob >> d; src.itsRA = d;
    blob >> d; src.itsDec = d;
    blob >> d; src.itsFluxImedian = d;
    blob >> d; src.itsFluxQmedian = d;
    blob >> d; src.itsFluxUmedian = d;
    blob >> d; src.itsFluxVmedian = d;
    blob >> d; src.itsRmsI = d;
    blob >> d; src.itsRmsQ = d;
    blob >> d; src.itsRmsU = d;
    blob >> d; src.itsRmsV = d;
    blob >> d; src.itsPolyCoeff0 = d;
    blob >> d; src.itsPolyCoeff1 = d;
    blob >> d; src.itsPolyCoeff2 = d;
    blob >> d; src.itsPolyCoeff3 = d;
    blob >> d; src.itsPolyCoeff4 = d;
    blob >> d; src.itsLambdaSqRef = d;
    blob >> d; src.itsRmsfFwhm = d;
    blob >> d; src.itsDetectionThreshold = d;
    blob >> d; src.itsDebiasThreshold = d;
    blob >> v; src.itsPintPeak = v;
    blob >> d; src.itsPintPeakDebias = d;
    blob >> v; src.itsPintPeakFit = v;
    blob >> d; src.itsPintPeakFitDebias = d;
    blob >> v; src.itsPintFitSNR = v;
    blob >> v; src.itsPhiPeak = v;
    blob >> v; src.itsPhiPeakFit = v;
    blob >> v; src.itsPolAngleRef = v;
    blob >> v; src.itsPolAngleZero = v;
    blob >> v; src.itsFracPol = v;
    blob >> d; src.itsComplexity = d;
    blob >> d; src.itsComplexity_screen = d;
    blob >> u; src.itsFlagDetection = u;
    blob >> u; src.itsFlagEdge = u;
    blob >> u; src.itsFlag3 = u;
    blob >> u; src.itsFlag4 = u;

    return blob;

}

}
}

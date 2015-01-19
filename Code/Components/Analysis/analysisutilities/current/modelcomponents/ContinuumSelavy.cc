/// @file
///
/// Provides utility functions for simulations package
///
/// @copyright (c) 2007 CSIRO
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///
#include <askap_analysisutilities.h>

#include <modelcomponents/Spectrum.h>
#include <modelcomponents/Continuum.h>
#include <modelcomponents/ContinuumSelavy.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <utility>
#include <string>
#include <stdlib.h>
#include <math.h>

ASKAP_LOGGER(logger, ".continuum");

namespace askap {

namespace analysisutilities {

ContinuumSelavy::ContinuumSelavy(bool flagUseDeconvolvedSizes):
    Continuum(), itsFlagUseDeconvolvedSizes(flagUseDeconvolvedSizes)
{
    this->defineSource(0., 0., 1400.);
}

ContinuumSelavy::ContinuumSelavy(Spectrum &s, bool flagUseDeconvolvedSizes):
    Continuum(s), itsFlagUseDeconvolvedSizes(flagUseDeconvolvedSizes)
{
    this->defineSource(0., 0., 1400.);
}

ContinuumSelavy::ContinuumSelavy(std::string &line, bool flagUseDeconvolvedSizes):
    itsFlagUseDeconvolvedSizes(flagUseDeconvolvedSizes)
{
    this->define(line);
}

void ContinuumSelavy::define(const std::string &line)
{

    std::stringstream ss(line);
    int flag;
    ss >> this->itsID >> this->itsName >> this->itsRA >> this->itsDec
       >> this->itsX >> this->itsY
       >> this->itsFint >> this->itsFpeak >> this->itsFintFIT >> this->itsFpeakFIT
       >> this->itsMajFIT >> this->itsMinFIT >> this->itsPAFIT
       >> this->itsMajDECONV >> this->itsMinDECONV >> this->itsPADECONV
       >> this->itsAlpha >> this->itsBeta
       >> this->itsChisq >> this->itsRMSimage >> this->itsRMSfit
       >> this->itsNfree >> this->itsNdof >> this->itsNpixFIT >> this->itsNpixObj >> flag;

    if (this->itsFlagUseDeconvolvedSizes) {
        this->setMaj(std::max(this->itsMajDECONV, this->itsMinDECONV));
        this->setMin(std::min(this->itsMajDECONV, this->itsMinDECONV));
        this->setPA(this->itsPADECONV);
    } else {
        this->setMaj(std::max(this->itsMajFIT, this->itsMinFIT));
        this->setMin(std::min(this->itsMajFIT, this->itsMinFIT));
        this->setPA(this->itsPAFIT);
    }
    this->setFluxZero(this->itsFintFIT);
    this->itsFlagGuess = (flag == 1);

    //ASKAPLOG_DEBUG_STR(logger, "Selavy source #" << this->itsID <<": " << this->itsRA << " " << this->itsDec << " " << this->itsFintFIT << " " << this->itsMaj << " " << this->itsMin << " " << this->itsPA);

}

ContinuumSelavy::ContinuumSelavy(const ContinuumSelavy& c):
    Continuum(c)
{
    operator=(c);
}

ContinuumSelavy& ContinuumSelavy::operator= (const ContinuumSelavy& c)
{
    if (this == &c) return *this;

    ((Continuum &) *this) = c;
    this->itsName = c.itsName;
    this->itsX = c.itsX;
    this->itsY = c.itsY;
    this->itsFint = c.itsFint;
    this->itsFpeak = c.itsFpeak;
    this->itsFintFIT = c.itsFintFIT;
    this->itsFpeakFIT = c.itsFpeakFIT;
    this->itsMajFIT = c.itsMajFIT;
    this->itsMinFIT = c.itsMinFIT;
    this->itsPAFIT = c.itsPAFIT;
    this->itsMajDECONV = c.itsMajDECONV;
    this->itsMinDECONV = c.itsMinDECONV;
    this->itsPADECONV = c.itsPADECONV;
    this->itsChisq = c.itsChisq;
    this->itsRMSimage = c.itsRMSimage;
    this->itsRMSfit = c.itsRMSfit;
    this->itsNfree = c.itsNfree;
    this->itsNdof = c.itsNdof;
    this->itsNpixFIT = c.itsNpixFIT;
    this->itsNpixObj = c.itsNpixObj;
    this->itsFlagGuess = c.itsFlagGuess;
    this->itsFlagUseDeconvolvedSizes = c.itsFlagUseDeconvolvedSizes;
    return *this;
}

ContinuumSelavy& ContinuumSelavy::operator= (const Spectrum& c)
{
    if (this == &c) return *this;

    ((Continuum &) *this) = c;
    this->itsFlagUseDeconvolvedSizes = false;
    this->defineSource(0., 0., 1400.);
    return *this;
}


void ContinuumSelavy::print(std::ostream& theStream)
{
    theStream.setf(std::ios::showpoint);
    theStream << std::setw(6) << this->itsID << " "
              << std::setw(14) << this->itsName << " "
              << std::setw(15) << std::setprecision(5) << this->itsRA << " "
              << std::setw(11) << std::setprecision(5) << this->itsDec << " "
              << std::setw(8) << std::setprecision(1) << this->itsX << " "
              << std::setw(8) << std::setprecision(1) << this->itsY << " "
              << std::setw(10) << std::setprecision(8) << this->itsFint << " "
              << std::setw(10) << std::setprecision(8) << this->itsFpeak << " "
              << std::setw(10) << std::setprecision(8) << this->itsFintFIT << " "
              << std::setw(10) << std::setprecision(8) << this->itsFpeakFIT << " "
              << std::setw(8) << std::setprecision(3) << this->itsMajFIT << " "
              << std::setw(8) << std::setprecision(3) << this->itsMinFIT << " "
              << std::setw(8) << std::setprecision(3) << this->itsPAFIT << " "
              << std::setw(8) << std::setprecision(3) << this->itsMajDECONV << " "
              << std::setw(8) << std::setprecision(3) << this->itsMinDECONV << " "
              << std::setw(8) << std::setprecision(3) << this->itsPADECONV << " "
              << std::setw(6) << std::setprecision(3) << this->itsAlpha << " "
              << std::setw(6) << std::setprecision(3) << this->itsBeta << " "
              << std::setw(27) << std::setprecision(9) << this->itsChisq << " "
              << std::setw(10) << std::setprecision(8) << this->itsRMSimage << " "
              << std::setw(15) << std::setprecision(6) << this->itsRMSfit << " "
              << std::setw(11) << this->itsNfree << " "
              << std::setw(10) << this->itsNdof << " "
              << std::setw(10) << this->itsNpixFIT << " "
              << std::setw(10) << this->itsNpixObj << " "
              << std::setw(7)  << this->itsFlagGuess << "\n";
}
std::ostream& operator<< (std::ostream& theStream, ContinuumSelavy &cont)
{
    cont.print(theStream);
    return theStream;
}
}


}

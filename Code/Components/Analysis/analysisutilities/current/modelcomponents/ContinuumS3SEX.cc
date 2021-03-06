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
#include <modelcomponents/ContinuumS3SEX.h>

#include <gsl/gsl_multifit.h>

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

ASKAP_LOGGER(logger, ".continuumS3SEX");

namespace askap {

namespace analysisutilities {

ContinuumS3SEX::ContinuumS3SEX():
    Continuum()
{
    this->defineSource(0., 0., 1400.);
    this->defaultSEDtype();
}

ContinuumS3SEX::ContinuumS3SEX(const Continuum &c):
    Continuum(c)
{
    this->defineSource(0., 0., 1400.);
    this->defaultSEDtype();
}

ContinuumS3SEX::ContinuumS3SEX(const Spectrum &s):
    Continuum(s)
{
    this->defineSource(0., 0., 1400.);
    this->defaultSEDtype();
}

ContinuumS3SEX::ContinuumS3SEX(const std::string &line, const float nuZero)
{
    setNuZero(nuZero);
    this->define(line);
    this->defaultSEDtype();
}

ContinuumS3SEX::ContinuumS3SEX(const float alpha, const float beta, const float nuZero):
    Continuum(alpha, beta, nuZero)
{
}

ContinuumS3SEX::ContinuumS3SEX(const float alpha,
                               const float beta,
                               const float nuZero,
                               const float fluxZero):
    Continuum(alpha, beta, nuZero, fluxZero)
{
}

void ContinuumS3SEX::define(const std::string &line)
{

    std::stringstream ss(line);
    ss >> itsComponentNum >> itsGalaxyNum >> itsStructure
       >> itsRA >> itsDec >> itsPA >> itsMaj >> itsMin
       >> itsI151 >> itsI610 >> itsI1400 >> itsI4860 >> itsI18000;

    std::stringstream idstring;
    idstring << itsComponentNum;
    itsID = idstring.str();

    itsFreqValues = std::vector<float>(5);
    for (int i = 0; i < 5; i++) itsFreqValues[i] = freqValuesS3SEX[i];

    // set the flux for now to be the reference one. Need to set properly using prepareForUse()
    itsFlux = pow(10, itsI1400);

    this->checkShape();

}

void ContinuumS3SEX::prepareForUse()
{

    double flux;
    if (itsSEDtype == SIMPLE_POWERLAW) {
        itsFlux = pow(10., itsI1400);
        itsAlpha = (log10(itsFlux) - itsI610) / log10(1400. / 610.);
        itsBeta = 0.;
    } else if (itsSEDtype == POWERLAW) {
        if (itsNuZero < 610.e6) {
            itsAlpha = (itsI610 - itsI151) / log10(610. / 151.);
            flux = itsI151 + itsAlpha * log10(itsNuZero / 151.e6);
        } else if (itsNuZero < 1400.e6) {
            itsAlpha = (itsI1400 - itsI610) / log10(1400. / 610.);
            flux = itsI610 + itsAlpha * log10(itsNuZero / 610.e6);
        } else if (itsNuZero < 4.86e9) {
            itsAlpha = (itsI4860 - itsI1400) / log10(4860. / 1400.);
            flux = itsI1400 + itsAlpha * log10(itsNuZero / 1400.e6);
        } else {
            itsAlpha = (itsI18000 - itsI4860) / log10(18000. / 4860.);
            flux = itsI4860 + itsAlpha * log10(itsNuZero / 4860.e6);
        }
        itsFlux = pow(10., flux);
        itsBeta = 0.;
    } else if (itsSEDtype == FIT) {
        std::vector<float> xdat(5), ydat(5), fit;
        // Set the frequency values, normalised by the reference frequency nuZero.
        // Note that the fitting is done in log-space (and **NOT** log10-space!!)
        for (int i = 0; i < 5; i++) {
            xdat[i] = log(itsFreqValues[i] / itsNuZero);
        }
        // Convert the flux values to log-space for fitting.
        ydat[0] = log(pow(10, itsI151));
        ydat[1] = log(pow(10, itsI610));
        ydat[2] = log(pow(10, itsI1400));
        ydat[3] = log(pow(10, itsI4860));
        ydat[4] = log(pow(10, itsI18000));

        int ndata = 5, nterms = 5;
        double chisq;
        gsl_matrix *x, *cov;
        gsl_vector *y, *w, *c;
        x = gsl_matrix_alloc(ndata, nterms);
        y = gsl_vector_alloc(ndata);
        w = gsl_vector_alloc(ndata);
        c = gsl_vector_alloc(nterms);
        cov = gsl_matrix_alloc(nterms, nterms);
        for (int i = 0; i < ndata; i++) {
            gsl_matrix_set(x, i, 0, 1.);
            gsl_matrix_set(x, i, 1, xdat[i]);
            gsl_matrix_set(x, i, 2, xdat[i]*xdat[i]);
            gsl_matrix_set(x, i, 3, xdat[i]*xdat[i]*xdat[i]);
            gsl_matrix_set(x, i, 4, xdat[i]*xdat[i]*xdat[i]*xdat[i]);

            gsl_vector_set(y, i, ydat[i]);
            gsl_vector_set(w, i, 1.);
        }

        gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc(ndata, nterms);
        gsl_multifit_wlinear(x, w, y, c, cov, &chisq, work);
        gsl_multifit_linear_free(work);

        // ASKAPLOG_DEBUG_STR(logger, "GSL fit: chisq="<<chisq
        //             <<", results: [0]="<<gsl_vector_get(c,0)<<" [1]="<<gsl_vector_get(c,1)
        //             <<" [2]="<<gsl_vector_get(c,2)<<" [3]="<<gsl_vector_get(c,3)
        //             <<" [4]="<<gsl_vector_get(c,4));

        flux = gsl_vector_get(c, 0);
        itsFlux = exp(flux);
        itsAlpha = gsl_vector_get(c, 1);
        itsBeta = gsl_vector_get(c, 2);

        gsl_matrix_free(x);
        gsl_vector_free(y);
        gsl_vector_free(w);
        gsl_vector_free(c);
        gsl_matrix_free(cov);

        ASKAPLOG_DEBUG_STR(logger, "From Fit::  S3SEX source: ID=" << itsComponentNum
                           << ", RA,DEC=" << itsRA << "," << itsDec
                           << ", I151=" << itsI151 << ", I610=" << itsI610
                           << ", I1400=" << itsI1400 << ", I4860=" << itsI4860
                           << ", I18000=" << itsI18000 << ", nu0=" << itsNuZero
                           << ", flux=" << log10(itsFlux)
                           << ", alpha=" << itsAlpha
                           << ", beta=" << itsBeta);
    } else {
        ASKAPLOG_ERROR_STR(logger, "Unknown SED type in ContinuumS3SEX");
    }
}

ContinuumS3SEX::ContinuumS3SEX(const ContinuumS3SEX& c):
    Continuum(c)
{
    operator=(c);
}

ContinuumS3SEX& ContinuumS3SEX::operator= (const ContinuumS3SEX& c)
{
    if (this == &c) return *this;

    ((Continuum &) *this) = c;
    itsAlpha      = c.itsAlpha;
    itsBeta       = c.itsBeta;
    itsNuZero     = c.itsNuZero;
    itsFreqValues = c.itsFreqValues;
    return *this;
}

ContinuumS3SEX& ContinuumS3SEX::operator= (const Spectrum& c)
{
    if (this == &c) return *this;

    ((Continuum &) *this) = c;
    this->defineSource(0., 0., 1400.);
    return *this;
}


void ContinuumS3SEX::print(std::ostream& theStream)
{
    theStream.setf(std::ios::showpoint);
    theStream.setf(std::ios::fixed);
    theStream << std::setw(11) << itsComponentNum << " "
              << std::setw(9) << itsGalaxyNum << " "
              << std::setw(9)  << itsStructure << " "
              << std::setw(15) << std::setprecision(6) << itsRA << " "
              << std::setw(11) << std::setprecision(6) << itsDec << " "
              << std::setw(14) << std::setprecision(3) << itsPA << " "
              << std::setw(10) << std::setprecision(3) << itsMaj << " "
              << std::setw(10) << std::setprecision(3) << itsMin << " "
              << std::setw(7) << std::setprecision(4) << itsI151 << " "
              << std::setw(7) << std::setprecision(4) << itsI610 << " "
              << std::setw(7) << std::setprecision(4) << itsI1400 << " "
              << std::setw(7) << std::setprecision(4) << itsI4860 << " "
              << std::setw(7) << std::setprecision(4) << itsI18000 << "\n";
}
std::ostream& operator<< (std::ostream& theStream, ContinuumS3SEX &cont)
{

    cont.print(theStream);
    return theStream;
}
}


}

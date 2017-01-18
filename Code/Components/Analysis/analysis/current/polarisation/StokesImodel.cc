/// @file
///
/// A class to encapsulate the modelling of a Stokes I spectrum, for
/// use with the Rotation Measure Synthesis.
///
/// @copyright (c) 2017 CSIRO
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
#include <polarisation/StokesImodel.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <polarisation/StokesSpectrum.h>
#include <catalogues/CasdaComponent.h>

#include <gsl/gsl_multifit.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".stokesimodel");

namespace askap {

namespace analysis {

StokesImodel::StokesImodel(const LOFAR::ParameterSet &parset):
    itsRefFreq(0.)
{
    itsType = parset.getString("modelType", "taylor");
    if (itsType != "taylor" && itsType != "poly") {
        itsType = "taylor";
    }
    itsOrder = parset.getUint("modelPolyOrder", 3);

}

void StokesImodel::initialise(StokesSpectrum &I,
                              CasdaComponent *comp)
{

    itsFreqs = I.frequencies();
    itsIspectrum = I.spectrum();

    if (itsType == "taylor") {

        itsCoeffs = casa::Vector<float>(3);
        itsCoeffs[0] = comp->intFlux(I.bunit().getName());
        itsCoeffs[1] = comp->alpha();
        itsCoeffs[2] = comp->beta();
        itsRefFreq = comp->freq(I.freqUnit());

    } else {

        itsCoeffs = casa::Vector<float>(itsOrder);
        fit();

    }

    itsModelSpectrum = casa::Vector<float>(itsFreqs.size(), 0.);
    for (size_t i = 0; i < itsFreqs.size(); i++) {
        itsModelSpectrum[i] = flux(itsFreqs[i]);
    }

}

float StokesImodel::coeff(unsigned int i)
{
    if (i < itsCoeffs.size()) {
        return itsCoeffs[i];
    } else {
        return 0.;
    }
}

float StokesImodel::flux(float frequency)
{
    float flux;

    if (itsType == "taylor") {

        float lognu = log(frequency / itsRefFreq);
        float logflux = log(itsCoeffs[0]) + itsCoeffs[1] * lognu + itsCoeffs[2] * lognu * lognu;
        flux = exp(logflux);

    } else {

        flux = 0.;
        for (size_t i = 0; i < itsCoeffs.size(); i++) {
            flux += itsCoeffs[i] * pow(frequency, i);
        }

    }

    return flux;

}


void StokesImodel::fit()
{

    int size = itsIspectrum.size();

    gsl_matrix *x, *cov;
    gsl_vector *y, *w, *c;
    x = gsl_matrix_alloc(size, itsOrder);
    y = gsl_vector_alloc(size);
    w = gsl_vector_alloc(size);
    c = gsl_vector_alloc(itsOrder);
    cov = gsl_matrix_alloc(itsOrder, itsOrder);
    for (int i = 0; i < size; i++) {
        for (int t = 0; t < itsOrder; t++) {
            gsl_matrix_set(x, i, t, pow(itsFreqs[i], t));
        }
        gsl_vector_set(y, i, itsIspectrum[i]);
        gsl_vector_set(w, i, 1.);
    }

    gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc(size, itsOrder);
    double chisq;
    gsl_multifit_wlinear(x, w, y, c, cov, &chisq, work);
    gsl_multifit_linear_free(work);

    for (size_t i = 0; i < itsOrder; i++) {
        itsCoeffs[i] = gsl_vector_get(c, i);
    }

    gsl_matrix_free(x);
    gsl_vector_free(y);
    gsl_vector_free(w);
    gsl_vector_free(c);
    gsl_matrix_free(cov);

}


}
}


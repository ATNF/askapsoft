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

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

#include <polarisation/StokesSpectrum.h>
#include <catalogues/CasdaComponent.h>

#include <gsl/gsl_multifit.h>
#include <mathsutils/FitTT.h>

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
    if (itsType == "poly") {
        itsOrder = parset.getUint("modelPolyOrder", 3);
    } else {
        itsOrder = 3;
    }
    itsTaylorNterms = parset.getUint("taylor.nterms", 3);

    recomputeAlphaBeta = parset.getBool("recomputeAlphaBeta", false);
    itsRefFreq = parset.getFloat("referenceFreq", -1.);

}

void StokesImodel::initialise(StokesSpectrum &I,
                              CasdaComponent *comp)
{

    ASKAPLOG_DEBUG_STR(logger, "Obtaining frequencies and spectra");
    itsFreqs = I.frequencies();
    ASKAPLOG_DEBUG_STR(logger, "frequencies = " << itsFreqs);
    if (itsRefFreq < 0.) {
        itsRefFreq = itsFreqs[itsFreqs.size() / 2];
    }
    ASKAPLOG_DEBUG_STR(logger, "reference freq = " << itsRefFreq);
    itsIspectrum = I.spectrum();
    itsInoise = I.noiseSpectrum();

    ASKAPLOG_DEBUG_STR(logger, "spectrum = " << itsIspectrum);

    if (itsType == "taylor") {

        if (recomputeAlphaBeta) {
            itsCoeffs = casacore::Vector<float>(itsOrder);
            itsCoeffErrs = casacore::Vector<float>(itsOrder);
            ASKAPLOG_DEBUG_STR(logger, "About to fit to spectrum");
            fit();
            ASKAPLOG_DEBUG_STR(logger, "Fitting complete");
        } else {
            itsCoeffs = casacore::Vector<float>(3);
            itsCoeffErrs = casacore::Vector<float>(3);
            itsCoeffs[0] = comp->intFlux(I.bunit().getName());
            itsCoeffs[1] = comp->alpha();
            itsCoeffs[2] = comp->beta();
            itsCoeffErrs[0] = comp->intFluxErr(I.bunit().getName());
            itsCoeffErrs[1] = comp->alphaErr();
            itsCoeffErrs[2] = comp->betaErr();
            itsRefFreq = comp->freq(I.freqUnit());
        }

    } else {

        itsCoeffs = casacore::Vector<float>(itsOrder);
        fit();

    }

    itsModelSpectrum = casacore::Vector<float>(itsFreqs.size(), 0.);
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

float StokesImodel::coeffErr(unsigned int i)
{
    if (i < itsCoeffs.size()) {
        return itsCoeffErrs[i];
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
    if (itsType == "taylor") {
        fitTT();
    } else {
        fitPoly();
    }
}


void StokesImodel::fitPoly()
{
    unsigned int size = itsIspectrum.size();

    gsl_matrix *x, *cov;
    gsl_vector *y, *w, *c;
    x = gsl_matrix_alloc(size, itsOrder);
    y = gsl_vector_alloc(size);
    w = gsl_vector_alloc(size);
    c = gsl_vector_alloc(itsOrder);
    cov = gsl_matrix_alloc(itsOrder, itsOrder);
    for (unsigned int i = 0; i < size; i++) {
        for (unsigned int t = 0; t < itsOrder; t++) {
            gsl_matrix_set(x, i, t, pow(itsFreqs[i], t));
        }
        gsl_vector_set(y, i, itsIspectrum[i]);
        gsl_vector_set(w, i, 1. / itsInoise[i]);
    }

    gsl_multifit_linear_workspace * work = gsl_multifit_linear_alloc(size, itsOrder);
    double chisq;

    int returnval = gsl_multifit_wlinear(x, w, y, c, cov, &chisq, work);
    ASKAPLOG_DEBUG_STR(logger, "Fitting returned with value " << returnval << " and chisq = " << chisq);
    gsl_multifit_linear_free(work);

    std::stringstream ss;
    ss << "List of coefficients: [";
    for (size_t i = 0; i < itsOrder; i++) {
        itsCoeffs[i] = gsl_vector_get(c, i);
        itsCoeffErrs[i] = sqrt(gsl_matrix_get(cov, i, i));
        if (i > 0) ss << ",";
        ss << itsCoeffs[i];
    }
    ss << "]";
    ASKAPLOG_DEBUG_STR(logger, ss.str());

    gsl_matrix_free(x);
    gsl_vector_free(y);
    gsl_vector_free(w);
    gsl_vector_free(c);
    gsl_matrix_free(cov);

}

void StokesImodel::fitTT()
{

    ASKAPLOG_DEBUG_STR(logger, "Defining fitter");
    // Define the spectral index & curvature fitter
    analysisutilities::FitTT fitter(itsTaylorNterms);
    ASKAPLOG_DEBUG_STR(logger, "Fitting");
    // Normalise the frequency array to the reference frequency, and fit
    casacore::Array<float> normalisedFreqs = itsFreqs / itsRefFreq;
    fitter.fit(normalisedFreqs, itsIspectrum, itsInoise);
    ASKAPLOG_DEBUG_STR(logger, "Complete");
    // Get the coefficients and their errors
    itsCoeffs[0] = fitter.fluxZero();
    itsCoeffs[1] = fitter.alpha();
    itsCoeffs[2] = fitter.beta();
    itsCoeffErrs[0] = fitter.fluxZeroErr();
    itsCoeffErrs[1] = fitter.alphaErr();
    itsCoeffErrs[2] = fitter.betaErr();
    ASKAPLOG_DEBUG_STR(logger, "Finished parameterisation");
}


}
}


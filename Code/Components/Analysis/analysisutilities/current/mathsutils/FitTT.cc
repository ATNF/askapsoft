/// @file
///
/// Implementation of class to handle Taylor-term fitting to a spectrum
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
/// published by the Free Software Foundation; either version 3 of the License,
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
#include <mathsutils/FitTT.h>
#include <askap_analysisutilities.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <gsl/gsl_multifit.h>
#include <gsl/gsl_multifit_nlin.h>
#include <casacore/casa/Arrays/Array.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".fitTT");

namespace askap {

namespace analysisutilities {

FitTT::FitTT(unsigned int nterms):
    itsNterms(nterms),
    itsFluxZero(0.),
    itsAlpha(0.),
    itsBeta(0.),
    itsFluxZeroErr(0.),
    itsAlphaErr(0.),
    itsBetaErr(0.)
{
    if (itsNterms > 3) {
        ASKAPLOG_WARN_STR(logger, "Taylor term fitting only supports nterms<=3 - setting nterms=3");
        itsNterms = 3;
    }
    if (itsNterms == 0) {
        ASKAPLOG_WARN_STR(logger, "Taylor term fitting requires at least one term - setting nterms=1");
        itsNterms = 1;
    }
}

int FitTT::taylor_f1(const gsl_vector * p, void *data,  gsl_vector * f)
{
    // Function to be fit - F = f0 where x=(nu/nu0) (nterms=1)

    size_t n = ((struct data *)data)->ndata;
    double *ydat = ((struct data *)data)->ydat;
    double *weights = ((struct data *)data)->weights;

    double f0;
    f0 = gsl_vector_get(p, 0);

    size_t i;

    for (i = 0; i < n; i++) {
        // Model Yi = f0
        double Yi = f0;
        gsl_vector_set(f, i, (Yi - ydat[i]) / weights[i]);
    }

    return GSL_SUCCESS;
}

int FitTT::taylor_df1(const gsl_vector * p, void *data,  gsl_matrix * J)
{
    size_t n = ((struct data *)data)->ndata;

    double f0;

    f0 = gsl_vector_get(p, 0);

    size_t i;

    for (i = 0; i < n; i++) {
        // Jacobian matrix J(i,j) = dfi / dxj,
        // where fi = (Yi - yi)/sigma[i],
        //       Yi = f0
        // and the xj are the parameters (f0)
        gsl_matrix_set(J, i, 0, 0);
    }
    return GSL_SUCCESS;
}

int FitTT::taylor_fdf1(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J)
{
    taylor_f1(x, data, f);
    taylor_df1(x, data, J);

    return GSL_SUCCESS;
}

int FitTT::taylor_f2(const gsl_vector * p, void *data,  gsl_vector * f)
{
    // Function to be fit - F = f0 * x^(alpha) where x=(nu/nu0)  (ie. nterms=1)

    size_t n = ((struct data *)data)->ndata;
    double *xdat = ((struct data *)data)->xdat;
    double *ydat = ((struct data *)data)->ydat;
    double *weights = ((struct data *)data)->weights;

    double f0, alpha;
    f0 = gsl_vector_get(p, 0);
    alpha = gsl_vector_get(p, 1);

    size_t i;

    for (i = 0; i < n; i++) {
        // Model Yi = f0 * x^(alpha + beta*logx)
        double Yi = f0 * pow(xdat[i], alpha);
        gsl_vector_set(f, i, (Yi - ydat[i]) / weights[i]);
    }

    return GSL_SUCCESS;
}

int FitTT::taylor_df2(const gsl_vector * p, void *data,  gsl_matrix * J)
{
    size_t n = ((struct data *)data)->ndata;
    double *xdat = ((struct data *)data)->xdat;
    double *weights = ((struct data *)data)->weights;

    double f0, alpha;

    f0 = gsl_vector_get(p, 0);
    alpha = gsl_vector_get(p, 1);

    size_t i;

    for (i = 0; i < n; i++) {
        // Jacobian matrix J(i,j) = dfi / dxj,
        // where fi = (Yi - yi)/sigma[i],
        //       Yi = f0 * x^(alpha)
        // and the xj are the parameters (f0,alpha)
        double logx = log(xdat[i]);
        double e = pow(xdat[i], alpha);
        double w = weights[i];
        gsl_matrix_set(J, i, 0, e / w);
        gsl_matrix_set(J, i, 1, f0 * logx * e / w);
    }
    return GSL_SUCCESS;
}

int FitTT::taylor_fdf2(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J)
{
    taylor_f2(x, data, f);
    taylor_df2(x, data, J);

    return GSL_SUCCESS;
}

int FitTT::taylor_f3(const gsl_vector * p, void *data,  gsl_vector * f)
{
    // Function to be fit - F = f0 * x^(alpha + beta*logx) where x=(nu/nu0)

    size_t n = ((struct data *)data)->ndata;
    double *xdat = ((struct data *)data)->xdat;
    double *ydat = ((struct data *)data)->ydat;
    double *weights = ((struct data *)data)->weights;

    double f0, alpha, beta;
    f0 = gsl_vector_get(p, 0);
    alpha = gsl_vector_get(p, 1);
    beta = gsl_vector_get(p, 2);

    size_t i;

    for (i = 0; i < n; i++) {
        // Model Yi = f0 * x^(alpha + beta*logx)
        double Yi = f0 * pow(xdat[i], alpha + beta * log(xdat[i]));
        gsl_vector_set(f, i, (Yi - ydat[i]) / weights[i]);
    }

    return GSL_SUCCESS;
}

int FitTT::taylor_df3(const gsl_vector * p, void *data,  gsl_matrix * J)
{
    size_t n = ((struct data *)data)->ndata;
    double *xdat = ((struct data *)data)->xdat;
    double *weights = ((struct data *)data)->weights;

    double f0, alpha, beta;

    f0 = gsl_vector_get(p, 0);
    alpha = gsl_vector_get(p, 1);
    beta = gsl_vector_get(p, 2);

    size_t i;

    for (i = 0; i < n; i++) {
        // Jacobian matrix J(i,j) = dfi / dxj,
        // where fi = (Yi - yi)/sigma[i],
        //       Yi = f0 * x^(alpha + beta*logx)
        // and the xj are the parameters (f0,alpha,beta)
        double logx = log(xdat[i]);
        double e = pow(xdat[i], alpha + beta * logx);
        double w = weights[i];
        gsl_matrix_set(J, i, 0, e / w);
        gsl_matrix_set(J, i, 1, f0 * logx * e / w);
        gsl_matrix_set(J, i, 2, f0 * e * logx * logx / w);
    }
    return GSL_SUCCESS;
}

int FitTT::taylor_fdf3(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J)
{
    taylor_f3(x, data, f);
    taylor_df3(x, data, J);

    return GSL_SUCCESS;
}

void FitTT::fit(casa::Array<float> &x, casa::Array<float> &y)
{
    ASKAPASSERT(x.size() == y.size());
    size_t ndata = x.size();
    double xdata[ndata], ydata[ndata], weights[ndata];
    for (size_t i = 0; i < ndata; i++) {
        xdata[i] = x.data()[i];
        ydata[i] = y.data()[i];
        weights[i] = 1.;
    }
    fit(ndata, xdata, ydata, weights);

}

void FitTT::fit(casa::Array<float> &x, casa::Array<float> &y, casa::Array<float> &w)
{
    ASKAPASSERT(x.size() == y.size());
    ASKAPASSERT(x.size() == w.size());
    size_t ndata = x.size();
    double xdata[ndata], ydata[ndata], weights[ndata];
    for (size_t i = 0; i < ndata; i++) {
        xdata[i] = x.data()[i];
        ydata[i] = y.data()[i];
        weights[i] = w.data()[i];
    }
    fit(ndata, xdata, ydata, weights);

}


void FitTT::fit(size_t ndata, double *xdata, double *ydata, double *weights)
{
    ASKAPLOG_DEBUG_STR(logger, "Taylor term fitting for spectrum of size " << ndata);
    struct data d = { ndata, xdata, ydata, weights };
    const size_t p = itsNterms;
    gsl_matrix *covar = gsl_matrix_alloc(p, p);
    int status;

    // Initial values for the parameters - start with flat spectrum at flux at central pixel
    double x_init[itsNterms];
    x_init[0] = ydata[ndata / 2];
    if (itsNterms > 1) {
        x_init[1] = 0.0;
    }
    if (itsNterms > 2) {
        x_init[2] = 0.0;
    }

    // Set up fitting structures 
    gsl_vector_view x = gsl_vector_view_array(x_init, p);
    gsl_multifit_function_fdf f;
    /* define the function to be minimized */
    if (itsNterms == 1) {
        f.f = this->taylor_f1;
        f.df = this->taylor_df1;
        f.fdf = this->taylor_fdf1;
    } else if (itsNterms == 2) {
        f.f = this->taylor_f2;
        f.df = this->taylor_df2;
        f.fdf = this->taylor_fdf2;
    } else if (itsNterms == 3) {
        f.f = this->taylor_f3;
        f.df = this->taylor_df3;
        f.fdf = this->taylor_fdf3;
    }
    f.n = ndata;
    f.p = p;
    f.params = &d;
    ASKAPLOG_DEBUG_STR(logger, "Completed setup");

    // Set up and run the fitter
    const gsl_multifit_fdfsolver_type *T;
    gsl_multifit_fdfsolver *s;
    T = gsl_multifit_fdfsolver_lmsder;
    s = gsl_multifit_fdfsolver_alloc(T, ndata, p);
    gsl_multifit_fdfsolver_set(s, &f, &x.vector);
    unsigned int iter = 0;
    do {
        iter++;
        status = gsl_multifit_fdfsolver_iterate(s);
        if (status)
            break;
        status = gsl_multifit_test_delta(s->dx, s->x,
                                         1e-4, 1e-4);
    } while (status == GSL_CONTINUE && iter < 10);

    // Find the covariance matrix
    gsl_multifit_covar(s->J, 0.0, covar);

    // Extract the parameters and their errors
    itsFluxZero = gsl_vector_get(s->x, 0);
    itsFluxZeroErr = sqrt(gsl_matrix_get(covar, 0, 0));
    if (itsNterms > 1) {
        itsAlpha = gsl_vector_get(s->x, 1);
        itsAlphaErr = sqrt(gsl_matrix_get(covar, 1, 1));
    }
    if (itsNterms > 2) {
        itsBeta = gsl_vector_get(s->x, 2);
        itsBetaErr = sqrt(gsl_matrix_get(covar, 2, 2));
    }

    ASKAPLOG_DEBUG_STR(logger, "Fitting returned: I0=" << itsFluxZero << ", alpha=" << itsAlpha << ", beta=" << itsBeta);
    ASKAPLOG_DEBUG_STR(logger, "Fitting returned errors: e(I0)=" << itsFluxZeroErr << ", e(alpha)=" << itsAlphaErr << ", e(beta)=" << itsBetaErr);

    // clean up
    gsl_multifit_fdfsolver_free(s);
    gsl_matrix_free(covar);

}

}
}

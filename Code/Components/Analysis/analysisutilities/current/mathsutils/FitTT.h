/// @file
///
/// Class to handle Taylor-term fitting to a spectrum
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
#ifndef ASKAP_ANALYSISUTILS_FIT_TT_H_
#define ASKAP_ANALYSISUTILS_FIT_TT_H_

#include <gsl/gsl_multifit.h>
#include <gsl/gsl_multifit_nlin.h>
#include <casacore/casa/Arrays/Array.h>

namespace askap {

namespace analysisutilities {

/// Data structure used within the fitting as a way of accessing the
/// various arrays within the GSL functions.
struct data {
    size_t ndata;
    double * xdat;
    double * ydat;
    double * weights;
};


/// @brief Class to perform non-linear fitting to find alpha & beta.
/// @details A non-linear function describing a
/// power-law-with-curvature function is fitted to a spectrum using
/// non-linear Levenberg-Marquardt algorithm. The function is F(nu) =
/// F_0 (nu/nu_0) ^ (alpha + beta * log(nu/nu_0)), where F_0 is the
/// flux at the reference frequency nu_0. It is a weighted fit,
/// taking into account a provided noise array. The number of terms is
/// user-selectable, from 1 (constant flux), 2 (constant flux +
/// spectral index) or 3 (add spectral curvature).
class FitTT {
    public:
        /// Initialise, setting the number of terms that should be fit.
        FitTT(unsigned int nterms);
        virtual ~FitTT() {};

        /// @brief Fit to provided arrays.
        /// @details For spectral-index fitting, x should be the
        /// frequencies divided by the reference frequency, and y should
        /// be the flux values. No weights array is provided, so weights
        /// are implicitly set to 1.  Front end to fit(size_t ndata,
        /// double *xdata, double *ydata, double *weights).
        void fit(casa::Array<float> &x, casa::Array<float> &y);

        /// @brief Fit to provided arrays, with associated weights array.
        /// @details For spectral-index fitting, x should be the
        /// frequencies divided by the reference frequency, y should be
        /// the flux values, and w the weights on the flux values
        /// (ie. noise). Front end to fit(size_t ndata, double *xdata,
        /// double *ydata, double *weights).
        void fit(casa::Array<float> &x, casa::Array<float> &y, casa::Array<float> &w);

        /// @brief Fit to provided arrays - main function that does the fitting.
        /// @details For spectral-index fitting, xdata should be the
        /// frequencies divided by the reference frequency, ydata should be
        /// the flux values, and weights the weights on the flux values
        /// (ie. noise), with ndata the size of the arrays.
        void fit(size_t ndata, double *xdata, double *ydata, double *weights);

        /// @brief Model function, for nterms=1
        static int taylor_f1(const gsl_vector * p, void *data,  gsl_vector * f);
        /// @brief Jacobian function, for nterms=1
        static int taylor_df1(const gsl_vector * p, void *data,  gsl_matrix * J);
        /// @brief Combined model & Jacobian function, for nterms=1
        static int taylor_fdf1(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
        /// @brief Model function, for nterms=2
        static int taylor_f2(const gsl_vector * p, void *data,  gsl_vector * f);
        /// @brief Jacobian function, for nterms=2
        static int taylor_df2(const gsl_vector * p, void *data,  gsl_matrix * J);
        /// @brief Combined model & Jacobian function, for nterms=2
        static int taylor_fdf2(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);
        /// @brief Model function, for nterms=3
        static int taylor_f3(const gsl_vector * p, void *data,  gsl_vector * f);
        /// @brief Jacobian function, for nterms=3
        static int taylor_df3(const gsl_vector * p, void *data,  gsl_matrix * J);
        /// @brief Combined model & Jacobian function, for nterms=3
        static int taylor_fdf3(const gsl_vector * x, void *data, gsl_vector * f, gsl_matrix * J);

        /// @brief Return the fitted value of the flux at the reference pixel
        float fluxZero() {return itsFluxZero;};
        /// @brief Return the fitted value of the spectral index
        float alpha() {return itsAlpha;};
        /// @brief Return the fitted value of the spectral curvature
        float beta() {return itsBeta;};

        /// @brief Return the error on the fitted value of the flux at the reference frequency
        float fluxZeroErr() {return itsFluxZeroErr;};
        /// @brief Return the error on the fitted value of the spectral index
        float alphaErr() {return itsAlphaErr;};
        /// @brief Return the error on the fitted value of the spectral curvature
        float betaErr() {return itsBetaErr;};

    protected:
        /// @brief How many terms to fit?
        unsigned int itsNterms;

        /// @brief Flux at reference frequency
        float itsFluxZero;
        /// @brief Spectral index
        float itsAlpha;
        /// @brief Spectral curvature
        float itsBeta;
        /// @brief Error on flux at reference frequency
        float itsFluxZeroErr;
        /// @brief Error on spectral index
        float itsAlphaErr;
        /// @brief Error on spectral curvature
        float itsBetaErr;

};


}
}
#endif

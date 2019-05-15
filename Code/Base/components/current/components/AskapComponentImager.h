/// @file AskapComponentImager.h
///
/// @copyright (c) 2016 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>

#ifndef ASKAP_COMPONENTS_ASKAPCOMPONENTIMAGER_H
#define ASKAP_COMPONENTS_ASKAPCOMPONENTIMAGER_H

// ASKAPsoft includes
#include "casacore/casa/aipstype.h"
#include "casacore/images/Images/ImageInterface.h"
#include "casarest/components/ComponentModels/ComponentList.h"
#include "casarest/components/ComponentModels/SkyComponent.h"
#include "casacore/images/Images/ImageInterface.h"
#include "casacore/coordinates/Coordinates/DirectionCoordinate.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/casa/Arrays/IPosition.h"
#include "components/ComponentModels/Flux.h"
#include "casacore/scimath/Functionals/Gaussian2D.h"

namespace askap {
namespace components {

/// @brief Project the componentlist onto the image.
/// This class is designed to be interface compatible (in the method signature sense,
/// not in the OO sense, since there is no interface class) with the casacore
/// Component Imager. This class is based on the implementation of the casacore
/// ComponentImager however is implemented in a manner which makes it significantly
/// more performant.
class AskapComponentImager {
    public:
        /// Project the componentlist onto the image.
        ///
        /// Note: Currently only talour terms 0-2 are supported, and are define as follows:
        /// \li I0 = I(v0)
        /// \li I1 = I(v0) * alpha
        /// \li I2 = I(v0) * (0.5 * alpha * (alpha - 1) + beta)
        ///
        /// Where alpha is the spectral index and beta is the spectral curvature.
        ///
        /// @param[inout] image the image onto which the components will be projected.
        /// @param[in] list the list of components to project.
        /// @param[in] term the taylor term to image.
        template <class T>
        static void project(casacore::ImageInterface<T>& image,
                            const casacore::ComponentList& list,
                            const unsigned int term = 0);


        /// @brief Front-end to the different functions for calculating
        /// the flux due to a Gaussian component in a single pixel.
        /// @param[in] gauss       the gaussian function to be evaluated
        /// @param[in] xpix        the x-coordinate of the pixel
        /// @param[in] ypix        the y-coordinate of the pixel
        template <class T>
        static double evaluateGaussian(const casacore::Gaussian2D<T> &gauss,
                                       const int xpix, const int ypix);

    private:
        /// Project a point shape on to the image
        template <class T>
        static void projectPointShape(casacore::ImageInterface<T>& image,
                                      const casacore::SkyComponent& c,
                                      const casacore::Int latAxis, const casacore::Int longAxis,
                                      const casacore::DirectionCoordinate& dirCoord,
                                      const casacore::Int freqAxis, const casacore::uInt freqIdx,
                                      const casacore::Flux<casacore::Double>& flux,
                                      const casacore::Int polAxis, const casacore::uInt polIdx,
                                      const casacore::Stokes::StokesTypes& stokes);

        /// Project a gaussian shape on to the image
        template <class T>
        static void projectGaussianShape(casacore::ImageInterface<T>& image,
                                         const casacore::SkyComponent& c,
                                         const casacore::Int latAxis, const casacore::Int longAxis,
                                         const casacore::DirectionCoordinate& dirCoord,
                                         const casacore::Int freqAxis, const casacore::uInt freqIdx,
                                         const casacore::Flux<casacore::Double>& flux,
                                         const casacore::Int polAxis, const casacore::uInt polIdx,
                                         const casacore::Stokes::StokesTypes& stokes);

        /// Make an IPosition given the passed axis information.
        /// The returned IPosition will have one dimension for each of latAxis,
        /// longAxis, spectralAxis, and polAxis which have values >= 0. The index
        /// values are per the *Idx parameters.
        ///
        /// @param[in] latAxis the (zero based) index number on which the latitude
        ///            axis exists, or a negative number if the latitude axis is
        ///            not present.
        /// @param[in] longAxis the (zero based) index number on which the longitude
        ///            axis exists, or a negative number if the longitude axis is
        ///            not present.
        /// @param[in] spectralAxis the (zero based) index number on which the spectral
        ///            axis exists, or a negative number if the spectral axis is
        ///            not present.
        /// @param[in] polAxis the (zero based) index number on which the polarisation
        ///            axis exists, or a negative number if the polarisation axis is
        ///            not present.
        ///
        /// @param[in] latIdx   the index value on the latitude axis in the
        ///                     returned IPosition instance.
        /// @param[in] longIdx the index value on the longitude axis in the
        ///                     returned IPosition instance.
        /// @param[in] spectralIdx  the index value on the latitude axis in the
        ///                         returned IPosition instance.
        /// @param[in] polIdx   the index value on the polarisation axis in the
        ///                     returned IPosition instance.
        ///
        /// @return an IPosition object which represents all in the input parameters.
        static casacore::IPosition makePosition(const casacore::Int latAxis, const casacore::Int longAxis,
                                            const casacore::Int spectralAxis, const casacore::Int polAxis,
                                            const casacore::uInt latIdx, const casacore::uInt longIdx,
                                            const casacore::uInt spectralIdx, const casacore::uInt polIdx);

        /// Given a SkyComponent, determine the flux value (or appropriate
        /// value for talylor terms > 0) for the given channel frequency.
        ///
        /// @param[in] c    the sky component for which the flux value will
        ///                 be calculated.
        /// @param[in] chanFrequency    the channel frequency for which the flux
        ///                             is to be calculated.
        /// @param[in] term the taylor term to calculate the value for.
        static casacore::Flux<casacore::Double> makeFlux(const casacore::SkyComponent& c,
                const casacore::MFrequency& chanFrequency,
                const unsigned int term);

        /// Determine the number of pixels to sample before the gaussian tapers
        /// off to below the flux limit.
        ///
        /// This function samples the gaussian in the x and y dimensions and returns
        /// the number of pixels at which the gaussian has tapered off in both
        /// dimensions. That is, it keeps testing in both directions and will return
        /// when both sampled fluxes are < fluxLimit or when cutoff is > spatialLimit.
        ///
        /// @param[in] gauss        the gaussian function for which the cutoff is
        ///                         to be calcuated.
        /// @param[in] spatialLimit a spatial limit (in pixels) at which sampling
        ///                         will cease, even if the power is greater than
        ///                         fluxLimit.
        /// @param[in] fluxLimit    the flux limit which governs the cutoff.
        /// @return the number of pixels from the centre of the gaussian at which point
        ///         the power is < flux limit.
        template <class T>
        static int findCutoff(const casacore::Gaussian2D<T>& gauss, const int spatialLimit,
                              const double fluxLimit);

        /// @brief Calculate the flux in a single pixel due to a 2D
        /// Gaussian component. This integrates over the pixel to
        /// accurately measure the flux going in, thereby taking into
        /// account Gaussians that are comparable to or smaller in
        /// size than the pixel extent. The pixel location given
        /// (integer numbers) is assumed to be at the centre of the
        /// pixel.
        /// @param[in] gauss       the gaussian function to be evaluated
        /// @param[in] xpix        the x-coordinate of the pixel
        /// @param[in] ypix        the y-coordinate of the pixel
        template <class T>
        static double evaluateGaussian2D(const casacore::Gaussian2D<T> &gauss,
                                         const int xpix, const int ypix);

        /// @brief Calculate the flux in a single pixel due to a
        /// one-dimensional Gaussian component - that is, a 2D
        /// Gaussian component with zero minor axis size. This allows
        /// the calculations to be simplified and the flux in the
        /// pixel directly calculated from the error function. The
        /// pixel location given (integer numbers) is assumed to be at
        /// the centre of the pixel.
        /// @param[in] gauss       the gaussian function to be evaluated
        /// @param[in] xpix        the x-coordinate of the pixel
        /// @param[in] ypix        the y-coordinate of the pixel
        template <class T>
        static double evaluateGaussian1D(const casacore::Gaussian2D<T> &gauss,
                                         const int xpix, const int ypix);

};

// Explicit instantiations exist for float and double types only
extern template void
AskapComponentImager::project(casacore::ImageInterface<float>&,
                              const casacore::ComponentList&, const unsigned int);
extern template void
AskapComponentImager::project(casacore::ImageInterface<double>&,
                              const casacore::ComponentList&, const unsigned int);
extern template double
AskapComponentImager::evaluateGaussian(const casacore::Gaussian2D<float> &gauss,
                                       const int xpix, const int ypix);
extern template double
AskapComponentImager::evaluateGaussian(const casacore::Gaussian2D<double> &gauss,
                                       const int xpix, const int ypix);

}
}

#endif

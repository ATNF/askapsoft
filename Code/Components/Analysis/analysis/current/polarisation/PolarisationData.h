/// @file
///
/// Class to hold the input data for the polarisation pipeline
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#ifndef ASKAP_ANALYSIS_POL_DATA_H_
#define ASKAP_ANALYSIS_POL_DATA_H_

#include <polarisation/StokesSpectrum.h>
#include <polarisation/StokesImodel.h>
#include <catalogues/CasdaComponent.h>

#include <casacore/casa/Arrays/Array.h>

#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

/// @brief Class to hold observed data used for polarisation analysis.
/// @details This class relates to a specific Stokes I component, and
/// holds extracted source, noise and model spectra in different Stokes
/// parameters, along with vectors holding the frequency and
/// lambda-squared values.
class PolarisationData {
    public:
        PolarisationData(const LOFAR::ParameterSet &parset);
        virtual ~PolarisationData() {};

        /// @brief Set up all spectra and associated arrays.
        /// @details This function extracts spectra in each Stokes
        /// parameter, and writes the spectra to disk if requested. The
        /// noise spectrum is computed (as the average of Q & U
        /// noise). The frequency and lambda-squared values are
        /// defined. The model Stokes I spectrum is then computed, using
        /// the StokesImodel class.
        void initialise(CasdaComponent *comp);

        /// @brief Return the Stokes I spectrum
        StokesSpectrum &I()
        {
            StokesSpectrum &ref = itsStokesI; return ref;
        }

        /// @brief Return the Stokes Q spectrum
        StokesSpectrum &Q()
        {
            StokesSpectrum &ref = itsStokesQ; return ref;
        }

        /// @brief Return the Stokes U spectrum
        StokesSpectrum &U()
        {
            StokesSpectrum &ref = itsStokesU; return ref;
        }

        /// @brief Return the Stokes V spectrum
        StokesSpectrum &V()
        {
            StokesSpectrum &ref = itsStokesV; return ref;
        }

        /// @brief Return the model Stokes I spectrum as a vector
        casa::Vector<float> Imod()
        {
            return itsModelStokesI.modelSpectrum();
        }

        /// @brief Return the Stokes I model object
        StokesImodel &model() {return itsModelStokesI;};

        /// @brief Return the noise spectrum as a vector
        casa::Vector<float> &noise()
        {
            casa::Vector<float> &ref = itsAverageNoiseSpectrum; return ref;
        }

        /// @brief Return the vector of lambda-squared values
        casa::Vector<float> &l2()
        {
            casa::Vector<float> &ref = itsLambdaSquared; return ref;
        }

    protected:

        /// @brief Parset relating to RMSynthesis parameters
        LOFAR::ParameterSet itsParset;

        /// @brief Spectra extracted from cubes
        /// {
        StokesSpectrum      itsStokesI;
        StokesSpectrum      itsStokesQ;
        StokesSpectrum      itsStokesU;
        StokesSpectrum      itsStokesV;
        /// }

        /// @brief The Stokes I model spectrum
        StokesImodel        itsModelStokesI;

        /// @brief The noise spectrum, averaged between Q & U
        casa::Vector<float> itsAverageNoiseSpectrum;

        /// @brief The frequency values for the spectra
        casa::Vector<float> itsFrequencies;
        /// @brief The lambda-squared values for the spectra
        casa::Vector<float> itsLambdaSquared;

};


}

}




#endif

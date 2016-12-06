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
#include <catalogues/CasdaComponent.h>

#include <casacore/casa/Arrays/Array.h>

#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

class PolarisationData {
    public:
        PolarisationData(const LOFAR::ParameterSet &parset);
        virtual ~PolarisationData(){};

        void initialise(CasdaComponent *comp);

        StokesSpectrum &I()
        {
            StokesSpectrum &ref = itsStokesI; return ref;
        }
        StokesSpectrum &Q()
        {
            StokesSpectrum &ref = itsStokesQ; return ref;
        }
        StokesSpectrum &U()
        {
            StokesSpectrum &ref = itsStokesU; return ref;
        }
        StokesSpectrum &V()
        {
            StokesSpectrum &ref = itsStokesV; return ref;
        }

        casa::Vector<float> &Imod()
        {
            casa::Vector<float> &ref = itsModelStokesI; return ref;
        }
        casa::Vector<float> &noise()
        {
            casa::Vector<float> &ref = itsAverageNoiseSpectrum; return ref;
        }
        casa::Vector<float> &l2()
        {
            casa::Vector<float> &ref = itsLambdaSquared; return ref;
        }

    protected:

        LOFAR::ParameterSet itsParset;
    
        /// @brief Spectra extracted from cubes
        /// {
        StokesSpectrum itsStokesI;
        StokesSpectrum itsStokesQ;
        StokesSpectrum itsStokesU;
        StokesSpectrum itsStokesV;
        /// }

        casa::Vector<float> itsModelStokesI;

        casa::Vector<float> itsAverageNoiseSpectrum;

        casa::Vector<float> itsFrequencies;
        casa::Vector<float> itsLambdaSquared;
        casa::Vector<float> itsStokesIcoeffs;

        /// @brief Width of extraction box, defined through parset
        unsigned int itsExtractBoxSize;

        float itsBoxNormalisation;

};


}

}




#endif

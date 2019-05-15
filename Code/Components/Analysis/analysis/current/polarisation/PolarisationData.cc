/// @file
///
/// Extracting spectra and other polarised data from continuum cubes,
/// ready for Rotation Measure Synthesis.
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
#include <polarisation/PolarisationData.h>
#include <askap_analysis.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>

#include <extraction/SourceSpectrumExtractor.h>
#include <extraction/NoiseSpectrumExtractor.h>
#include <catalogues/CasdaComponent.h>

#include <casacore/casa/Quanta/Quantum.h>
#include <Common/ParameterSet.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".poldata");

namespace askap {

namespace analysis {

PolarisationData::PolarisationData(const LOFAR::ParameterSet &parset):
    itsParset(parset),
    itsStokesI(parset, "I"),
    itsStokesQ(parset, "Q"),
    itsStokesU(parset, "U"),
    itsStokesV(parset, "V"),
    itsModelStokesI(parset)
{

    // record extraction parameters

    itsParset.replace("useDetectedPixels", "false");
    itsParset.replace("scaleSpectraByBeam", "true");

}

void PolarisationData::initialise(CasdaComponent *comp)
{

    // extract Stokes I,Q,U and noise spectra
    itsStokesI.setComponent(comp);
    itsStokesI.extract();
    unsigned int size = itsStokesI.spectrum().size();
    itsStokesQ.setComponent(comp);
    itsStokesQ.extract();
    itsStokesU.setComponent(comp);
    itsStokesU.extract();
    itsStokesV.setComponent(comp);
    itsStokesV.extract();

    // write out extracted spectra
    if (itsParset.getBool("writeSpectra", "true")) {
        itsStokesI.write();
        itsStokesQ.write();
        itsStokesU.write();
        itsStokesV.write();
    }

    // compute "average noise"
    itsAverageNoiseSpectrum = (itsStokesQ.noiseSpectrum() +
                               itsStokesU.noiseSpectrum()) / 2.;

    // get frequency array and compute lambda-squared array
    itsFrequencies = itsStokesI.frequencies();
    ASKAPASSERT(itsFrequencies.size() == size);
    itsLambdaSquared = casa::Vector<Float>(size);
    for (unsigned int i = 0; i < size; i++) {
        float lambda = QC::c.getValue() / itsFrequencies[i];
        itsLambdaSquared[i] = lambda * lambda;
    }

    // find Stokes I model
    itsModelStokesI.initialise(itsStokesI, comp);

}


}
}

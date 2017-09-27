/// @file
///
/// Class to hold the extracted data for a single HI source
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
#include <extraction/HIdata.h>

#include <askap_analysis.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <boost/shared_ptr.hpp>
#include <extraction/SourceSpectrumExtractor.h>
#include <extraction/NoiseSpectrumExtractor.h>
#include <extraction/MomentMapExtractor.h>
#include <extraction/CubeletExtractor.h>
#include <Common/ParameterSet.h>
#include <busyfit/BusyFit.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>

ASKAP_LOGGER(logger, ".hidata");

namespace askap {

namespace analysis {

HIdata::HIdata(const LOFAR::ParameterSet &parset):
    itsParset(parset)
{
    itsCubeName = parset.getString("image", "");
    ASKAPCHECK(itsCubeName != "", "No cube name given");

    itsBeamLog = parset.getString("beamLog", "");

    itsBFparams = casa::Vector<double>(BUSYFIT_FREE_PARAM, 0.);
    itsBFerrors = casa::Vector<double>(BUSYFIT_FREE_PARAM, 0.);

    itsMom0Fit = casa::Vector<double>(3, 0.);
    itsMom0FitError = casa::Vector<double>(3, 0.);

    // Define and create (if need be) the directories to hold the extracted data products
    std::stringstream cmd;
    std::string spectraDir = itsParset.getString("HiEmissionCatalogue.spectraDir", "Spectra");
    std::string momentDir = itsParset.getString("HiEmissionCatalogue.momentDir", "Moments");
    std::string cubeletDir = itsParset.getString("HiEmissionCatalogue.cubeletDir", "Cubelets");
    cmd << " mkdir -p " << spectraDir << " " << momentDir << " " << cubeletDir;
    const int status = system(cmd.str().c_str());
    if (status != 0) {
        ASKAPTHROW(AskapError, "Error making directories for extracted data products: code = " << status
                   << " - Command = " << cmd.str());
    }

    // Define the parset used to set up the source extractor
    LOFAR::ParameterSet specParset;
    specParset.add(LOFAR::KVpair("spectralCube", itsCubeName));
    specParset.add(LOFAR::KVpair("spectralOutputBase", spectraDir + "/spectrum"));
    specParset.add(LOFAR::KVpair("useDetectedPixels", true));
    specParset.add(LOFAR::KVpair("scaleSpectraByBeam", true));
    specParset.add(LOFAR::KVpair("beamLog", itsBeamLog));
    specParset.add("imagetype", itsParset.getString("imagetype", "fits"));
    itsSpecExtractor = boost::shared_ptr<SourceSpectrumExtractor>(new SourceSpectrumExtractor(specParset));

    // Define the parset used to set up the noise extractor
    LOFAR::ParameterSet noiseParset;
    noiseParset.add(LOFAR::KVpair("spectralCube", itsCubeName));
    noiseParset.add(LOFAR::KVpair("spectralOutputBase", spectraDir + "/noiseSpectrum"));
    noiseParset.add(LOFAR::KVpair("noiseArea",
                                  itsParset.getFloat("HiEmissionCatalogue.noiseArea", 50.)));
    noiseParset.add(LOFAR::KVpair("robust",
                                  itsParset.getBool("robust", true)));
    noiseParset.add(LOFAR::KVpair("useDetectedPixels", true));
    noiseParset.add(LOFAR::KVpair("scaleSpectraByBeam", false));
    noiseParset.add("imagetype", itsParset.getString("imagetype", "fits"));
    itsNoiseExtractor = boost::shared_ptr<NoiseSpectrumExtractor>(new NoiseSpectrumExtractor(noiseParset));

    // Define the parset used to set up the moment-map extractor
    LOFAR::ParameterSet momentParset;
    momentParset.add(LOFAR::KVpair("spectralCube", itsCubeName));
    momentParset.add(LOFAR::KVpair("momentOutputBase", momentDir + "/mom%m"));
    momentParset.add("moments", itsParset.getString("HiEmissionCatalogue.moments", "[0,1,2]"));
    momentParset.add(LOFAR::KVpair("beamLog", itsBeamLog));
    momentParset.add("imagetype", itsParset.getString("imagetype", "fits"));
    itsMomentExtractor = boost::shared_ptr<MomentMapExtractor>(new MomentMapExtractor(momentParset));

    // Define the parset used to set up the cubelet extractor
    LOFAR::ParameterSet cubeletParset;
    cubeletParset.add(LOFAR::KVpair("spectralCube", itsCubeName));
    cubeletParset.add(LOFAR::KVpair("cubeletOutputBase", cubeletDir + "/cubelet"));
    cubeletParset.add(LOFAR::KVpair("beamLog", itsBeamLog));
    cubeletParset.add("imagetype", itsParset.getString("imagetype", "fits"));
    itsCubeletExtractor = boost::shared_ptr<CubeletExtractor>(new CubeletExtractor(cubeletParset));


}

void HIdata::findVoxelStats()
{

    itsFluxMax = itsSource->getPeakFlux();
    casa::IPosition start = itsCubeletExtractor->slicer().start();
    casa::Array<float> cubelet = itsCubeletExtractor->array();
    std::vector<PixelInfo::Voxel> voxelList = itsSource->getPixelSet();
    float min, sumf = 0., sumff = 0.;
    std::vector<PixelInfo::Voxel>::iterator vox = voxelList.begin();
    for (; vox < voxelList.end(); vox++) {
        //float flux = vox->getF();
        if (itsSource->isInObject(*vox)) {
            // The Stokes axis, if present, will be of length 1, and will be either location 2 or 3 in the IPosition
            casa::IPosition loc;
            if (start.size() == 2) {
                loc = casa::IPosition(start.size(), vox->getX(), vox->getY());
            } else if (start.size() == 3) {
                loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ());
            } else {
                if (itsCubeletExtractor->slicer().length()[2] == 1) {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), 0, vox->getZ());
                } else {
                    loc = casa::IPosition(start.size(), vox->getX(), vox->getY(), vox->getZ(), 0);
                }
            }
            float flux = cubelet(loc - start);
            if (vox == voxelList.begin()) {
                min = flux;
            } else {
                min = std::min(min, flux);
            }
            sumf += flux;
            sumff += flux * flux;
        }
    }
    float size = float(voxelList.size());
    itsFluxMin = min;
    itsFluxMean = sumf / size;
    itsFluxStddev = sqrt(sumff / size - sumf * sumf / size / size);
    itsFluxRMS = sqrt(sumff / size);

}


void HIdata::extract()
{
    extractSpectrum();
    extractNoise();
    extractMoments();
    extractCubelet();
}

void HIdata::extractSpectrum()
{
    itsSpecExtractor->setSource(itsSource);
    itsSpecExtractor->extract();
}

void HIdata::extractNoise()
{
    itsNoiseExtractor->setSource(itsSource);
    itsNoiseExtractor->extract();
}

void HIdata::extractMoments()
{
    itsMomentExtractor->setSource(itsSource);
    itsMomentExtractor->extract();
}

void HIdata::extractCubelet()
{
    itsCubeletExtractor->setSource(itsSource);
    itsCubeletExtractor->extract();
}

void HIdata::write()
{

    itsSpecExtractor->writeImage();
    itsNoiseExtractor->writeImage();
    itsMomentExtractor->writeImage();
    itsCubeletExtractor->writeImage();

}

int HIdata::busyFunctionFit()
{

    //convert to doubles
    // casa::Array<double> spectrum(itsSpecExtractor->array().shape(),
    //                              (double *)itsSpecExtractor->array().data());
    // casa::Array<double> noise(itsNoiseExtractor->array().shape(),
    //                           (double *)itsNoiseExtractor->array().data());

    std::vector<double> spectrum(itsSpecExtractor->array().size());
    for (size_t i = 0; i < spectrum.size(); i++) {
        spectrum[i] = double(itsSpecExtractor->array().tovector()[i]);
    }
    std::vector<double> noise(itsSpecExtractor->array().size());
    for (size_t i = 0; i < noise.size(); i++) {
        noise[i] = double(itsNoiseExtractor->array().tovector()[i]);
    }
    
    BusyFit *theFitter = new BusyFit();

    bool plotsTurnedOff = true;
    bool relax = false;
    bool verbose = false;

    theFitter->setup(spectrum.size(), spectrum.data(), noise.data(),
                     plotsTurnedOff, relax, verbose);


    if (status == 0) {
        theFitter->getResult(itsBFparams.data(), itsBFerrors.data(),
                             itsBFchisq, itsBFredChisq, itsBFndof);
    }

    return status;

}


void HIdata::fitToMom0()
{
    casa::Array<float> mom0 = itsMomentExtractor->mom0();
    casa::IPosition start = itsMomentExtractor->slicer().start().nonDegenerate();
    casa::LogicalArray mom0mask = itsMomentExtractor->mom0mask();
    size_t momSize = mom0.size();
    casa::IPosition momShape = mom0.shape();

    casa::Matrix<casa::Double> pos;
    casa::Vector<casa::Double> f;
    casa::Vector<casa::Double> sigma;
    pos.resize(momSize, 2);
    f.resize(momSize);
    sigma.resize(momSize);
    casa::Vector<casa::Double> curpos(2);
    curpos = 0;

    for (size_t y = 0; y < momShape[1]; y++) {
        for (size_t x = 0; x < momShape[0]; x++) {
            size_t i = x + y * momShape[0];
            curpos(0) = x;
            curpos(1) = y;
            pos.row(i) = curpos;
            if (!mom0mask.data()[i]) {
                f(i) = 0.;
            } else {
                f(i) = mom0.data()[i];
            }
            sigma(i) = 1.;
            // ASKAPLOG_DEBUG_STR(logger, "i="<<i<<", (x,y)=("<<x<<","<<y<<"), f(i)="<<f(i));
        }
    }

    // Get the restoring beam, to use as the initial guess
    casa::Vector<Quantum<Double> > beam = itsMomentExtractor->inputBeam();
    double cellsize = fabs(itsMomentExtractor->inputCoordSys().directionCoordinate().increment()[0]);
    std::vector<SubComponent> initial(1);
    initial[0].setX(itsSource->getXcentre() - start[0]);
    initial[0].setY(itsSource->getYcentre() - start[1]);
    initial[0].setPeak(casa::max(mom0));
    initial[0].setMajor(beam[0].getValue("rad") / cellsize);
    initial[0].setMinor(beam[1].getValue("rad") / cellsize);
    initial[0].setPA(beam[2].getValue());

    FittingParameters fitparams(itsParset);
    fitparams.setMaxRMS(50.);

    fitparams.setFlagFitThisParam("full");
    Fitter fullfit(fitparams);
    fullfit.setNumGauss(1);
    fullfit.setEstimates(initial);
    fullfit.setRetries();
    fullfit.setMasks();
    fullfit.fit(pos, f, sigma);

    // Fit a PSF-shaped Gaussian, fixing it to be at the centre
    fitparams.setFlagFitThisParam("psf");
    Fitter psffit(fitparams);
    psffit.setNumGauss(1);
    psffit.setEstimates(initial);
    psffit.setRetries();
    psffit.setMasks();
    psffit.fit(pos, f, sigma);

    FitResults fullres;
    fullres.saveResults(fullfit);
    casa::Gaussian2D<casa::Double> full = fullres.gaussian(0);
    casa::Vector<casa::Double> fullErrors = fullfit.error(0);
    itsMom0Fit[0] = full.majorAxis() * cellsize;
    itsMom0Fit[1] = full.minorAxis() * cellsize;
    itsMom0Fit[2] = full.PA();
    itsMom0FitError[0] = fullErrors[3] * cellsize;
    itsMom0FitError[1] = fullErrors[4] * cellsize;
    itsMom0FitError[2] = fullErrors[5];

    // If the PSF fit is rejected, it means the source is resolved.
    itsMom0Resolved = !psffit.acceptable();


}




}

}

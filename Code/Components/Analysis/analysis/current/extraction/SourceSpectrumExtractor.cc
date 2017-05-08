/// @file
///
/// Class to handle extraction of a summed spectrum corresponding to a source.
///
/// @copyright (c) 2011 CSIRO
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
#include <extraction/SourceSpectrumExtractor.h>
#include <askap_analysis.h>
#include <extraction/SourceDataExtractor.h>
#include <extraction/SpectralBoxExtractor.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>

#include <imageaccess/CasaImageAccess.h>
#include <imageaccess/BeamLogger.h>

#include <duchamp/PixelMap/Object2D.hh>

#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/MaskedArray.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/images/Images/ImageInterface.h>
#include <casacore/images/Images/ImageOpener.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/MIRIADImage.h>
#include <casacore/images/Images/SubImage.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/measures/Measures/Stokes.h>

#include <Common/ParameterSet.h>

using namespace askap::analysis::sourcefitting;

ASKAP_LOGGER(logger, ".sourcespectrumextractor");

namespace askap {

namespace analysis {

SourceSpectrumExtractor::SourceSpectrumExtractor(const LOFAR::ParameterSet& parset):
    SpectralBoxExtractor(parset)
{

    itsFlagUseDetection = parset.getBool("useDetectedPixels", false);
    if (itsFlagUseDetection) {
        itsBoxWidth = -1;
        if (parset.isDefined("spectralBoxWidth")) {
            ASKAPLOG_WARN_STR(logger, "useDetectedPixels option selected, " <<
                              "so setting spectralBoxWidth=-1");
        }
    }

    itsFlagDoScale = parset.getBool("scaleSpectraByBeam", true);
    itsBeamLog = parset.getString("beamLog", "");

    for (size_t stokes = 0; stokes < itsStokesList.size(); stokes++) {
        itsCurrentStokes = itsStokesList[stokes];
        itsInputCube = itsCubeStokesMap[itsCurrentStokes];
        this->initialiseArray();
    }

}


void SourceSpectrumExtractor::setBeamScale()
{
    for (size_t stokes = 0; stokes < itsStokesList.size(); stokes++) {

        // get either the matching image for the current stokes value,
        // or the first&only in the input list
        itsCurrentStokes = itsStokesList[stokes];
        itsInputCube = itsCubeStokesMap[itsCurrentStokes];

        itsBeamScaleFactor[itsCurrentStokes] = std::vector<float>();
        ASKAPLOG_DEBUG_STR(logger, "About to find beam scale for Stokes " << itsCurrentStokes << " and image " << itsInputCube);

        if (itsFlagDoScale) {

            if (this->openInput()) {

                // Change the output units to remove the "/beam" extension
                std::string inunit=itsInputUnits.getName();
                if (inunit.substr(inunit.size()-5,inunit.size()) == "/beam"){
                    itsOutputUnits.setName(inunit.substr(0,inunit.size()-5));
                }
    
                std::vector< casa::Vector<Quantum<Double> > > beamvec;

                casa::Vector<Quantum<Double> >
                inputBeam = itsInputCubePtr->imageInfo().restoringBeam().toVector();

                ASKAPLOG_DEBUG_STR(logger, "Setting beam scaling factor. BeamLog=" <<
                                   itsBeamLog << ", image beam = " << inputBeam);

                if (itsBeamLog == "") {
                    if (inputBeam.size() == 0) {
                        ASKAPLOG_WARN_STR(logger, "Input image \"" << itsInputCube <<
                                          "\" has no beam information. Not scaling spectra by beam");
                        itsBeamScaleFactor[itsCurrentStokes].push_back(1.);
                    } else {
                        beamvec.push_back(inputBeam);
                        ASKAPLOG_DEBUG_STR(logger, "Beam for input cube = " << inputBeam);
                    }
                } else {
                    std::string beamlogfile = itsBeamLog;
                    if (itsBeamLog.find("%p") != std::string::npos) {
                        // the beam log has a "%p" string, meaning
                        // polarisation substitution is possible
                        casa::Stokes stokes;
                        casa::String stokesname(stokes.name(itsCurrentStokes));
                        stokesname.downcase();
                        ASKAPLOG_DEBUG_STR(logger, "Input beam log: replacing \"%p\" with " <<
                                           stokesname.c_str() << " in " << beamlogfile);
                        beamlogfile.replace(beamlogfile.find("%p"), 2, stokesname.c_str());
                    }
                    accessors::BeamLogger beamlog(beamlogfile);
                    beamlog.read();
                    beamvec = beamlog.beamlist();

                    if (int(beamvec.size()) != itsInputCubePtr->shape()(itsSpcAxis)) {
                        ASKAPLOG_ERROR_STR(logger, "Beam log " << itsBeamLog <<
                                           " has " << beamvec.size() <<
                                           " entries - was expecting " <<
                                           itsInputCubePtr->shape()(itsSpcAxis));
                        beamvec = std::vector< Vector<Quantum<Double> > >(1, inputBeam);
                    }
                }

                if (beamvec.size() > 0) {

                    for (size_t i = 0; i < beamvec.size(); i++) {

                        int dirCoNum = itsInputCoords.findCoordinate(casa::Coordinate::DIRECTION);
                        casa::DirectionCoordinate
                        dirCoo = itsInputCoords.directionCoordinate(dirCoNum);
                        double fwhmMajPix = beamvec[i][0].getValue(dirCoo.worldAxisUnits()[0]) /
                                            fabs(dirCoo.increment()[0]);
                        double fwhmMinPix = beamvec[i][1].getValue(dirCoo.worldAxisUnits()[1]) /
                                            fabs(dirCoo.increment()[1]);

                        if (itsFlagUseDetection) {
                            double bpaDeg = beamvec[i][2].getValue("deg");
                            duchamp::DuchampBeam beam(fwhmMajPix, fwhmMinPix, bpaDeg);
                            itsBeamScaleFactor[itsCurrentStokes].push_back(beam.area());
                            if (itsBeamLog == "") {
                                ASKAPLOG_DEBUG_STR(logger, "Stokes " << itsCurrentStokes << " has beam scale factor = " <<
                                                   itsBeamScaleFactor[itsCurrentStokes] << " using beam of " <<
                                                   fwhmMajPix << "x" << fwhmMinPix);
                            }
                        } else {

                            double costheta = cos(beamvec[i][2].getValue("rad"));
                            double sintheta = sin(beamvec[i][2].getValue("rad"));

                            double majSDsq = fwhmMajPix * fwhmMajPix / 8. / M_LN2;
                            double minSDsq = fwhmMinPix * fwhmMinPix / 8. / M_LN2;

                            int hw = (itsBoxWidth - 1) / 2;
                            double scaleFactor = 0.;
                            for (int y = -hw; y <= hw; y++) {
                                for (int x = -hw; x <= hw; x++) {
                                    double u = x * costheta + y * sintheta;
                                    double v = x * sintheta - y * costheta;
                                    scaleFactor += exp(-0.5 * (u * u / majSDsq + v * v / minSDsq));
                                }
                            }
                            itsBeamScaleFactor[itsCurrentStokes].push_back(scaleFactor);

                            if (itsBeamLog == "") {
                                ASKAPLOG_DEBUG_STR(logger, "Stokes " << itsCurrentStokes << " has beam scale factor = " <<
                                                   itsBeamScaleFactor[itsCurrentStokes]);
                            }

                        }
                    }

                }

                ASKAPLOG_DEBUG_STR(logger,
                                   "Defined the beam scale factor vector of size " <<
                                   itsBeamScaleFactor[itsCurrentStokes].size());

                this->closeInput();
            } else {
                ASKAPLOG_ERROR_STR(logger, "Could not open image \"" << itsInputCube << "\".");
            }
        }
    }
}

void SourceSpectrumExtractor::extract()
{

    this->setBeamScale();

    for (size_t stokes = 0; stokes < itsStokesList.size(); stokes++) {

        // get either the matching image for the current stokes value,
        // or the first&only in the input list
        itsCurrentStokes = itsStokesList[stokes];
        itsInputCube = itsCubeStokesMap[itsCurrentStokes];
        ASKAPLOG_INFO_STR(logger, "Extracting spectrum for Stokes " << itsCurrentStokes
                          << " from image \"" << itsInputCube << "\".");
        this->defineSlicer();
        if (this->openInput()) {
            casa::Stokes stk;
            ASKAPLOG_INFO_STR(logger, "Extracting spectrum from " << itsInputCube <<
                              " with shape " << itsInputCubePtr->shape() <<
                              " for source ID " << itsSourceID <<
                              " using slicer " << itsSlicer <<
                              " and Stokes " << stk.name(itsCurrentStokes));

            boost::shared_ptr<SubImage<Float> >
            sub(new SubImage<Float>(*itsInputCubePtr, itsSlicer));

            ASKAPASSERT(sub->size() > 0);
            const casa::MaskedArray<Float> msub(sub->get(), sub->getMask());
            casa::Array<Float> subarray(sub->shape());
            subarray = msub;

            casa::IPosition outBLC(itsArray.ndim(), 0), outTRC(itsArray.shape() - 1);
            if (itsStkAxis > -1) {
                // If there is a Stokes axis in the input file
                outBLC(itsStkAxis) = outTRC(itsStkAxis) = stokes;
            }

            if (!itsFlagUseDetection) {
                casa::Array<Float> sumarray = partialSums(subarray, IPosition(2, 0, 1));
                itsArray(outBLC, outTRC) = sumarray.reform(itsArray(outBLC, outTRC).shape());

            } else {
                ASKAPASSERT(itsSource);
                ASKAPLOG_INFO_STR(logger,
                                  "Extracting integrated spectrum using " <<
                                  "all detected spatial pixels");
                IPosition shape = itsInputCubePtr->shape();

                PixelInfo::Object2D spatmap = itsSource->getSpatialMap();
                casa::IPosition blc(shape.size(), 0);
                casa::IPosition trc(shape.size(), 0);
                casa::IPosition inc(shape.size(), 1);

                trc(itsSpcAxis) = shape[itsSpcAxis] - 1;
                if (itsStkAxis > -1) {
                    casa::Stokes stk;
                    blc(itsStkAxis) = trc(itsStkAxis) =
                                          itsInputCoords.stokesPixelNumber(stk.name(itsCurrentStokes));
                }

                for (int x = itsSource->getXmin(); x <= itsSource->getXmax(); x++) {
                    for (int y = itsSource->getYmin(); y <= itsSource->getYmax(); y++) {
                        if (spatmap.isInObject(x, y)) {
                            blc(itsLngAxis) = trc(itsLngAxis) = x - itsSource->getXmin();
                            blc(itsLatAxis) = trc(itsLatAxis) = y - itsSource->getYmin();
                            casa::Array<Float> spec = subarray(blc, trc, inc);
                            spec = spec.reform(itsArray(outBLC, outTRC).shape());
                            itsArray(outBLC, outTRC) = itsArray(outBLC, outTRC) + spec;
                        }
                    }
                }
            }
            this->closeInput();
        } else {
            ASKAPLOG_ERROR_STR(logger, "Could not open image \"" << itsInputCube << "\".");
        }
    }


    if (itsFlagDoScale) {

        if (itsBeamScaleFactor[itsCurrentStokes].size() == 1) {
            itsArray /= itsBeamScaleFactor[itsCurrentStokes][0];
        } else {
            casa::IPosition start(itsArray.ndim(), 0);
            casa::IPosition end = itsArray.shape() - 1;
            start(itsLngAxis) = start(itsLatAxis) = 0;
            for (int z = 0; z < itsArray.shape()(itsSpcAxis); z++) {
                start(itsSpcAxis) = end(itsSpcAxis) = z;
                itsArray(start, end) = itsArray(start, end) / itsBeamScaleFactor[itsCurrentStokes][z];
            }
        }

    }

}


}
}

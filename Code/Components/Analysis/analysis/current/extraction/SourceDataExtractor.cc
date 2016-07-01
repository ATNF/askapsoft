/// @file
///
/// Base class for handling extraction of image data corresponding to a source
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
#include <extraction/SourceDataExtractor.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <string>
#include <sourcefitting/RadioSource.h>
#include <casainterface/CasaInterface.h>
#include <imageaccess/CasaImageAccess.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/CasdaIsland.h>

#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/images/Images/ImageInterface.h>
#include <casacore/images/Images/ImageOpener.h>
#include <casacore/images/Images/FITSImage.h>
#include <casacore/images/Images/MIRIADImage.h>
#include <casacore/lattices/Lattices/LatticeBase.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <Common/ParameterSet.h>
#include <casacore/measures/Measures/Stokes.h>
#include <boost/shared_ptr.hpp>

#include <utils/PolConverter.h>

ASKAP_LOGGER(logger, ".sourcedataextractor");

namespace askap {

namespace analysis {

SourceDataExtractor::SourceDataExtractor(const LOFAR::ParameterSet& parset)
{
    itsSource = 0;
    itsInputCube = ""; // start off with this blank. Needs to be
    // set before calling openInput()

    itsInputCubeList = parset.getStringVector("spectralCube",
                       std::vector<std::string>(0));

    // itsCentreType = parset.getString("pixelCentre", "peak");

    // Take the following from SynthesisParamsHelper.cc in Synthesis
    // there could be many ways to define stokes, e.g. ["XX YY"] or
    // ["XX","YY"] or "XX,YY" to allow some flexibility we have to
    // concatenate all elements first and then allow the parser from
    // PolConverter to take care of extracting the products.
    const std::vector<std::string>
    stokesVec = parset.getStringVector("polarisation",
                                       std::vector<std::string>(1, "I"));
    std::string stokesStr;
    for (size_t i = 0; i < stokesVec.size(); ++i) {
        stokesStr += stokesVec[i];
    }
    itsStokesList = scimath::PolConverter::fromString(stokesStr);

    this->verifyInputs();

}

SourceDataExtractor::~SourceDataExtractor()
{
    itsInputCubePtr.reset();
}

SourceDataExtractor::SourceDataExtractor(const SourceDataExtractor& other)
{
    this->operator=(other);
}

SourceDataExtractor& SourceDataExtractor::operator=(const SourceDataExtractor& other)
{
    if (this == &other) return *this;
    itsSource = other.itsSource;
    itsCentreType = other.itsCentreType;
    itsSlicer = other.itsSlicer;
    itsInputCube = other.itsInputCube;
    itsInputCubeList = other.itsInputCubeList;
    itsInputCubePtr = other.itsInputCubePtr;
    itsStokesList = other.itsStokesList;
    itsCurrentStokes = other.itsCurrentStokes;
    itsOutputFilenameBase = other.itsOutputFilenameBase;
    itsOutputFilename = other.itsOutputFilename;
    itsArray = other.itsArray;
    itsInputCoords = other.itsInputCoords;
    itsLngAxis = other.itsLngAxis;
    itsLatAxis = other.itsLatAxis;
    itsSpcAxis = other.itsSpcAxis;
    itsStkAxis = other.itsStkAxis;
    return *this;
}

casa::IPosition SourceDataExtractor::getShape(std::string image)
{
    itsInputCube = image;
    casa::IPosition shape;
    if (this->openInput()) {
        shape = itsInputCubePtr->shape();
        this->closeInput();
    }
    return shape;
}
//---------------
template <class T> double SourceDataExtractor::getRA(T &object)
{
    return object.ra();
}
template double SourceDataExtractor::getRA<CasdaComponent>(CasdaComponent &object);
template double SourceDataExtractor::getRA<CasdaIsland>(CasdaIsland &object);
template <> double SourceDataExtractor::getRA<RadioSource>(RadioSource &object)
{
    return object.getRA();
}
//---------------
template <class T> double SourceDataExtractor::getDec(T &object)
{
    return object.dec();
}
template double SourceDataExtractor::getDec<CasdaComponent>(CasdaComponent &object);
template double SourceDataExtractor::getDec<CasdaIsland>(CasdaIsland &object);
template <> double SourceDataExtractor::getDec<RadioSource>(RadioSource &object)
{
    return object.getDec();
}
//---------------
template <class T> std::string SourceDataExtractor::getID(T &obj)
{
    int ID = obj.getID();
    std::stringstream ss;
    ss << ID;
    return ss.str();
}
template std::string SourceDataExtractor::getID<RadioSource>(RadioSource &obj);
template <> std::string SourceDataExtractor::getID<CasdaComponent>(CasdaComponent &obj)
{
    return obj.componentID();
}
template <> std::string SourceDataExtractor::getID<CasdaIsland>(CasdaIsland &obj)
{
    return obj.id();
}
//---------------
template <class T>
void SourceDataExtractor::setSourceLoc(T* src)
{
    itsSourceID = getID(*src);
    std::stringstream ss;
    ss << itsOutputFilenameBase << "_" << itsSourceID;
    itsOutputFilename = ss.str();

    casa::DirectionCoordinate dc = itsInputCoords.directionCoordinate();
    casa::Vector<casa::Double> pix(2);
    MDirection refDir(casa::Quantity(getRA(*src), "deg"),
                      casa::Quantity(getDec(*src), "deg"),
                      dc.directionType());
    dc.toPixel(pix, refDir);
    itsXloc = pix[0];
    itsYloc = pix[1];

}
template void SourceDataExtractor::setSourceLoc<CasdaComponent>(CasdaComponent* src);
template void SourceDataExtractor::setSourceLoc<RadioSource>(RadioSource* src);

void SourceDataExtractor::setSource(RadioSource* src)
{

    itsSource = src;

    if (itsSource) {
        setSourceLoc(src);
    }
}
void SourceDataExtractor::setSource(CasdaComponent* src)
{
    setSourceLoc(src);
}
//---------------



void SourceDataExtractor::checkPol(std::string image,
                                   casa::Stokes::StokesTypes stokes,
                                   int nStokesRequest)
{

    itsInputCube = image;
    std::vector<casa::Stokes::StokesTypes> stokesvec(1, stokes);
    std::string polstring = scimath::PolConverter::toString(stokesvec)[0];

    if (this->openInput()) {
        int stokeCooNum = itsInputCubePtr->coordinates().polarizationCoordinateNumber();
        if (stokeCooNum > -1) {

            const casa::StokesCoordinate
            stokeCoo = itsInputCubePtr->coordinates().stokesCoordinate(stokeCooNum);
            if (stokeCooNum == -1 || itsStkAxis == -1) {
                ASKAPCHECK(polstring == "I", "Extraction: Input cube " << image <<
                           " has no polarisation axis, but you requested " << polstring);
            } else {
                int nstoke = itsInputCubePtr->shape()[itsStkAxis];
                ASKAPCHECK(nstoke == nStokesRequest, "Extraction: input cube " << image <<
                           " has " << nstoke << " polarisations, whereas you requested " <<
                           nStokesRequest);
                bool haveMatch = false;
                for (int i = 0; i < nstoke; i++) {
                    haveMatch = haveMatch || (stokeCoo.stokes()[i] == stokes);
                }
                ASKAPCHECK(haveMatch, "Extraction: input cube " << image <<
                           " does not have requested polarisation " << polstring);
            }
        }
        this->closeInput();
    } else ASKAPLOG_ERROR_STR(logger, "Could not open image");
}

void SourceDataExtractor::verifyInputs()
{
    std::vector<std::string>::iterator im;
    std::vector<std::string> pollist = scimath::PolConverter::toString(itsStokesList);
    ASKAPCHECK(itsInputCubeList.size() > 0,
               "Extraction: You have not provided a spectralCube input");
    ASKAPCHECK(itsStokesList.size() > 0,
               "Extraction: You have not provided a list of Stokes parameters " <<
               "(input parameter \"polarisation\")");

    if (itsInputCubeList.size() > 1) { // multiple input cubes provided
        ASKAPCHECK(itsInputCubeList.size() == itsStokesList.size(),
                   "Extraction: Sizes of spectral cube and polarisation lists do not match");

        int ct = 0;
        for (im = itsInputCubeList.begin(); im < itsInputCubeList.end(); im++, ct++) {
            this->checkPol(*im, itsStokesList[ct], 1);
        }

        // check they are all the same shape
        casa::IPosition refShape = this->getShape(itsInputCubeList[0]);
        for (size_t i = 1; i < itsInputCubeList.size(); i++) {
            ASKAPCHECK(refShape == this->getShape(itsInputCubeList[i]),
                       "Extraction: shapes of " << itsInputCubeList[0] <<
                       " and " << itsInputCubeList[i] << " do not match");
        }
    } else {
        // only have a single input cube

        if (itsStokesList.size() == 1) {
            // only single Stokes parameter requested -- check if it matches the image
            this->checkPol(itsInputCubeList[0], itsStokesList[0], 1);
        } else {
            // multiple Stokes parameters requested
            if (itsInputCubeList[0].find("%p") != std::string::npos) {
                // the filename has a "%p" string, meaning
                // polarisation substitution is possible
                std::string input = itsInputCubeList[0];
                itsInputCubeList = std::vector<std::string>(itsStokesList.size());
                casa::Stokes stokes;
                for (size_t i = 0; i < itsStokesList.size(); i++) {
                    casa::String stokesname(stokes.name(itsStokesList[i]));
                    stokesname.downcase();
                    ASKAPLOG_DEBUG_STR(logger, "Input cube name: replacing \"%p\" with " <<
                                       stokesname.c_str() << " in " << input);
                    itsInputCubeList[i] = input;
                    itsInputCubeList[i].replace(input.find("%p"), 2, stokesname.c_str());
                    this->checkPol(itsInputCubeList[i], itsStokesList[i], 1);
                }
            } else {
                // get list of polarisations in that one image - are
                // all the requested ones there?
                ASKAPCHECK(itsInputCubeList.size() == 1,
                           "Extraction: For multiple polarisations, either use %p " <<
                           "substitution or provide a single image cube.");
                for (size_t i = 0; i < itsStokesList.size(); i++) {
                    this->checkPol(itsInputCubeList[0],
                                   itsStokesList[i],
                                   itsStokesList.size());
                }
                // else{
                //   std::string polset="[";
                //   std::vector<std::string> pols=this->polarisations();
                //   for(size_t i=0;i<pols.size();i++) polset+=pols[i]+(i!=pols.size()-1?",":"");
                //   polset+="]";
                //   ASKAPTHROW(AskapError, "Extraction: You have provided more than one stokes parameter ("<<polset<<"\") but only one input cube that doesn't contain all of these");
                // }
            }
        }
    }
}


void SourceDataExtractor::writeBeam(std::string &filename)
{
    casa::Vector<Quantum<Double> >
    inputBeam = itsInputCubePtr->imageInfo().restoringBeam().toVector();

    if (inputBeam.size() > 0) {
        accessors::CasaImageAccess ia;
        ia.setBeamInfo(filename,
                       inputBeam[0].getValue("rad"),
                       inputBeam[1].getValue("rad"),
                       inputBeam[2].getValue("rad"));
    } else {
        ASKAPLOG_WARN_STR(logger,
                          "Input cube has no restoring beam, so cannot write to output image.");
    }
}


bool SourceDataExtractor::openInput()
{
    bool isOK = (itsInputCube != "");

    if (isOK) {
        itsInputCubePtr.reset();
        itsInputCubePtr = analysisutilities::openImage(itsInputCube);
        isOK = (itsInputCubePtr.get() != 0); // make sure it worked.
        if (isOK) {
            itsInputCoords = itsInputCubePtr->coordinates();
            itsLngAxis = itsInputCoords.directionAxesNumbers()[0];
            itsLatAxis = itsInputCoords.directionAxesNumbers()[1];
            itsSpcAxis = itsInputCoords.spectralAxisNumber();
            itsStkAxis = itsInputCoords.polarizationAxisNumber();
        }
    }
    return isOK;
}

void SourceDataExtractor::closeInput()
{
    itsInputCubePtr.reset();

}

}

}

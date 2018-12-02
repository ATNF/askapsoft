/// @file
///
/// Base class for handling extraction of image data corresponding to a source
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
#include <extraction/SourceDataExtractor.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <string>
#include <sourcefitting/RadioSource.h>
#include <casainterface/CasaInterface.h>
#include <imageaccess/ImageAccessFactory.h>
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
#include <casacore/casa/Quanta/Unit.h>
#include <Common/ParameterSet.h>
#include <casacore/measures/Measures/Stokes.h>
#include <boost/shared_ptr.hpp>

#include <utils/PolConverter.h>

ASKAP_LOGGER(logger, ".sourcedataextractor");

namespace askap {

namespace analysis {

SourceDataExtractor::SourceDataExtractor(const LOFAR::ParameterSet& parset):
    itsParset(parset),
    itsSource(0),
    itsComponent(0),
    itsObjID(""),
    itsObjectName("")
{
    itsInputCube = ""; // start off with this blank. Needs to be
    // set before calling openInput()
    itsInputCubeList = parset.getStringVector("spectralCube",
                       std::vector<std::string>(0));

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

    openInput();
    itsOutputUnits = itsInputUnits;

}

SourceDataExtractor::~SourceDataExtractor()
{
    itsInputCubePtr.reset();
}

casa::Vector<Quantum<Double> > SourceDataExtractor::inputBeam()
{
    casa::Vector<Quantum<Double> > inputBeam(3, 0.);
    if (this->openInput()) {
        inputBeam = itsInputCubePtr->imageInfo().restoringBeam().toVector();
    }
    return inputBeam;
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
    ss << itsOutputFilenameBase;
    if (itsObjID==""){
        ss << "_" << itsSourceID;
    } else {
        ss << "_" << itsObjID;
    }
    itsOutputFilename = ss.str();
    ASKAPLOG_DEBUG_STR(logger, "SourceDataExtractor for source " << itsOutputFilename);
    casa::DirectionCoordinate dc = itsInputCoords.directionCoordinate();
    casa::Vector<casa::Double> pix(2);
    MDirection refDir(casa::Quantity(getRA(*src), "deg"),
                      casa::Quantity(getDec(*src), "deg"),
                      dc.directionType());
    dc.toPixel(pix, refDir);
    ASKAPLOG_DEBUG_STR(logger, "Converting to pixel coords: refDir="<<refDir<<", pix="<<pix);
    ASKAPLOG_DEBUG_STR(logger, "Direction coordinate ref: " << dc.referenceValue() << " at " << dc.referencePixel());
    ASKAPLOG_DEBUG_STR(logger, "Direction coordinate inc: " << dc.increment());
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
    itsComponent = src;

    setSourceLoc(src);
}
//---------------



bool SourceDataExtractor::checkPol(std::string image,
                                   casa::Stokes::StokesTypes stokes)
{

    itsInputCube = image;
    std::vector<casa::Stokes::StokesTypes> stokesvec(1, stokes);
    std::string polstring = scimath::PolConverter::toString(stokesvec)[0];

    bool haveMatch = false;
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
                for (int i = 0; i < nstoke && !haveMatch; i++) {
                    haveMatch = haveMatch || (stokeCoo.stokes()[i] == stokes);
                }
            }
        } else {
            ASKAPLOG_WARN_STR(logger, "Input cube has no Stokes axis - assuming it is Stokes I");
            // No Stokes axis - assume it is Stokes I
            haveMatch = (stokes == casa::Stokes::I);
        }
        this->closeInput();
    } else ASKAPLOG_ERROR_STR(logger, "Could not open image");
    return haveMatch;
}

void SourceDataExtractor::verifyInputs()
{
    std::vector<std::string>::iterator im;
    std::vector<std::string> pollist = scimath::PolConverter::toString(itsStokesList);
    casa::Stokes stokes;
    ASKAPCHECK(itsInputCubeList.size() > 0,
               "Extraction: You have not provided a spectralCube input");
    ASKAPCHECK(itsStokesList.size() > 0,
               "Extraction: You have not provided a list of Stokes parameters " <<
               "(input parameter \"polarisation\")");

    if (itsInputCubeList.size() > 1) { // multiple input cubes provided

        // check they are all the same shape
        casa::IPosition refShape = this->getShape(itsInputCubeList[0]);
        for (size_t i = 1; i < itsInputCubeList.size(); i++) {
            ASKAPCHECK(refShape == this->getShape(itsInputCubeList[i]),
                       "Extraction: shapes of " << itsInputCubeList[0] <<
                       " and " << itsInputCubeList[i] << " do not match");
        }

        for (im = itsInputCubeList.begin(); im < itsInputCubeList.end(); im++) {
            for (size_t i = 0; i < itsStokesList.size(); i++) {
                if (checkPol(*im, itsStokesList[i])) {
                    ASKAPLOG_DEBUG_STR(logger, "Stokes " << stokes.name(itsStokesList[i]) << " has image " << *im);
                    itsCubeStokesMap.insert(
                        std::pair<casa::Stokes::StokesTypes, std::string>(itsStokesList[i], *im));
                }
            }
        }

    } else {
        // only have a single input cube

        if (itsInputCubeList[0].find("%p") != std::string::npos) {
            // the filename has a "%p" string, meaning
            // polarisation substitution is possible
            for (size_t i = 0; i < itsStokesList.size(); i++) {
                casa::String stokesname(stokes.name(itsStokesList[i]));
                stokesname.downcase();
                std::string input = itsInputCubeList[0];
                ASKAPLOG_DEBUG_STR(logger, "Input cube name: replacing \"%p\" with " <<
                                   stokesname.c_str() << " in " << input);
                input.replace(input.find("%p"), 2, stokesname.c_str());
                if (checkPol(input, itsStokesList[i])) {
                    ASKAPLOG_DEBUG_STR(logger, "Stokes " << stokes.name(itsStokesList[i]) << " has image " << input);
                    itsCubeStokesMap.insert(
                        std::pair<casa::Stokes::StokesTypes, std::string>(itsStokesList[i], input));
                }
            }
        } else {
            // We aren't using the %p wildcard - does its polarisation match one of the ones provided?
            bool hasMatch = false;
            for (size_t i = 0; i < itsStokesList.size(); i++) {
                hasMatch = checkPol(itsInputCubeList[0], itsStokesList[i]);
                if (hasMatch) {
                    ASKAPLOG_DEBUG_STR(logger, "Stokes " << stokes.name(itsStokesList[i]) << " has image " << itsInputCubeList[0]);
                    itsCubeStokesMap.insert(
                        std::pair<casa::Stokes::StokesTypes, std::string>(itsStokesList[i],
                                itsInputCubeList[0]));
                }
            }
            ASKAPCHECK(hasMatch,
                       "Image " << itsInputCubeList[0] << " does not match any requested Stokes");
        }
    }
    ASKAPLOG_DEBUG_STR(logger, "CubeStokesMap: " << itsCubeStokesMap);

}


void SourceDataExtractor::writeBeam(std::string &filename)
{
    casa::Vector<Quantum<Double> >
    inputBeam = itsInputCubePtr->imageInfo().restoringBeam().toVector();

    if (inputBeam.size() > 0) {
        boost::shared_ptr<accessors::IImageAccess> ia = accessors::imageAccessFactory(itsParset);
        ia->setBeamInfo(filename,
                        inputBeam[0].getValue("rad"),
                        inputBeam[1].getValue("rad"),
                        inputBeam[2].getValue("rad"));
    } else {
        ASKAPLOG_WARN_STR(logger,
                          "Input cube has no restoring beam, so cannot write to output image.");
    }
}

casa::Unit SourceDataExtractor::bunit()
{
    casa::Unit bunits;
    if (openInput()) {
        bunits = itsInputCubePtr->units();
        closeInput();
    }
    return bunits;
}


bool SourceDataExtractor::openInput()
{
    bool isOK = (itsInputCube != "");
    if (!isOK) {
        ASKAPLOG_ERROR_STR(logger, "Image name is empty - cannot open!");
    } else {
        itsInputCubePtr.reset();
        itsInputCubePtr = analysisutilities::openImage(itsInputCube);
        isOK = (itsInputCubePtr.get() != 0); // make sure it worked.
        if (isOK) {
            itsInputCoords = itsInputCubePtr->coordinates();
            itsLngAxis = itsInputCoords.directionAxesNumbers()[0];
            itsLatAxis = itsInputCoords.directionAxesNumbers()[1];
            itsSpcAxis = itsInputCoords.spectralAxisNumber();
            itsStkAxis = itsInputCoords.polarizationAxisNumber();
            itsInputUnits = itsInputCubePtr->units();
        }
    }
    return isOK;
}

void SourceDataExtractor::setObjectIDs(const std::string &objid, const std::string &objectname)
{
    itsObjID = objid;
    itsObjectName = objectname;
}
    

void SourceDataExtractor::updateHeaders(const std::string &filename)
{

    boost::shared_ptr<accessors::IImageAccess> ia = accessors::imageAccessFactory(itsParset);

    // set the object ID and object name keywords    
    if (itsObjID != ""){
        ia->setMetadataKeyword(filename, "OBJID", itsObjID, "Object ID");
    }
    if (itsObjectName != "" ){
        ia->setMetadataKeyword(filename, "OBJECT", itsObjectName, "IAU-format Object Name");
    }

    std::string infile = itsInputCube;
    LOFAR::ParameterSet inputImageParset;
    // Need to remove any ".fits" extension, as this will be added by the accessor
    if (infile.find(".fits") != std::string::npos) {
        if (infile.substr(infile.rfind("."), std::string::npos) == ".fits") {
            infile.erase(infile.rfind("."), std::string::npos);
        }
        inputImageParset.add("imagetype", "fits");
    } else {
        inputImageParset.add("imagetype", "casa");
    }
    boost::shared_ptr<accessors::IImageAccess> iaInput = accessors::imageAccessFactory(inputImageParset);

    
    std::string value;
    // set the other required keywords by copying from input file
    value = iaInput->getMetadataKeyword(infile, "DATE-OBS");
    if (value != ""){
        ia->setMetadataKeyword(filename, "DATE-OBS", value, "Date of observation");
    }
    value = iaInput->getMetadataKeyword(infile, "DURATION");
    if (value != ""){
        ia->setMetadataKeyword(filename, "DURATION", value, "Length of observation");
    }
    value = iaInput->getMetadataKeyword(infile, "PROJECT");
    if (value != ""){
        ia->setMetadataKeyword(filename, "PROJECT", value, "Project ID");
    }
    value = iaInput->getMetadataKeyword(infile, "SBID");
    if (value != ""){
        ia->setMetadataKeyword(filename, "SBID", value, "Scheduling block ID");
    }    

    if (itsParset.isDefined("imageHistory")) {
        std::vector<std::string> historyMessages = itsParset.getStringVector("imageHistory", "");
        if (historyMessages.size() > 0) {
            for (std::vector<std::string>::iterator history = historyMessages.begin();
                 history < historyMessages.end(); history++) {
                ASKAPLOG_DEBUG_STR(logger, "Writing history string to " << filename <<": " << *history);
                ia->addHistory(filename, *history);
            }
        }
    }
    
}


void SourceDataExtractor::closeInput()
{
    itsInputCubePtr.reset();

}

}

}

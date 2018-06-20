/// @file
///
/// Simple class to write out the Faraday Dispersion Function calculated elsewhere
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
#include <polarisation/FDFwriter.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <polarisation/PolarisationData.h>
#include <polarisation/RMSynthesis.h>
#include <casainterface/CasaInterface.h>
#include <imageaccess/ImageAccessFactory.h>

#include <complex>
#include <Common/ParameterSet.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/LinearCoordinate.h>
#include <casacore/images/Images/PagedImage.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".fdfwriter");

namespace askap {

namespace analysis {

FDFwriter::FDFwriter(LOFAR::ParameterSet &parset,
                     PolarisationData &poldata,
                     RMSynthesis &rmsynth):
    itsParset(parset),
    itsObjID(""),
    itsObjectName(""),
    itsInputCube(poldata.I().cubeName()),
    itsFlagWriteAsComplex(parset.getBool("writeComplexFDF", true)),
    itsOutputBase(parset.getString("outputBase", "")),
    itsSourceID(poldata.I().specExtractor()->sourceID())

{
    // Object ID & name
    itsObjID = itsParset.getString("objid", "");
    itsObjectName = itsParset.getString("objectname", "");

    // Set up coordinate systems
    const boost::shared_ptr<casa::ImageInterface<Float> > inputCubePtr =
        askap::analysisutilities::openImage(itsInputCube);
    const casa::CoordinateSystem inputcoords = inputCubePtr->coordinates();

    const int dirCoNum = inputcoords.findCoordinate(casa::Coordinate::DIRECTION);
    casa::DirectionCoordinate dircoo(inputcoords.directionCoordinate(dirCoNum));

    itsCoordSysForFDF.addCoordinate(dircoo);
    itsCoordSysForRMSF.addCoordinate(dircoo);

    // shift the origin of the direction axes to the component position
    casa::Vector<Float> shift(2, 0);
    casa::Vector<Float> incrFrac(2, 1);
    shift(itsCoordSysForFDF.directionAxesNumbers()[0]) = poldata.I().specExtractor()->srcXloc();
    shift(itsCoordSysForFDF.directionAxesNumbers()[1]) = poldata.I().specExtractor()->srcYloc();
    casa::IPosition dirshape(2, 1);
    itsCoordSysForFDF.subImageInSitu(shift, incrFrac, dirshape.asVector());
    itsCoordSysForRMSF.subImageInSitu(shift, incrFrac, dirshape.asVector());

    // Define the linear coordinate for the Faraday depth axis.
    // First for the FDF
    casa::Vector<Double> crpix(1); crpix = 0.0;
    casa::Vector<Double> crval(1); crval = rmsynth.phi()[0];
    casa::Vector<Double> cdelt(1); cdelt = rmsynth.deltaPhi();
    casa::Matrix<Double> pc(1, 1); pc = 0; pc.diagonal() = 1.0;
    casa::Vector<String> name(1);  name = "Faraday depth";
    casa::Vector<String> units(1); units = "rad/m2";
    casacore::LinearCoordinate FDcoordFDF(name, units, crval, cdelt, pc, crpix);
    itsCoordSysForFDF.addCoordinate(FDcoordFDF);

    // Then for the RMSF - this should be twice the length, so only the crval changes
    crval = rmsynth.phi_rmsf()[0];
    casacore::LinearCoordinate FDcoordRMSF(name, units, crval, cdelt, pc, crpix);
    itsCoordSysForRMSF.addCoordinate(FDcoordRMSF);

    // Define the shapes of the output images, and reform the arrays
    casa::IPosition fdfShape(3, 1);
    fdfShape[itsCoordSysForFDF.linearAxesNumbers()[0]] = rmsynth.fdf().size();
    itsFDF = rmsynth.fdf().reform(fdfShape);

    casa::IPosition rmsfShape(3, 1);
    rmsfShape[itsCoordSysForRMSF.linearAxesNumbers()[0]] = rmsynth.rmsf().size();
    itsRMSF = rmsynth.rmsf().reform(rmsfShape);

    // Base name for the output files

}

void FDFwriter::write()
{
    std::string idstring;
    if (itsObjID == "") {
        idstring = itsSourceID;
    } else {
        idstring = itsObjID;
    }

    std::stringstream ss;
    if ((itsFlagWriteAsComplex) && (itsParset.getString("imagetype", "fits") == "casa")) {
        // write a single file for each, holding a complex array
        //   NOTE - CAN ONLY DO THIS FOR CASA-FORMAT OUTPUT
        ss.str("");
        ss << itsOutputBase << "_FDF_" << idstring;
        std::string fdfName = ss.str();
        casa::PagedImage<casa::Complex> imgF(casa::TiledShape(itsFDF.shape()), itsCoordSysForFDF, fdfName);
        imgF.put(itsFDF);

        ss.str("");
        ss << itsOutputBase << "_RMSF_" << idstring;
        std::string rmsfName = ss.str();
        casa::PagedImage<casa::Complex> imgR(casa::TiledShape(itsRMSF.shape()), itsCoordSysForRMSF, rmsfName);
        imgR.put(itsRMSF);

    } else {
        if (itsFlagWriteAsComplex) {
            ASKAPLOG_WARN_STR(logger, "Writing FDF & RMSF as separate phase & amplitude - cannot write complex data to FITS");
        }
        boost::shared_ptr<accessors::IImageAccess> imageAcc = accessors::imageAccessFactory(itsParset);
        // write separate files for the amplitude and the phase for each array
        ss.str("");
        ss << itsOutputBase << "_FDF_amp_" << idstring;
        std::string fdfName = ss.str();
        // casa::PagedImage<float> imgFa(casa::TiledShape(itsFDF.shape()), itsCoordSysForFDF, fdfName);
        // imgFa.put(casa::amplitude(itsFDF));
        imageAcc->create(fdfName, itsFDF.shape(), itsCoordSysForFDF);
        imageAcc->write(fdfName, casa::amplitude(itsFDF));
        updateHeaders(fdfName);

        ss.str("");
        ss << itsOutputBase << "_FDF_phase_" << idstring;
        fdfName = ss.str();
        // casa::PagedImage<float> imgFp(casa::TiledShape(itsFDF.shape()), itsCoordSysForFDF, fdfName);
        // imgFp.put(casa::phase(itsFDF));
        imageAcc->create(fdfName, itsFDF.shape(), itsCoordSysForFDF);
        imageAcc->write(fdfName, casa::phase(itsFDF));
        updateHeaders(fdfName);

        ss.str("");
        ss << itsOutputBase << "_RMSF_amp_" << idstring;
        std::string rmsfName = ss.str();
        // casa::PagedImage<float> imgRa(casa::TiledShape(itsRMSF.shape()), itsCoordSysForRMSF, rmsfName);
        // imgRa.put(casa::amplitude(itsRMSF));
        imageAcc->create(rmsfName, itsRMSF.shape(), itsCoordSysForRMSF);
        imageAcc->write(rmsfName, casa::amplitude(itsRMSF));
        updateHeaders(rmsfName);

        ss.str("");
        ss << itsOutputBase << "_RMSF_phase_" << idstring;
        rmsfName = ss.str();
        // casa::PagedImage<float> imgRp(casa::TiledShape(itsRMSF.shape()), itsCoordSysForRMSF, rmsfName);
        // imgRp.put(casa::phase(itsRMSF));
        imageAcc->create(rmsfName, itsRMSF.shape(), itsCoordSysForRMSF);
        imageAcc->write(rmsfName, casa::phase(itsRMSF));
        updateHeaders(rmsfName);

    }

}

void FDFwriter::updateHeaders(const std::string &filename)
{

    boost::shared_ptr<accessors::IImageAccess> ia = accessors::imageAccessFactory(itsParset);

    // set the object ID and object name keywords
    if (itsObjID != "") {
        ia->setMetadataKeyword(filename, "OBJID", itsObjID, "Object ID");
    }
    if (itsObjectName != "") {
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
    if (value != "") {
        ia->setMetadataKeyword(filename, "DATE-OBS", value, "Date of observation");
    }
    value = iaInput->getMetadataKeyword(infile, "DURATION");
    if (value != "") {
        ia->setMetadataKeyword(filename, "DURATION", value, "Length of observation");
    }
    value = iaInput->getMetadataKeyword(infile, "PROJECT");
    if (value != "") {
        ia->setMetadataKeyword(filename, "PROJECT", value, "Project ID");
    }
    value = iaInput->getMetadataKeyword(infile, "SBID");
    if (value != "") {
        ia->setMetadataKeyword(filename, "SBID", value, "Scheduling block ID");
    }

    if (itsParset.isDefined("imageHistory")) {
        std::vector<std::string> historyMessages = itsParset.getStringVector("imageHistory", "");
        if (historyMessages.size() > 0) {
            for (std::vector<std::string>::iterator history = historyMessages.begin();
                    history < historyMessages.end(); history++) {
                ASKAPLOG_DEBUG_STR(logger, "Writing history string to " << filename << ": " << *history);
                ia->addHistory(filename, *history);
            }
        }
    }

}



}

}

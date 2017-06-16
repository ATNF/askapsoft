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
    itsFlagWriteAsComplex(parset.getBool("writeComplexFDF", true)),
    itsOutputBase(parset.getString("outputBase", "")),
    itsSourceID(poldata.I().specExtractor()->sourceID())

{

    // Set up coordinate systems
    const std::string cubename = poldata.I().cubeName();
    const boost::shared_ptr<casa::ImageInterface<Float> > inputCubePtr =
        askap::analysisutilities::openImage(cubename);
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
    std::stringstream ss;
    if( (itsFlagWriteAsComplex) && (itsParset.getString("imagetype","casa") == "casa") ){
        // write a single file for each, holding a complex array
        //   NOTE - CAN ONLY DO THIS FOR CASA-FORMAT OUTPUT
        ss.str("");
        ss << itsOutputBase << "_FDF_" << itsSourceID;
        std::string fdfName = ss.str();
        casa::PagedImage<casa::Complex> imgF(casa::TiledShape(itsFDF.shape()), itsCoordSysForFDF, fdfName);
        imgF.put(itsFDF);
        
        ss.str("");
        ss << itsOutputBase << "_RMSF_" << itsSourceID;
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
        ss << itsOutputBase << "_FDF_amp_" << itsSourceID;
        std::string fdfName = ss.str();
        // casa::PagedImage<float> imgFa(casa::TiledShape(itsFDF.shape()), itsCoordSysForFDF, fdfName);
        // imgFa.put(casa::amplitude(itsFDF));
        imageAcc->create(fdfName, itsFDF.shape(), itsCoordSysForFDF);
        imageAcc->write(fdfName, casa::amplitude(itsFDF));

        ss.str("");
        ss << itsOutputBase << "_FDF_phase_" << itsSourceID;
        fdfName = ss.str();
        // casa::PagedImage<float> imgFp(casa::TiledShape(itsFDF.shape()), itsCoordSysForFDF, fdfName);
        // imgFp.put(casa::phase(itsFDF));
        imageAcc->create(fdfName, itsFDF.shape(), itsCoordSysForFDF);
        imageAcc->write(fdfName, casa::phase(itsFDF));

        ss.str("");
        ss << itsOutputBase << "_RMSF_amp_" << itsSourceID;
        std::string rmsfName = ss.str();
        // casa::PagedImage<float> imgRa(casa::TiledShape(itsRMSF.shape()), itsCoordSysForRMSF, rmsfName);
        // imgRa.put(casa::amplitude(itsRMSF));
        imageAcc->create(rmsfName, itsRMSF.shape(), itsCoordSysForRMSF);
        imageAcc->write(rmsfName, casa::amplitude(itsRMSF));
        
        ss.str("");
        ss << itsOutputBase << "_RMSF_phase_" << itsSourceID;
        rmsfName = ss.str();
        // casa::PagedImage<float> imgRp(casa::TiledShape(itsRMSF.shape()), itsCoordSysForRMSF, rmsfName);
        // imgRp.put(casa::phase(itsRMSF));
        imageAcc->create(rmsfName, itsRMSF.shape(), itsCoordSysForRMSF);
        imageAcc->write(rmsfName, casa::phase(itsRMSF));

    }

}


}

}

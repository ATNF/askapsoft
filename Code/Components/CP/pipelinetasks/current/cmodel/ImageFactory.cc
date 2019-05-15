/// @file ImageFactory.cc
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include own header file first
#include "cmodel/ImageFactory.h"

// Include package level header file
#include "askap_pipelinetasks.h"

// System includes
#include <string>

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"
#include "Common/ParameterSet.h"

// Casacore includes
#include "casacore/casa/aipstype.h"
#include "casacore/casa/Quanta.h"
#include "casacore/casa/Quanta/Quantum.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/measures/Measures/MDirection.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/casa/Arrays/IPosition.h"
#include "casacore/lattices/Lattices/TiledShape.h"
#include "casacore/coordinates/Coordinates/CoordinateSystem.h"
#include "casacore/coordinates/Coordinates/DirectionCoordinate.h"
#include "casacore/coordinates/Coordinates/SpectralCoordinate.h"
#include "casacore/coordinates/Coordinates/CoordinateUtil.h"

// Using
using namespace askap;
using namespace askap::cp::pipelinetasks;
using namespace casacore;

ASKAP_LOGGER(logger, ".ImageFactory");

casacore::TempImage<casacore::Float> ImageFactory::createTempImage(const LOFAR::ParameterSet& parset)
{
    const casacore::uInt nx = parset.getUintVector("shape").at(0);
    const casacore::uInt ny = parset.getUintVector("shape").at(1);
    const std::string units = parset.getString("bunit");

    // Create the Coordinate System
    CoordinateSystem coordsys = createCoordinateSystem(nx, ny, parset);

    // Open the image
    IPosition shape(4, nx, ny, getNumStokes(coordsys), 1);
    casacore::TempImage<casacore::Float> image(TiledShape(shape), coordsys);
    image.set(0.0);

    // Set brightness units
    image.setUnits(casacore::Unit(units));
    return image;
}

casacore::PagedImage<casacore::Float> ImageFactory::createPagedImage(const LOFAR::ParameterSet& parset,
        const std::string& filename)
{
    const casacore::uInt nx = parset.getUintVector("shape").at(0);
    const casacore::uInt ny = parset.getUintVector("shape").at(1);
    const std::string units = parset.getString("bunit");

    // Create the Coordinate System
    CoordinateSystem coordsys = createCoordinateSystem(nx, ny, parset);

    // Open the image
    IPosition shape(4, nx, ny, getNumStokes(coordsys), 1);
    casacore::PagedImage<casacore::Float> image(TiledShape(shape), coordsys, filename);
    image.set(0.0);

    // Set brightness units
    image.setUnits(casacore::Unit(units));
    return image;
}

casacore::CoordinateSystem ImageFactory::createCoordinateSystem(casacore::uInt nx, casacore::uInt ny,
        const LOFAR::ParameterSet& parset)
{
    CoordinateSystem coordsys;
    const std::vector<std::string> dirVector = parset.getStringVector("direction");
    const std::vector<std::string> cellSizeVector = parset.getStringVector("cellsize");

    // Direction Coordinate
    {
        Matrix<Double> xform(2, 2);
        xform = 0.0;
        xform.diagonal() = 1.0;
        const Quantum<Double> ra = asQuantity(dirVector.at(0), "deg");
        const Quantum<Double> dec = asQuantity(dirVector.at(1), "deg");
        ASKAPLOG_DEBUG_STR(logger, "Direction: " << ra.getValue() << " degrees, "
                << dec.getValue() << " degrees");

        const Quantum<Double> xcellsize = asQuantity(cellSizeVector.at(0), "arcsec") * -1.0;
        const Quantum<Double> ycellsize = asQuantity(cellSizeVector.at(1), "arcsec");
        ASKAPLOG_DEBUG_STR(logger, "Cellsize: " << xcellsize.getValue()
                << " arcsec, " << ycellsize.getValue() << " arcsec");

        casacore::MDirection::Types type;
        casacore::MDirection::getType(type, dirVector.at(2));
        const DirectionCoordinate radec(type, Projection(Projection::SIN),
                                        ra, dec, xcellsize, ycellsize, xform, nx / 2, ny / 2);

        coordsys.addCoordinate(radec);
    }

    // Stokes Coordinate
    {
        Vector<Int> stokes;
        if (parset.isDefined("stokes")) {
            stokes = parseStokes(parset.getStringVector("stokes"));
        } else {
            stokes.resize(1);
            stokes(0) = Stokes::I;
        }

        const StokesCoordinate stokescoord(stokes);
        coordsys.addCoordinate(stokescoord);
    }

    // Spectral Coordinate
    {
        const Quantum<Double> f0 = asQuantity(parset.getString("frequency"), "Hz");
        const Quantum<Double> inc = asQuantity(parset.getString("increment"), "Hz");
        const Double refPix = 0.0;  // is the reference pixel
        const SpectralCoordinate sc(MFrequency::TOPO, f0, inc, refPix);

        coordsys.addCoordinate(sc);
    }

    return coordsys;
}

Vector<casacore::Int> ImageFactory::parseStokes(const std::vector<std::string>& input)
{
    const size_t size = input.size();
    Vector<Int> stokes(size);

    for (size_t i = 0; i < size; ++i) {
        if (input[i].compare("I") == 0) {
            stokes(i) = Stokes::I;
        } else if (input[i].compare("Q") == 0) {
            stokes(i) = Stokes::Q;
        } else if (input[i].compare("U") == 0) {
            stokes(i) = Stokes::U;
        } else if (input[i].compare("V") == 0) {
            stokes(i) = Stokes::V;
        } else {
            ASKAPTHROW(AskapError, "Unknown stokes parameter in parset");
        }
    }

    return stokes;
}

casacore::uInt ImageFactory::getNumStokes(const casacore::CoordinateSystem& coordsys)
{
    Vector<Stokes::StokesTypes> stokes;
    CoordinateUtil::findStokesAxis(stokes, coordsys);
    return stokes.size();
}

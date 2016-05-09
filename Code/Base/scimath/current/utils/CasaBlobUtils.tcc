/// @file CasaBlobUtils.tcc
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
/// @author Daniel Mitchell <daniel.mitchell@csiro.au>
///

#ifndef SCIMATH_CASABLOBUTILS_H
#error "Don't include this file directly, include CasaBlobUtils.h instead"
#endif

// ASKAPsoft includes
#include <casacore/coordinates/Coordinates/CoordinateSystem.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/SpectralCoordinate.h>
#include <casacore/coordinates/Coordinates/StokesCoordinate.h>

#include <Blob/BlobSTL.h>

/// @brief increment this if there is any change to the stuff written into blob
const int coordSysBlobVersion = 2;

using namespace askap;

namespace LOFAR
{

    /// @brief blob support for casa::CoordinateSystem
    LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream& os,
                                   const casa::CoordinateSystem& cSys)
    {
        os.putStart("CoordinateSystem",coordSysBlobVersion);

        int nCoordinates = cSys.nCoordinates();
        int coordinateCount = 0;
        int dcPos = cSys.findCoordinate(casa::Coordinate::DIRECTION,-1);
        int fcPos = cSys.findCoordinate(casa::Coordinate::SPECTRAL,-1);
        int pcPos = cSys.findCoordinate(casa::Coordinate::STOKES,-1);

        os << nCoordinates;
        os << dcPos << fcPos << pcPos;
        if (dcPos >= 0) {
          const casa::DirectionCoordinate& dc = cSys.directionCoordinate(dcPos);
          os << casa::MDirection::showType(dc.directionType()) <<
                dc.projection().name() << dc.referenceValue() <<
                dc.increment() << dc.linearTransform() <<
                dc.referencePixel() << dc.worldAxisUnits();
          coordinateCount++;
        }
        if (fcPos >= 0) {
          const casa::SpectralCoordinate& fc = cSys.spectralCoordinate(fcPos);
          os << casa::MFrequency::showType(fc.frequencySystem()) <<
                fc.referenceValue() << fc.increment() << fc.referencePixel() <<
                fc.restFrequency() << fc.worldAxisUnits();
          coordinateCount++;
        }
        if (pcPos >= 0) {
          const casa::StokesCoordinate& pc = cSys.stokesCoordinate(pcPos);
          os << pc.stokes();
          coordinateCount++;
        }

        ASKAPCHECK(coordinateCount == nCoordinates,
            "BlobOStream currently only supports DIRECTION, SPECTRAL and STOKES coordinates");

        os.putEnd();
        return os;
    }

    /// @brief blob support for casa::CoordinateSystem
    LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream& is,
                                   casa::CoordinateSystem& cSys)
    {
        const int version = is.getStart("CoordinateSystem");
        ASKAPCHECK(version == coordSysBlobVersion, "Attempting to read from" <<
            " a blob stream a CoordinateSystem object of the wrong version," <<
            " expect "<< coordSysBlobVersion<<" got "<<version);

        int nCoordinates;
        is >> nCoordinates;
        int dcPos, fcPos, pcPos;
        is >> dcPos >> fcPos >> pcPos;

        cSys = casa::CoordinateSystem();

        if (dcPos >= 0) {
          casa::String dirTypeStr;
          is >> dirTypeStr;
          casa::MDirection::Types dirType;
          casa::MDirection::getType(dirType, dirTypeStr);
          casa::String projectionName;
          is >> projectionName;
          casa::Vector<casa::Double> increment, refPix, refVal;
          casa::Matrix<casa::Double> xform;
          is >> refVal >> increment >> xform >> refPix;
          ASKAPCHECK(increment.nelements() == 2,
              "Direction axis increment should be a vector of size 2");
          ASKAPCHECK(refPix.nelements() == 2,
              "Direction axis reference pixel should be a vector of size 2");
          ASKAPCHECK(refVal.nelements() == 2,
              "Direction axis reference value should be a vector of size 2");
          ASKAPCHECK(xform.shape() == casa::IPosition(2,2,2),
              "Direction axis transform matrix should be 2x2");
          casa::DirectionCoordinate dc(dirType,
              casa::Projection::type(projectionName), refVal[0],refVal[1],
              increment[0],increment[1], xform, refPix[0],refPix[1]);
          casa::Vector<casa::String> worldAxisUnits;
          is >> worldAxisUnits;
          dc.setWorldAxisUnits(worldAxisUnits);
          cSys.addCoordinate(dc);
        }
        if (fcPos >= 0) {
          casa::String freqTypeStr;
          is >> freqTypeStr;
          casa::MFrequency::Types freqType;
          casa::MFrequency::getType(freqType, freqTypeStr);
          casa::Double restFreq;
          casa::Vector<casa::Double> increment, refPix, refVal;
          is >> refVal >> increment >> refPix >> restFreq;
          ASKAPCHECK(increment.nelements() == 1,
              "Spectral axis increment should be a vector of size 1");
          ASKAPCHECK(refPix.nelements() == 1,
              "Spectral axis reference pixel should be a vector of size 1");
          ASKAPCHECK(refVal.nelements() == 1,
              "Spectral axis reference value should be a vector of size 1");
          casa::SpectralCoordinate fc(freqType, refVal[0], increment[0],
              refPix[0], restFreq);
          casa::Vector<casa::String> worldAxisUnits;
          is >> worldAxisUnits;
          fc.setWorldAxisUnits(worldAxisUnits);
          cSys.addCoordinate(fc);
        }
        if (pcPos >= 0) {
          casa::Vector<casa::Int> whichStokes;
          is >> whichStokes;
          casa::StokesCoordinate pc(whichStokes);
          cSys.addCoordinate(pc);
        }

        is.getEnd();
        return is;
    }

} // End namespace LOFAR

/// @file VisChunk.cc
///
/// @copyright (c) 2010-2014 CSIRO
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
///

// Include own header file first
#include "VisChunk.h"

// ASKAPsoft includes
#include "askap/AskapError.h"
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/casa/Quanta/MVDirection.h"
#include "casacore/casa/Arrays/Array.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Cube.h"
#include "casacore/scimath/Mathematics/RigidVector.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/measures/Measures/MDirection.h"

// Using
using namespace askap::cp::common;

VisChunk::VisChunk(const casa::uInt nRow,
                   const casa::uInt nChannel,
                   const casa::uInt nPol,
                   const casa::uInt nAntenna)
        : itsNumberOfRows(nRow),
        itsNumberOfChannels(nChannel),
        itsNumberOfPolarisations(nPol),
        itsNumberOfAntennas(nAntenna),
        itsTime(-1),
        itsInterval(-1),
        itsAntenna1(nRow),
        itsAntenna2(nRow),
        itsBeam1(nRow),
        itsBeam2(nRow),
        itsBeam1PA(nRow),
        itsBeam2PA(nRow),
        itsPhaseCentre(nRow),
        itsTargetPointingCentre(nAntenna),
        itsActualPointingCentre(nAntenna),
        itsActualPolAngle(nAntenna),
        itsActualAzimuth(nAntenna),
        itsActualElevation(nAntenna),
        itsOnSourceFlag(nAntenna),
        itsVisibility(nRow, nChannel, nPol),
        itsFlag(nRow, nChannel, nPol),
        itsUVW(nRow),
        itsFrequency(nChannel),
        itsChannelWidth(-1),
        itsStokes(nPol),
        itsDirectionFrame(casa::MDirection::DEFAULT)
{
}

/// @brief copy constructor
/// @details
/// @param[in] src instance to copy from
VisChunk::VisChunk(const VisChunk &src) : itsNumberOfRows(src.itsNumberOfRows), 
    itsNumberOfChannels(src.itsNumberOfChannels), itsNumberOfPolarisations(src.itsNumberOfPolarisations),
    itsNumberOfAntennas(src.itsNumberOfAntennas), itsTime(src.itsTime), itsTargetName(src.itsTargetName),
    itsInterval(src.itsInterval), itsScan(src.itsScan), itsAntenna1(src.itsAntenna1.copy()), 
    itsAntenna2(src.itsAntenna2.copy()), itsBeam1(src.itsBeam1.copy()), itsBeam2(src.itsBeam2.copy()),
    itsBeam1PA(src.itsBeam1PA.copy()), itsBeam2PA(src.itsBeam2PA.copy()), itsPhaseCentre(src.itsPhaseCentre.copy()),
    itsTargetPointingCentre(src.itsTargetPointingCentre.copy()), itsActualPointingCentre(src.itsActualPointingCentre.copy()),
    itsActualPolAngle(src.itsActualPolAngle.copy()),
    itsActualAzimuth(src.itsActualAzimuth.copy()), itsActualElevation(src.itsActualElevation.copy()), 
    itsOnSourceFlag(src.itsOnSourceFlag.copy()), itsVisibility(src.itsVisibility.copy()), itsFlag(src.itsFlag.copy()),
    itsUVW(src.itsUVW.copy()), itsFrequency(src.itsFrequency.copy()), itsChannelWidth(src.itsChannelWidth), 
    itsStokes(src.itsStokes.copy()), itsDirectionFrame(src.itsDirectionFrame), itsBeamOffsets(src.itsBeamOffsets.copy())
{
}

/// @brief assignment operator
/// @details It is not supposed to be used, but added to avoid creation of
/// implicit operator by the compiler.
const VisChunk& VisChunk::operator=(const VisChunk &)
{
   ASKAPTHROW(AskapError, "Assignment operator is not supposed to be used for VisChunk!");
   return *this;
}

casa::uInt VisChunk::nRow() const
{
    return itsNumberOfRows;
}

casa::uInt VisChunk::nChannel() const
{
    return itsNumberOfChannels;
}

casa::uInt VisChunk::nPol() const
{
    return itsNumberOfPolarisations;
}

casa::uInt VisChunk::nAntenna() const
{
    return itsNumberOfAntennas;
}

casa::uInt& VisChunk::scan()
{
    return itsScan;
}

const casa::uInt& VisChunk::scan() const
{
    return itsScan;
}

casa::Vector<casa::uInt>& VisChunk::antenna1()
{
    return itsAntenna1;
}

const casa::Vector<casa::uInt>& VisChunk::antenna1() const
{
    return itsAntenna1;
}

casa::Vector<casa::uInt>& VisChunk::antenna2()
{
    return itsAntenna2;
}

const casa::Vector<casa::uInt>& VisChunk::antenna2() const
{
    return itsAntenna2;
}

casa::Vector<casa::uInt>& VisChunk::beam1()
{
    return itsBeam1;
}

const casa::Vector<casa::uInt>& VisChunk::beam1() const
{
    return itsBeam1;
}

casa::Vector<casa::uInt>& VisChunk::beam2()
{
    return itsBeam2;
}

const casa::Vector<casa::uInt>& VisChunk::beam2() const
{
    return itsBeam2;
}

casa::Vector<casa::Float>& VisChunk::beam1PA()
{
    return itsBeam1PA;
}

const casa::Vector<casa::Float>& VisChunk::beam1PA() const
{
    return itsBeam1PA;
}

casa::Vector<casa::Float>& VisChunk::beam2PA()
{
    return itsBeam2PA;
}

const casa::Vector<casa::Float>& VisChunk::beam2PA() const
{
    return itsBeam2PA;
}

casa::Vector<casa::MVDirection>& VisChunk::phaseCentre()
{
    return itsPhaseCentre;
}

const casa::Vector<casa::MVDirection>& VisChunk::phaseCentre() const
{
    return itsPhaseCentre;
}

casa::Vector<casa::MDirection>& VisChunk::targetPointingCentre()
{
    return itsTargetPointingCentre;
}

const casa::Vector<casa::MDirection>& VisChunk::targetPointingCentre() const
{
    return itsTargetPointingCentre;
}

casa::Vector<casa::MDirection>& VisChunk::actualPointingCentre()
{
    return itsActualPointingCentre;
}

const casa::Vector<casa::MDirection>& VisChunk::actualPointingCentre() const
{
    return itsActualPointingCentre;
}

casa::Vector<casa::Quantity>& VisChunk::actualPolAngle()
{
    return itsActualPolAngle;
}

const casa::Vector<casa::Quantity>& VisChunk::actualPolAngle() const
{
    return itsActualPolAngle;
}

casa::Vector<casa::Quantity>& VisChunk::actualAzimuth()
{
    return itsActualAzimuth;
}

const casa::Vector<casa::Quantity>& VisChunk::actualAzimuth() const
{
    return itsActualAzimuth;
}

casa::Vector<casa::Quantity>& VisChunk::actualElevation()
{
    return itsActualElevation;
}

const casa::Vector<casa::Quantity>& VisChunk::actualElevation() const
{
    return itsActualElevation;
}

casa::Vector<bool>& VisChunk::onSourceFlag()
{
    return itsOnSourceFlag;
}

const casa::Vector<bool>& VisChunk::onSourceFlag() const
{
    return itsOnSourceFlag;
}

casa::Cube<casa::Complex>& VisChunk::visibility()
{
    return itsVisibility;
}

const casa::Cube<casa::Complex>& VisChunk::visibility() const
{
    return itsVisibility;
}

casa::Cube<casa::Bool>& VisChunk::flag()
{
    return itsFlag;
}

const casa::Cube<casa::Bool>& VisChunk::flag() const
{
    return itsFlag;
}

casa::Vector<casa::RigidVector<casa::Double, 3> >& VisChunk::uvw()
{
    return itsUVW;
}

const casa::Vector<casa::RigidVector<casa::Double, 3> >& VisChunk::uvw() const
{
    return itsUVW;
}

casa::MVEpoch& VisChunk::time()
{
    return itsTime;
}

const casa::MVEpoch& VisChunk::time() const
{
    return itsTime;
}

std::string& VisChunk::targetName()
{
    return itsTargetName;
}

const std::string& VisChunk::targetName() const
{
    return itsTargetName;
}

casa::Double& VisChunk::interval()
{
    return itsInterval;
}

const casa::Double& VisChunk::interval() const
{
    return itsInterval;
}

casa::Vector<casa::Double>& VisChunk::frequency()
{
    return itsFrequency;
}

const casa::Vector<casa::Double>& VisChunk::frequency() const
{
    return itsFrequency;
}

casa::Double& VisChunk::channelWidth()
{
    return itsChannelWidth;
}

const casa::Double& VisChunk::channelWidth() const
{
    return itsChannelWidth;
}

casa::Vector<casa::Stokes::StokesTypes>& VisChunk::stokes()
{
    return itsStokes;
}

const casa::Vector<casa::Stokes::StokesTypes>& VisChunk::stokes() const
{
    return itsStokes;
}

casa::MDirection::Ref& VisChunk::directionFrame()
{
    return itsDirectionFrame;
}

const casa::MDirection::Ref& VisChunk::directionFrame() const
{
    return itsDirectionFrame;
}

/// @brief beam offsets
/// @return a reference to matrix with beam offsets
/// @note This matrix may be uninitialised, if static beam offsets are used
/// Otherwise, the matrix is 2 x nBeam
casa::Matrix<double>& VisChunk::beamOffsets()
{
   return itsBeamOffsets;
}

/// @copydoc VisChunk::beamOffsets()
const casa::Matrix<double>& VisChunk::beamOffsets() const
{
   return itsBeamOffsets;
}

void VisChunk::resize(const casa::Cube<casa::Complex>& visibility,
        const casa::Cube<casa::Bool>& flag,
        const casa::Vector<casa::Double>& frequency)
{
    if ((visibility.nrow() != itsNumberOfRows) && (flag.nrow() != itsNumberOfRows)) {
        ASKAPTHROW(AskapError,
                "New cubes must have the same number of rows as the existing cubes");
    }

    if ((visibility.nplane() != itsNumberOfPolarisations) && (flag.nplane() != itsNumberOfPolarisations)) {
        ASKAPTHROW(AskapError,
                "New cubes must have the same number of polarisations as the existing cubes");
    }

    const casa::uInt newNChan = visibility.ncolumn();
    if (newNChan != flag.ncolumn() || newNChan != frequency.size()) {
        ASKAPTHROW(AskapError, "Number of channels must be equal for all input containers");
    }

    itsVisibility.assign(visibility);
    itsFlag.assign(flag);
    itsFrequency.assign(frequency);

    itsNumberOfChannels = newNChan;
}

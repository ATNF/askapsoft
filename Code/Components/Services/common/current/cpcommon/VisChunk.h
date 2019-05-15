/// @file VisChunk.h
///
/// @copyright (c) 2010 CSIRO
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
#ifndef ASKAP_CP_INGEST_VISCHUNK_H
#define ASKAP_CP_INGEST_VISCHUNK_H

// System includes
#include <string>

// ASKAPsoft includes
#include "casacore/casa/aips.h"
#include "casacore/casa/Quanta/MVEpoch.h"
#include "casacore/casa/Quanta/MVDirection.h"
#include "casacore/casa/Arrays/Vector.h"
#include "casacore/casa/Arrays/Matrix.h"
#include "casacore/casa/Arrays/Cube.h"
#include "casacore/scimath/Mathematics/RigidVector.h"
#include "casacore/measures/Measures/Stokes.h"
#include "casacore/measures/Measures/MDirection.h"
#include "boost/shared_ptr.hpp"

namespace askap {
namespace cp {
namespace common {

class VisChunk {
    public:
        /// @brief Constructor.
        /// Construct a VisChunk where its containers are created with
        /// the dimensions specified.
        ///
        /// @param[in] nRow containers with a nRow dimension will be created
        ///                 with this size for that dimension.
        /// @param[in] nChannel containers with a nChannel dimension will
        ///                     be created with this size for that dimension.
        /// @param[in] nPol containers with a nPol dimension will be created
        ///                 with this size for that dimension.
        VisChunk(const casacore::uInt nRow,
                 const casacore::uInt nChannel,
                 const casacore::uInt nPol,
                 const casacore::uInt nAntenna);

        /// @brief copy constructor
        /// @details
        /// @param[in] src instance to copy from
        VisChunk(const VisChunk &src);

        /// @brief assignment operator
        /// @details It is not supposed to be used, but added to avoid creation of
        /// implicit operator by the compiler.
        const VisChunk& operator=(const VisChunk &);
 


        /// The number of rows in this chunk
        /// @return the number of rows in this chunk
        casacore::uInt nRow() const;

        // The following methods implement metadata access

        /// The number of spectral channels (equal for all rows)
        /// @return the number of spectral channels
        casacore::uInt nChannel() const;

        /// The number of polarization products (equal for all rows)
        /// @return the number of polarization products (can be 1,2 or 4)
        casacore::uInt nPol() const;

        /// The number of antennas
        /// @return the number antennas.
        casacore::uInt nAntenna() const;

        /// Timestamp for this correlator integration
        /// @return a timestamp for this buffer. Absolute time expressed as
        /// seconds since MJD=0 UTC.
        casacore::MVEpoch& time();

        /// @copydoc VisChunk::time()
        const casacore::MVEpoch& time() const;

        /// Target (field/source) name
        std::string& targetName();

        /// @copydoc VisChunk::targetName()
        const std::string& targetName() const;

        /// Data sampling interval.
        /// Units: Seconds
        casacore::Double& interval();

        /// @copydoc VisChunk::interval()
        const casacore::Double& interval() const;

        /// Scan index number (zero based).
        casacore::uInt& scan();

        /// @copydoc VisChunk::scan()
        const casacore::uInt& scan() const;

        /// First antenna IDs for all rows
        ///
        /// @note Antenna ID is zero based
        ///
        /// @return a vector with IDs of the first antenna corresponding
        /// to each visibility (one for each row)
        casacore::Vector<casacore::uInt>& antenna1();

        /// @copydoc VisChunk::antenna1()
        const casacore::Vector<casacore::uInt>& antenna1() const;

        /// Second antenna IDs for all rows
        ///
        /// @note Antenna ID is zero based
        ///
        /// @return a vector with IDs of the second antenna corresponding
        /// to each visibility (one for each row)
        casacore::Vector<casacore::uInt>& antenna2();

        /// @copydoc VisChunk::antenna2()
        const casacore::Vector<casacore::uInt>& antenna2() const;

        /// First beam IDs for all rows
        ///
        /// @note beam ID is zero based
        ///
        /// @return a vector with IDs of the first beam corresponding
        /// to each visibility (one for each row)
        casacore::Vector<casacore::uInt>& beam1();

        /// @copydoc VisChunk::beam1()
        const casacore::Vector<casacore::uInt>& beam1() const;

        /// Second beam IDs for all rows
        ///
        /// @note beam ID is zero based.
        ///
        /// @return a vector with IDs of the second beam corresponding
        /// to each visibility (one for each row)
        casacore::Vector<casacore::uInt>& beam2();

        /// @copydoc VisChunk::beam2()
        const casacore::Vector<casacore::uInt>& beam2() const;

        /// Position angles of the first beam for all rows
        /// @return a vector with position angles of the
        /// first beam corresponding to each visibility
        /// Units: Radians
        casacore::Vector<casacore::Float>& beam1PA();

        /// @copydoc VisChunk::beam1PA()
        const casacore::Vector<casacore::Float>& beam1PA() const;

        /// Position angles of the second beam for all rows
        /// @return a vector with position angles of the
        /// second beam corresponding to each visibility
        /// Units: Radians
        casacore::Vector<casacore::Float>& beam2PA();

        /// @copydoc VisChunk::beamsPA()
        const casacore::Vector<casacore::Float>& beam2PA() const;

        /// Return phase centre directions the the given row of data
        /// @return a vector with direction measures 
        casacore::Vector<casacore::MVDirection>& phaseCentre();

        /// @copydoc VisChunk::phaseCentre()
        const casacore::Vector<casacore::MVDirection>& phaseCentre() const;

        /// Returns the TARGET dish pointing centre for each antenna.
        /// The length of the vector will be of length nAntennas, and the 
        /// vector indexing matches the index returned from either the
        /// antenna1() or antenna2() methods.
        /// @return a vector of direction measures, one for each antenna
        casacore::Vector<casacore::MDirection>& targetPointingCentre();

        /// @copydoc VisChunk::targetPointingCentre()
        const casacore::Vector<casacore::MDirection>& targetPointingCentre() const;

        /// Returns the ACTUAL dish pointing centre for each antenna.
        /// The length of the vector will be of length nAntennas, and the 
        /// vector indexing matches the index returned from either the
        /// antenna1() or antenna2() methods.
        /// @return a vector of direction measures, one for each antenna
        casacore::Vector<casacore::MDirection>& actualPointingCentre();

        /// Returns the ACTUAL polarisation axis position  for each antenna.
        /// The length of the vector will be of length nAntennas, and the 
        /// vector indexing matches the index returned from either the
        /// antenna1() or antenna2() methods.
        /// @return a vector of quantities, one for each antenna
        const casacore::Vector<casacore::MDirection>& actualPointingCentre() const;

        /// Actual polarisation axis offset for each antenna
        /// @brief pol axis angle for each antenna
        casacore::Vector<casacore::Quantity>& actualPolAngle();

        /// @copydoc VisChunk::actualPolAngle()
        const casacore::Vector<casacore::Quantity>& actualPolAngle() const;

        /// Actual azimuth for each antenna
        /// @brief azimuth angle for each antenna as reported by TOS
        casacore::Vector<casacore::Quantity>& actualAzimuth();

        /// @copydoc VisChunk::actualAzimuth()
        const casacore::Vector<casacore::Quantity>& actualAzimuth() const;

        /// Actual elevation for each antenna
        /// @brief elevation angle for each antenna as reported by TOS
        casacore::Vector<casacore::Quantity>& actualElevation();

        /// @copydoc VisChunk::actualElevation()
        const casacore::Vector<casacore::Quantity>& actualElevation() const;

        /// On-source flag for each antenna
        /// @brief true for each antenna which was on-source according to TOS
        casacore::Vector<bool>& onSourceFlag();

        /// @copydoc VisChunk::onSourceFlag()
        const casacore::Vector<bool>& onSourceFlag() const;

        /// VisChunk (a cube is nRow x nChannel x nPol; each element is
        /// a complex visibility)
        /// @return a reference to nRow x nChannel x nPol cube, containing
        /// all visibility data
        casacore::Cube<casacore::Complex>& visibility();

        /// @copydoc VisChunk::visibility()
        const casacore::Cube<casacore::Complex>& visibility() const;

        /// Cube of flags corresponding to the output of visibility()
        /// @return a reference to nRow x nChannel x nPol cube with flag
        ///         information. If True, the corresponding element is flagged.
        casacore::Cube<casacore::Bool>& flag();

        /// @copydoc VisChunk::flag()
        const casacore::Cube<casacore::Bool>& flag() const;

        /// UVW
        /// @return a reference to vector containing uvw-coordinates
        /// packed into a 3-D rigid vector
        casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& uvw();

        /// @copydoc VisChunk::uvw()
        const casacore::Vector<casacore::RigidVector<casacore::Double, 3> >& uvw() const;

        /// Frequency for each channel.
        /// Units: Hz
        /// @return a reference to vector containing frequencies for each
        ///         spectral channel (vector size is nChannel).
        casacore::Vector<casacore::Double>& frequency();

        /// @copydoc VisChunk::frequency()
        const casacore::Vector<casacore::Double>& frequency() const;

        /// Channel width of each spectral channel.
        /// All spectral channels in the frequency vector have a channel
        /// width which can be derived from frequency() by differencing,
        /// however is stored here for efficiency.
        /// Units: Hz
        /// @return  a refernece to the channel width of each spectral channel.
        casacore::Double& channelWidth();

        /// @copydoc VisChunk::channelWidth()
        const casacore::Double& channelWidth() const;

        /// @brief polarisation type for each product
        /// @return a reference to vector containing polarisation types for
        /// each product in the visibility cube (nPol() elements).
        /// @note All rows of the accessor have the same structure of the visibility
        /// cube, i.e. polarisation types returned by this method are valid for all rows.
        casacore::Vector<casacore::Stokes::StokesTypes>& stokes();

        /// @copydoc VisChunk::stokes()
        const casacore::Vector<casacore::Stokes::StokesTypes>& stokes() const;

        /// @brief direction reference frame for all MVDirection instances
        /// in this class.
        /// @return a reference to the MDirection:Ref.
        casacore::MDirection::Ref& directionFrame();

        /// @copydoc VisChunk::directionFrame()
        const casacore::MDirection::Ref& directionFrame() const;

        /// @brief beam offsets
        /// @return a reference to matrix with beam offsets
        /// @note This matrix may be uninitialised, if static beam offsets are used
        /// Otherwise, the matrix is 2 x nBeam
        casacore::Matrix<double>& beamOffsets();

        /// @copydoc VisChunk::beamOffsets()
        const casacore::Matrix<double>& beamOffsets() const;

        /// Allows the VisChunk's nChannel dimension to be resized.
        /// This allows resizing in the nChannel dimension only, and by
        /// allowing new visibility, flag and frequency containers to
        /// be assigned.
        ///
        /// @note This exists to support the channel averaging task.
        ///
        /// The following conditions must be met otherwise an
        /// AskapError exception is thrown:
        /// @li The visibility and flag cubes must have the same number
        ///     of rows and polarisations as the existing cubes.
        /// @li The visibility and flag cubes and the frequency vector
        ///     must have the same size channel dimension.
        ///
        /// @throw AskapError If one of the above mentioned conditions
        ///     not met.
        ///
        /// @param[in] visibility the new visibility cube to assign.
        /// @param[in] flag  the new flag cube to assign.
        /// @param[in] frequency the new frequency vector to assign.
        void resize(const casacore::Cube<casacore::Complex>& visibility,
                    const casacore::Cube<casacore::Bool>& flag,
                    const casacore::Vector<casacore::Double>& frequency);

        /// @brief Shared pointer typedef
        typedef boost::shared_ptr<VisChunk> ShPtr;

    private:

        /// Number of rows
        casacore::uInt itsNumberOfRows;

        /// Number of channels
        casacore::uInt itsNumberOfChannels;

        /// Number of polarisations
        casacore::uInt itsNumberOfPolarisations;

        /// Number of antennas
        casacore::uInt itsNumberOfAntennas;

        /// Time
        casacore::MVEpoch itsTime;

        /// Target Name
        std::string itsTargetName;

        /// Interval
        casacore::Double itsInterval;

        /// Scan Index
        casacore::uInt itsScan;

        /// Antenna1
        casacore::Vector<casacore::uInt> itsAntenna1;

        /// Antenna2
        casacore::Vector<casacore::uInt> itsAntenna2;

        /// Beam1
        casacore::Vector<casacore::uInt> itsBeam1;

        /// Beam2
        casacore::Vector<casacore::uInt> itsBeam2;

        /// Beam1 position angle
        casacore::Vector<casacore::Float> itsBeam1PA;

        /// Beam2 position angle
        casacore::Vector<casacore::Float> itsBeam2PA;

        /// Phase centre of for the given row (beam/baseline)
        casacore::Vector<casacore::MVDirection> itsPhaseCentre;

        /// Target dish pointing direction for each antenna
        casacore::Vector<casacore::MDirection> itsTargetPointingCentre;

        /// Actual dish pointing direction for each antenna
        casacore::Vector<casacore::MDirection> itsActualPointingCentre;

        /// Actual polarisation axis offset for each antenna
        casacore::Vector<casacore::Quantity> itsActualPolAngle;

        /// Actual azimuth axis position for each antenna
        casacore::Vector<casacore::Quantity> itsActualAzimuth;

        /// Actual elevation axis position for each antenna
        casacore::Vector<casacore::Quantity> itsActualElevation;

        /// on-source flag for each antenna
        casacore::Vector<bool> itsOnSourceFlag;

        /// Visibility
        casacore::Cube<casacore::Complex> itsVisibility;

        /// Flag
        casacore::Cube<casacore::Bool> itsFlag;

        /// UVW
        casacore::Vector<casacore::RigidVector<casacore::Double, 3> > itsUVW;

        /// Frequency
        casacore::Vector<casacore::Double> itsFrequency;

        /// Channel Width
        casacore::Double itsChannelWidth;

        /// Stokes
        casacore::Vector<casacore::Stokes::StokesTypes> itsStokes;

        /// Direction frame
        casacore::MDirection::Ref itsDirectionFrame;

        /// Beam offsets (2xnBeam or empty matrix)
        casacore::Matrix<casacore::Double> itsBeamOffsets;

};

} // end of namespace common
} // end of namespace cp
} // end of namespace askap

#endif

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
#ifndef ASKAP_ANALYSIS_FDF_WRITER_H_
#define ASKAP_ANALYSIS_FDF_WRITER_H_
#include <polarisation/PolarisationData.h>
#include <polarisation/RMSynthesis.h>
#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {

/// @brief A class to handle the writing out of the Faraday Dispersion
/// Function (FDF) and the Rotation Measure Spread Function (RMSF) to
/// image files on disk.

/// @details This class provides a mechanism to obtain the FDF & RMSF
/// from the RMsynthesis results, and write them out to image files on
/// disk. The world coordinate system will encapsulate the Faraday
/// depth axis, as well as degenerate RA/Dec axes that record where
/// the component is.
class FDFwriter {
    public:
        /// @brief Constructor.
        /// @details Initialises arrays and coordinate systems using the
        /// information in poldata and rmsynth.
        FDFwriter(LOFAR::ParameterSet &parset,
                  PolarisationData &poldata,
                  RMSynthesis &rmsynth);
        virtual ~FDFwriter() {};

        /// @details Create and write the arrays to the image files
        void write();

    protected:

        /// @brief User flag indicating whether the images should be
        /// written as complex-valued (true) or separate files for phase &
        /// amplitude.
        bool                       itsFlagWriteAsComplex;

        /// @brief The base name for the output file, taken from the
        /// input parset
        std::string                itsOutputBase;
        /// @brief ID for the component - incorporated into the image names
        std::string                itsSourceID;

        /// @brief Coordinate system used for the FDF image
        casacore::CoordinateSystem itsCoordSysForFDF;
        /// @brief Coordinate system used for the RMSF image
        casacore::CoordinateSystem itsCoordSysForRMSF;

        /// @brief Array containing the FDF - reshaped to suit the output image
        casa::Array<casa::Complex> itsFDF;
        /// @brief Array containing the RMSF - reshaped to suit the output image
        casa::Array<casa::Complex> itsRMSF;

};

}
}

#endif

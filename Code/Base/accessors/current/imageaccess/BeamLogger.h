/// @file BeamLogger.h
///
/// Class to log the restoring beams of individual channels of a spectral cube
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
#ifndef ASKAP_ACCESSORS_BEAM_LOGGER_H
#define ASKAP_ACCESSORS_BEAM_LOGGER_H

// System includes
#include <string>
#include <vector>

// ASKAPsoft includes
#include <casa/Arrays/Vector.h>
#include <casa/Quanta/Quantum.h>
#include <Common/ParameterSet.h>

namespace askap {
namespace accessors {

/// @brief Class to handle writing & reading of channel-level
/// beam information for a spectral cube.
/// @details This class wraps up the functionality required to
/// create and access the beam log files. These files are
/// created by the makecube application to record the
/// restoring beam of the individual channel images that are
/// combined to form the spectral cube. The class also
/// provides the ability to straightforwardly read the beam
/// log to extract the channel-level beam information.

class BeamLogger {
    public:
        BeamLogger();
        BeamLogger(const LOFAR::ParameterSet &parset);
        BeamLogger(const std::string &filename);
        virtual ~BeamLogger() {};

        /// Set the name of the beam log file
        void setFilename(const std::string& filename) {itsFilename = filename;};

        std::string filename() {return itsFilename;};

        /// @brief Extract the beam information for each channel image
        /// provided in the the imageList
        /// @param imageList A vector list of image names
        void extractBeams(const std::vector<std::string>& imageList);

        /// @brief Write the beam information to the beam log
        /// @details The beam information for each channel is written
        /// to the beam log. The log is in ASCII format, with each
        /// line having columns: number | major axis [arcsec] | minor
        /// axis [arcsec] | position angle [deg]. Each column is
        /// separated by a single space. The first line is a comment
        /// line (starting with a '#') that indicates what each column
        /// contains.
        void write();

        /// @brief Read the beam information from a beam log
        /// @details The beam log file is opened and each channel's
        /// beam information is read and stored in the vector of beam
        /// values. The list of channel image names is also filled. If
        /// the beam log can not be opened, both vectors are cleared
        /// and an error message is written to the log.
        void read();

        /// @brief Return the beam information
        std::vector< casa::Vector<casa::Quantum<double> > > beamlist() const {return itsBeamList;};

        /// @brief Return the beam information
        std::vector< casa::Vector<casa::Quantum<double> > > &beamlist() {return itsBeamList;};

    protected:
        /// @brief The disk file to be read from / written to
        std::string itsFilename;

        /// @brief The list of beam information. Each element of the outer
        /// vector is a 3-point casa::Vector containing the major axis,
        /// minor axis and position angle of a beam.
        std::vector< casa::Vector<casa::Quantum<double> > > itsBeamList;

};

}
}

#endif

/// @file
///
/// Class for specifying an entry in the Island catalogue
///
/// @copyright (c) 2015 CSIRO
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
#ifndef ASKAP_ANALYSIS_CASDA_ISLAND_H_
#define ASKAP_ANALYSIS_CASDA_ISLAND_H_

#include <catalogues/CasdaObject.h>
#include <catalogues/Casda.h>
#include <sourcefitting/RadioSource.h>
#include <Common/ParameterSet.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>

namespace askap {

namespace analysis {

/// @brief A class defining an entry in the CASDA Island catalogue.
/// @details This class holds all information that will be written to
/// the CASDA island catalogue for a single island. It allows
/// translation from a RadioSource object and provides methods to
/// write out the Island to a VOTable or other type of catalogue file.
class CasdaIsland : public CasdaObject {
    public:
        /// Default constructor that does nothing.
        CasdaIsland();

        /// Constructor that builds the Island object from a
        /// RadioSource. The number of fitted components is used,
        /// otherwise it is essentially the information contained in
        /// the duchamp::Detection object. The parset is passed to the
        /// base CasdaObject object, and used to get the scheduling
        /// block ID and image name, for constructing the
        /// islandID. The fitType is used when calculating the
        /// statistics of the fit residuals.
        CasdaIsland(sourcefitting::RadioSource &obj,
                    const LOFAR::ParameterSet &parset,
                    const std::string fitType = casda::componentFitType);

        /// Default destructor
        virtual ~CasdaIsland() {};

        /// Return the RA of the Island.
        const float ra();

        /// Return the Declination of the Island.
        const float dec();

        /// Return the ID string
        const std::string id();

        ///  Print a single value (column) into an output table. The
        ///  column's correct value is extracted according to the
        ///  Catalogues::COLNAME key in the column given.
        ///  \param stream Where the output is written
        ///  \param column The Column object defining the formatting.
        void printTableEntry(std::ostream &stream,
                             duchamp::Catalogues::Column &column);

        /// Allow the Column provided to check its width against that
        /// required by the value for this Island, and increase its
        /// width if need be. The correct value is chose according to
        /// the COLNAME key. If a key is given that was not expected,
        /// an Askap Error is thrown. The column must be non-const as
        /// it could change.
        void checkCol(duchamp::Catalogues::Column &column, bool checkTitle);

        /// @brief Functions allowing CasdaIsland objects to be passed
        /// over LOFAR Blobs
        /// @name
        /// @{
        /// @brief Pass a CasdaIsland object into a Blob
        /// @details This function provides a mechanism for passing the
        /// entire contents of a CasdaIsland object into a
        /// LOFAR::BlobOStream stream
        friend LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &stream,
                                              CasdaIsland& src);
        /// @brief Receive a CasdaIsland object from a Blob
        /// @details This function provides a mechanism for receiving the
        /// entire contents of a CasdaIsland object from a
        /// LOFAR::BlobIStream stream
        friend LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &stream,
                                              CasdaIsland& src);

        /// @}

        /// @brief Comparison operator, using the island ID
        friend bool operator< (CasdaIsland lhs, CasdaIsland rhs)
        {
            return (lhs.id() < rhs.id());
        }

    protected:
        /// The unique ID for the island
        std::string itsIslandID;
        /// The J2000 IAU-format name
        std::string itsName;
        /// The number of components that were fitted to this island
        unsigned int itsNumComponents;
        /// The RA in string format: 12:34:56.7
        std::string itsRAs;
        /// The Declination in string format: -12:34:45.78
        std::string itsDECs;
        /// The RA in decimal degrees
        double itsRA;
        /// The Declination in decimal degrees
        double itsDEC;
        /// The frequency of the image
        double itsFreq;
        /// The estimated major axis of the island
        double itsMaj;
        /// The estimated minor axis of the island
        double itsMin;
        /// The position angle of the island's major axis
        double itsPA;
        /// The integrated flux of the pixels in the island.
        casda::ValueError itsFluxInt;
        /// The flux of the brightest pixel in the island
        double itsFluxPeak;
        /// The mean value of the background level across the island
        double itsMeanBackground;
        /// The average noise level in the background across the island
        double itsBackgroundNoise;
        /// The maximum residual after subtraction of fitted Gaussian(s)
        double itsMaxResidual;
        /// The minimum residual after subtraction of fitted Gaussian(s)
        double itsMinResidual;
        /// The average residual after subtraction of fitted Gaussian(s)
        double itsMeanResidual;
        /// The RMS residual after subtraction of fitted Gaussian(s)
        double itsRMSResidual;
        /// The standard deviation in the residuals after subtraction of fitted Gaussian(s)
        double itsStddevResidual;
        /// The minimum x pixel value for the island
        int itsXmin;
        /// The maximum x pixel value for the island
        int itsXmax;
        /// The minimum y pixel value for the island
        int itsYmin;
        /// The maximum y pixel value for the island
        int itsYmax;
        /// The number of pixels in the island
        unsigned int itsNumPix;
        /// The solid angle subtended by the island (area on the sky, [arcmin^2])
        double itsSolidAngle;
        /// The area of the beam on the sky, using the Full-Width-Half-Maximum ellipse
        double itsBeamArea;
        /// The average x-value of all pixels in the island
        double itsXaverage;
        /// The average y-value of all pixels in the island
        double itsYaverage;
        /// The flux-weighted average x-value of all pixels in the island
        double itsXcentroid;
        /// The flux-weighted average y-value of all pixels in the island
        double itsYcentroid;
        /// The x-value of the brightest pixel of the island
        int itsXpeak;
        /// The y-value of the brightest pixel of the island
        int itsYpeak;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag1;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag2;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag3;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag4;
        /// A comment string, not used as yet.
        std::string itsComment;

};


}

}

#endif

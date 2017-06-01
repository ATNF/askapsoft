/// @file
///
/// Class for specifying an entry in the HI Emission Object catalogue
///
/// @copyright (c) 2014 CSIRO
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
#ifndef ASKAP_ANALYSIS_CASDA_HI_EMISSION_H_
#define ASKAP_ANALYSIS_CASDA_HI_EMISSION_H_

#include <catalogues/Casda.h>
#include <catalogues/CatalogueEntry.h>
#include <catalogues/CasdaComponent.h>
#include <sourcefitting/RadioSource.h>
#include <Common/ParameterSet.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/columns.hh>
#include <vector>

namespace askap {

namespace analysis {

/// @brief A class defining an entry in the CASDA HI Emission-line catalogue.
/// @details This class holds all information that will be written to
/// the CASDA HI Emission-line catalogue for a single detected object. It
/// allows extraction from a RadioSource object and provides methods
/// to write out the Component to a VOTable or other type of catalogue
/// file.
class CasdaHiEmissionObject : public CatalogueEntry {
    public:
        /// Default constructor that does nothing.
        CasdaHiEmissionObject();
    
        /// Constructor that builds the Emission-line object from a
        /// RadioSource. It takes a single detection, stored as a
        /// RadioSource. The parset is used to...
        CasdaHiEmissionObject(sourcefitting::RadioSource &obj,
                              const LOFAR::ParameterSet &parset);

        /// Default destructor
        virtual ~CasdaHiEmissionObject() {};

        /// Return the RA (in decimal degrees)
        const float ra();
        /// Return the Declination (in decimal degrees)
        const float dec();
        // Return the ID string
        const std::string id();

        ///  Print a row of values for the objet into an
        ///  output table. Each column from the catalogue
        ///  specification is sent to printTableEntry for output.
        ///  \param stream Where the output is written
        ///  \param columns The vector list of Column objects
        void printTableRow(std::ostream &stream,
                           duchamp::Catalogues::CatalogueSpecification &columns);

        ///  Print a single value (a column) into an output table. The
        ///  column's correct value is extracted according to the
        ///  Catalogues::COLNAME key in the column given.
        ///  \param stream Where the output is written
        ///  \param column The Column object defining the formatting.
        void printTableEntry(std::ostream &stream,
                             duchamp::Catalogues::Column &column);

        /// Allow the Column provided to check its width against that
        /// required by the value for this object, and increase its
        /// width if need be. The correct value is chose according to
        /// the COLNAME key. If a key is given that was not expected,
        /// an Askap Error is thrown. Column must be non-const as it
        /// could change.
        void checkCol(duchamp::Catalogues::Column &column);

        /// Perform the column check for all colums in
        /// specification. If allColumns is false, only the columns
        /// with type=char are checked, otherwise all are.
        void checkSpec(duchamp::Catalogues::CatalogueSpecification &spec, bool allColumns = true);

    /// @brief Functions allowing CasdaPolarisationEntry objects to be passed
        /// over LOFAR Blobs
        /// @name
        /// @{
        /// @brief Pass a CasdaPolarisationEntry object into a Blob
        /// @details This function provides a mechanism for passing the
        /// entire contents of a CasdaPolarisationEntry object into a
        /// LOFAR::BlobOStream stream
        friend LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &stream,
                                              CasdaHiEmissionObject& src);
        /// @brief Receive a CasdaPolarisationEntry object from a Blob
        /// @details This function provides a mechanism for receiving the
        /// entire contents of a CasdaPolarisationEntry object from a
        /// LOFAR::BlobIStream stream
        friend LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &stream,
                                              CasdaHiEmissionObject& src);

        /// @}

        /// @brief Comparison operator, using the component ID
        friend bool operator< (CasdaHiEmissionObject lhs, CasdaHiEmissionObject rhs)
        {
            return (lhs.id() < rhs.id());
        }

    protected:
        /// The unique ID for this object
        std::string itsObjectID;
        /// The J2000 IAU-format name
        std::string itsName;

        /// The RA in string format: 12:34:56.7, based on the weighted average of the voxels
        std::string itsRAs_w;
        /// The Declination in string format: 12:34:56.7, based on the weighted average of the voxels
        std::string itsDECs_w;
        /// The RA in decimal degrees, based on the weighted average of the voxels
        casda::ValueError itsRA_w;
        /// The Declination in decimal degrees, based on the weighted average of the voxels
        casda::ValueError itsDEC_w;
        /// The RA in decimal degrees, based on the unweighted average of the voxels
        casda::ValueError itsRA_uw;
        /// The Declination in decimal degrees, based on the unweighted average of the voxels
        casda::ValueError itsDEC_uw;
        /// The Galactic longitude in decimal degrees, based on the weighted average of the voxels
        casda::ValueError itsGlong_w;
        /// The Galactic latitude in decimal degrees, based on the weighted average of the voxels
        casda::ValueError itsGlat_w;
        /// The Galactic longitude in decimal degrees, based on the unweighted average of the voxels
        casda::ValueError itsGlong_uw;
        /// The Galactic latitude in decimal degrees, based on the unweighted average of the voxels
        casda::ValueError itsGlat_uw;

        /// The major axis of the moment-0 map, determined from the principle axes
        double itsMajorAxis;
        /// The minor axis of the moment-0 map, determined from the principle axes
        double itsMinorAxis;
        /// The postion angle of the major axis as determined from the principle axes
        double itsPositionAngle;
        /// The major axis of the moment-0 map, determined from a 2D Gaussian fit
        casda::ValueError itsMajorAxis_fit;
        /// The minor axis of the moment-0 map, determined from a 2D Gaussian fit
        casda::ValueError itsMinorAxis_fit;
        /// The postion angle of the major axis as determined from a 2D Gaussian fit
        casda::ValueError itsPositionAngle_fit;
        /// The size of the bounding box of detected voxels, in the x-direction
        int itsSizeX;
        /// The size of the bounding box of detected voxels, in the y-direction
        int itsSizeY;
        /// The size of the bounding box of detected voxels, in the z-direction
        int itsSizeZ;
        /// The total number of detected voxels
        int itsNumVoxels;
        /// The asymmetry in the flux of the moment-0 map - ranges from 0 (=uniform) to 1
        casda::ValueError itsAsymmetry2d;
        /// The asymmetry in the flux of the 3D distribution of voxels - ranges from 0 (=uniform) to 1
        casda::ValueError itsAsymmetry3d;

        /// The frequency of the object, unweighted average
        casda::ValueError itsFreq_uw;
        /// The frequency of the object, weighted average
        casda::ValueError itsFreq_w;
        /// The frequency of the peak flux of the object
        double itsFreq_peak;
        /// The HI velocity for the unweighted average frequency of the object
        casda::ValueError itsVelHI_uw;
        /// The HI velocity for the weighted average frequency of the object
        casda::ValueError itsVelHI_w;
        /// The HI velocity for the frequency of the peak flux
        double itsVelHI_peak;

        /// The integrated flux, summed over all detected voxels and
        /// corrected for the beam area
        casda::ValueError itsIntegFlux;
        /// Maximum voxel flux value
        double itsFluxMax;
        /// Minimum detected voxel flux value
        double itsFluxMin;
        /// Mean detected flux value
        double itsFluxMean;
        /// Standard deviation of the flux values of the detected voxels
        double itsFluxStddev;
        /// The rms of the flux values of the detected voxels
        double itsFluxRMS;
        /// The local RMS noise of the image cube surrounding the object
        double itsRMSimagecube;


        /// The frequency width of the object at 50% of the peak optical depth
        casda::ValueError itsW50_freq;
        /// The frequency width of the object at 20% of the peak optical depth
        casda::ValueError itsW20_freq;
        /// The frequency width of the object measured from the integrated
        /// spectrum's cumulative flux distribution, using bounds taken
        /// from where a Gaussian profile is above 50% of its peak flux
        /// density
        casda::ValueError itsCW50_freq;
        /// The frequency width of the object measured from the integrated
        /// spectrum's cumulative flux distribution, using bounds taken
        /// from where a Gaussian profile is above 50% of its peak flux
        /// density
        casda::ValueError itsCW20_freq;
        /// The velocity width of the object at 50% of the peak optical depth
        casda::ValueError itsW50_vel;
        /// The velocity width of the object at 20% of the peak optical depth
        casda::ValueError itsW20_vel;
        /// The velocity width of the object measured from the integrated
        /// spectrum's cumulative flux distribution, using bounds taken
        /// from where a Gaussian profile is above 50% of its peak flux
        /// density
        casda::ValueError itsCW50_vel;
        /// The velocity width of the object measured from the integrated
        /// spectrum's cumulative flux distribution, using bounds taken
        /// from where a Gaussian profile is above 50% of its peak flux
        /// density
        casda::ValueError itsCW20_vel;

        /// The frequency determined from the unweighted average of voxels above 50% of the peak flux
        casda::ValueError itsFreq_W50clip_uw;
        /// The frequency determined from the unweighted average of voxels above 20% of the peak flux
        casda::ValueError itsFreq_W20clip_uw;
        /// The frequency determined from the unweighted average of voxels above the CW50 flux limits
        casda::ValueError itsFreq_CW50clip_uw;
        /// The frequency determined from the unweighted average of voxels above the CW20 flux limits
        casda::ValueError itsFreq_CW20clip_uw;
        /// The frequency determined from the weighted average of voxels above 50% of the peak flux
        casda::ValueError itsFreq_W50clip_w;
        /// The frequency determined from the weighted average of voxels above 20% of the peak flux
        casda::ValueError itsFreq_W20clip_w;
        /// The frequency determined from the weighted average of voxels above the CW50 flux limits
        casda::ValueError itsFreq_CW50clip_w;
        /// The frequency determined from the weighted average of voxels above the CW20 flux limits
        casda::ValueError itsFreq_CW20clip_w;

        /// The HI velocity determined from the unweighted average of voxels above 50% of the peak flux
        casda::ValueError itsVelHI_W50clip_uw;
        /// The HI velocity determined from the unweighted average of voxels above 20% of the peak flux
        casda::ValueError itsVelHI_W20clip_uw;
        /// The HI velocity determined from the unweighted average of voxels above the CW50 flux limits
        casda::ValueError itsVelHI_CW50clip_uw;
        /// The HI velocity determined from the unweighted average of voxels above the CW20 flux limits
        casda::ValueError itsVelHI_CW20clip_uw;
        /// The HI velocity determined from the weighted average of voxels above 50% of the peak flux
        casda::ValueError itsVelHI_W50clip_w;
        /// The HI velocity determined from the weighted average of voxels above 20% of the peak flux
        casda::ValueError itsVelHI_W20clip_w;
        /// The HI velocity determined from the weighted average of voxels above the CW50 flux limits
        casda::ValueError itsVelHI_CW50clip_w;
        /// The HI velocity determined from the weighted average of voxels above the CW20 flux limits
        casda::ValueError itsVelHI_CW20clip_w;

        /// The integrated flux determined from the sum of the fluxes of all voxels above 50% of the peak flux
        casda::ValueError itsIntegFlux_W50clip;
        /// The integrated flux determined from the sum of the fluxes of all voxels above 20% of the peak flux
        casda::ValueError itsIntegFlux_W20clip;
        /// The integrated flux determined from the sum of the fluxes of all voxels above the CW50 flux limits
        casda::ValueError itsIntegFlux_CW50clip;
        /// The integrated flux determined from the sum of the fluxes of all voxels above the CW20 flux limits
        casda::ValueError itsIntegFlux_CW20clip;

        /// The amplitude scaling factor, 'a', from a busy-function fit
        casda::ValueError itsBFfit_a;
        /// The half-width parameter, 'w', from a busy-function fit
        casda::ValueError itsBFfit_w;
        /// The slope of the first error function, 'b1', from a busy-function fit
        casda::ValueError itsBFfit_b1;
        /// The slope of the first error function, 'b2', from a busy-function fit
        casda::ValueError itsBFfit_b2;
        /// The offset parameter for the error function, 'xe', from a busy-function fit
        casda::ValueError itsBFfit_xe;
        /// The offset parameter for the polynomial function, 'xp', from a busy-function fit
        casda::ValueError itsBFfit_xp;
        /// The parameter governing the amplitude of the central trough, 'c', from a busy-function fit
        casda::ValueError itsBFfit_c;
        /// The degree of the polynomial, 'n', from a busy-function fit
        casda::ValueError itsBFfit_n;

        /// A flag indicating whether the object's continuum component is resolved spatially
        unsigned int itsFlagResolved;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag2;
        /// A yet-to-be-identified quality flag
        unsigned int itsFlag3;
        /// A comment string, not used as yet.
        std::string itsComment;

};

}

}

#endif

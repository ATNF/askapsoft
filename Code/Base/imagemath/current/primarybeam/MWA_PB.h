/// @file MWA_PB.h

/// @brief Standard MWA Primary Beam
/// @details
///

#ifndef ASKAP_MWA_H
#define ASKAP_MWA_H

#include "PrimaryBeam.h"

#include <boost/shared_ptr.hpp>
#include <Common/ParameterSet.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

namespace askap {
namespace imagemath {

    class MWA_PB : public PrimaryBeam

    {

    public:

        typedef boost::shared_ptr<MWA_PB> ShPtr;

        MWA_PB();

        static inline std::string PrimaryBeamName() { return "MWA_PB";}

        virtual ~MWA_PB();

        MWA_PB(const MWA_PB &other);

        static PrimaryBeam::ShPtr createDefaultPrimaryBeam();

        static PrimaryBeam::ShPtr createPrimaryBeam(const LOFAR::ParameterSet &parset);

        /// Get some parameters

        double getDipoleSeparation();
        double getDipoleHeight();

        virtual double evaluateAtOffset(double offsetPA, double offsetDist, double frequency);
        virtual double evaluateAtOffset(double offsetDist, double frequency);
        virtual casacore::Matrix<casacore::Complex> getJonesAtOffset(double offset, double frequency);
        virtual casacore::Matrix<casacore::Complex> getJonesAtOffset(double azimuth, double zenithAngle,
                                                        double frequency);

        /// Some sets.

        void setLatitude(double lat) {latitude=lat;};
        void setLongitude(double lon) {longitude=lon;};

        void setDipoleSeparation(double sep) {DipoleSeparation=sep;};
        void setDipoleHeight(double hgt) {DipoleHeight=hgt;};

    private:

        // Location of the MWA
        double latitude;
        double longitude;

        // Number of dipole rows and columns in MWA tiles
        int numDipoleRows;
        int numDipoleColumns;

        // Separation of MWA dipoles ("Short-Wide" tiles)
        double DipoleSeparation;

        // Height of MWA dipoles ("Short-Wide" tiles)
        double DipoleHeight;

    };

} // namespace imagemath

} // namespace askap


#endif // #ifndef ASKAP_MWA_H

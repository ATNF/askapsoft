/// @file GaussianPB.h

/// @brief Standard Gaussian Primary Beam
/// @details
///

#ifndef ASKAP_GAUSSIAN_H
#define ASKAP_GAUSSIAN_H

#include "PrimaryBeam.h"

#include <boost/shared_ptr.hpp>
#include <Common/ParameterSet.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

namespace askap {
namespace imagemath {

    class GaussianPB : public PrimaryBeam

    {

    public:

        typedef boost::shared_ptr<GaussianPB> ShPtr;

        GaussianPB();

        static inline std::string PrimaryBeamName() { return "GaussianPB";}

        virtual ~GaussianPB();

        GaussianPB(const GaussianPB &other);

        static PrimaryBeam::ShPtr createPrimaryBeam(const LOFAR::ParameterSet &parset);

        /// Set some parameters

        void setApertureSize(double apsize) {this->ApertureSize = apsize; };

        void setFWHMScaling(double fwhmScale) {this->FWHMScaling = fwhmScale;};

        void setExpScaling(double expScale) {this->ExpScaling = expScale;};

        /// Get some parameters

        double getFWHM(const double frequency);

        double getExpScaling()
        {return this->ExpScaling;};

        virtual double evaluateAtOffset(double offset, double frequency);

        /// Probably should have a "generate weight" - that calls evaluate for
        /// every pixel ....

        virtual casa::Matrix<casa::Complex> getJonesAtOffset(double offset, double frequency);

    private:

        // Size of the telescope aperture
        double ApertureSize;

        // scaling of FWHM to match simulations
        double FWHMScaling;

        // Further scaling of the Gaussian exponent
        double ExpScaling;

    };

} // namespace imagemath

} // namespace askap


#endif // #ifndef ASKAP_SYNTHESIS_GAUSSIAN_H

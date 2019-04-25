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

        static PrimaryBeam::ShPtr createDefaultPrimaryBeam();

        static PrimaryBeam::ShPtr createPrimaryBeam(const LOFAR::ParameterSet &parset);

        /// Set some parameters

        void setApertureSize(double apsize) {this->ApertureSize = apsize; };

        void setFWHMScaling(double fwhmScale) {this->FWHMScaling = fwhmScale;};

        void setExpScaling(double expScale) {this->ExpScaling = expScale;};

        /// Get some parameters

        double getFWHM(const double frequency,const double width);

        double getExpScaling()
        {return this->ExpScaling;};

        double getXwidth();

        double getYwidth();


        virtual double evaluateAtOffset(double offsetPA, double offsetDist, double frequency);

        virtual double evaluateAtOffset(double offsetDist, double frequency);

        /// Probably should have a "generate weight" - that calls evaluate for
        /// every pixel ....

        virtual casacore::Matrix<casacore::Complex> getJonesAtOffset(double offset, double frequency);

        /// Some sets.

        void setAlpha(double angle)  {Alpha=angle;};
        void setXwidth(double x)     {Xwidth=x;};
        void setYwidth(double y)     {Ywidth=y;};
        void setXoff(double x)       {Xoff=x;};
        void setYoff(double y)       {Yoff=y;};


    private:

        // Size of the telescope aperture
        double ApertureSize;

        // scaling of FWHM to match simulations
        double FWHMScaling;

        // Further scaling of the Gaussian exponent
        double ExpScaling;

        // Some parameters of the GaussianPB
        /// Rotation of the elliptical beam relative to the meridian, positive is North. In a clockwise direction

        double Alpha;
        /// Width of the X-Gaussian - orientated North-South
        double Xwidth;

        /// Width of hte Y-Gaussian - orientated West-East
        double Ywidth;

        /// offset of the peak of the X-Gaussian from the centre - North is positive
        double Xoff;

        /// offset off the peak of the Y-Gaussian from the centre - East is positive
        double Yoff;



    };

} // namespace imagemath

} // namespace askap


#endif // #ifndef ASKAP_SYNTHESIS_GAUSSIAN_H

/// @file PrimaryBeam.h
///
/// @abstract
/// Base class for primary beams
/// @ details
/// defines the interface to the Primary Beam structures for the purpose of image
/// based weighting or (via an illumination) the gridding.
///

#ifndef ASKAP_PRIMARYBEAM_H

#define ASKAP_PRIMARYBEAM_H

#include <Common/ParameterSet.h>
#include <boost/shared_ptr.hpp>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

namespace askap
{
    namespace imagemath
    {

        class PrimaryBeam {

        public:

            /// Shared pointer definition
            typedef boost::shared_ptr<PrimaryBeam> ShPtr;

            PrimaryBeam();
            virtual ~PrimaryBeam();

            PrimaryBeam(const PrimaryBeam &other);

            /// This has to be static as we need to access it in the register even
            /// if there is not instantiated class.
            static ShPtr createPrimaryBeam(const LOFAR::ParameterSet& parset);

            virtual double evaluateAtOffset(double offset, double frequency) = 0;

            virtual casa::Matrix<casa::Complex> getJonesAtOffset(double offset, double frequency) = 0;

        private:

        }; // class
    } // imagemath
} //askap



#endif

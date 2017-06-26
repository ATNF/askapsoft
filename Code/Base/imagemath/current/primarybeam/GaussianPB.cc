/// @file GaussianPB.cc
///
/// @abstract
/// Derived from PrimaryBeams this is the Gaussian beam as already implemented
/// @ details
/// Implements the methods that evaluate the primary beam gain ain the case of a
/// Gaussian
///
#include "askap_imagemath.h"


#include "PrimaryBeam.h"
#include "GaussianPB.h"
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>
#include <Common/ParameterSet.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

ASKAP_LOGGER(logger, ".primarybeam.gaussianpb");
namespace askap {
    namespace imagemath {


            GaussianPB::GaussianPB() {
                ASKAPLOG_DEBUG_STR(logger,"GaussianPB default contructor");
            }


            GaussianPB::~GaussianPB() {

            }
            GaussianPB::GaussianPB(const GaussianPB &other) :
            PrimaryBeam(other)
            {
                ASKAPLOG_DEBUG_STR(logger,"GaussianPB copy contructor");
            }
            PrimaryBeam::ShPtr GaussianPB::createPrimaryBeam(const LOFAR::ParameterSet &parset)
            {
               ASKAPLOG_DEBUG_STR(logger, "createPrimaryBeam for the Gaussian Primary Beam ");

               // this is static so use this to create the instance....

               // just for logging, declare private handle to avoid issues with template
               // log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger(askap::generateLoggerName(std::string("createGaussianPB")));
               //

               // These pretty much define the pb as
               // exp(-1(offset*offset)*expscaling/fwhm/fwhm)
               // fwhm is a function of frequency so is only known when that is known


               GaussianPB::ShPtr ptr;

               // We need to pull all the parameters out of the parset - and set
               // all the private variables required to define the beam


               ptr.reset( new GaussianPB());

               ptr->setApertureSize(parset.getDouble("aperture",12));
               ptr->setFWHMScaling(parset.getDouble("fwhmscaling", 1.00));
               ptr->setExpScaling(parset.getDouble("expscaling", 4.*log(2.)));

               ASKAPLOG_DEBUG_STR(logger,"Created Gaussian PB instance");
               return ptr;

            }

            double GaussianPB::evaluateAtOffset(double offset, double frequency) {

                double pb = exp(-offset*offset*getExpScaling()/(getFWHM(frequency)*getFWHM(frequency)));
                return pb;

            }

            casa::Matrix<casa::Complex> GaussianPB::getJonesAtOffset(double offset, double frequency) {

                casa::IPosition shape(2,2,2);
                casa::Matrix<casa::Complex> Jones(shape);
                Jones = 0.0;

                Jones(casa::IPosition(2,0,0)) = casa::Complex(this->evaluateAtOffset(offset,frequency),0.0);
                Jones(casa::IPosition(2,1,1)) = Jones(casa::IPosition(2,0,0));

                return Jones;

            }

            double GaussianPB::getFWHM(const double frequency) {
                double sol = 299792458.0;
                double fwhm = sol/frequency/this->ApertureSize;
                return fwhm;
            }
    }
}

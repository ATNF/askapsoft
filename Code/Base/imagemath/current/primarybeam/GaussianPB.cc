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
            PrimaryBeam::ShPtr GaussianPB::createDefaultPrimaryBeam(){

               GaussianPB::ShPtr ptr;

               ptr.reset( new GaussianPB());

               ptr->setApertureSize(12.0);
               ptr->setFWHMScaling(1.00);
               // changed this default to deal with the double Gaussian
               ptr->setExpScaling(4.*log(2.));

               // New parameters for Dave M.'s 2-D Gaussian FITS

               ptr->setXwidth(0.0);
               ptr->setYwidth(0.0);
               ptr->setAlpha(0.0);
               ptr->setXoff(0.0);
               ptr->setYoff(0.0);

               ASKAPLOG_DEBUG_STR(logger,"Created Default Gaussian PB instance");
               return ptr;
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
               ptr->setFWHMScaling(parset.getDouble("fwhmscaling", 1.09));
               // changed this default to deal with the double Gaussian
               ptr->setExpScaling(parset.getDouble("expscaling", 4.*log(2.)));

               // New parameters for Dave M.'s 2-D Gaussian FITS

               ptr->setXwidth(parset.getDouble("xwidth",0.0));
               ptr->setYwidth(parset.getDouble("ywidth",0.0));
               ptr->setAlpha(parset.getDouble("alpha",0.0));
               ptr->setXoff(parset.getDouble("xoff",0.0));
               ptr->setYoff(parset.getDouble("yoff",0.0));

               if (ptr->getXwidth() > 0 || ptr->getYwidth() > 0) {
                 ASKAPLOG_WARN_STR(logger,"Note a width given so frequency likely to be ignored");
               }
               if (ptr->getXwidth() > 0) {
                 ASKAPCHECK(ptr->getYwidth()>0,"Both X and Y width must be given if either is");
               }
               if (ptr->getYwidth() > 0) {
                 ASKAPCHECK(ptr->getXwidth()>0,"Both X and Y width must be given if either is");
               }
               ASKAPLOG_DEBUG_STR(logger,"Created Gaussian PB instance");
               return ptr;

            }
            double GaussianPB::evaluateAtOffset(double offsetDist,double frequency) {
                // x-direction is assumed along the meridian in the direction of north celestial pole

                return evaluateAtOffset(0.0,offsetDist,frequency);

            }
            double GaussianPB::getXwidth() {

                return Xwidth;

            }
            double GaussianPB::getYwidth() {

              return Ywidth;

            }

            double GaussianPB::evaluateAtOffset(double offsetPAngle, double offsetDist,double frequency) {
                // x-direction is assumed along the meridian in the direction of north celestial pole
                // the offsetPA angle is relative to the meridian
                // the Alpha angle is the rotation of the beam pattern relative to the meridian
                // Therefore the offset relative to the
                double x_angle = offsetDist * cos(offsetPAngle-Alpha);
                double y_angle = offsetDist * sin(offsetPAngle-Alpha);
                // widths
                double x_fwhm = getFWHM(frequency,getXwidth());
                double y_fwhm = getFWHM(frequency,getYwidth());
                // offset from assumed centre
                double x_off = Xoff;
                double y_off = Yoff;

                double x_pb = exp(-1*getExpScaling()*pow((x_angle-x_off)/x_fwhm,2.));
                double y_pb = exp(-1*getExpScaling()*pow((y_angle-y_off)/y_fwhm,2.));
/*
                ASKAPLOG_INFO_STR(logger,"xoffset: " << (x_angle-x_off) << ":" << offsetDist);
                ASKAPLOG_INFO_STR(logger,"yoffset: " << (y_angle-y_off) << ":" << offsetDist);
                ASKAPLOG_INFO_STR(logger,"xwidth: " << x_width << ":" << getFWHM(frequency));
                ASKAPLOG_INFO_STR(logger,"ywidth: " << y_width << ":" << getFWHM(frequency));
                double pb = exp(-offsetDist*offsetDist*getExpScaling()/(getFWHM(frequency)*getFWHM(frequency)));
                ASKAPLOG_INFO_STR(logger,"old: " << pb << " new: " << x_pb << " " << y_pb);
*/
                return x_pb*y_pb;

            }

            casa::Matrix<casa::Complex> GaussianPB::getJonesAtOffset(double offset, double frequency) {

                casa::IPosition shape(2,2,2);
                casa::Matrix<casa::Complex> Jones(shape);
                Jones = 0.0;

                Jones(casa::IPosition(2,0,0)) = casa::Complex(this->evaluateAtOffset(offset,frequency),0.0);
                Jones(casa::IPosition(2,1,1)) = Jones(casa::IPosition(2,0,0));

                return Jones;

            }

            double GaussianPB::getFWHM(const double frequency, const double width=0) {
                double sol = 299792458.0;

                double fwhm = 0.;

                if (frequency != 0) {
                  fwhm = sol/frequency/this->ApertureSize;
                  fwhm = FWHMScaling*fwhm;
                }
                // Note if widht is <given> then Frequency is ignored

                if (width !=0) {

                  fwhm = 2 * width * pow(2*log(2),0.5);
                }

                return fwhm;
            }
    }
}

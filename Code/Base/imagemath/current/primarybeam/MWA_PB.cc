/// @file MWA_PB.cc
///
/// @abstract
/// Derived from PrimaryBeams this is the MWA beam
/// @ details
/// Implements the methods that evaluate the MWA primary beam gain
///
#include "askap_imagemath.h"


#include "PrimaryBeam.h"
#include "MWA_PB.h"
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>
#include <Common/ParameterSet.h>

#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/Arrays/IPosition.h>

ASKAP_LOGGER(logger, ".primarybeam.mwapb");
namespace askap {
    namespace imagemath {


            MWA_PB::MWA_PB() : numDipoleRows(4), numDipoleColumns(4) {
                ASKAPLOG_DEBUG_STR(logger,"MWA_PB default contructor");
                ASKAPLOG_DEBUG_STR(logger,"DAM: MWA_PB default contructor");
            }


            MWA_PB::~MWA_PB() {
            }

            MWA_PB::MWA_PB(const MWA_PB &other) : PrimaryBeam(other),
                numDipoleRows(other.numDipoleRows), numDipoleColumns(other.numDipoleColumns)
            {
                ASKAPLOG_DEBUG_STR(logger,"MWA_PB copy contructor");
            }

            PrimaryBeam::ShPtr MWA_PB::createDefaultPrimaryBeam() {

               MWA_PB::ShPtr ptr;

               ptr.reset( new MWA_PB());

               ptr->setLatitude(-26.703319 * casa::C::pi/180.);
               ptr->setLongitude(116.67081 * casa::C::pi/180.);

               ptr->setDipoleSeparation(1.10);
               ptr->setDipoleHeight(0.30);

               ASKAPLOG_DEBUG_STR(logger,"Created Default MWA PB instance");
               return ptr;
            }

            PrimaryBeam::ShPtr MWA_PB::createPrimaryBeam(const LOFAR::ParameterSet &parset)
            {
               ASKAPLOG_DEBUG_STR(logger, "createPrimaryBeam for the MWA Primary Beam ");

               // this is static so use this to create the instance....

               // just for logging, declare private handle to avoid issues with template
               // log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger(askap::generateLoggerName(std::string("createMWA_PB")));
               //

               MWA_PB::ShPtr ptr;

               // We need to pull all the parameters out of the parset - and set
               // all the private variables required to define the beam

               ptr.reset( new MWA_PB());

               ptr->setLatitude(parset.getDouble("latitude",-26.703319 * casa::C::pi/180.));
               ptr->setLongitude(parset.getDouble("longitude",116.67081 * casa::C::pi/180.));

               ptr->setDipoleSeparation(parset.getDouble("dipole.separation",1.10));
               ptr->setDipoleHeight(parset.getDouble("dipole.height",0.30));

               ASKAPCHECK(ptr->getDipoleSeparation() > 0,"dipole.separation must be greater than zero");
               ASKAPCHECK(ptr->getDipoleHeight() > 0,"dipole.height must be greater than zero");

               ASKAPLOG_DEBUG_STR(logger,"Created MWA PB instance");
               return ptr;

            }

            double MWA_PB::getDipoleSeparation() {
                return DipoleSeparation;
            }

            double MWA_PB::getDipoleHeight() {
                return DipoleHeight;
            }

            double MWA_PB::evaluateAtOffset(double offsetDist,double frequency) {
                ASKAPLOG_WARN_STR(logger,"MWA_PB::evaluateAtOffset: unsupported option. Returning 1");
                return 1.0;
            }

            double MWA_PB::evaluateAtOffset(double offsetPAngle, double offsetDist, double frequency) {
                ASKAPLOG_WARN_STR(logger,"MWA_PB::evaluateAtOffset: unsupported option. Returning 1");
                return 1.0;
            }

            casacore::Matrix<casacore::Complex> MWA_PB::getJonesAtOffset(double offset, double frequency) {
                ASKAPLOG_WARN_STR(logger,"MWA_PB::getJonesAtOffset: unsupported option. Returning I");
                casacore::IPosition shape(2,2,2);
                casacore::Matrix<casacore::Complex> Jones(shape);
                Jones = 0.0;
                Jones(casacore::IPosition(2,0,0)) = casacore::Complex(1.0,0.0);
                Jones(casacore::IPosition(2,1,1)) = casacore::Complex(1.0,0.0);
                return Jones;
            }

            casacore::Matrix<casacore::Complex> MWA_PB::getJonesAtOffset(double az, double za, double frequency) {

                ASKAPCHECK(za >= 0.0, "za must not be negative");
                ASKAPCHECK(za <= casa::C::pi/2.0, "za must not be larger than pi/2");

                const double lambda = casa::C::c/frequency;

                // move outside
                const double lat = latitude;
                const double sl = sin(lat);
                const double cl = cos(lat);

                const double sa = sin(az);
                const double ca = cos(az);
                const double cz = cos(za);
                const double sz = sin(za);
              
                const double proj_e = sz*sa;
                const double proj_n = sz*ca;
                const double proj_z = cz;
    
                const double x = -ca*sz*sl + cz*cl;
                const double y = -sa*sz;;
                const double z =  ca*sz*cl + cz*sl;
                const double r = sqrt (x*x + y*y);
                const double ha = atan2(y,x);
                const double dec = atan2(z,r);

                // move outside?
                const casacore::IPosition shape(2,2,2);
                casacore::Matrix<casacore::Complex> Jones(shape);

                const int numDipoles = numDipoleColumns*numDipoleRows;
                casacore::Vector<double> delays(numDipoles,0.);

                const double mult = 2. * casa::C::pi / lambda;
                casacore::Complex arrayFactor(0.,0.);

                // loop over dipoles
                // will need to use J here directly when incorporating dipole weights or flags
                for ( int i = 0; i < numDipoleColumns; i++ ) {
                    for ( int j = 0; j < numDipoleRows; j++ ) {
                        int k = j*numDipoleColumns + i;
                        double dipl_e = (i - 1.5) * DipoleSeparation;
                        double dipl_n = (j - 1.5) * DipoleSeparation;
                        //dipl_z = 0.0;
                        double phase = mult*(dipl_e*proj_e + dipl_n*proj_n - delays[k]*435.0e-12*casa::C::c);
                        double real_shift = cos(phase);
                        double imag_shift = sin(phase);
                        arrayFactor += casacore::Complex(real_shift,imag_shift);
                    }
                }
                arrayFactor /= double(numDipoles);

                float groundPlane = 2.*sin(2.*casa::C::pi*DipoleHeight/lambda*cos(za));
                // probably want this scaling as an optional, user-defined option
                groundPlane /= 2.*sin(2.*casa::C::pi*DipoleHeight/lambda);

                Jones(0,0) = arrayFactor * groundPlane * float(  cos(lat)*cos(dec) + sin(lat)*sin(dec)*cos(ha) );
                Jones(0,1) = arrayFactor * groundPlane * float( -sin(lat)*sin(ha) );
                Jones(1,0) = arrayFactor * groundPlane * float(  sin(dec)*sin(ha) );
                Jones(1,1) = arrayFactor * groundPlane * float(  cos(ha) );

                return Jones;

            }

    }
}

/// @file
///
/// Unit tests for the ContinuumSelavy class
///
/// @copyright (c) 2008 CSIRO
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///
#include <askap_analysisutilities.h>
#include <cppunit/extensions/HelperMacros.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <modelcomponents/ContinuumSelavy.h>

namespace askap {

  namespace analysisutilities {

//Original input was as follows - just the header of a selavy-results.components.txt file, with a single line
/* 
#                                                island_id                                              component_id component_name ra_hms_cont dec_dms_cont ra_deg_cont dec_deg_cont     ra_err    dec_err       freq  flux_peak flux_peak_err flux_int flux_int_err maj_axis min_axis pos_ang maj_axis_err min_axis_err pos_ang_err   maj_axis_deconv min_axis_deconv pos_ang_deconv  chi_squared_fit rms_fit_gauss spectral_index spectral_curvature  rms_image has_siblings fit_is_estimate flag_c3 flag_c4                                                                                             comment
#                                                       --                                                        --                                               [deg]        [deg]   [arcsec]   [arcsec]      [MHz] [mJy/beam]    [mJy/beam]    [mJy]        [mJy] [arcsec] [arcsec]   [deg]     [arcsec]     [arcsec]       [deg]          [arcsec]        [arcsec]          [deg]               --    [mJy/beam]             --                 -- [mJy/beam]

  SBnull_SB_609_617_639_643_659_no617b6_withBeam_Freq_1578 SBnull_SB_609_617_639_643_659_no617b6_withBeam_Freq_1578a J225711-614024  22:57:11.5    -61:40:24  344.298050   -61.673410       0.00       0.00      864.0      1.858         0.000    1.823        0.000    78.48    44.34  144.81         0.00         0.00        0.00           30.0609            0.00         -69.26           0.0200        40.848           0.00               0.00      0.296            0               0       0       0
*/ 

  const std::string ContinuumSelavyInput="  SBnull_SB_609_617_639_643_659_no617b6_withBeam_Freq_1578 SBnull_SB_609_617_639_643_659_no617b6_withBeam_Freq_1578a J225711-614024  22:57:11.5    -61:40:24  344.298050   -61.673410       0.00       0.00      864.0      1.858         0.000    1.823        0.000    78.48    44.34  144.81         0.00         0.00        0.00           30.0609            0.00         -69.26           0.0200        40.848           0.00               0.00      0.296            0               0       0       0";

      const std::string RA = ContinuumSelavyInput.substr(133,10);
      const std::string Dec = ContinuumSelavyInput.substr(147,9);
      const double flux = atof(ContinuumSelavyInput.substr(243,7).c_str());
      const double alpha = atof(ContinuumSelavyInput.substr(416,8).c_str());
      const double maj = atof(ContinuumSelavyInput.substr(265,6).c_str());
      const double min = atof(ContinuumSelavyInput.substr(274,6).c_str());
      const double pa = atof(ContinuumSelavyInput.substr(281,6).c_str());
  const double EPSILON=1.e-5;

      class ContinuumSelavyTest : public CppUnit::TestFixture {
	  CPPUNIT_TEST_SUITE(ContinuumSelavyTest);
	  CPPUNIT_TEST(testParameters);
	  CPPUNIT_TEST(testFluxes);
	  CPPUNIT_TEST_SUITE_END();
	
      private:
	  // members
	  ContinuumSelavy itsComponent;
	
      public:

	  void setUp(){

	      itsComponent.define(ContinuumSelavyInput);
	  
	  }

/*****************************************/
	  void tearDown() {
	  }

/*****************************************/
	  void testParameters() {
              // ASKAPLOG_DEBUG_STR(logger, "ra() = " << itsComponent.ra());
              // ASKAPLOG_DEBUG_STR(logger, "RA = " << RA);
              std::cout << "pa() = " << itsComponent.pa() << "\n";
              std::cout << "pa = " << pa << "\n";
              std::cout << itsComponent.pa() - pa << "\n";
	      CPPUNIT_ASSERT(itsComponent.ra()==RA);
	      CPPUNIT_ASSERT(itsComponent.dec()==Dec);
	      CPPUNIT_ASSERT(fabs(itsComponent.fluxZero()-flux)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.alpha()-alpha)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.maj()-maj)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.min()-min)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.pa()-pa)<EPSILON);
	      CPPUNIT_ASSERT(!itsComponent.isGuess());
	  }

	  void testFluxes(){
	      const double f1400 = flux * pow(1400./itsComponent.nuZero(), alpha);
	      const double f1000 = flux * pow(1000./itsComponent.nuZero(), alpha);
	      const double f2000 = flux * pow(2000./itsComponent.nuZero(), alpha);

	      CPPUNIT_ASSERT(fabs(itsComponent.fluxZero()-flux)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.flux(1400.)-f1400)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.flux(1000.)-f1000)<EPSILON);
	      CPPUNIT_ASSERT(fabs(itsComponent.flux(2000.)-f2000)<EPSILON);
	  }

      };
  }
}

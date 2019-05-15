/// @file
/// 
///
/// @copyright (c) 2007 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>


// ImplCalWeightSolver -  implementation of the algorithm which 
// solves for the best FPA weights for an optimum calibration on a
// given sky model

#include <casacore/casa/aips.h>
#include <components/ComponentModels/SkyComponent.h>
#include <components/ComponentModels/ComponentList.h>
#include <casacore/measures/Measures/MDirection.h>
#include <casacore/casa/Exceptions/Error.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/images/Images/PagedImage.h>
#include <casacore/casa/BasicSL/Complex.h>


#include <boost/shared_ptr.hpp>
#include <gridding/IBasicIllumination.h>

class ImplCalWeightSolver {
    casacore::MDirection pc; // dish pointing centre
    casacore::ComponentList cl; // model of sky brightness
    mutable casacore::Matrix<casacore::Complex> vismatrix; // visibilities for each element (column)
                                           // and measurement (row)
    casacore::ImageInterface<casacore::Float> *vp_real; // voltage pattern of a single element
    casacore::ImageInterface<casacore::Float> *vp_imag; // voltage pattern of a single element
       
public:
    //static const double lambda=0.2;   // wavelength in metres
    ImplCalWeightSolver() throw();
    ~ImplCalWeightSolver() throw(casacore::AipsError);
    // set up calculation for a given pointing centre and sky model
    void setSky(const casacore::MDirection &ipc, 
		const casacore::String &clname) throw(casacore::AipsError);
		
	/// @brief make synthetic beam
	/// @details This method constructs synthetic primary beam for the given weights.
	/// @param[in] name output image name
	/// @param[in] weights vector of weights
	void makeSyntheticPB(const std::string &name, 
	                     const casacore::Vector<casacore::Complex> &weights);
		
    // set up the voltage pattern from a disk-based image
    void setVP(const casacore::String &namer,const casacore::String &namei) 
	      throw(casacore::AipsError); 
    // main method
    casacore::Matrix<casacore::Complex>
         solveWeights(const casacore::Matrix<casacore::Double> &feed_offsets,
		      const casacore::Vector<casacore::Double> &uvw) const
	                            throw(casacore::AipsError);
   // solve for eigenvectors for the VP matrix. The first vector
   // (column=0) corresponds to the maximum attainable flux,
   // the last one (column=Nfeeds-1) corresponds to the
   // weight set for an optimal rejection of all known sources.
   // pa - parallactic angle to rotate all source offsets (in radians)
   // if skycat!="", a table with this name will be filled with the offsets
   // w.r.t. the dish pointing centre
   casacore::Matrix<casacore::Complex>
         eigenWeights(const casacore::Matrix<casacore::Double> &feed_offsets,
	 casacore::Double pa = 0., const casacore::String &skycat="")
	                        const throw(casacore::AipsError);

   // solve for a basis in the space of weights, which is best for calibration
   // in the sense that the flux from known sources is maximized
   // The result matrix contains the basis vectors in its columns
   // ndim is a number of basis vectors required (should be less than or
   // equal to the number of feeds.
   // pa - parallactic angle to rotate all source offsets (in radians)
   // if skycat!="", a table with this name will be filled with the offsets
   // w.r.t. the dish pointing centre
   casacore::Matrix<casacore::Complex>
         calBasis(const casacore::Matrix<casacore::Double> &feed_offsets,
	          casacore::uInt ndim, casacore::Double pa=0.,
		  const casacore::String &skycat="")
		                      const throw(casacore::AipsError);
				      
protected:
    // calculate visibility matrix for given feed_offsets
    // uvw - a vector with the uvw coordinates (in the units of wavelength)
    // vismatrix will have visibilities for each element (column) and 
    // measurement (row)
    void formVisMatrix(const casacore::Matrix<casacore::Double> &feed_offsets,
		       const casacore::Vector<casacore::Double> &uvw) const
	                throw(casacore::AipsError);

    // this version fills vismatrix with sum(F_i E_k*E_l^H), where 
    // E is the voltage pattern value at the source position and
    // F_i is the flux of the ith source
    // pa - parallactic angle to rotate all source offsets (in radians)
    // if skycat!="", a table with this name will be filled with the offsets
    // w.r.t. the dish pointing centre
    void formVPMatrix(const casacore::Matrix<casacore::Double> &feed_offsets,
                casacore::Double pa = 0., const casacore::String &skycat = "")
	          const  throw(casacore::AipsError);
    
    
    // an auxiliary function to extract a Value of the Voltage Pattern
    // at the given offset (in radians). Return True if successful, and
    // False if the requested offset lies outside the model
    casacore::Bool getVPValue(casacore::Complex &val, casacore::Double l, casacore::Double m)
                            const throw(casacore::AipsError);
private:
    boost::shared_ptr<askap::synthesis::IBasicIllumination> itsIllumination;
};

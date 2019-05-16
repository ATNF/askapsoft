/// @file
/// 
/// @brief utilities related to illumination pattern
/// @details This class is written for experiments with eigenbeams and synthetic beams.
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

#include <calweightsolver/IlluminationUtils.h>

#include <casacore/images/Images/PagedImage.h>
//#include <casacore/coordinates/Coordinates/StokesCoordinate.h>
//#include <casacore/coordinates/Coordinates/SpectralCoordinate.h>
#include <casacore/coordinates/Coordinates/DirectionCoordinate.h>
#include <casacore/coordinates/Coordinates/LinearCoordinate.h>
#include <casacore/coordinates/Coordinates/Projection.h>
#include <casacore/measures/Measures/MDirection.h>
#include <casacore/lattices/Lattices/ArrayLattice.h>
#include <casacore/casa/Arrays/ArrayMath.h>
#include <gridding/BasicCompositeIllumination.h>
#include <casacore/scimath/Mathematics/RigidVector.h>
#include <fft/FFTWrapper.h>

#include <Common/ParameterSet.h>


#include <gridding/UVPattern.h>

#include <askap/askap/AskapError.h>
#include <askap/askap/AskapLogging.h>
#include <casacore/casa/Logging/LogIO.h>
#include <askap/askap/Log4cxxLogSink.h>

namespace askap {
namespace accessors{}}

// after logger has been initialised
#include <gridding/AProjectGridderBase.h>

namespace askap {
namespace synthutils {

/// @brief constructor
/// @details 
/// @param[in] illum illumination pattern to work with 
/// @param[in] size desired image size
/// @param[in] cellsize uv-cell size 
/// @param[in] oversample oversampling factor (default 1) 
IlluminationUtils::IlluminationUtils(const boost::shared_ptr<synthesis::IBasicIllumination> &illum,
        size_t size, double cellsize, size_t oversample) :
        itsElementIllumination(illum), itsIllumination(illum), itsSize(size), 
        itsCellSize(cellsize), itsOverSample(oversample)
{}


/// @brief constructor from a parset file 
/// @details
/// This version extracts all required parameters from the supplied parset file 
/// using the same factory, which provides illumination patterns for gridders.
/// @param[in] parset parset file name 
IlluminationUtils::IlluminationUtils(const std::string &parset)
{
  LOFAR::ParameterSet params(parset);
  itsElementIllumination = itsIllumination = synthesis::AProjectGridderBase::makeIllumination(params);
  itsCellSize = params.getDouble("cellsize");


  const int size = params.getInt32("size"); 
  ASKAPCHECK(size>0,"Size is supposed to be positive, you have "<<size);
  itsSize = size_t(size);
  
  const int oversample = params.getInt32("oversample");
  ASKAPCHECK(oversample>0,"Oversample is supposed to be positive, you have "<<oversample);
  itsOverSample = size_t(oversample);  
}

/// @brief switch to the single element case
void IlluminationUtils::useSingleElement()
{
  itsIllumination = itsElementIllumination;
}

/// @brief switch the code to synthetic pattern
/// @details
/// @param[in] offsets a matrix with offsets of the elements (number of columns should be 2,
/// number of rows is the number of elements).
/// @param[in] weights a vector of complex weights
void IlluminationUtils::useSyntheticPattern(const casacore::Matrix<double> &offsets, 
                            const casacore::Vector<casacore::Complex> &weights)
{
  ASKAPASSERT(itsElementIllumination);
  ASKAPASSERT(offsets.ncolumn() == 2);
  ASKAPASSERT(offsets.nrow() == weights.nelements());
  casacore::Vector<casacore::RigidVector<casacore::Double, 2> > elementOffsets(offsets.nrow());
  for (casacore::uInt elem = 0; elem<elementOffsets.nelements(); ++elem) {
       elementOffsets[elem](0) = offsets(elem,0);
       elementOffsets[elem](1) = offsets(elem,1);
  }
  itsIllumination.reset(new synthesis::BasicCompositeIllumination(itsElementIllumination,
                   elementOffsets, weights));
}
   
/// @brief save the pattern into an image
/// @details 
/// @param[in] name file name
/// @param[in] what type of the image requested, e.g. amplitude (default),
/// real, imag, phase, complex. Minimum match applies.
void IlluminationUtils::save(const std::string &name, const std::string &what)
{
   ASKAPDEBUGASSERT(itsIllumination);
   const double freq=1.4e9;
   synthesis::UVPattern pattern(itsSize, itsSize, itsCellSize, itsCellSize, itsOverSample);
   itsIllumination->getPattern(freq, pattern);
   
   casacore::Matrix<double> xform(2,2);
   xform = 0.; xform.diagonal() = 1.;
   casacore::Vector<casacore::String> names(2);
   names[0]="U"; names[1]="V";
   
   casacore::Vector<double> increment(2);
   increment[0]=-itsCellSize/double(itsOverSample);
   increment[1]=itsCellSize/double(itsOverSample);
   
   casacore::LinearCoordinate linear(names, casacore::Vector<casacore::String>(2,"lambda"),
          casacore::Vector<double>(2,0.), increment, xform, 
          casacore::Vector<double>(2,double(itsSize)/2));
   
   casacore::CoordinateSystem coords;
   coords.addCoordinate(linear);    
   
   casacore::Array<casacore::Complex> buf(pattern.pattern().shape());
   casacore::convertArray<casacore::Complex, casacore::DComplex>(buf,pattern.pattern());
   saveComplexImage(name,coords,buf,what);
}

/// @brief save the voltage pattern into an image
/// @details 
/// @param[in] name file name
/// @param[in] what type of the image requested, e.g. amplitude (default),
/// real, imag, phase, complex. Minimum match applies.
void IlluminationUtils::saveVP(const std::string &name, const std::string &what)
{
   ASKAPDEBUGASSERT(itsIllumination);
   ASKAPASSERT(itsOverSample>=1);
   ASKAPASSERT(itsSize%2 == 0);
   const double freq=1.4e9;
   synthesis::UVPattern pattern(itsSize, itsSize, itsCellSize, itsCellSize, itsOverSample);
   itsIllumination->getPattern(freq, pattern);
   casacore::Array<casacore::DComplex> scratch(pattern.pattern().copy());
   scimath::fft2d(scratch,false);
   scratch/=casacore::max(casacore::abs(scratch));
   
   casacore::Matrix<double> xform(2,2);
   xform = 0.; xform.diagonal() = 1.;
   double angularCellSize = double(itsOverSample)/itsCellSize/double(itsSize);
   casacore::IPosition blc(scratch.shape().nelements(),0);
   blc[0]=blc[1]=itsSize*(itsOverSample-1)/itsOverSample/2;
   casacore::IPosition length(scratch.shape());
   length[0]=length[1]=itsSize/itsOverSample;
   casacore::Array<casacore::DComplex> slice = scratch(casacore::Slicer(blc,length));
   casacore::DirectionCoordinate azel(casacore::MDirection::AZEL, casacore::Projection::SIN, 0.,0.,
                 -angularCellSize, angularCellSize, 
                 xform, length[0]/2, length[1]/2);
      
   casacore::CoordinateSystem coords;
   coords.addCoordinate(azel);    
   casacore::Array<casacore::Complex> buf(slice.shape());
   casacore::convertArray<casacore::Complex, casacore::DComplex>(buf,slice);
   saveComplexImage(name,coords,buf,what);
 }

/// @brief save complex array into an image
/// @details 
/// @param[in] name file name
/// @param[in] coords coordinate system
/// @param[in] arr array to take the data from
/// @param[in] what type of the image requested, e.g. amplitude (default),
/// real, imag, phase, complex. Minimum match applies.
void IlluminationUtils::saveComplexImage(const std::string &name, 
            const casacore::CoordinateSystem &coords, 
            const casacore::Array<casacore::Complex> &arr,
            const std::string &what)
{
  if (what.find("complex") == 0) {
       casacore::PagedImage<casacore::Complex> result(casacore::TiledShape(arr.shape()), coords, name);
       casacore::ArrayLattice<casacore::Complex> patternLattice(arr);                
       result.setUnits("Jy/pixel");             
   } else {
       casacore::PagedImage<casacore::Float> result(casacore::TiledShape(arr.shape()), coords, name);
       casacore::Array<casacore::Float> workArray;
       if (what.find("amp")==0) {
           workArray = casacore::amplitude(arr);
       } else if (what.find("real") == 0) {
           workArray = casacore::real(arr);
       } else if (what.find("imag") == 0) {
           workArray = casacore::imag(arr);
       } else if (what.find("phase") == 0) {
           workArray = casacore::phase(arr);
       } else {
          ASKAPTHROW(AskapError, "Unknown type of image requested from IlluminationUtils::saveComplexImage ("<<
                     what<<")");
       }
       casacore::ArrayLattice<casacore::Float> patternLattice(workArray);
       result.copyData(patternLattice);
       result.setUnits("Jy/pixel");             
   }
}

}

}


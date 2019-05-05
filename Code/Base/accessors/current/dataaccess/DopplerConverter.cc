/// @file DopplerConverter.cc
/// @brief A class for interconversion between frequencies
/// and velocities.
/// @details This is an implementation of a relatively low-level
/// interface, which is used within the implementation of the data
//// accessor. The end user interacts with the IDataConverter class only.
///
/// The idea behind this class is very similar to CASA's VelocityMachine,
/// but we require a bit different interface to use the class efficiently
/// (and the interface conversion would be equivalent in complexity to
/// the transformation itself). Hence, we will use this class 
/// instead of the VelocityMachine
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
///

/// CASA include
#include <casacore/casa/Quanta/MVDoppler.h>
// temporary
#include <casacore/casa/Exceptions/Error.h>

/// own includes
#include <dataaccess/DopplerConverter.h>

using namespace askap;
using namespace askap::accessors;


/// constructor
/// @param restFreq The rest frequency used for interconversion between
///                 frequencies and velocities
/// @param velType velocity (doppler) type (i.e. radio, optical)
/// Default is radio definition.
DopplerConverter::DopplerConverter(const casacore::MVFrequency &restFreq,
                                   casacore::MDoppler::Types velType) :
     itsToBettaConv(velType, casacore::MDoppler::BETA),
     itsFromBettaConv(casacore::MDoppler::BETA, velType),
     itsRestFrequency(restFreq.getValue()) {}

/// setting the measure frame doesn't make sense for this class
/// because we're not doing conversions here. This method is empty.
/// Defined here to make the compiler happy.
///
/// @param frame  MeasFrame object (can be constructed from
///               MPosition or MEpoch on-the-fly). Not used.
void DopplerConverter::setMeasFrame(const casacore::MeasFrame &)
{
}

/// convert specified frequency to velocity in the same reference
/// frame. Velocity definition (i.e. optical or radio, etc) is
/// determined by the implementation class.
///
/// @param freq an MFrequency measure to convert.
/// @return a reference on MRadialVelocity object with the result
const casacore::MRadialVelocity&
DopplerConverter::operator()(const casacore::MFrequency &freq) const
{  
  casacore::Double t=freq.getValue().getValue(); // extract frequency in Hz

  // need to change the next line to a proper Askap error when available
  ASKAPDEBUGASSERT(t!=0);
  
  t/=itsRestFrequency; // form nu/nu_0
  t*=t; // form (nu/nu_0)^2  
  itsRadialVelocity=casacore::MRadialVelocity::fromDoppler(itsFromBettaConv(
                 casacore::MVDoppler((1.-t)/(1.+t))),
                 freqToVelType(casacore::MFrequency::castType(
		               freq.getRef().getType())));
  return itsRadialVelocity;		 
}


/// convert specified velocity to frequency in the same reference
/// frame. Velocity definition (i.e. optical or radio, etc) is
/// determined by the implementation class.
///
/// @param vel an MRadialVelocity measure to convert.
/// @return a reference on MFrequency object with the result
const casacore::MFrequency&
DopplerConverter::operator()(const casacore::MRadialVelocity &vel) const
{
  itsFrequency=casacore::MFrequency::fromDoppler(
        itsToBettaConv(casacore::MVDoppler(vel.getValue().get())),
	               casacore::MVFrequency(itsRestFrequency),
		       velToFreqType(casacore::MRadialVelocity::castType(
		                    vel.getRef().getType())));
  return itsFrequency;		       
}

/// convert frequency frame type to velocity frame type
/// @param type frequency frame type to convert
/// @return resulting velocity frame type
///
/// Note, an exception is thrown if the the frame type is
/// MFrequency::REST (it doesn't make sense to always return zero
/// velocity).
casacore::MRadialVelocity::Types
DopplerConverter::freqToVelType(casacore::MFrequency::Types type)
                                throw(DataAccessLogicError)
{
  switch(type) {
    case casacore::MFrequency::LSRK: return casacore::MRadialVelocity::LSRK;
    case casacore::MFrequency::LSRD: return casacore::MRadialVelocity::LSRD;
    case casacore::MFrequency::BARY: return casacore::MRadialVelocity::BARY;
    case casacore::MFrequency::GEO: return casacore::MRadialVelocity::GEO;
    case casacore::MFrequency::TOPO: return casacore::MRadialVelocity::TOPO;
    case casacore::MFrequency::GALACTO: return casacore::MRadialVelocity::GALACTO;
    case casacore::MFrequency::LGROUP: return casacore::MRadialVelocity::LGROUP;
    case casacore::MFrequency::CMB: return casacore::MRadialVelocity::CMB;
    default: throw DataAccessLogicError("DopplerConverter: Unable to convert "
                              "freqency frame type to velocity frame type");
  };

  // to keep the compiler happy. It should never go this far.
  return casacore::MRadialVelocity::LSRK; 
}

/// convert velocity frame type to frequency frame type
/// @param type velocity frame type to convert
/// @return resulting frequency frame type
casacore::MFrequency::Types
DopplerConverter::velToFreqType(casacore::MRadialVelocity::Types type)
                                             throw(DataAccessLogicError)
{
  switch(type) {
    case casacore::MRadialVelocity::LSRK: return casacore::MFrequency::LSRK;
    case casacore::MRadialVelocity::LSRD: return casacore::MFrequency::LSRD;
    case casacore::MRadialVelocity::BARY: return casacore::MFrequency::BARY;
    case casacore::MRadialVelocity::GEO: return casacore::MFrequency::GEO;
    case casacore::MRadialVelocity::TOPO: return casacore::MFrequency::TOPO;
    case casacore::MRadialVelocity::GALACTO: return casacore::MFrequency::GALACTO;
    case casacore::MRadialVelocity::LGROUP: return casacore::MFrequency::LGROUP;
    case casacore::MRadialVelocity::CMB: return casacore::MFrequency::CMB;
    default: throw DataAccessLogicError("DopplerConverter: Unable to convert "
                             "velocity frame type to frequency frame type");
  };

  // to keep the compiler happy. It should never go this far.
  return casacore::MFrequency::LSRK;
}

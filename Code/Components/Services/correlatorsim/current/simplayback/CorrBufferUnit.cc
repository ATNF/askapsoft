/// @file CorrBufferUnit.cc
///
/// @copyright (c) 2015 CSIRO
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
/// @author Paulus Lahur <paulus.lahur@csiro.au>
///
/// Buffer unit that provides convenient intermediate data format during
/// conversion from measurement set to datagram

#include <iostream>
#include "CorrBufferUnit.h"

using namespace askap;
using namespace askap::cp;
using namespace std;


// Forward declaration

bool sameFloat(const float a, const float b, const float small);



CorrBufferUnit::CorrBufferUnit()
{
	init();
}



CorrBufferUnit::~CorrBufferUnit()
{
}



void CorrBufferUnit::init()
{
	vis.real = 0.0;
	vis.imag = 0.0;
#ifdef NEW_BUFFER
	status = CorrBufferUnitStatus::EMPTY;
#endif
}



void CorrBufferUnit::insert(const FloatComplex& visIn)
{
#ifdef NEW_BUFFER
	if (status == CorrBufferUnitStatus::EMPTY) {
		vis.real = visIn.real;
		vis.imag = visIn.imag;
		status = CorrBufferUnitStatus::FULL;
	}
#ifdef VERBOSE
	else {
		cout << "WARNING in CorrBufferUnit::insert: fail" << endl;
	}
#endif

#else   // old buffer
    vis.real = visIn.real;
    vis.imag = visIn.imag;
#endif
}


void CorrBufferUnit::insert(const float& realIn, const float& imagIn)
{
#ifdef NEW_BUFFER
	if (status == CorrBufferUnitStatus::EMPTY) {
    	vis.real = realIn;
    	vis.imag = imagIn;
		status = CorrBufferUnitStatus::FULL;
	}
#ifdef VERBOSE
	else {
		cout << "WARNING in CorrBufferUnit::insert: fail" << endl;
	}
#endif

#else   // old buffer
    vis.real = realIn;
    vis.imag = imagIn;
#endif
}



void CorrBufferUnit::insert(const CorrBufferUnit& bufferUnitIn)
{
#ifdef NEW_BUFFER
    if (status == CorrBufferUnitStatus::EMPTY) {
        vis.real = bufferUnitIn.vis.real;
        vis.imag = bufferUnitIn.vis.imag;
        status = bufferUnitIn.status;
    }
#ifdef VERBOSE
    else {
        cout << "WARNING in CorrBufferUnit::insert: fail" << endl;
    }
#endif

#else   // old buffer
    vis.real = bufferUnitIn.vis.real;
    vis.imag = bufferUnitIn.vis.imag;
#endif
}



/*
FloatComplex CorrBufferUnit::remove()
{
    if (status == CorrBufferUnitStatus::FULL) {
        FloatComplex visOut;
        visOut.real = vis.real;
        visOut.imag = vis.imag;
        init();
        return (visOut);
    }
#ifdef VERBOSE
    else {
        cout << "WARNING in CorrBufferUnit::remove: no data" << endl;
    }
#endif
}
*/


FloatComplex CorrBufferUnit::query() const
{
#ifdef NEW_BUFFER
#ifdef VERBOSE
	if (status != CorrBufferUnitStatus::FULL) {
		cout << "WARNING in CorrBufferUnit::query: no data" << endl;
	}
#endif
#endif
	return(vis);
}


#ifdef NEW_BUFFER

uint32_t CorrBufferUnit::queryStatus() const
{
	return (status);
}



bool isFull() const
{
	if (status == CorrBufferUnitStatus::FULL) {
		return true;
	}
	else {
		return false;
	}
}



bool isEmpty() const
{
	if (status == CorrBufferUnitStatus::EMPTY) {
		return true;
	}
	else {
		return false;
	}
}

#endif


bool CorrBufferUnit::isSame(const CorrBufferUnit& unit2, 
		const float small) const
{
#ifdef NEW_BUFFER
	// If both contains data, compare them
	if ((status == CorrBufferUnitStatus::FULL) &&
			(unit2.status == CorrBufferUnitStatus::FULL)) {
		return ((sameFloat(vis.real, unit2.vis.real, small) &&
				sameFloat(vis.imag, unit2.vis.imag, small)));
	}

	if (status == unit2.status) {
#ifdef VERBOSE
		cout << "CorrBufferUnit::isSame: no data but same status" << endl;
#endif
		return (true);
	}
	else {
#ifdef VERBOSE
		cout << "CorrBufferUnit::isSame: different status" << endl;
#endif
        return (false);
	}

#else   // old buffer
    return ((sameFloat(vis.real, unit2.vis.real, small) &&
            sameFloat(vis.imag, unit2.vis.imag, small)));
#endif
}



void CorrBufferUnit::print() const
{
#ifdef NEW_BUFFER
	if (status == CorrBufferUnitStatus::FULL) {
    	cout << "vis: [" << vis.real << ", " << vis.imag << "] " << endl;
	}
#ifdef VERBOSE
	else {
		cout << "WARNING in CorrBufferUnit::print: no data" << endl;
    }
#endif

#else   // old buffer
    cout << "vis: [" << vis.real << ", " << vis.imag << "] " << endl;
#endif
}


// Internal functions


// Return true if 2 floating numbers are (essentially) the same, that is,
// within a specified small tolerance.

bool sameFloat(const float a, const float b, const float small) 
{
	if ((a < b-small) || (a > b+small)) return false;
	else return true;
}


#ifdef VERBOSE
#undef VERBOSE
#endif

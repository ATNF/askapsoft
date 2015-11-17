/// @file CorrBuffer.cc
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
#include "CorrBuffer.h"

using namespace askap;
using namespace askap::cp;
using namespace std;


CorrBuffer::CorrBuffer () : timeStamp(0), ready(false)
{
}



CorrBuffer::~CorrBuffer ()
{
}



void CorrBuffer::init (uint32_t nCorrProd, uint32_t nChannel)
{
    data.resize(nCorrProd);
    for (uint32_t i = 0; i < nCorrProd; ++i) {
        data[i].resize(nChannel);
    }
}



void CorrBuffer::print ()
{
    cout << "Buffer timestamp: " << timeStamp << endl;
    cout << "Buffer ready    : " << ready << endl;
    cout << "Buffer content  : " << endl;
    cout << endl;
    for (int corrProd = 0; corrProd < data.size(); ++corrProd) {
        cout << "----------------------------------------------------" << endl;
        cout << "corr product " << corrProd << endl;
        for (int channel = 0; channel < data[corrProd].size(); ++channel) {
            cout << "corr product " << corrProd << 
                    ", channel " << channel << endl;
            data[corrProd][channel].print();
        }
    }
    cout << "=====================================================" << endl;
}



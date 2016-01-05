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
#include "askap/AskapError.h"

using namespace askap;
using namespace askap::cp;
using namespace std;


CorrBuffer::CorrBuffer() : timeStamp(0), beam(0), ready(false), nChanMeas(0),
        nCard(0)
{
}



CorrBuffer::~CorrBuffer()
{
}



void CorrBuffer::init(uint32_t nCorrProd, uint32_t nChannel)
{
    ASKAPCHECK(nCorrProd > 0, "Illegal correlation product count " <<
            nCorrProd);
    ASKAPCHECK(nChannel > 0, "Illegal channel count " << nChannel);

    data.resize(nCorrProd);
    for (uint32_t cp = 0; cp < nCorrProd; ++cp) {
        data[cp].resize(nChannel);
    }
    corrProdIsOriginal.resize(nCorrProd);
    corrProdIsFilled.resize(nCorrProd);
    freqId.resize(nChannel);
    reset();
    //cout << "check size: " << corrProdIsFilled.size() << endl;
}



void CorrBuffer::reset()
{
    ready = false;
    for (uint32_t cp = 0; cp < corrProdIsFilled.size(); ++cp) {
        corrProdIsOriginal[cp] = false;
        corrProdIsFilled[cp] = false;
    }
}



int32_t CorrBuffer::findNextEmptyCorrProd(int32_t startCP)
{
    for (uint32_t cp = startCP+1; cp < data.size(); ++cp) {
        if (!corrProdIsFilled[cp]) {
            return cp;
        }
    }
    return (-1); // cannot find the next empty correlation product
}



int32_t CorrBuffer::findNextOriginalCorrProd(int32_t startCP)
{
    for (uint32_t cp = startCP+1; cp < data.size(); ++cp) {
        if (corrProdIsOriginal[cp]) {
            return cp;
        }
    }
    return (-1); // cannot find the next original correlation product
}



void CorrBuffer::copyCorrProd(int32_t source, int32_t destination) 
{
    for (uint32_t chan = 0; chan < data[0].size(); ++chan) {
        data[destination][chan] = data[source][chan];
    }
    corrProdIsFilled[destination] = true;
}



void CorrBuffer::print ()
{
    cout << "Buffer contents" << endl;
    cout << "Time stamp                      : " << timeStamp << endl;
    cout << "Beam                            : " << beam << endl;
    cout << "Ready                           : " << ready << endl;
    cout << "Channel count in measurement set: " << nChanMeas << endl;
    cout << "Card count                      : " << nCard << endl;
    for (uint32_t count = 0; count < freqId.size(); ++count) {
        cout << count << ": ";
        freqId[count].print();
    }
    cout << endl;
    /*
    for (uint32_t corrProd = 0; corrProd < data.size(); ++corrProd) {
        cout << "----------------------------------------------------" << endl;
        cout << "corr product " << corrProd << endl;
        for (uint32_t channel = 0; channel < data[corrProd].size(); ++channel) {
            cout << "corr product " << corrProd << 
                    ", channel " << channel << endl;
            data[corrProd][channel].print();
        }
    }
    cout << "=====================================================" << endl;
    */
}



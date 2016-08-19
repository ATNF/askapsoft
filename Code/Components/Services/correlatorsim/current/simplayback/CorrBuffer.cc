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
#ifndef NEW_BUFFER
    corrProdIsOriginal.resize(nCorrProd);
    corrProdIsFilled.resize(nCorrProd);
#endif
    freqId.resize(nChannel);
    reset();
    //cout << "check size: " << corrProdIsFilled.size() << endl;
}



void CorrBuffer::reset()
{
    ready = false;
#ifndef NEW_BUFFER
    for (uint32_t cp = 0; cp < corrProdIsFilled.size(); ++cp) {
        corrProdIsOriginal[cp] = false;
        corrProdIsFilled[cp] = false;
    }
#endif
	for (uint32_t i = 0; i < data.size(); ++i) {
		for (uint32_t j = 0; j < data[i].size(); ++j) {
			data[i][j].init();
		}	
	}
}



#ifdef NEW_BUFFER

void CorrBuffer::insert(uint32_t cp, uint32_t chan, const FloatComplex& vis)
{
    ASKAPCHECK(cp < data.size(),"Correlation product is out of range");
    ASKAPCHECK(chan < data[0].size(),"Channel is out of range");
	data[cp][chan].insert(vis);
}



boost::optional<uint32_t> CorrBuffer::findNextEmptyCorrProd(uint32_t startCP, 
		uint32_t chan) const
{
	ASKAPCHECK(chan < data[0].size(),"Channel is out of range");
    for (uint32_t cp = startCP; cp < data.size(); ++cp) {
        if (data[cp][chan].isEmpty()) {
            return cp;
        }
    }
    return (boost::none); // cannot find the next empty correlation product
}

#else

int32_t CorrBuffer::findNextEmptyCorrProd(int32_t startCP) const
{
	// any negative input will start search from the beginning
    for (uint32_t cp = max(0,startCP+1); cp < data.size(); ++cp) {
        if (!corrProdIsFilled[cp]) {
            return cp;
        }
    }
    return (-1); // cannot find the next empty correlation product
}

#endif



#ifdef NEW_BUFFER

boost::optional<uint32_t> CorrBuffer::findNextFullCorrProd(uint32_t startCP,
        uint32_t chan) const
{
    ASKAPCHECK(chan < data[0].size(),"Channel is out of range");
    for (uint32_t cp = startCP; cp < data.size(); ++cp) {
        if (data[cp][chan].isFull()) {
            return cp;
        }
    }
    return (boost::none); // cannot find the next empty correlation product
}

#else

int32_t CorrBuffer::findNextOriginalCorrProd(int32_t startCP) const
{
	// any negative input will start search from the beginning
    for (uint32_t cp = max(0,startCP+1); cp < data.size(); ++cp) {
        if (corrProdIsOriginal[cp]) {
            return cp;
        }
    }
    return (-1); // cannot find the next original correlation product
}

#endif



#ifdef NEW_BUFFER

boost::optional<uint32_t> CorrBuffer::findNextEmptyChannel(uint32_t cp, 
        uint32_t startChan) const
{
    ASKAPCHECK(CP < data.size(),"Correlation product is out of range");
    for (uint32_t cc = startChan; cc < data[0].size(); ++cc) {
        if (data[cp][cc].isEmpty()) {
            return cc;      // Found the next empty channel
        }
    }
    return (boost::none);   // cannot find the next empty channel
}



boost::optional<uint32_t> CorrBuffer::findNextFullChannel(uint32_t cp, 
        uint32_t startChan) const
{
    ASKAPCHECK(cp < data.size(),"Correlation product is out of range");
    for (uint32_t cc = startChan; cc < data[0].size(); ++cc) {
        if (data[cp][cc].isFull()) {
            return cc;      // Found the next full channel
        }
    }
    return (boost::none);   // cannot find the next full channel
}



void CorrBuffer::fillOneChannel(uint32_t chan) 
{
    ASKAPCHECK(chan < data[0].size(),"Channel is out of range");
	// Find the first corr product with data
    if (boost::optional<uint32_t> oSourceCP = findNextFullCorrProd(0,chan)) {
        sourceCP = *oSourceCP;
        // If this is not the first corr product in the array
        if (sourceCP > 0) {
            // Fill all preceding (empty) corr products
            for (uint32_t cp = 0; cp < sourceCP; ++cp) {
                data[cp][chan].insert(data[sourceCP][chan]);
            }
        }
        // Now fill all the following empty corr products
        uint32_t souceCP1 = sourceCP;
        for (uint32_t cp = sourceCP1 + 1; cp < data.size(); ++cp) {
            // If the corr product is empty, copy from the source
            if (data[cp][chan].isEmpty()) {
                data[cp][chan].insert(data[sourceCP][chan]);
            }
            // Else (if it contains data), get this as the new source
            else {
                sourceCP = cp;
            }
        }
    }
    // Else (not even a single corr product with data), 
    // just finish without doing anything.
}



void CorrBuffer::fillOneCorrProduct(uint32_t cp) 
{
    ASKAPCHECK(cp < data.size(),"Correlation product is out of range");
    // Find the first channel with data
    if (boost::optional<uint32_t> oSourceChan = findNextFullChannel(cp,0)) {
        sourceChan = *oSourceChan;
        // If this is not the first channel in the array
        if (sourceChan > 0) {
            // Fill all preceding (empty) channels
            for (uint32_t chan = 0; chan < sourceChan; ++chan) {
                data[cp][chan].insert(data[cp][sourceChan]);
            }
        }
        // Now fill all the following empty channels
        uint32_t sourceChan1 = sourceChan;
        for (uint32_t chan = sourceChan1 + 1; chan < data[0].size(); ++chan) {
            // If the channel is empty, copy from the source
            if (data[cp][chan].isEmpty()) {
                data[cp][chan].insert(data[cp][sourceChan]);
            }
            // Else (if it contains data), get this as the new source
            else {
                sourceChan = chan;
            }
        }
    }
    // Else (not even a single channel with data),
    // just finish without doing anything.
}

#endif



void CorrBuffer::copyCorrProd(int32_t source, int32_t target) 
{
    for (uint32_t chan = 0; chan < data[0].size(); ++chan) {
#ifdef NEW_BUFFER
		data[target][chan].insert(data[source][chan]);
#else
        data[target][chan] = data[source][chan];
#endif
    }
#ifndef NEW_BUFFER
    corrProdIsFilled[target] = true;
#endif
}



void CorrBuffer::copyChannel(int32_t source, int32_t target)
{
    for (uint32_t cp = 0; cp < data.size(); ++cp) {
#ifdef NEW_BUFFER
        data[cp][target].insert(data[cp][source]);
#else
        data[cp][target] = data[cp][source];
#endif
    }
}



uint32_t CorrBuffer::getRowCount() const
{
	return (data.size());
}



uint32_t CorrBuffer::getColumnCount() const
{
	return (data[0].size());
}



uint32_t CorrBuffer::countSameVisibility(const CorrBuffer& buffer2,
		const float small) const
{
	uint32_t nSame = 0;
	for (uint32_t i = 0; i < data.size(); ++i) {
		for (uint32_t j = 0; j < data[0].size(); j++) {
			if (data[i][j].isSame(buffer2.data[i][j],small)) {
				++nSame;
				//data[i][j].print();
			}
			else {
				cout << "Different entry in row " << i << ", column " <<
						j << endl; 
				data[i][j].print();
				buffer2.data[i][j].print();
				cout << "---------------------------------------" << endl;
			}
		}
	}
	return nSame;
}



void CorrBuffer::print() const
{
    cout << "Buffer contents" << endl;
    cout << "Time stamp                      : " << timeStamp << endl;
    cout << "Beam                            : " << beam << endl;
    cout << "Ready                           : " << ready << endl;
    cout << "Channel count in measurement set: " << nChanMeas << endl;
    cout << "Card count                      : " << nCard << endl;
}



void CorrBuffer::print(const string& option) const
{
    cout << "Buffer contents" << endl;
    cout << "Time stamp                      : " << timeStamp << endl;
    cout << "Beam                            : " << beam << endl;
    cout << "Ready                           : " << ready << endl;
    cout << "Channel count in measurement set: " << nChanMeas << endl;
    cout << "Card count                      : " << nCard << endl;

	if ((option == "freq") || (option == "all")) {
	    for (uint32_t count = 0; count < freqId.size(); ++count) {
    	    cout << count << ": ";
        	freqId[count].print();
		}
    	cout << endl;
    }

	if ((option == "vis") || (option == "all")) {
	    for (uint32_t corrProd = 0; corrProd < data.size(); ++corrProd) {
    	    cout << "----------------------------------------------------" << 
					endl;
        	//cout << "corr product " << corrProd << endl;
        	for (uint32_t channel = 0; channel < data[corrProd].size(); 
					++channel) {
            	cout << "corr product " << corrProd <<
                    	", channel " << channel << ", ";
            	data[corrProd][channel].print();
			}
        }
    	cout << "=====================================================" << endl;
    }
}


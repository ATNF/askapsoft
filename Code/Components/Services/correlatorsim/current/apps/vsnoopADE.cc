/// @file vsnoopADE.cc
///
/// @description
/// This program is used to receive the UDP visibility stream from the
/// correlator (or correlator control computer). It decoded the stream
/// and writes it to stdout.
/// 
/// @copyright (c) 2010 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

// Include package level header file
#include "askap_correlatorsim.h"

// System includes
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <complex>

// ASKAPsoft includes
#include "cpcommon/VisDatagramADE.h"
#include "boost/asio.hpp"
#include "boost/array.hpp"
#include "CommandLineParser.h"

using boost::asio::ip::udp;
using askap::cp::VisDatagramADE;
using namespace askap::cp;

// Globals
static unsigned int verbose = 0;
static unsigned long count = 0;

// When a SIGTERM is sent to the process this signal handler is called
// in order to report the number of UDP datagrams received
static void termination_handler(int signum)
{
    std::cout << "Received " << count << " datagrams" << std::endl;
    exit(0);
}

// Print the visibilities. Only called when verbose == 2
// The format of the output is:
//
// Visibilities:
//     baseline0: (0.123, 0.456)
//     baseline1: (0.789, 0.012)
//     ..
//     ..
static void printAdditional(const VisDatagramADE& v)
{
    std::cout << "\tVisibilities:" << std::endl;
    for (unsigned int i = 0; i < VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE; 
	++i) {
        std::cout << "\t\tbaseline" << v.baseline1+i << ": " << "("
            << v.vis[i].real << ", " << v.vis[i].imag << ")";
    }
}

// Print the contents of the payload (except the visibilities). This is only
// called when verbose == 2.
// The format of the output is:
//
// Timestamp:  4679919826828364
//    Slice:      0
//    BaselineID: 0
//    BeamID:     0
//    ..
//    ..
static void printPayload(const VisDatagramADE& v)
{
    std::cout << "Timestamp:\t" << v.timestamp << std::endl;
    std::cout << "\tSlice:\t\t" << v.slice << std::endl;
    std::cout << "\tBlock:\t" << v.block << std::endl;
    std::cout << "\tCard:\t" << v.card << std::endl;
    std::cout << "\tChannel:\t" << v.channel << std::endl;
    std::cout << "\tFreq:\t" << v.freq << std::endl;
    std::cout << "\tBeamID:\t" << v.beamid << std::endl;
    std::cout << "\tBaseline range:\t" << v.baseline1 << " ~ " << v.baseline2 << std::endl;

    if (verbose == 2) {
        printAdditional(v);
    }
    std::cout << std::endl;
}

// Print product ranges with non-zero data
// Note, only first beam and first channel are considered.
//
// Timestamp:  4679919826828364
//    Slice:      0
//    Non-zero products: 1 ~ 657
static void printNonZeroProducts(const VisDatagramADE &v)
{
  if ((v.beamid == 1) && (v.channel == 1)) {
       std::cout << "Timestamp:\t" << v.timestamp << std::endl;
       std::cout << "\tSlice:\t\t" << v.slice << std::endl;

       // obtain a list of product ranges with non-zero data
       // pair has first and last product in the range (could be the same)
       std::vector<std::pair<uint32_t, uint32_t> > productRanges;
       productRanges.reserve(VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE / 2);

       uint32_t count = 0;
       for (uint32_t product = v.baseline1, offset = 0; product <= v.baseline2; ++product, ++offset) {
            if (offset >= VisDatagramTraits<VisDatagramADE>::MAX_BASELINES_PER_SLICE) {
                std::cout<<"\tCorrupted baseline range" << std::endl;
                return;
            }
            const float threshold = 1e-10;
            const bool isNonZero = (std::abs(std::complex<float>(v.vis[offset].real, v.vis[offset].imag)) > threshold);
            if (isNonZero) {
                ++count;
                if (productRanges.size()) {
                    if (productRanges.back().second + 1 == product) {
                        productRanges.back().second = product;
                        continue;
                    }
                }
                productRanges.push_back(std::pair<uint32_t, uint32_t>(product, product));
            }
       }

       // print the list of non-zero products
       std::cout<<"\tNon-zero products:\t\t";
       if (productRanges.size() == 0) {
           std::cout<<"none"<<std::endl;
       } else {
           for (size_t i=0; i < productRanges.size(); ++i) {
                if (i>0) {
                    std::cout<<", ";
                }
                const std::pair<uint32_t, uint32_t> range = productRanges[i];
                if (range.first == range.second) {
                    std::cout<< range.first;
                } else {
                    std::cout<< range.first << " ~ " << range.second;
                }
           }
           std::cout<<" ("<<count<<" in total)"<<std::endl;
       }
  }
}

int main(int argc, char *argv[])
{
    // Parse additional command line parameters
    cmdlineparser::Parser parser;
    cmdlineparser::FlagParameter verbosePar("-v");
    cmdlineparser::FlagParameter veryVerbosePar("-vv");
    cmdlineparser::FlagParameter printNonZeroPar("-nz");
    cmdlineparser::FlaggedParameter<int> portPar("-p", 3000);
    parser.add(verbosePar, cmdlineparser::Parser::return_default);
    parser.add(veryVerbosePar, cmdlineparser::Parser::return_default);
    parser.add(printNonZeroPar, cmdlineparser::Parser::return_default);
    parser.add(portPar, cmdlineparser::Parser::return_default);

    try {
        parser.process(argc, const_cast<char**> (argv));
        if (verbosePar.defined()) verbose = 1;
        if (veryVerbosePar.defined()) verbose = 2;
    } catch (const cmdlineparser::XParserExtra&) {
        std::cerr << "usage: " << argv[0] << " [-v] [-vv] [-nz] [-p <udp port#>]" << std::endl;
        std::cerr << "  -v            \t Verbose, partially display payload" << std::endl;
        std::cerr << "  -vv           \t Very vebose, display entire payload" << std::endl;
        std::cerr << "  -nz           \t List products with non-zero data for first beam and channel"<<std::endl;
        std::cerr << "  -p <udp port#>\t UDP Port number to listen on" << std::endl;
        return 1;
    }

    // Setup a signal handler for SIGTERM
    signal(SIGTERM, termination_handler);

    // Create socket
    boost::asio::io_service io_service;
    udp::socket socket(io_service, udp::endpoint(udp::v4(), portPar));

    // Set an 16MB receive buffer to help deal with the bursty nature of the
    // communication
    boost::asio::socket_base::receive_buffer_size option(1024 * 1024 * 16);
    boost::system::error_code soerror;
    socket.set_option(option, soerror);
    if (soerror) {
        std::cerr << "Warning: Could not set socket option. "
            << " This may result in dropped packets" << std::endl;
    }

    // Create receive buffer
    VisDatagramADE vis;

    // Receive a buffer
    std::cout << "Listening on UDP port " << portPar << 
        " (press CTRL-C to exit)..." <<  std::endl;
    while (true) {
        udp::endpoint remote_endpoint;
        boost::system::error_code error;
        const size_t len = socket.receive_from(boost::asio::buffer(&vis, sizeof(VisDatagramADE)), remote_endpoint, 0, error);
        if (error) {
            throw boost::system::system_error(error);
        }
        if (len != sizeof(VisDatagramADE)) {
            std::cout << "Error: Failed to read a full VisDatagramADE struct" << std::endl;
            continue;
        }
        if (vis.version != VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION) {
            std::cout << "Version mismatch. Expected " << VisDatagramTraits<VisDatagramADE>::VISPAYLOAD_VERSION << " got " << vis.version << std::endl;
            continue;
        }
        if (printNonZeroPar.defined()) {
            printNonZeroProducts(vis);
        }
        if (verbose) {
            printPayload(vis);
        } else {
            if (count % 10000 == 0) {
                std::cout << "Received " << count << " datagrams" << std::endl;
            }
        }
        count++;
    }

    return 0;
}

/// @file
///
/// Implementation of base class functions for distribution of objects
/// for parameterisation.
///
/// @copyright (c) 2017 CSIRO
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#include <parallelanalysis/DistributedParameteriserBase.h>

#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askapparallel/AskapParallel.h>

#include <parallelanalysis/DuchampParallel.h>

#include <casainterface/CasaInterface.h>

#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
using namespace LOFAR::TYPES;

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".distribparambase");

namespace askap {
namespace analysis {

DistributedParameteriserBase::DistributedParameteriserBase(askap::askapparallel::AskapParallel& comms,
        const LOFAR::ParameterSet &parset,
        std::vector<sourcefitting::RadioSource> sourcelist):
    itsComms(&comms),
    itsInputList(sourcelist),
    itsTotalListSize(sourcelist.size())
{
    // Make a copy via makeSubset, as simple copy actually keeps a
    // pointer to the original ParameterSetImpl, so changing the new
    // one would change the original.
    itsReferenceParset = parset.makeSubset("");

    itsReferenceParset.replace("nsubx", "1");
    itsReferenceParset.replace("nsuby", "1");
    itsReferenceParset.replace("nsubz", "1");
    itsDP = boost::shared_ptr<DuchampParallel>(new DuchampParallel(comms, itsReferenceParset));
    itsDP->cube().setReconFlag(false);
    itsDP->getMetadata();
    itsCube = itsDP->pCube();
}

DistributedParameteriserBase::~DistributedParameteriserBase()
{
    itsDP.reset();
}

void DistributedParameteriserBase::distribute()
{
    if (itsComms->isParallel()) {

        if (itsComms->isMaster()) {
            // send objects in itsInputList to workers in round-robin fashion
            // broadcast 'finished' signal

            // First send total number of sources to all workers
            LOFAR::BlobString bs;
            LOFAR::BlobOBufString bob(bs);
            LOFAR::BlobOStream out(bob);
            bs.resize(0);
            bob = LOFAR::BlobOBufString(bs);
            out = LOFAR::BlobOStream(bob);
            out.putStart("DP", 1);
            out << itsTotalListSize;
            out.putEnd();
            for (int i = 1; i < itsComms->nProcs(); ++i) {
                itsComms->sendBlob(bs, i);
            }

            if (itsTotalListSize > 0) {
                for (size_t i = 0; i <= itsInputList.size(); i++) {
                    unsigned int rank = i % (itsComms->nProcs() - 1);
                    bs.resize(0);
                    LOFAR::BlobOBufString bob(bs);
                    LOFAR::BlobOStream out(bob);
                    out.putStart("DP", 1);
                    out << (i < itsInputList.size());
                    if (i < itsInputList.size()) {
                        out << itsInputList[i];
                    }
                    out.putEnd();
                    if (i < itsInputList.size()) {
                        itsComms->sendBlob(bs, rank + 1);
                    } else {
                        for (int i = 1; i < itsComms->nProcs(); ++i) {
                            itsComms->sendBlob(bs, i);
                        }
                    }
                }

            }

        } else {
            itsInputList.clear();
            // receive objects and put in itsInputList until receive 'finished' signal
            LOFAR::BlobString bs;

            itsComms->receiveBlob(bs, 0);
            LOFAR::BlobIBufString bib(bs);
            LOFAR::BlobIStream in(bib);
            int version = in.getStart("DP");
            ASKAPASSERT(version == 1);
            in >> itsTotalListSize;
            in.getEnd();

            if (itsTotalListSize > 0) {
                // now read individual sources
                bool isOK = true;
                itsInputList.clear();
                while (isOK) {
                    sourcefitting::RadioSource src;
                    itsComms->receiveBlob(bs, 0);
                    LOFAR::BlobIBufString bib(bs);
                    LOFAR::BlobIStream in(bib);
                    int version = in.getStart("DP");
                    ASKAPASSERT(version == 1);
                    in >> isOK;
                    if (isOK) {
                        in >> src;
                        src.haveNoParams();
                        src.setHeader(itsCube->header());
                        itsInputList.push_back(src);
                    }
                    in.getEnd();
                }
                ASKAPLOG_DEBUG_STR(logger, "Worker " << itsComms->rank() <<
                                   " received " << itsInputList.size() <<
                                   " objects to parameterise.");
            }

        }
    }
}


}
}


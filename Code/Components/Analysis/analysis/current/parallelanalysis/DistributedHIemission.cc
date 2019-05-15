/// @file
///
/// Handle the parameterisation of objects that require reading from a file on disk
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
#include <parallelanalysis/DistributedHIemission.h>
#include <parallelanalysis/DistributedParameteriserBase.h>

#include <askap_analysis.h>

#include <askap/askap/AskapLogging.h>
#include <askap/askap/AskapError.h>
#include <askap/askapparallel/AskapParallel.h>

#include <catalogues/CasdaHiEmissionObject.h>

#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
using namespace LOFAR::TYPES;

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".distribhiemission");

namespace askap {
namespace analysis {

DistributedHIemission::DistributedHIemission(askap::askapparallel::AskapParallel& comms,
                                             const LOFAR::ParameterSet &parset,
                                             std::vector<sourcefitting::RadioSource> sourcelist):
    DistributedParameteriserBase(comms, parset, sourcelist)
{
}

DistributedHIemission::~DistributedHIemission()
{
}


void DistributedHIemission::parameterise()
{
    
    if (itsComms->isWorker()) {

        // Master does not need to do this, as we get one HI object per
        // RadioSource object, so comparison can be done with input list.
        std::vector<sourcefitting::RadioSource>::iterator obj;
        for (obj = itsInputList.begin(); obj != itsInputList.end(); obj++) {
            CasdaHiEmissionObject object(*obj, itsReferenceParset);
            itsOutputList.push_back(object);
        }

    }

}

void DistributedHIemission::gather()
{
    if (itsComms->isParallel()) {

        if (itsTotalListSize > 0) {

            if (itsComms->isMaster()) {
                // for each worker, read completed objects until we get a 'finished' signal

                // now read back the sources from the workers
                LOFAR::BlobString bs;
                for (int n = 0; n < itsComms->nProcs() - 1; n++) {
                    int numSrc;
                    itsComms->receiveBlob(bs, n + 1);
                    LOFAR::BlobIBufString bib(bs);
                    LOFAR::BlobIStream in(bib);
                    int version = in.getStart("HIfinal");
                    ASKAPASSERT(version == 1);
                    in >> numSrc;
                    for (int i = 0; i < numSrc; i++) {
                        CasdaHiEmissionObject src;
                        in >> src;
                        itsOutputList.push_back(src);
                    }
                    in.getEnd();
                }

                // Make sure we have the correct amount of sources
                ASKAPASSERT(itsInputList.size() == itsOutputList.size());

                // sort by id:
                std::sort(itsOutputList.begin(), itsOutputList.end());


            } else { // WORKER
                // for each object in itsOutputList, send to master
                LOFAR::BlobString bs;
                bs.resize(0);
                LOFAR::BlobOBufString bob(bs);
                LOFAR::BlobOStream out(bob);
                out.putStart("HIfinal", 1);
                out << int(itsOutputList.size());
                for (size_t i = 0; i < itsOutputList.size(); i++) {
                    out << itsOutputList[i];
                }
                out.putEnd();
                itsComms->sendBlob(bs, 0);
            }

        }

    }

}

}


}

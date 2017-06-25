/// @file
///
/// Handle the parameterisation of objects that require reading from a file on disk
///
/// @copyright (c) 2014 CSIRO
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
#include <parallelanalysis/DistributedContinuumParameterisation.h>
#include <parallelanalysis/DistributedParameteriserBase.h>

#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askapparallel/AskapParallel.h>

#include <catalogues/CasdaIsland.h>
#include <catalogues/CasdaComponent.h>
#include <catalogues/Casda.h>
#include <components/AskapComponentImager.h>
#include <imageaccess/CasaImageAccess.h>

#include <casacore/casa/Arrays/Array.h>
#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
using namespace LOFAR::TYPES;

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".distribcontparam");

namespace askap {
namespace analysis {

DistributedContinuumParameterisation::DistributedContinuumParameterisation(askap::askapparallel::AskapParallel& comms,
                                                                           const LOFAR::ParameterSet &parset,
                                                                           std::vector<sourcefitting::RadioSource> sourcelist):
    DistributedParameteriserBase(comms, parset, sourcelist)
{
    std::string inputImage = parset.getString("image","");
    ASKAPCHECK(inputImage != "", "No image name provided in parset with parameter 'image'");

    DuchampParallel dp(comms,parset);
    dp.getMetadata();
    itsInputSlicer = analysisutilities::subsectionToSlicer(dp.cube().pars().section());
    analysisutilities::fixSlicer(itsInputSlicer, dp.cube().header().getWCS());
    
    casa::IPosition arrshape = itsInputSlicer.end();
    arrshape -= itsInputSlicer.start();
    arrshape += 1;
    itsComponentImage = casa::Array<float>(arrshape,0.);
}

DistributedContinuumParameterisation::~DistributedContinuumParameterisation()
{
}


void DistributedContinuumParameterisation::parameterise()
{

    if (itsComms->isWorker()) {

        // Master does not need to do this, as we get one HI object per
        // RadioSource object, so comparison can be done with input list.
        std::vector<sourcefitting::RadioSource>::iterator obj;
        for (obj = itsInputList.begin(); obj != itsInputList.end(); obj++) {
            CasdaIsland island(*obj, itsReferenceParset);
            itsIslandList.push_back(island);
            std::vector<casa::Gaussian2D<Double> > gaussians = obj->gaussFitSet(casda::componentFitType);
            ASKAPASSERT(gaussians.size() == obj->numFits());
            for (size_t i = 0; i < obj->numFits(); i++) {
                CasdaComponent component(*obj, itsReferenceParset, i, casda::componentFitType);
                itsComponentList.push_back(component);
                
                addToComponentImage(gaussians[i]);

            }
            
        }

    }

}

void DistributedContinuumParameterisation::addToComponentImage(casa::Gaussian2D<Double> &gauss)
{

    // Calculate region of influence for Gaussian - where is its flux >0?
    //
    float majorSigma = gauss.majorAxis() / (2. * M_SQRT2 * sqrt(M_LN2));
    float zeroPoint = majorSigma * sqrt(-2.*log(1. / (MAXFLOAT * gauss.height())));
    int xmin = std::max(lround(gauss.xCenter() - zeroPoint), 0L);
    int xmax = std::min(lround(gauss.xCenter() + zeroPoint), long(itsComponentImage.shape()[0] - 1));
    int ymin = std::max(lround(gauss.yCenter() - zeroPoint), 0L);
    int ymax = std::min(lround(gauss.yCenter() + zeroPoint), long(itsComponentImage.shape()[1] - 1));

    casa::Vector<double> pos(2);
    casa::IPosition loc(itsComponentImage.ndim(),0);
    for (int y=ymin; y<=ymax; y++){
        for (int x=xmin; x<=xmax; x++){
            loc[0]=x;
            loc[1]=y;
            pos(0)=x*1.;
            pos(1)=y*1.;
            itsComponentImage(loc) += gauss(pos);
        }
    }
    
}


void DistributedContinuumParameterisation::gather()
{
    if (itsComms->isParallel()) {

        if (itsTotalListSize > 0) {

            if (itsComms->isMaster()) {
                // for each worker, read completed objects until we get a 'finished' signal

                // now read back the sources from the workers
                LOFAR::BlobString bs;
                for (int n = 0; n < itsComms->nProcs() - 1; n++) {
                    int numIslands,numComponents;
                    itsComms->receiveBlob(bs, n + 1);
                    LOFAR::BlobIBufString bib(bs);
                    LOFAR::BlobIStream in(bib);
                    int version = in.getStart("Contfinal");
                    ASKAPASSERT(version == 1);
                    in >> numIslands;
                    for (int i = 0; i < numIslands; i++) {
                        CasdaIsland isle;
                        in >> isle;
                        itsIslandList.push_back(isle);
                    }
                    in >> numComponents;
                    for (int i = 0; i < numComponents; i++) {
                        CasdaComponent comp;
                        in >> comp;
                        itsComponentList.push_back(comp);
                    }

                    // read image
                    int nelements;
                    int val;
                    in >> nelements;
                    casa::IPosition shape(nelements);
                    for(int i=0;i<nelements;i++){
                        in >> val;
                        shape[i] = val;
                    }
                    std::vector<float> imageAsVector(shape.product(),0.);
                    for(int i=0;i<shape.product();i++){
                        in >> imageAsVector[i];
                    }
                    itsComponentImage += casa::Array<float>(shape,imageAsVector.data());
                        
                    in.getEnd();
                }

                // Make sure we have the correct amount of sources
                ASKAPASSERT(itsInputList.size() == itsIslandList.size());

                // sort by id:
                std::sort(itsIslandList.begin(), itsIslandList.end());
                std::sort(itsComponentList.begin(), itsComponentList.end());


            } else { // WORKER
                // for each object in itsOutputList, send to master
                LOFAR::BlobString bs;
                bs.resize(0);
                LOFAR::BlobOBufString bob(bs);
                LOFAR::BlobOStream out(bob);
                out.putStart("Contfinal", 1);
                out << int(itsIslandList.size());
                for (size_t i = 0; i < itsIslandList.size(); i++) {
                    out << itsIslandList[i];
                }
                out << int(itsComponentList.size());
                for (size_t i = 0; i < itsComponentList.size(); i++) {
                    out << itsComponentList[i];
                }

                // send image
                casa::IPosition shape = itsComponentImage.shape();
                out << shape.nelements();
                for(size_t i=0;i<shape.nelements();i++){
                    int val = shape[i];
                    out << val;
                }
                std::vector<float> imageAsVector=itsComponentImage.tovector();
                for(size_t i=0;i<imageAsVector.size();i++){
                    out << imageAsVector[i];
                }
                out.putEnd();
                itsComms->sendBlob(bs, 0);
            }

        }

    }

}

}


}

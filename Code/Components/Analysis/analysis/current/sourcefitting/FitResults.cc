/// @file
///
/// @copyright (c) 2008 CSIRO
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>
///

#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/FitResults.h>
#include <sourcefitting/Fitter.h>
#include <sourcefitting/SubComponent.h>

#include <casacore/scimath/Fitting/FitGaussian.h>
#include <casacore/scimath/Functionals/Gaussian1D.h>
#include <casacore/scimath/Functionals/Gaussian2D.h>
#include <casacore/scimath/Functionals/Gaussian3D.h>
#include <casacore/casa/namespace.h>

#include <Common/LofarTypedefs.h>
using namespace LOFAR::TYPES;
#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>
#include <Common/Exceptions.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <utility>
#include <math.h>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".sourcefitting");

using namespace duchamp;

namespace askap {
namespace analysis {

namespace sourcefitting {

FitResults::FitResults():
    itsFitExists(false),
    itsFitIsGood(false),
    itsChisq(0.),
    itsRedChisq(0.),
    itsRMS(0.),
    itsNumDegOfFreedom(0),
    itsNumFreeParam(0),
    itsNumPix(0),
    itsNumGauss(0),
    itsFlagFitIsGuess(false),
    itsGaussFitSet(),
    itsGaussFitErrorSet()
{


}

void FitResults::saveResults(Fitter &fit)
{

    itsFitExists = fit.passConverged();
    itsFitIsGood = fit.passChisq();
    itsFlagFitIsGuess = false;
    itsChisq = fit.chisq();
    itsRedChisq = fit.redChisq();
    itsRMS = fit.RMS();
    itsNumDegOfFreedom = fit.ndof();
    itsNumFreeParam = fit.params().numFreeParam();
    itsNumGauss = fit.numGauss();
    itsNumPix = itsNumDegOfFreedom + itsNumGauss * itsNumFreeParam + 1;
    // Make a map so that we can output the fitted components in order of peak flux
    std::multimap<double, int> fitMap = fit.peakFluxList();
    // Need to use reverse_iterator so that brightest component's listed first
    std::multimap<double, int>::reverse_iterator rfit = fitMap.rbegin();

    for (; rfit != fitMap.rend(); rfit++) {
        itsGaussFitSet.push_back(fit.gaussian(rfit->second));
        itsGaussFitErrorSet.push_back(fit.error(rfit->second));
    }
}
//**************************************************************//

void FitResults::saveGuess(std::vector<SubComponent> cmpntList)
{

    itsFitExists = false;
    itsFitIsGood = false;
    itsFlagFitIsGuess = true;
    itsChisq = 999.;
    itsRedChisq = 999.;
    itsRMS = 0.;
    itsNumDegOfFreedom = 0;
    itsNumFreeParam = 0;
    itsNumGauss = cmpntList.size();
    itsNumPix = 0;
    // Make a map so that we can output the fitted components in order of peak flux
    std::multimap<double, int> fitMap;
    for (unsigned int i = 0; i < itsNumGauss; i++) {
        fitMap.insert(std::pair<double, int>(cmpntList[i].peak(), i));
    }
    // Need to use reverse_iterator so that brightest component's listed first
    std::multimap<double, int>::reverse_iterator rfit = fitMap.rbegin();

    for (; rfit != fitMap.rend(); rfit++) {
        itsGaussFitSet.push_back(cmpntList[rfit->second].asGauss());
        itsGaussFitErrorSet.push_back(casa::Vector<casa::Double>(6,0.));
    }
}

//**************************************************************//

std::vector<SubComponent> FitResults::getCmpntList()
{
    std::vector<SubComponent> output(itsGaussFitSet.size());
    std::vector<casa::Gaussian2D<Double> >::iterator gauss = itsGaussFitSet.begin();
    int comp = 0;

    for (; gauss < itsGaussFitSet.end(); gauss++) {
        output[comp].setX(gauss->xCenter());
        output[comp].setY(gauss->yCenter());
        output[comp].setPeak(gauss->height());
        output[comp].setMajor(gauss->majorAxis());
        output[comp].setMinor(gauss->minorAxis());
        output[comp].setPA(gauss->PA());
        comp++;
    }

    return output;
}


void FitResults::logIt(std::string loc)
{
    std::vector<casa::Gaussian2D<Double> >::iterator gauss;
    unsigned int i=0;
    for (gauss = itsGaussFitSet.begin(); gauss < itsGaussFitSet.end(); gauss++) {
        std::stringstream outmsg;
        outmsg << "Component FluxPeak,X0,Y0,MAJ,MIN,PA = ";
        outmsg.precision(8);
        outmsg.setf(ios::fixed);
        outmsg << gauss->height()    << " (" << itsGaussFitErrorSet[i][0] <<"), ";
        outmsg.precision(3);
        outmsg.setf(ios::fixed);
        outmsg << gauss->xCenter()   << " (" << itsGaussFitErrorSet[i][1] <<"), "
               << gauss->yCenter()   << " (" << itsGaussFitErrorSet[i][2] <<"), "
               << gauss->majorAxis() << " (" << itsGaussFitErrorSet[i][3] <<"), "
               << gauss->minorAxis() << " (" << itsGaussFitErrorSet[i][4] <<"), "
               << gauss->PA()        << " (" << itsGaussFitErrorSet[i][5] <<")";
        if (loc == "DEBUG") {
            ASKAPLOG_DEBUG_STR(logger, outmsg.str());
        } else if (loc == "INFO") {
            ASKAPLOG_INFO_STR(logger, outmsg.str());
        }
        
        i++;
    }
}


LOFAR::BlobOStream& operator<<(LOFAR::BlobOStream &blob, FitResults& result)
{
    blob << result.itsFitExists;
    blob << result.itsFitIsGood;
    blob << result.itsChisq;
    blob << result.itsRedChisq;
    blob << result.itsRMS;
    blob << result.itsNumDegOfFreedom;
    blob << result.itsNumFreeParam;
    blob << result.itsNumPix;
    blob << result.itsNumGauss;
    blob << result.itsFlagFitIsGuess;
    uint32 i = result.itsGaussFitSet.size(); blob << i;
    std::vector<casa::Gaussian2D<Double> >::iterator fit = result.itsGaussFitSet.begin();

    for (; fit < result.itsGaussFitSet.end(); fit++) {
        blob << fit->height();
        blob << fit->xCenter();
        blob << fit->yCenter();
        blob << fit->majorAxis();
        blob << fit->axialRatio();
        blob << fit->PA();
    }
    for (size_t i=0;i<result.itsGaussFitErrorSet.size();i++){
        for (size_t j=0;j<6;j++){
            blob << result.itsGaussFitErrorSet[i][j];
        }
    }

    return blob;
}


LOFAR::BlobIStream& operator>>(LOFAR::BlobIStream &blob, FitResults& result)
{
    blob >> result.itsFitExists;
    blob >> result.itsFitIsGood;
    blob >> result.itsChisq;
    blob >> result.itsRedChisq;
    blob >> result.itsRMS;
    blob >> result.itsNumDegOfFreedom;
    blob >> result.itsNumFreeParam;
    blob >> result.itsNumPix;
    blob >> result.itsNumGauss;
    blob >> result.itsFlagFitIsGuess;
    uint32 i, size;
    blob >> size;
    result.itsGaussFitSet.clear();

    for (i = 0; i < size; i++) {
        Double d1, d2, d3, d4, d5, d6;
        blob >> d1;
        blob >> d2;
        blob >> d3;
        blob >> d4;
        blob >> d5;
        blob >> d6;
        casa::Gaussian2D<Double> fit(d1, d2, d3, d4, d5, d6);
        result.itsGaussFitSet.push_back(fit);
    }

    result.itsGaussFitErrorSet.clear();
    for(i=0;i<size;i++){
        casa::Vector<casa::Double> err(6);
        for(size_t j=0;j<6;j++){
            Double val;
            blob >> val;
            err[j]=val;
        }
        result.itsGaussFitErrorSet.push_back(err);
    }

    return blob;
}


}

}

}

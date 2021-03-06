/// @file
///
/// XXX Notes on program XXX
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
#include <cppunit/extensions/HelperMacros.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <sourcefitting/RadioSource.h>
#include <sourcefitting/SubComponent.h>
#include <sourcefitting/FittingParameters.h>
#include <outputs/CataloguePreparation.h>
#include <mathsutils/MathsUtils.h>

#include <cppunit/extensions/HelperMacros.h>

#include <duchamp/duchamp.hh>
#include <duchamp/Detection/detection.hh>
#include <duchamp/Cubes/cubes.hh>
#include <duchamp/fitsHeader.hh>
#include <duchamp/PixelMap/Object2D.hh>
#include <duchamp/Utils/Statistics.hh>
#include <duchamp/Detection/finders.hh>
#include <duchamp/FitsIO/DuchampBeam.hh>

#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Vector.h>
#include <Common/ParameterSet.h>

#include <string>
#include <vector>
#include <math.h>

ASKAP_LOGGER(logger, ".radioSourceTest");

namespace askap {

namespace analysis {

namespace sourcefitting {

const size_t srcDim = 10;
const size_t srcSize = srcDim*srcDim;
const size_t arrayDim = 10;
const size_t arraySize = arrayDim*arrayDim;
const double gaussNorm = 10.;
const double gaussXFWHM = 4.;
const double gaussYFWHM = 2.;
const double gauss2FWHM = 3.;
const double gauss2offset = 3.;
const double gaussX0 = 5.;
const double gaussY0 = 5.;
const double gaussPA = M_PI / 2.;
const double SIGMAtoFWHM = 2. * M_SQRT2 * sqrt(M_LN2);
const double BMAJ = 2;
const double BMIN = 2;
const double BPA = 0.;
const double gaussDeconvXFWHM = sqrt(12.);
const double gaussDeconvYFWHM = 0.;
const double gaussDeconvPA = M_PI / 2.;

class RadioSourceTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(RadioSourceTest);
        CPPUNIT_TEST(findSource);
        CPPUNIT_TEST(sourceBox);
        CPPUNIT_TEST(findGaussSource);
        CPPUNIT_TEST(testShapeGaussSource);
        CPPUNIT_TEST(subthreshold);
        CPPUNIT_TEST(fitSource);
        CPPUNIT_TEST(componentDeconvolution);
        CPPUNIT_TEST(fitDouble);
        CPPUNIT_TEST(suffixGeneration);
        CPPUNIT_TEST_SUITE_END();

    private:

        casa::Vector<float>              itsArray;
        std::vector<size_t>             itsDim;
        std::vector<PixelInfo::Object2D> itsObjlist;
        std::vector<SubComponent>        itsSublist;
        RadioSource                      itsSource;
        FittingParameters                itsFitparams;
        duchamp::Section                 itsSection;

        std::vector<float>               itsGaussArray;
        std::vector<PixelInfo::Object2D> itsGaussObjlist;
        RadioSource                      itsGaussSource;
        std::vector<float>               itsGauss2Array;
        std::vector<PixelInfo::Object2D> itsGauss2Objlist;
        RadioSource                      itsGauss2Source;

    public:

        /*****************************************/
        void setUp()
        {

            const float src[arraySize] = {1., 1., 1., 1., 1., 1., 1., 1., 1., 1.,
                                          1., 1., 1., 1., 1., 1., 1., 1., 1., 1.,
                                          1., 1., 1., 1., 1., 1., 1., 1., 1., 1.,
                                          1., 1., 1., 1., 1., 9., 11., 1., 1., 1.,
                                          1., 1., 1., 1., 1., 10., 10., 1., 1., 1.,
                                          1., 1., 1., 40., 39., 51., 50., 20., 19., 1.,
                                          1., 1., 1., 41., 40., 50., 49., 20., 22., 1.,
                                          1., 1., 1., 1., 1., 28., 30., 1., 1., 1.,
                                          1., 1., 1., 1., 1., 33., 27., 1., 1., 1.,
                                          1., 1., 1., 1., 1., 1., 1., 1., 1., 1.
                                         };
            float gaussSrc[arraySize];
            for (size_t i = 0; i < arraySize; i++) gaussSrc[i] = 0.;
            float gaussXSigma = gaussXFWHM / SIGMAtoFWHM;
            float gaussYSigma = gaussYFWHM / SIGMAtoFWHM;
            for (size_t y = 0; y < arrayDim; y++) {
                for (size_t x = 0; x < arrayDim; x++) {
                    float xterm = (x - gaussX0) / gaussXSigma;
                    float yterm = (y - gaussY0) / gaussYSigma;
                    gaussSrc[x + y * arrayDim] +=
                        gaussNorm * exp(-(0.5 * xterm * xterm + 0.5 * yterm * yterm));
                }
            }

            float gauss2src[arraySize];
            for (size_t i = 0; i < arraySize; i++) gauss2src[i] = 0.;
            float gauss2Sigma = gauss2FWHM / SIGMAtoFWHM;
            for (size_t y = 0; y < arrayDim; y++) {
                for (size_t x = 0; x < arrayDim; x++) {
                    float xterm, yterm;
                    float off = gauss2offset / 2.;
                    xterm = (x - (gaussX0 - off)) / gauss2Sigma;
                    yterm = (y - gaussY0) / gauss2Sigma;
                    gauss2src[x + y * arrayDim] +=
                        gaussNorm * exp(-(0.5 * xterm * xterm + 0.5 * yterm * yterm));
                    xterm = (x - (gaussX0 + off)) / gauss2Sigma;
                    yterm = (y - gaussY0) / gauss2Sigma;
                    gauss2src[x + y * arrayDim] +=
                        0.5 * gaussNorm * exp(-(0.5 * xterm * xterm + 0.5 * yterm * yterm));
                }
            }

            itsDim = std::vector<size_t>(2, arrayDim);

            std::string secstring = duchamp::nullSection(2);
            itsSection = duchamp::Section(secstring);
            itsSection.parse(itsDim.data(), 2);

            itsFitparams = FittingParameters(LOFAR::ParameterSet());
            itsFitparams.setFitTypes(std::vector<std::string>(1, "full"));
            itsFitparams.setMaxNumGauss(1);
            itsFitparams.setNumSubThresholds(100);
            itsFitparams.setMaxRMS(5.);

            float thresh = 5.;
            itsArray = casa::Vector<float>(casa::IPosition(1, arraySize), src);
            duchamp::Image *itsImage = new duchamp::Image(itsDim.data());
            itsImage->saveArray(itsArray.data(), arraySize);
            itsImage->stats().setThreshold(thresh);
            itsImage->setMinSize(1);
            itsImage->pars().setFlagBlankPix(false);
            itsObjlist = itsImage->findSources2D();
            CPPUNIT_ASSERT(itsObjlist.size() == 1);
            duchamp::Detection det;
            det.addChannel(0, itsObjlist[0]);
            det.calcFluxes(itsArray.data(), itsDim.data());
            itsSource = RadioSource(det);
            itsSource.setFitParams(itsFitparams);
            itsSource.defineBox(itsSection, 2);
            itsSource.setDetectionThreshold(thresh);
            itsSource.setNoiseLevel(1.);
            duchamp::FitsHeader head;
            itsSource.setHeader(head);
            itsSource.setFitParams(itsFitparams);
            delete itsImage;

            float gaussThresh = 1.;
            itsGaussArray = std::vector<float>(gaussSrc, gaussSrc + arraySize);
            itsImage = new duchamp::Image(itsDim.data());
            itsImage->saveArray(itsGaussArray.data(), arraySize);
            itsImage->stats().setThreshold(gaussThresh);
            itsImage->setMinSize(1);
            itsImage->pars().setFlagBlankPix(false);
            itsGaussObjlist = itsImage->findSources2D();
            CPPUNIT_ASSERT(itsGaussObjlist.size() == 1);
            duchamp::Detection detg;
            detg.addChannel(0, itsGaussObjlist[0]);
            detg.calcFluxes(itsGaussArray.data(), itsDim.data());
            itsGaussSource = RadioSource(detg);
            itsGaussSource.setFitParams(itsFitparams);
            itsGaussSource.defineBox(itsSection, 2);
            itsGaussSource.setDetectionThreshold(gaussThresh);
            itsGaussSource.setNoiseLevel(1.);
            delete itsImage;

            itsGauss2Array = std::vector<float>(gauss2src, gauss2src + arraySize);
            itsImage = new duchamp::Image(itsDim.data());
            itsImage->saveArray(itsGauss2Array.data(), arraySize);
            itsImage->stats().setThreshold(gaussThresh);
            itsImage->setMinSize(1);
            itsImage->pars().setFlagBlankPix(false);
            itsGauss2Objlist = itsImage->findSources2D();
            CPPUNIT_ASSERT(itsGauss2Objlist.size() == 1);
            duchamp::Detection detg2;
            detg2.addChannel(0, itsGauss2Objlist[0]);
            detg2.calcFluxes(itsGauss2Array.data(), itsDim.data());
            itsGauss2Source = RadioSource(detg2);
            itsGauss2Source.setFitParams(itsFitparams);
            itsGauss2Source.defineBox(itsSection, 2);
            itsGauss2Source.setDetectionThreshold(gaussThresh);
            itsGauss2Source.setNoiseLevel(1.);
            delete itsImage;

        }

        /*****************************************/
        void tearDown()
        {
            itsObjlist.clear();
            itsGaussObjlist.clear();
        }

        /*****************************************/
        void findSource()
        {
            CPPUNIT_ASSERT(itsObjlist.size() == 1);
        }

        /*****************************************/
        void sourceBox()
        {
            itsFitparams.setBoxPadSize(0);
            itsSource.setFitParams(itsFitparams);
            itsSource.defineBox(itsSection, 2);

            CPPUNIT_ASSERT(itsSource.boxXmin() == 3);
            CPPUNIT_ASSERT(itsSource.boxYmin() == 3);
            CPPUNIT_ASSERT(itsSource.boxXmax() == 8);
            CPPUNIT_ASSERT(itsSource.boxXmax() == 8);

        }

        /*****************************************/
        void findGaussSource()
        {
            CPPUNIT_ASSERT(itsGaussObjlist.size() == 1);
        }

        /*****************************************/
        void testShapeGaussSource()
        {
            double maj, min, pa;
            CPPUNIT_ASSERT(itsGaussObjlist.size() == 1);
            itsFitparams.setBoxPadSize(0);
            itsGaussSource.setFitParams(itsFitparams);
            itsGaussSource.defineBox(itsSection, 2);
            std::vector<float> fluxarray(itsGaussSource.boxSize(), 0.);
            PixelInfo::Object2D spatMap = itsGaussSource.getSpatialMap();
            for (size_t x = 0; x < arrayDim; x++) {
                for (size_t y = 0; y < arrayDim; y++) {
                    if (spatMap.isInObject(x, y)) {
                        int loc = (x - itsGaussSource.boxXmin()) +
                                  itsGaussSource.boxXsize() * (y - itsGaussSource.boxYmin());
                        fluxarray[loc] = itsGaussArray[x + y * itsDim[0]];
                    }
                }
            }
            itsGaussSource.getFWHMestimate(fluxarray, pa, maj, min);
            CPPUNIT_ASSERT(fabs(maj - 2) < 1.e-6);
            CPPUNIT_ASSERT(fabs(min - 1.) < 1.e-6);
            CPPUNIT_ASSERT(fabs(pa - M_PI / 2) < 1.e-6);
        }

        /*****************************************/
        void subthreshold()
        {
            CPPUNIT_ASSERT(itsObjlist.size() == 1);

            casa::Matrix<casa::Double> itsPos;
            casa::Vector<casa::Double> itsF;
            itsPos.resize(arraySize, 2);
            itsF.resize(arraySize);
            casa::Vector<casa::Double> curpos(2);
            curpos = 0;
            for (size_t x = 0; x < arrayDim; x++) {
                for (size_t y = 0; y < arrayDim; y++) {
                    itsF(x + y * arrayDim) = itsArray[x + y * arrayDim];
                    curpos(0) = x;
                    curpos(1) = y;
                    itsPos.row(x + y * arrayDim) = curpos;
                }
            }
            itsSublist = itsSource.getSubComponentList(itsPos, itsF);

            CPPUNIT_ASSERT(itsSublist.size() == 5);
        }

        /*****************************************/
        void fitSource()
        {
            CPPUNIT_ASSERT(itsGaussObjlist.size() == 1);
            duchamp::FitsHeader head;
            head.beam().define(1, 1, 0, duchamp::PARAM);
            itsGaussSource.setHeader(head);
            itsGaussSource.setFitParams(itsFitparams);

            itsGaussSource.fitGauss(itsGaussArray, itsDim);

            std::vector<casa::Gaussian2D<Double> > fits = itsGaussSource.gaussFitSet();
            ASKAPLOG_DEBUG_STR(logger, "Have fit " << fits[0]);
            CPPUNIT_ASSERT(fits.size() == 1);
            CPPUNIT_ASSERT(fabs(fits[0].height() - gaussNorm) < 1.e-6);
            CPPUNIT_ASSERT(fabs(fits[0].majorAxis() - gaussXFWHM) < 1.e-6);
            CPPUNIT_ASSERT(fabs(fits[0].minorAxis() - gaussYFWHM) < 1.e-6);
            CPPUNIT_ASSERT(fabs(fits[0].PA() - gaussPA) < 1.e-6);
            CPPUNIT_ASSERT(fabs(fits[0].xCenter() - gaussX0) < 1.e-6);
            CPPUNIT_ASSERT(fabs(fits[0].yCenter() - gaussY0) < 1.e-6);
        }

        /*****************************************/
        void componentDeconvolution()
        {

            CPPUNIT_ASSERT(itsGaussObjlist.size() == 1);
            duchamp::FitsHeader head;
            head.beam().define(1, 1, 0, duchamp::PARAM);
            itsGaussSource.setHeader(head);
            itsGaussSource.setFitParams(itsFitparams);

            itsGaussSource.fitGauss(itsGaussArray, itsDim);

            std::vector<casa::Gaussian2D<Double> > fits = itsGaussSource.gaussFitSet();
            duchamp::DuchampBeam beam(BMAJ, BMIN, BPA);
            std::vector<double> deconvShape = analysisutilities::deconvolveGaussian(fits[0], beam);
            ASKAPLOG_DEBUG_STR(logger, "Deconvolved gaussian to get shape " << deconvShape);
            // Only use a limit of 1/1000 here, as small errors in the
            // shape from the fitting can get amplified in the
            // deconvolution - 1.e-6 was too strict
            CPPUNIT_ASSERT(fabs(deconvShape[0] - gaussDeconvXFWHM) < 1.e-3);
            CPPUNIT_ASSERT(fabs(deconvShape[2] - gaussDeconvPA) < 1.e-3);
            CPPUNIT_ASSERT(fabs(deconvShape[1] - gaussDeconvYFWHM) < 1.e-3);

        }

        /*****************************************/
        void fitDouble()
        {
            CPPUNIT_ASSERT(itsGauss2Objlist.size() == 1);
            duchamp::FitsHeader head;
            head.beam().define(1, 1, 0, duchamp::PARAM);
            itsGauss2Source.setHeader(head);
            itsGauss2Source.setFitParams(itsFitparams);
            itsGauss2Source.setNoiseLevel(0.1);
            itsGauss2Source.fitGauss(itsGauss2Array, itsDim);

            std::vector<casa::Gaussian2D<Double> > fits = itsGauss2Source.gaussFitSet();
            ASKAPLOG_DEBUG_STR(logger, "Have fit " << fits[0]);
            CPPUNIT_ASSERT(fits.size() == 2);
        }

        /*****************************************/
        void suffixGeneration()
        {
            // A simple test to make sure we get the correct suffix
            // for various component numbers.
            unsigned int number[16] = {0, 1, 2, 3, 25,
                                       26, 27, 51, 52, 53, 701,
                                       702, 703, 18277, 18278, 18279
                                      };
            std::string suffix[16] = {"a", "b", "c", "d", "z",
                                      "aa", "ab", "az", "ba", "bb", "zz",
                                      "aaa", "aab", "zzz", "aaaa", "aaab"
                                     };
            for (int i = 0; i < 16; i++) {
                std::string newsuff = getSuffix(number[i]);
                CPPUNIT_ASSERT(newsuff == suffix[i]);
            }

        }


};

}

}

}

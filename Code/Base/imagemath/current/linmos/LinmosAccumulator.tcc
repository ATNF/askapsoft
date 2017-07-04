/// @file LinmosAccumulator.tcc
///
/// @brief combine a number of images as a linear mosaic
/// @details
///
/// @copyright (c) 2012,2014,2015 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>
/// @author Daniel Mitchell <daniel.mitchell@csiro.au>

// other 3rd party
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/images/Images/ImageRegrid.h>
#include <casacore/lattices/LatticeMath/LatticeMathUtil.h>

// ASKAPsoft includes
#include <askap/Application.h>
#include <askap/AskapError.h>
#include <askap/AskapLogging.h>
#include <askap/AskapUtil.h>
#include <askap/StatReporter.h>
#include <utils/MultiDimArrayPlaneIter.h>
#include <primarybeam/PrimaryBeam.h>
#include <primarybeam/PrimaryBeamFactory.h>

ASKAP_LOGGER(linmoslogger, ".linmosaccumulator");

using namespace casa;


// variables functions used by the linmos accumulator class

// See loadParset for these options.
enum weight_types {FROM_WEIGHT_IMAGES=0, FROM_BP_MODEL};
// FROM_WEIGHT_IMAGES   Obtain pixel weights from weight images (parset "weights" entries)
// FROM_BP_MODEL        Generate pixel weights using a Gaussian primary-beam model
enum weight_states {CORRECTED=0, INHERENT, WEIGHTED};
// CORRECTED            Direction-dependent beams/weights have been divided out of input images
// INHERENT             Input images retain the natural primary-beam weighting of the visibilities
// WEIGHTED             Input images have full primary-beam-squared weighting

/// @brief function to convert RA and Dec strings to a MVDirection
/// @param[in] const std::string &ra
/// @param[in] const std::string &dec
/// @return MVDirection
MVDirection convertDir(const std::string &ra, const std::string &dec) {
    Quantity tmpra,tmpdec;
    Quantity::read(tmpra, ra);
    Quantity::read(tmpdec,dec);
    return MVDirection(tmpra,tmpdec);
}

namespace askap {

    namespace imagemath {

        /// @brief Base class supporting linear mosaics (linmos)

        template<typename T>
        LinmosAccumulator<T>::LinmosAccumulator() : itsMethod("linear"),
                                                    itsDecimate(3),
                                                    itsReplicate(false),
                                                    itsForce(false),
                                                    itsWeightType(-1),
                                                    itsWeightState(-1),
                                                    itsNumTaylorTerms(-1),
                                                    itsCutoff(0.01),
                                                    itsMosaicTag("linmos"),
                                                    itsTaylorTag("taylor.0") {}

        // functions in the linmos accumulator class

        template<typename T>
        bool LinmosAccumulator<T>::loadParset(const LOFAR::ParameterSet &parset) {

            const vector<string> inImgNames = parset.getStringVector("names", true);
            const vector<string> inWgtNames = parset.getStringVector("weights", vector<string>(), true);
            const string weightTypeName = parset.getString("weighttype");
            const string weightStateName = parset.getString("weightstate", "Corrected");

            const bool findMosaics = parset.getBool("findmosaics", false);

            // Check the input images
            ASKAPCHECK(inImgNames.size()>0, "Number of input images should be greater than 0");

            // Check weighting options. One of the following must be set:
            //  - weightTypeName==FromWeightImages: get weights from input weight images
            //    * the number of weight images and their shapes must match the input images
            //  - weightTypeName==FromPrimaryBeamModel: set weights using a Gaussian beam model
            //    * the direction coordinate centre will be used as beam centre, unless ...
            //    * an output weight image will be written, so an output file name is required

            if (boost::iequals(weightTypeName, "FromWeightImages")) {
                itsWeightType = FROM_WEIGHT_IMAGES;
                ASKAPLOG_INFO_STR(linmoslogger, "Weights are coming from weight images");
            } else if (boost::iequals(weightTypeName, "FromPrimaryBeamModel")) {
                itsWeightType = FROM_BP_MODEL;
                ASKAPLOG_INFO_STR(linmoslogger, "Weights to be set using a Gaussian primary-beam models");
            } else {
                ASKAPLOG_ERROR_STR(linmoslogger, "Unknown weighttype " << weightTypeName);
                return false;
            }

            if (findMosaics) {

                ASKAPLOG_INFO_STR(linmoslogger, "Image names to be automatically generated. Searching...");
                // check for useless parameters
                if (parset.isDefined("outname") || parset.isDefined("outweight")) {
                    ASKAPLOG_WARN_STR(linmoslogger, "  - output file names are specified in parset but ignored.");
                }
                if (parset.isDefined("nterms")) {
                    ASKAPLOG_WARN_STR(linmoslogger, "  - nterms is specified in parset but ignored.");
                }

                findAndSetMosaics(inImgNames);

                ASKAPCHECK(itsInImgNameVecs.size() > 0, "No suitable mosaics found.");
                ASKAPLOG_INFO_STR(linmoslogger, itsInImgNameVecs.size() << " suitable mosaics found.");

            } else {

                string outImgName = parset.getString("outname");
                string outWgtName = parset.getString("outweight");

                // If reading weights from images, check the input for those
                if (itsWeightType == FROM_WEIGHT_IMAGES) {
                    ASKAPCHECK(inImgNames.size()==inWgtNames.size(), "# weight images should equal # images");
                }

                // Check for taylor terms

                if (parset.isDefined("nterms")) {

                    itsNumTaylorTerms = parset.getInt32("nterms");
                    findAndSetTaylorTerms(inImgNames, inWgtNames, outImgName, outWgtName);

                } else {

                    setSingleMosaic(inImgNames, inWgtNames, outImgName, outWgtName);

                }

            }

            if (itsWeightType == FROM_WEIGHT_IMAGES) {

                // if reading weights from images, check for inputs associated with other kinds of weighting
                if (parset.isDefined("feeds.centre") ||
                    parset.isDefined("feeds.centreref") ||
                    parset.isDefined("feeds.offsetsfile") ||
                    parset.isDefined("feeds.names") ||
                    parset.isDefined("feeds.spacing") ) {
                    ASKAPLOG_WARN_STR(linmoslogger,
                        "Beam information specified in parset but ignored. Using weight images");
                }

            } else if (itsWeightType == FROM_BP_MODEL) {

                // check for inputs associated with other kinds of weighting
                if (inWgtNames.size()>0) {
                    ASKAPLOG_WARN_STR(linmoslogger,
                        "Weight images specified in parset but ignored. Using a primary-beam model");
                }

            }

            // Check the initial weighting state of the input images

            if (boost::iequals(weightStateName, "Corrected")) {
                ASKAPLOG_INFO_STR(linmoslogger,
                        "Input image state: Direction-dependent beams/weights have been divided out");
                itsWeightState = CORRECTED;
            } else if (boost::iequals(weightStateName, "Inherent")) {
                ASKAPLOG_INFO_STR(linmoslogger,
                        "Input image state: natural primary-beam weighting of the visibilities is retained");
                itsWeightState = INHERENT;
            } else if (boost::iequals(weightStateName, "Weighted")) {
                ASKAPLOG_INFO_STR(linmoslogger,
                        "Input image state: full primary-beam-squared weighting");
                itsWeightState = WEIGHTED;
            } else {
                ASKAPLOG_ERROR_STR(linmoslogger,
                        "Unknown weightstyle " << weightStateName);
                return false;
            }

            if (parset.isDefined("cutoff")) itsCutoff = parset.getFloat("cutoff");

            if (parset.isDefined("regrid.method")) itsMethod = parset.getString("regrid.method");
            if (parset.isDefined("regrid.decimate")) itsDecimate = parset.getInt("regrid.decimate");
            if (parset.isDefined("regrid.replicate")) itsReplicate = parset.getBool("regrid.replicate");
            if (parset.isDefined("regrid.force")) itsForce = parset.getBool("regrid.force");

            if (parset.isDefined("psfref")) {
                ASKAPCHECK(parset.getUint("psfref")<inImgNames.size(), "PSF reference-image number is too large");
            }

            // sort out primary beam
            itsPB = PrimaryBeamFactory::make(parset);

            return true;

        }

        template<typename T>
        void LinmosAccumulator<T>::setSingleMosaic(const vector<string> &inImgNames,
                                                   const vector<string> &inWgtNames,
                                                   const string &outImgName,
                                                   const string &outWgtName) {

            // set some variables for the sensitivity image searches
            string image_tag = "image", restored_tag = ".restored", tmpName;
            // set false if any sensitivity images are missing or if not an image* mosaic
            itsDoSensitivity = true;

            // Check the input images
            for (size_t img = 0; img < inImgNames.size(); ++img) {

                // make sure the output image will not be overwritten
                ASKAPCHECK(inImgNames[img]!=outImgName,
                    "Output image, "<<outImgName<<", is present among the inputs");

                // if this is an "image*" file, see if there is an appropriate sensitivity image
                if (itsDoSensitivity) {
                    tmpName = inImgNames[img];
                    size_t image_pos = tmpName.find(image_tag);
                    // if the file starts with image_tag, look for a sensitivity image
                    if (image_pos == 0) {
                        tmpName.replace(image_pos, image_tag.length(), "sensitivity");
                        // remove any ".restored" sub-string from the file name
                        size_t restored_pos = tmpName.find(restored_tag);
                        if (restored_pos != string::npos) {
                            tmpName.replace(restored_pos, restored_tag.length(), "");
                        }
                        if (boost::filesystem::exists(tmpName)) {
                            itsInSenNameVecs[outImgName].push_back(tmpName);
                        } else {
                            ASKAPLOG_WARN_STR(linmoslogger,
                                "Cannot find file "<<tmpName<<" . Ignoring sensitivities.");
                            itsDoSensitivity = false;
                        }
                    } else {
                        ASKAPLOG_WARN_STR(linmoslogger,
                            "Input not an image* file. Ignoring sensitivities.");
                        itsDoSensitivity = false;
                    }
                }

            }

            // set a single key for the various file-name maps
            itsOutWgtNames[outImgName] = outWgtName;
            itsInImgNameVecs[outImgName] = inImgNames;
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                itsInWgtNameVecs[outImgName] = inWgtNames;
            }
            if (itsDoSensitivity) {
                itsGenSensitivityImage[outImgName] = true;
                // set an output sensitivity file name
                tmpName = outImgName;
                tmpName.replace(0, image_tag.length(), "sensitivity");
                // remove any ".restored" sub-string from the weights file name
                size_t restored_pos = tmpName.find(restored_tag);
                if (restored_pos != string::npos) {
                    tmpName.replace(restored_pos, restored_tag.length(), "");
                }
                itsOutSenNames[outImgName] = tmpName;
            } else {
                itsGenSensitivityImage[outImgName] = false;
                // if some but not all sensitivity images were found, remove this key from itsInSenNameVecs
                if (itsInSenNameVecs.find(outImgName)!=itsInSenNameVecs.end()) {
                    itsInSenNameVecs.erase(outImgName);
                }
            }

        } // LinmosAccumulator<T>::setSingleMosaic()

        template<typename T>
        void LinmosAccumulator<T>::findAndSetTaylorTerms(const vector<string> &inImgNames,
                                                         const vector<string> &inWgtNames,
                                                         const string &outImgNameOrig,
                                                         const string &outWgtNameOrig) {

            ASKAPLOG_INFO_STR(linmoslogger, "Looking for "<<itsNumTaylorTerms<<" taylor terms");
            ASKAPCHECK(itsNumTaylorTerms>=0, "Number of taylor terms should be greater than or equal to 0");

            size_t pos0, pos1;
            pos0 = outImgNameOrig.find(itsTaylorTag);
            ASKAPCHECK(pos0!=string::npos, "Cannot find "<<itsTaylorTag<<" in output file "<<outImgNameOrig);
            pos1 = outImgNameOrig.find(itsTaylorTag, pos0+1); // make sure there aren't multiple entries.
            ASKAPCHECK(pos1==string::npos,
                "There are multiple  "<<itsTaylorTag<<" strings in output file "<<outImgNameOrig);

            // set some variables for the sensitivity image searches
            string image_tag = "image", restored_tag = ".restored", tmpName;
            itsDoSensitivity = true; // set false if any sensitivity images are missing or if not an image* mosaic

            for (int n = 0; n < itsNumTaylorTerms; ++n) {

                string outImgName = outImgNameOrig;
                string outWgtName = outWgtNameOrig;
                const string taylorN = "taylor." + boost::lexical_cast<string>(n);

                // set a new key for the various output file-name maps
                outImgName.replace(outImgName.find(itsTaylorTag), itsTaylorTag.length(), taylorN);
                outWgtName.replace(outWgtName.find(itsTaylorTag), itsTaylorTag.length(), taylorN);
                itsOutWgtNames[outImgName] = outWgtName;

                for (uint img = 0; img < inImgNames.size(); ++img) {

                    // do some tests
                    string inImgName = inImgNames[img]; // short cut
                    pos0 = inImgName.find(itsTaylorTag);
                    ASKAPCHECK(pos0!=string::npos, "Cannot find "<<itsTaylorTag<<" in input file "<<inImgName);
                    pos1 = inImgName.find(itsTaylorTag, pos0+1); // make sure there aren't multiple entries.
                    ASKAPCHECK(pos1==string::npos,
                        "There are multiple "<<itsTaylorTag<<" strings in input file "<<inImgName);

                    // set a new key for the input file-name-vector map
                    inImgName.replace(pos0, itsTaylorTag.length(), taylorN);
                    itsInImgNameVecs[outImgName].push_back(inImgName);

                    // Check the input image
                    ASKAPCHECK(inImgName!=outImgName,
                        "Output image, "<<outImgName<<", is present among the inputs");

                    if (itsWeightType == FROM_WEIGHT_IMAGES) {
                        // do some tests
                        string inWgtName = inWgtNames[img]; // short cut
                        pos0 = inWgtName.find(itsTaylorTag);
                        ASKAPCHECK(pos0!=string::npos,
                        "Cannot find "<<itsTaylorTag<< " in input weight file "<<inWgtName);
                        pos1 = inWgtName.find(itsTaylorTag, pos0+1); // make sure there aren't multiple entries.
                        ASKAPCHECK(pos1==string::npos, "There are multiple " << itsTaylorTag <<
                                                       " strings in input file "<<inWgtName);

                        // set a new key for the input weights file-name-vector map
                        inWgtName.replace(pos0, itsTaylorTag.length(), taylorN);
                        itsInWgtNameVecs[outImgName].push_back(inWgtName);

                        // Check the input weights image
                        ASKAPCHECK(inWgtName!=outWgtName,
                            "Output wgt image, "<<outWgtName<<", is among the inputs");
                    }

                    // if this is an "image*" file, see if there is an appropriate sensitivity image
                    if (itsDoSensitivity) {
                        tmpName = inImgName;
                        size_t image_pos = tmpName.find(image_tag);
                        // if the file starts with image_tag, look for a sensitivity image
                        if (image_pos == 0) {
                            tmpName.replace(image_pos, image_tag.length(), "sensitivity");
                            // remove any ".restored" sub-string from the file name
                            size_t restored_pos = tmpName.find(restored_tag);
                            if (restored_pos != string::npos) {
                                tmpName.replace(restored_pos, restored_tag.length(), "");
                            }
                            if (boost::filesystem::exists(tmpName)) {
                                itsInSenNameVecs[outImgName].push_back(tmpName);
                            } else {
                                ASKAPLOG_WARN_STR(linmoslogger,
                                    "Cannot find file "<<tmpName<<" . Ignoring sensitivities.");
                                itsDoSensitivity = false;
                            }
                        } else {
                            ASKAPLOG_WARN_STR(linmoslogger,
                                "Input not an image* file. Ignoring sensitivities.");
                            itsDoSensitivity = false;
                        }

                    }
                    ASKAPLOG_INFO_STR(linmoslogger,"Taylor Image: " << inImgName);
                } // img loop (input image)

                // check whether any sensitivity images were found
                if (itsDoSensitivity) {
                    itsGenSensitivityImage[outImgName] = true;
                    // set an output sensitivity file name
                    tmpName = outImgName;
                    tmpName.replace(0, image_tag.length(), "sensitivity");
                    // remove any ".restored" sub-string from the weights file name
                    size_t restored_pos = tmpName.find(restored_tag);
                    if (restored_pos != string::npos) {
                        tmpName.replace(restored_pos, restored_tag.length(), "");
                    }
                    itsOutSenNames[outImgName] = tmpName;
                } else {
                    itsGenSensitivityImage[outImgName] = false;
                    // if some but not all sensitivity images were found, remove this key
                    if (itsInSenNameVecs.find(outImgName)!=itsInSenNameVecs.end()) {
                        itsInSenNameVecs.erase(outImgName);
                    }
                }

            } // n loop (taylor term)


        } // void LinmosAccumulator<T>::findAndSetTaylorTerms()
        template<typename T>
        void LinmosAccumulator<T>::removeBeamFromTaylorTerms(Array<T>& taylor0,
                                                             Array<T>& taylor1,
                                                             Array<T>& taylor2,
                                                         const IPosition& curpos,
                                                          const CoordinateSystem& inSys) {

            // The basics of this are we need to remove the effect of the beam
            // from the Taylor terms
            // This is only required if you do not grid with the beam. One wonders
            // whether we should just implement the beam (A) projection in the gridding.

            // This means redistribution of some of Taylor terms (tt) into tt'
            //
            // tt0' = tt0 - no change
            // tt1' = tt1 - (tt0 x alpha)
            // tt2' = tt2 - tt1 x alpha - tt2 x (beta - alpha(alpha + 1.)/2. )
            //
            // we therefore need some partial products ...
            //
            //
            // tt0 x alpha = tt0Alpha
            // tt1 x alpha = tt1Alpha
            // tt2 x (beta - alpha(alpha + 1.)/2.) = tt2AlphaBeta
            //
            // the taylor terms have no frequency axis - by contruction - but
            // I'm keeping this assumption that there may be some frequency structure
            // I should probably clean this up.
            //
            // copy the pixel iterator containing all dimensions
            //
            //
            // The assumption is that we rescale each constituent image. But we do need to
            // group them.

            // We need the Taylor terms for each pointing grouped together.
            // So lets just get those first

#if 1
            IPosition fullpos(curpos);
            // set a pixel iterator that does not have the higher dimensions
            IPosition pos(2);
            // copy the pixel iterator containing all dimensions
            Vector<double> pixel(2,0.);

            MVDirection world0, world1;
            T offsetBeam, alpha, beta;

            // get coordinates of the spectral axis and the current frequency
            const int scPos = inSys.findCoordinate(Coordinate::SPECTRAL,-1);
            const SpectralCoordinate inSC = inSys.spectralCoordinate(scPos);
            int chPos = inSys.pixelAxes(scPos)[0];
            const T freq = inSC.referenceValue()[0] +
                (curpos[chPos] - inSC.referencePixel()[0]) * inSC.increment()[0];

            // set FWHM for the current beam
            // Removing the factor of 1.22 gives a good match to the simultation weight images
            // const T fwhm = 1.22*3e8/freq/12;

            const T fwhm = 3e8/freq/12.;

            // get coordinates of the direction axes
            const int dcPos = inSys.findCoordinate(Coordinate::DIRECTION,-1);
            const DirectionCoordinate inDC = inSys.directionCoordinate(dcPos);
            const DirectionCoordinate outDC = inSys.directionCoordinate(dcPos);

            // set the centre of the input beam (needs to be more flexible -- and correct...)
            inDC.toWorld(world0,inDC.referencePixel());

            // we need to interate through each of the taylor term images for all of the output
            // mosaics


            //
            // step through the pixels
            //




            Array<T> scr1 = taylor1.copy();
            Array<T> scr2 = taylor2.copy();

            ASKAPLOG_INFO_STR(linmoslogger,"Assuming Gaussian PB fwhm " << fwhm << " and freq " << freq);

            for (int x=0; x < taylor1.shape()[0];++x) {
                for (int y=0; y < taylor1.shape()[1];++y) {

                    fullpos[0] = x;
                    fullpos[1] = y;

                    // get the current pixel location and distance from beam centre
                    pixel[0] = double(x);
                    pixel[1] = double(y);
                    outDC.toWorld(world1,pixel);
                    offsetBeam = world0.separation(world1);
                    //
                    // set the alpha
                    // this assumes that the reference frequency is the current
                    // frequency.
                    //
                    alpha = -8. * log(2.) * pow((offsetBeam/fwhm),2.);
                    // ASKAPLOG_INFO_STR(linmoslogger, "alpha " << alpha);
                    T toPut = scr1(fullpos) - taylor0(fullpos) * alpha;
                    // build

                    taylor1(fullpos) = toPut;

                    beta = alpha;
                    toPut = scr2(fullpos) - scr1(fullpos) * alpha - taylor0(fullpos)*(beta - alpha * (alpha + 1)/2.);
                    //
                    taylor2(fullpos) = toPut;


                    //
                }
            } // for all pixels
#endif
        } // removeBeamFromTaylorTerms()
        template<typename T>
        void LinmosAccumulator<T>::findAndSetMosaics(const vector<string> &imageTags) {

            vector<string> prefixes;
            prefixes.push_back("image");
            prefixes.push_back("residual");
            //prefixes.push_back("weights"); // these need to be handled separately
            //prefixes.push_back("sensitivity"); // these need to be handled separately
            //prefixes.push_back("mask");

            // if this directory name changes from "./", the erase call below may also need to change
            boost::filesystem::path p (".");

            typedef vector<boost::filesystem::path> path_vec;
            path_vec v;

            copy(boost::filesystem::directory_iterator(p),
                 boost::filesystem::directory_iterator(), back_inserter(v));

            // find mosaics by looking for images that contain one of the tags.
            // Then see which of those contain all tags.
            const string searchTag = imageTags[0];

            for (path_vec::const_iterator it (v.begin()); it != v.end(); ++it) {

                // set name of the current file name and remove "./"
                string name = it->string();
                name.erase(0,2);

                // make sure this is a directory
                // a sym link to a directory will pass this test
                if (!boost::filesystem::is_directory(*it)) {
                    //ASKAPLOG_INFO_STR(linmoslogger, name << " is not a directory. Ignoring.");
                    continue;
                }

                // see if the name contains the desired tag (i.e., contains the first tag in "names")
                size_t pos = name.find(searchTag);
                if (pos == string::npos) {
                    //ASKAPLOG_INFO_STR(linmoslogger, name << " is not a match. Ignoring.");
                    continue;
                }

                // set some variables for problem sub-strings
                string restored_tag = ".restored";
                size_t restored_pos;

                // see if the name contains a desired prefix, and if so, check the other input names and weights
                int full_set = 0, full_wgt_set = 0;
                string mosaicName = name, nextName = name, tmpName;
                for (vector<string>::const_iterator pre (prefixes.begin()); pre != prefixes.end(); ++pre) {
                    if (name.find(*pre) == 0) {

                        // both of these must remain set to 1 for this mosaic to be established
                        full_set = 1;
                        full_wgt_set = 1;

                        // set the output mosaic name
                        mosaicName.replace(pos, searchTag.length(), itsMosaicTag);

                        // file seems good, but check that it is present in all input images
                        for (uint img = 0; img < imageTags.size(); ++img) {

                            // name is initially set for image 0
                            nextName = name;
                            // replace the image 0 tag with the current image's tag
                            if (img > 0) {
                                nextName.replace(pos, searchTag.length(), imageTags[img]);
                                // check that the file exists
                                if (!boost::filesystem::exists(nextName)) {
                                    full_set = -1;
                                    break;
                                }
                            }
                            // add the image to this mosaics inputs
                            itsInImgNameVecs[mosaicName].push_back(nextName);

                            // see if there is an appropriate sensitivity image
                            tmpName = nextName;
                            tmpName.replace(0, (*pre).length(), "sensitivity");
                            // remove any ".restored" sub-string from the weights file name
                            restored_pos = tmpName.find(restored_tag);
                            if (restored_pos != string::npos) {
                                tmpName.replace(restored_pos, restored_tag.length(), "");
                            }
                            if (boost::filesystem::exists(tmpName)) {
                                itsInSenNameVecs[mosaicName].push_back(tmpName);
                            }

                            // look for weights image if required (weights are
                            // not needed when combining sensitivity images)
                            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                                // replace the prefix with "weights"
                                nextName.replace(0, (*pre).length(), "weights");
                                // remove any ".restored" sub-string from the weights file name
                                restored_pos = nextName.find(restored_tag);
                                if (restored_pos != string::npos) {
                                    nextName.replace(restored_pos, restored_tag.length(), "");
                                }
                                // check that the file exists
                                if (!boost::filesystem::exists(nextName)) {
                                    full_wgt_set = -1;
                                    break;
                                }
                                // add the file to this mosaics inputs
                                itsInWgtNameVecs[mosaicName].push_back(nextName);
                            }

                        }

                        // set the output weights image name
                        // replace the mosaic prefix with "weights"
                        nextName = mosaicName;
                        nextName.replace(0, (*pre).length(), "weights");
                        // remove any ".restored" sub-string from the weights file name
                        restored_pos = nextName.find(restored_tag);
                        if (restored_pos != string::npos) {
                            nextName.replace(restored_pos, restored_tag.length(), "");
                        }
                        itsOutWgtNames[mosaicName] = nextName;

                        itsGenSensitivityImage[mosaicName] = false;
                        if (itsInSenNameVecs.find(mosaicName)!=itsInSenNameVecs.end()) { // if key is found
                            if (itsInImgNameVecs[mosaicName].size()==itsInSenNameVecs[mosaicName].size()) {
                                itsGenSensitivityImage[mosaicName] = true;
                                // set an output sensitivity file name
                                tmpName = mosaicName;
                                tmpName.replace(0, (*pre).length(), "sensitivity");
                                // remove any ".restored" sub-string from the weights file name
                                restored_pos = tmpName.find(restored_tag);
                                if (restored_pos != string::npos) {
                                    tmpName.replace(restored_pos, restored_tag.length(), "");
                                }
                                itsOutSenNames[mosaicName] = tmpName;
                            } else {
                                itsInSenNameVecs.erase(mosaicName);
                            }
                        }

                        break; // found the prefix, so leave the loop

                    }
                }

                if (full_set==0) {
                    // this file did not have a relevant prefix, so just move on
                    continue;
                }

                if ((full_set == -1) || ((itsWeightType == FROM_WEIGHT_IMAGES) && (full_wgt_set == -1))) {
                    // this file did have a relevant prefix, but failed
                    if (full_set == -1) {
                        ASKAPLOG_INFO_STR(linmoslogger, mosaicName << " does not have a full set of input files. Ignoring.");
                    }
                    if ((itsWeightType == FROM_WEIGHT_IMAGES) && (full_wgt_set == -1)) {
                        ASKAPLOG_INFO_STR(linmoslogger, mosaicName << " does not have a full set of weights files. Ignoring.");
                    }

                    // if any of these were started for the current failed key, clean up and move on
                    if (itsOutWgtNames.find(mosaicName)!=itsOutWgtNames.end()) {
                        itsOutWgtNames.erase(mosaicName);
                    }
                    if (itsOutSenNames.find(mosaicName)!=itsOutSenNames.end()) {
                        itsOutSenNames.erase(mosaicName);
                    }
                    if (itsInImgNameVecs.find(mosaicName)!=itsInImgNameVecs.end()) {
                        itsInImgNameVecs.erase(mosaicName);
                    }
                    if (itsInWgtNameVecs.find(mosaicName)!=itsInWgtNameVecs.end()) {
                        itsInWgtNameVecs.erase(mosaicName);
                    }
                    if (itsInSenNameVecs.find(mosaicName)!=itsInSenNameVecs.end()) {
                        itsInSenNameVecs.erase(mosaicName);
                    }

                    continue;
                }

                // double check the size of the various maps and vectors. These should have been caught already
                ASKAPCHECK(itsInImgNameVecs.size()==itsOutWgtNames.size(),
                    mosaicName << ": inconsistent name maps.");
                if (itsWeightType == FROM_WEIGHT_IMAGES) {
                    ASKAPCHECK(itsInImgNameVecs.size()==itsInWgtNameVecs.size(),
                        mosaicName << ": mosaic search error. Inconsistent name maps.");
                    ASKAPCHECK(itsInImgNameVecs[mosaicName].size()==itsInWgtNameVecs[mosaicName].size(),
                        mosaicName << ": mosaic search error. Inconsistent name vectors.");
                }

                ASKAPLOG_INFO_STR(linmoslogger, mosaicName << " seems complete. Mosaicking.");

                // It is possible that there may be duplicate itsOutWgtNames/itsOutSenNames
                // (e.g. for image.* and residual.*).
                // Check that the input is the same for these duplicates, and then only write once
                // if this is common, we should be avoiding more that just duplicate output.
                string mosaicOrig;
                itsOutWgtDuplicates[mosaicName] = false;
                for(map<string,string>::iterator ii=itsOutWgtNames.begin();
                    ii!=itsOutWgtNames.find(mosaicName); ++ii) {
                    if (itsOutWgtNames[mosaicName].compare((*ii).second) == 0) {
                        itsOutWgtDuplicates[mosaicName] = true;
                        mosaicOrig = (*ii).first;
                        break;
                    }
                }

                // if this is a duplicate, just remove it. Can't with weights because we need them unaveraged
                if (itsOutSenNames.find(mosaicName)!=itsOutSenNames.end()) {
                    for(map<string,string>::iterator ii=itsOutSenNames.begin();
                        ii!=itsOutSenNames.find(mosaicName); ++ii) {
                        if (itsOutSenNames[mosaicName].compare((*ii).second) == 0) {
                            ASKAPLOG_INFO_STR(linmoslogger,
                                "  - sensitivity image done in an earlier mosaic. Will not redo here.");
                            itsGenSensitivityImage[mosaicName] = false;
                            itsOutSenNames.erase(mosaicName);
                            itsInSenNameVecs.erase(mosaicName);
                            break;
                        }
                    }
                }
                if (itsOutSenNames.find(mosaicName)!=itsOutSenNames.end()) {
                    ASKAPLOG_INFO_STR(linmoslogger,
                        "  - sensitivity images found. Generating mosaic sens. image.");
                }

            } // it loop (over potential images in this directory)

        } // void LinmosAccumulator<T>::findAndSetMosaics()

        template<typename T>
        bool LinmosAccumulator<T>::outputBufferSetupRequired(void) {
            return ( itsOutBuffer.shape().nelements() == 0 );
        }

        template<typename T>
        void LinmosAccumulator<T>::setInputParameters(const IPosition& inShape,
                                                      const CoordinateSystem& inCoordSys,
                                                      const int n) {
            // set the input coordinate system and shape
            itsInShape = inShape;
            itsInCoordSys = inCoordSys;

            if (itsWeightType == FROM_BP_MODEL) {
                // set the centre of the beam
                if ( itsCentres.size() > n ) {
                    itsInCentre = itsCentres[n];
                } else {
                    // no other information, so set the centre of the beam to be the reference pixel
                    const int dcPos = itsInCoordSys.findCoordinate(Coordinate::DIRECTION,-1);
                    const DirectionCoordinate inDC = itsInCoordSys.directionCoordinate(dcPos);
                    inDC.toWorld(itsInCentre,inDC.referencePixel());
                }
            }
        }

        template<typename T>
        void LinmosAccumulator<T>::setOutputParameters(const IPosition& outShape,
                                                       const CoordinateSystem& outCoordSys) {
            itsOutShape = outShape;
            itsOutCoordSys = outCoordSys;
        }

        template<typename T>
        void LinmosAccumulator<T>::setOutputParameters(const vector<IPosition>& inShapeVec,
                                                       const vector<CoordinateSystem>& inCoordSysVec) {

            ASKAPLOG_INFO_STR(linmoslogger, "Determining output image based on the overlap of input images");
            ASKAPCHECK(inShapeVec.size()==inCoordSysVec.size(), "Input vectors are inconsistent");
            ASKAPCHECK(inShapeVec.size()>0, "Number of input vectors should be greater that 0");

            const IPosition refShape = inShapeVec[0];
            ASKAPDEBUGASSERT(refShape.nelements() >= 2);
            const CoordinateSystem refCS = inCoordSysVec[0];
            const int dcPos = refCS.findCoordinate(Coordinate::DIRECTION,-1);
            const DirectionCoordinate refDC = refCS.directionCoordinate(dcPos);
            IPosition refBLC(refShape.nelements(),0);
            IPosition refTRC(refShape);
            for (uInt dim=0; dim<refShape.nelements(); ++dim) {
                --refTRC(dim); // these are added back later. Is this just to deal with degenerate axes?
            }
            ASKAPDEBUGASSERT(refBLC.nelements() >= 2);
            ASKAPDEBUGASSERT(refTRC.nelements() >= 2);

            IPosition tempBLC = refBLC;
            IPosition tempTRC = refTRC;

            // Loop over input vectors, converting their image bounds to the ref system
            // and expanding the new overlapping image bounds where appropriate.
            for (uint img = 1; img < inShapeVec.size(); ++img ) {

                itsInShape = inShapeVec[img];
                itsInCoordSys = inCoordSysVec[img];

                // test to see if the loaded coordinate system is close enough to the reference system for merging
                ASKAPCHECK(coordinatesAreConsistent(itsInCoordSys, refCS),
                    "Input images have inconsistent coordinate systems");
                // could also test whether they are equal and set a regrid tag to false if all of them are

                Vector<IPosition> corners = convertImageCornersToRef(refDC);

                const IPosition newBLC = corners[0];
                const IPosition newTRC = corners[1];
                ASKAPDEBUGASSERT(newBLC.nelements() >= 2);
                ASKAPDEBUGASSERT(newTRC.nelements() >= 2);
                for (casa::uInt dim=0; dim<2; ++dim) {
                    if (newBLC(dim) < tempBLC(dim)) {
                        tempBLC(dim) = newBLC(dim);
                    }
                    if (newTRC(dim) > tempTRC(dim)) {
                        tempTRC(dim) = newTRC(dim);
                    }
                }

            }

            itsOutShape = refShape;
            itsOutShape(0) = tempTRC(0) - tempBLC(0) + 1;
            itsOutShape(1) = tempTRC(1) - tempBLC(1) + 1;
            ASKAPDEBUGASSERT(itsOutShape(0) > 0);
            ASKAPDEBUGASSERT(itsOutShape(1) > 0);
            Vector<Double> refPix = refDC.referencePixel();
            refPix[0] -= Double(tempBLC(0) - refBLC(0));
            refPix[1] -= Double(tempBLC(1) - refBLC(1));
            DirectionCoordinate newDC(refDC);
            newDC.setReferencePixel(refPix);

            // set up a coord system for the merged images
            itsOutCoordSys = refCS;
            itsOutCoordSys.replaceCoordinate(newDC, dcPos);

        }

        template<typename T>
        void LinmosAccumulator<T>::initialiseOutputBuffers(void) {
            // set up temporary images needed for regridding (which is
            // done on a plane-by-plane basis so ignore other dims)

            // set up the coord. sys.
            int dcPos = itsOutCoordSys.findCoordinate(Coordinate::DIRECTION,-1);
            ASKAPCHECK(dcPos>=0, "Cannot find the directionCoordinate");
            const DirectionCoordinate dcTmp = itsOutCoordSys.directionCoordinate(dcPos);
            CoordinateSystem cSysTmp;
            cSysTmp.addCoordinate(dcTmp);

            // set up the shape
            Vector<Int> shapePos = itsOutCoordSys.pixelAxes(dcPos);
            // check that the length is equal to 2 and the both elements are >= 0
            ASKAPCHECK(shapePos.nelements()>=2, "Cannot find the directionCoordinate");
            ASKAPCHECK((shapePos[0]==0 && shapePos[1]==1) || (shapePos[1]==0 && shapePos[0]==1),
                       "Linmos currently requires the direction coordinates to come before any others");

            IPosition shape = IPosition(2,itsOutShape(shapePos[0]),itsOutShape(shapePos[1]));

            // apparently the +100 forces it to use the memory
            double maxMemoryInMB = double(shape.product()*sizeof(T))/1024./1024.+100;
            itsOutBuffer = TempImage<T>(shape, cSysTmp, maxMemoryInMB);
            ASKAPCHECK(itsOutBuffer.shape().nelements()>0, "Output buffer does not appear to be set");
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                itsOutWgtBuffer = TempImage<T>(shape, cSysTmp, maxMemoryInMB);
                ASKAPCHECK(itsOutWgtBuffer.shape().nelements()>0,
                    "Output weights buffer does not appear to be set");
            }
            if (itsDoSensitivity) {
                itsOutSnrBuffer = TempImage<T>(shape, cSysTmp, maxMemoryInMB);
                ASKAPCHECK(itsOutSnrBuffer.shape().nelements()>0,
                    "Output sensitivity buffer does not appear to be set");
            }
        }

        template<typename T>
        void LinmosAccumulator<T>::initialiseInputBuffers() {
            // set up temporary images needed for regridding (which is
            // done on a plane-by-plane basis so ignore other dims)

            // set up a coord. sys. the planes
            int dcPos = itsInCoordSys.findCoordinate(Coordinate::DIRECTION,-1);
            ASKAPCHECK(dcPos>=0, "Cannot find the directionCoordinate");
            const DirectionCoordinate dc = itsInCoordSys.directionCoordinate(dcPos);
            CoordinateSystem cSys;
            cSys.addCoordinate(dc);

            // set up the shape
            Vector<Int> shapePos = itsInCoordSys.pixelAxes(dcPos);
            // check that the length is equal to 2 and the both elements are >= 0

            IPosition shape = IPosition(2,itsInShape(shapePos[0]),itsInShape(shapePos[1]));

            double maxMemoryInMB = double(shape.product()*sizeof(T))/1024./1024.+100;
            itsInBuffer = TempImage<T>(shape,cSys,maxMemoryInMB);
            ASKAPCHECK(itsInBuffer.shape().nelements()>0, "Input buffer does not appear to be set");
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                itsInWgtBuffer = TempImage<T>(shape,cSys,maxMemoryInMB);
                ASKAPCHECK(itsInWgtBuffer.shape().nelements()>0,
                    "Input weights buffer does not appear to be set");
            }
            if (itsDoSensitivity) {
                itsInSenBuffer = TempImage<T>(shape,cSys,maxMemoryInMB);
                itsInSnrBuffer = TempImage<T>(shape,cSys,maxMemoryInMB);
                ASKAPCHECK(itsInSnrBuffer.shape().nelements()>0,
                    "Input sensitivity buffer does not appear to be set");
            }

        }

        template<typename T>
        void LinmosAccumulator<T>::initialiseRegridder() {
            ASKAPLOG_INFO_STR(linmoslogger, "Initialising regridder for " << itsMethod << " interpolation");
            itsAxes = IPosition::makeAxisPath(itsOutBuffer.shape().nelements());
            itsEmethod = Interpolate2D::stringToMethod(itsMethod);
        }

        template<typename T>
        void LinmosAccumulator<T>::loadInputBuffers(const scimath::MultiDimArrayPlaneIter& planeIter,
                                                    Array<T>& inPix,
                                                    Array<T>& inWgtPix,
                                                    Array<T>& inSenPix) {
            itsInBuffer.put(planeIter.getPlane(inPix));
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                itsInWgtBuffer.put(planeIter.getPlane(inWgtPix));
            }
            if (itsDoSensitivity) {
                // invert sensitivities before regridding to avoid
                // artefacts at sharp edges in the sensitivity image
                itsInSenBuffer.put(planeIter.getPlane(inSenPix));
                T sensitivity;

                IPosition pos(2);
                for (int x=0; x<inSenPix.shape()[0];++x) {
                    for (int y=0; y<inSenPix.shape()[1];++y) {
                        pos[0] = x;
                        pos[1] = y;
                        sensitivity = itsInSenBuffer.getAt(pos);
                        if (sensitivity>0) {
                            itsInSnrBuffer.putAt(1.0 / (sensitivity * sensitivity), pos);
                        } else {
                            itsInSnrBuffer.putAt(0.0, pos);
                        }
                    }
                }
            }
        }

        template<typename T>
        void LinmosAccumulator<T>::regrid() {
            ASKAPLOG_INFO_STR(linmoslogger, " - regridding with dec="<<
                itsDecimate<<" rep="<<itsReplicate<<" force="<<itsForce);
            ASKAPCHECK(itsOutBuffer.shape().nelements()>0,
                "Output buffer does not appear to be set");
            itsRegridder.regrid(itsOutBuffer, itsEmethod, itsAxes, itsInBuffer,
                                itsReplicate, itsDecimate, false, itsForce);
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                itsRegridder.regrid(itsOutWgtBuffer, itsEmethod, itsAxes,
                    itsInWgtBuffer, itsReplicate, itsDecimate, false, itsForce);
            }
            if (itsDoSensitivity) {
                itsRegridder.regrid(itsOutSnrBuffer, itsEmethod, itsAxes,
                    itsInSnrBuffer, itsReplicate, itsDecimate, false, itsForce);
            }

        }

        template<typename T>
        void LinmosAccumulator<T>::accumulatePlane(Array<T>& outPix,
                                                   Array<T>& outWgtPix,
                                                   Array<T>& outSenPix,
                                                   const IPosition& curpos) {

            // copy the pixel iterator containing all dimensions
            IPosition fullpos(curpos);
            // set a pixel iterator that does not have the higher dimensions
            IPosition pos(2);

            // set the weights, either to those read in or using the primary-beam model
            TempImage<T> wgtBuffer;
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                wgtBuffer = itsOutWgtBuffer;
            } else {

                Vector<double> pixel(2,0.);
                MVDirection world0, world1;
                T offsetBeam, pb;

                // get coordinates of the spectral axis and the current frequency
                const int scPos = itsInCoordSys.findCoordinate(Coordinate::SPECTRAL,-1);
                const SpectralCoordinate inSC = itsInCoordSys.spectralCoordinate(scPos);
                int chPos = itsInCoordSys.pixelAxes(scPos)[0];
                const T freq = inSC.referenceValue()[0] +
                    (curpos[chPos] - inSC.referencePixel()[0]) * inSC.increment()[0];

                // set FWHM for the current beam
                // Removing the factor of 1.22 gives a good match to the simultation weight images
                //const T fwhm = 1.22*3e8/freq/12;
                //const T fwhm = 3e8/freq/12;

                // get coordinates of the direction axes
                const int dcPos = itsInCoordSys.findCoordinate(Coordinate::DIRECTION,-1);
                const DirectionCoordinate inDC = itsInCoordSys.directionCoordinate(dcPos);
                const DirectionCoordinate outDC = itsOutCoordSys.directionCoordinate(dcPos);

                // set the centre of the input beam (needs to be more flexible -- and correct...)
                inDC.toWorld(world0,inDC.referencePixel());

                // apparently the +100 forces it to use the memory
                double maxMemoryInMB = double(itsOutBuffer.shape().product()*sizeof(T))/1024./1024.+100;
                wgtBuffer = TempImage<T>(itsOutBuffer.shape(), itsOutBuffer.coordinates(), maxMemoryInMB);

                // step through the pixels, setting the weights (power primary beam squared)
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        pos[0] = x;
                        pos[1] = y;

                        // get the current pixel location and distance from beam centre
                        pixel[0] = double(x);
                        pixel[1] = double(y);
                        outDC.toWorld(world1,pixel);
                        offsetBeam = world0.separation(world1);

                        // set the weight
                        //pb = exp(-offsetBeam*offsetBeam*4.*log(2.)/fwhm/fwhm);
                        pb = itsPB->evaluateAtOffset(offsetBeam,freq);
                        wgtBuffer.putAt(pb * pb, pos);

                    }
                }

            }

            T minVal, maxVal;
            IPosition minPos, maxPos;
            minMax(minVal,maxVal,minPos,maxPos,wgtBuffer);
            T wgtCutoff = itsCutoff * itsCutoff * maxVal; // wgtBuffer is prop. to image (gain/sigma)^2

            // Accumulate the pixels of this slice.
            // Could restrict it (and the regrid) to a smaller region of interest.
            if (itsWeightState == CORRECTED) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        pos[0] = x;
                        pos[1] = y;
                        if (wgtBuffer.getAt(pos)>=wgtCutoff) {
                            outPix(fullpos) = outPix(fullpos) + itsOutBuffer.getAt(pos) * wgtBuffer.getAt(pos);
                            outWgtPix(fullpos) = outWgtPix(fullpos) + wgtBuffer.getAt(pos);
                        }
                    }
                }
            } else if (itsWeightState == INHERENT) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        pos[0] = x;
                        pos[1] = y;
                        if (wgtBuffer.getAt(pos)>=wgtCutoff) {
                            outPix(fullpos) = outPix(fullpos) + itsOutBuffer.getAt(pos) * sqrt(wgtBuffer.getAt(pos));
                            outWgtPix(fullpos) = outWgtPix(fullpos) + wgtBuffer.getAt(pos);
                        }
                    }
                }
            } else if (itsWeightState == WEIGHTED) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        pos[0] = x;
                        pos[1] = y;
                        if (wgtBuffer.getAt(pos)>=wgtCutoff) {
                            outPix(fullpos) = outPix(fullpos) + itsOutBuffer.getAt(pos);
                            outWgtPix(fullpos) = outWgtPix(fullpos) + wgtBuffer.getAt(pos);
                        }
                    }
                }
            }
            // Accumulate sensitivity for this slice.
            if (itsDoSensitivity) {
                T invVariance;
                minMax(minVal,maxVal,minPos,maxPos,itsOutSnrBuffer);
                T snrCutoff = itsCutoff * itsCutoff * maxVal;
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        pos[0] = x;
                        pos[1] = y;
                        invVariance = itsOutSnrBuffer.getAt(pos);
                        if (invVariance>=snrCutoff && wgtBuffer.getAt(pos)>=wgtCutoff) {
                            outSenPix(fullpos) = outSenPix(fullpos) + invVariance;
                        }
                    }
                }
            }

        }

        template<typename T>
        void LinmosAccumulator<T>::accumulatePlane(Array<T>& outPix,
                                                   Array<T>& outWgtPix,
                                                   Array<T>& outSenPix,
                                                   const Array<T>& inPix,
                                                   const Array<T>& inWgtPix,
                                                   const Array<T>& inSenPix,
                                                   const IPosition& curpos) {

            ASKAPASSERT(inPix.shape() == outPix.shape());

            // copy the pixel iterator containing all dimensions
            IPosition fullpos(curpos);
            // set up an indexing vector for the weights. If weight images are used, these are as in the image.
            IPosition wgtpos(curpos);

            Array<T> wgtPix;

            // set the weights, either to those read in or using the primary-beam model
            if (itsWeightType == FROM_WEIGHT_IMAGES) {
                wgtPix.reference(inWgtPix);
            } else {

                Vector<double> pixel(2,0.);
                MVDirection world;
                T offsetBeam, pb;

                // get coordinates of the spectral axis and the current frequency
                const int scPos = itsInCoordSys.findCoordinate(Coordinate::SPECTRAL,-1);
                const SpectralCoordinate inSC = itsInCoordSys.spectralCoordinate(scPos);
                int chPos = itsInCoordSys.pixelAxes(scPos)[0];
                const T freq = inSC.referenceValue()[0] +
                    (curpos[chPos] - inSC.referencePixel()[0]) * inSC.increment()[0];

                // set FWHM for the current beam
                // Removing the factor of 1.22 gives a good match to the simultation weight images
                //const T fwhm = 1.22*3e8/freq/12;
                //const T fwhm = 3e8/freq/12;

                // get coordinates of the direction axes
                const int dcPos = itsInCoordSys.findCoordinate(Coordinate::DIRECTION,-1);
                const DirectionCoordinate outDC = itsOutCoordSys.directionCoordinate(dcPos);

                // set the higher-order dimension to zero, as weights are on a 2D plane
                for (uInt dim=0; dim<curpos.nelements(); ++dim) {
                    wgtpos[dim] = 0;
                }
                // set the array
                wgtPix = Array<T>(itsInShape);

                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        wgtpos[0] = x;
                        wgtpos[1] = y;

                        // get the current pixel location and distance from beam centre
                        pixel[0] = double(x);
                        pixel[1] = double(y);
                        outDC.toWorld(world,pixel);
                        offsetBeam = itsInCentre.separation(world);

                        // set the weight
                        //pb = exp(-offsetBeam*offsetBeam*4.*log(2.)/fwhm/fwhm);
                        pb = itsPB->evaluateAtOffset(offsetBeam,freq);
                        wgtPix(wgtpos) = pb * pb;

                    }
                }
            }

            T minVal, maxVal;
            IPosition minPos, maxPos;
            minMax(minVal,maxVal,minPos,maxPos,wgtPix);
            T wgtCutoff = itsCutoff * itsCutoff * maxVal; // wgtPix is prop. to image (gain/sigma)^2

            if (itsWeightState == CORRECTED) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        wgtpos[0] = x;
                        wgtpos[1] = y;
                        if (wgtPix(wgtpos)>=wgtCutoff) {
                            outPix(fullpos)    = outPix(fullpos)    + inPix(fullpos) * wgtPix(wgtpos);
                            outWgtPix(fullpos) = outWgtPix(fullpos) + wgtPix(wgtpos);
                        }
                    }
                }
            } else if (itsWeightState == INHERENT) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        wgtpos[0] = x;
                        wgtpos[1] = y;
                        if (wgtPix(wgtpos)>=wgtCutoff) {
                            outPix(fullpos)    = outPix(fullpos)    + inPix(fullpos) * sqrt(wgtPix(wgtpos));
                            outWgtPix(fullpos) = outWgtPix(fullpos) + wgtPix(wgtpos);
                        }
                    }
                }
            } else if (itsWeightState == WEIGHTED) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        wgtpos[0] = x;
                        wgtpos[1] = y;
                        if (wgtPix(wgtpos)>=wgtCutoff) {
                            outPix(fullpos)    = outPix(fullpos)    + inPix(fullpos);
                            outWgtPix(fullpos) = outWgtPix(fullpos) + wgtPix(wgtpos);
                        }
                    }
                }
            }
            // Accumulate sensitivity for this slice.
            if (itsDoSensitivity) {
                double sensitivity;
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        sensitivity = inSenPix(fullpos);
                        // wgt and sen should be aligned.
                        if (wgtPix(wgtpos)>=wgtCutoff && sensitivity>0.0) {
                            outSenPix(fullpos) = outSenPix(fullpos) + 1.0 / (sensitivity * sensitivity);
                        }
                    }
                }
            }

        }

        template<typename T>
        void LinmosAccumulator<T>::deweightPlane(Array<T>& outPix,
                                                 const Array<T>& outWgtPix,
                                                 Array<T>& outSenPix,
                                                 const IPosition& curpos) {

            T minVal, maxVal;
            IPosition minPos, maxPos;
            minMax(minVal,maxVal,minPos,maxPos,outWgtPix);

            // copy the pixel iterator containing all dimensions
            IPosition fullpos(curpos);

            for (int x=0; x<outPix.shape()[0];++x) {
                for (int y=0; y<outPix.shape()[1];++y) {
                    fullpos[0] = x;
                    fullpos[1] = y;
                    if (isNaN(outWgtPix(fullpos))) {
                        setNaN(outPix(fullpos));
                    }
                    else if (outWgtPix(fullpos)>0.0) {
                        outPix(fullpos) = outPix(fullpos) / outWgtPix(fullpos);
                    } else {
                        outPix(fullpos) = 0.0;
                    }
                }
            }

            if (itsDoSensitivity) {
                for (int x=0; x<outPix.shape()[0];++x) {
                    for (int y=0; y<outPix.shape()[1];++y) {
                        fullpos[0] = x;
                        fullpos[1] = y;
                        if (isNaN(outWgtPix(fullpos))) {
                            setNaN(outSenPix(fullpos));
                        }
                        else if (outSenPix(fullpos)>0.0) {
                            outSenPix(fullpos) = sqrt(1.0 / outSenPix(fullpos));
                        } else {
                            outSenPix(fullpos) = 0.0;
                        }
                    }
                }
            }

        }

        template<typename T>
        Vector<IPosition> LinmosAccumulator<T>::convertImageCornersToRef(const DirectionCoordinate& refDC) {
            // based on SynthesisParamsHelper::facetSlicer, but don't want
            // to load every input image into a scimath::Param

            ASKAPDEBUGASSERT(itsInShape.nelements() >= 2);
            // add more checks

            const int coordPos = itsInCoordSys.findCoordinate(Coordinate::DIRECTION,-1);
            const DirectionCoordinate inDC = itsInCoordSys.directionCoordinate(coordPos);

            IPosition blc(itsInShape.nelements(),0);
            IPosition trc(itsInShape);
            for (uInt dim=0; dim<itsInShape.nelements(); ++dim) {
                 --trc(dim); // these are added back later. Is this just to deal with degenerate axes?
            }
            // currently blc,trc describe the whole input image; convert coordinates
            Vector<Double> pix(2);

            // first process BLC
            pix[0] = Double(blc[0]);
            pix[1] = Double(blc[1]);
            MDirection tempDir;
            Bool success = inDC.toWorld(tempDir, pix);
            ASKAPCHECK(success,
                "Pixel to world coordinate conversion failed for input BLC: "
                << inDC.errorMessage());
            success = refDC.toPixel(pix,tempDir);
            ASKAPCHECK(success,
                "World to pixel coordinate conversion failed for output BLC: "
                << refDC.errorMessage());
            blc[0] = casa::Int(round(pix[0]));
            blc[1] = casa::Int(round(pix[1]));

            // now process TRC
            pix[0] = Double(trc[0]);
            pix[1] = Double(trc[1]);
            success = inDC.toWorld(tempDir, pix);
            ASKAPCHECK(success,
                "Pixel to world coordinate conversion failed for input TRC: "
                << inDC.errorMessage());
            success = refDC.toPixel(pix,tempDir);
            ASKAPCHECK(success,
                "World to pixel coordinate conversion failed for output TRC: "
                << refDC.errorMessage());
            trc[0] = casa::Int(round(pix[0]));
            trc[1] = casa::Int(round(pix[1]));

            Vector<IPosition> corners(2);
            corners[0] = blc;
            corners[1] = trc;

            return corners;

        }

        template<typename T>
        bool LinmosAccumulator<T>::coordinatesAreConsistent(const CoordinateSystem& coordSys1,
                                                            const CoordinateSystem& coordSys2) {
            // Check to see if it makes sense to combine images with these coordinate systems.
            // Could get more tricky, but right now make sure any extra dimensions, such as frequency
            // and polarisation, are equal in the two systems.
            if ( coordSys1.nCoordinates() != coordSys2.nCoordinates() ) {
                //ASKAPLOG_INFO_STR(linmoslogger,
                //    "Coordinates are not consistent: dimension mismatch");
                return false;
            }
            if (!allEQ(coordSys1.worldAxisNames(), coordSys2.worldAxisNames())) {
                //ASKAPLOG_INFO_STR(linmoslogger,
                //    "Coordinates are not consistent: axis name mismatch");
                return false;
            }
            if (!allEQ(coordSys1.worldAxisUnits(), coordSys2.worldAxisUnits())) {
                //ASKAPLOG_INFO_STR(linmoslogger,
                //    "Coordinates are not consistent: axis unit mismatch");
                return false;
            }
            return true;
        }

        // Check that the input coordinate system is the same as the output
        template<typename T>
        bool LinmosAccumulator<T>::coordinatesAreEqual(void) {
            return coordinatesAreEqual(itsInCoordSys, itsOutCoordSys,
                                       itsInShape, itsOutShape);
        }

        // Check that two coordinate systems are the same
        template<typename T>
        bool LinmosAccumulator<T>::coordinatesAreEqual(const CoordinateSystem& coordSys1,
                                                       const CoordinateSystem& coordSys2,
                                                       const IPosition& shape1,
                                                       const IPosition& shape2) {

            // Set threshold for allowed small numerical differences
            double thresh = 1.0e-12;

            // Check that the shape is the same.
            if ( shape1 != shape2 ) {
                //ASKAPLOG_INFO_STR(linmoslogger,
                //    "Coordinates not equal: shape mismatch");
                return false;
            }

            // Check that the systems have consistent axes.
            if ( !coordinatesAreConsistent(coordSys1, coordSys2) ) {
                return false;
            }

            // test that the axes are equal
            for (casa::uInt dim=0; dim<coordSys1.nCoordinates(); ++dim) {
                if ( (coordSys1.referencePixel()[dim] != coordSys2.referencePixel()[dim]) ||
                     (fabs(coordSys1.referenceValue()[dim] - coordSys2.referenceValue()[dim]) > thresh) ||
                     (fabs(coordSys1.increment()[dim] - coordSys2.increment()[dim]) > thresh) ) {
                    //ASKAPLOG_INFO_STR(linmoslogger,
                    //    "Coordinates not equal: mismatch for dim " << dim);
                    return false;
                }
            }
            return true;
        }

    } // namespace imagemath

} // namespace askap

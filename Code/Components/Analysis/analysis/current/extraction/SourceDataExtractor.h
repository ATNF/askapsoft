/// @file
///
/// Base class for handling extraction of image data corresponding to a source
///
/// @copyright (c) 2011 CSIRO
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
#ifndef ASKAP_ANALYSIS_EXTRACTOR_H_
#define ASKAP_ANALYSIS_EXTRACTOR_H_
#include <askap_analysis.h>
#include <string>
#include <sourcefitting/RadioSource.h>
#include <catalogues/CasdaComponent.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Slicer.h>
#include <casacore/images/Images/ImageInterface.h>
#include <casacore/lattices/Lattices/LatticeBase.h>
#include <casacore/casa/Quanta/Unit.h>
#include <Common/ParameterSet.h>
#include <casacore/measures/Measures/Stokes.h>
#include <utils/PolConverter.h>
#include <boost/shared_ptr.hpp>

using namespace askap::analysis::sourcefitting;

namespace askap {
namespace analysis {


/// @brief The base class for handling the extraction of
/// different types of image data that correspond to a source.
/// @details The types of extraction envisaged include
/// extraction of an integrated spectrum of a source (either
/// summed over a box or integrated over the entirety of an
/// extended object), extraction of a subcube ("cubelet"),
/// extraction of a moment-0 map. Access to multiple input
/// images for different Stokes parameters is possible. This
/// base class details the basic functionality, and implements
/// constructors, input image verification, and opening of the
/// image.

class SourceDataExtractor {
    public:
        SourceDataExtractor() {};
        SourceDataExtractor(const LOFAR::ParameterSet& parset);
        virtual ~SourceDataExtractor();

        /// @brief Set the pointer to the source, and define the output filename based on its ID
        /// @details Sets the source to be used. Also sets the output
        /// filename correctly with the suffix indicating the object's
        /// ID.
        /// @param src The RadioSource detection used to centre the
        /// spectrum. The central pixel will be chosen to be the peak
        /// pixel, so this needs to be defined.
        void setSource(RadioSource *src);
        void setSource(CasdaComponent *src);

        virtual void extract() = 0;

        casa::Array<Float> array() {return itsArray;};
        std::string inputCube() {return itsInputCube;};
        std::vector<std::string> inputCubeList() {return itsInputCubeList;};
        std::string outputFileBase() {return itsOutputFilenameBase;};
        std::string outputFile() {return itsOutputFilename;};
        RadioSource* source() {return itsSource;};
        std::vector<std::string> polarisations()
        {
            return scimath::PolConverter::toString(itsStokesList);
        };
    casa::Unit bunit();
        virtual void writeImage() = 0;

    protected:
        bool openInput();
        void closeInput();
        virtual void verifyInputs();
        virtual void defineSlicer() = 0;

    /// Check whether the given image contains the given stokes parameter.
        bool checkPol(std::string image, casa::Stokes::StokesTypes stokes);
        casa::IPosition getShape(std::string image);
        virtual void initialiseArray() = 0;
        template <class T> double getRA(T &object);
        template <class T> double getDec(T &object);
        template <class T> std::string getID(T &object);
        template <class T> void setSourceLoc(T *src);
        void writeBeam(std::string &filename);

        RadioSource* itsSource;
        std::string itsSourceID;
        std::string itsCentreType;
        casa::Slicer itsSlicer;
        std::string itsInputCube;
        std::vector<std::string> itsInputCubeList;
    std::map<casa::Stokes::StokesTypes,std::string> itsCubeStokesMap;
        boost::shared_ptr<casa::ImageInterface<Float> > itsInputCubePtr;
        casa::Vector<casa::Stokes::StokesTypes> itsStokesList;
        casa::Stokes::StokesTypes itsCurrentStokes;
        std::string itsOutputFilenameBase;
        std::string itsOutputFilename;
        casa::Array<Float> itsArray;

        float itsXloc;
        float itsYloc;

        casa::CoordinateSystem itsInputCoords;
        int  itsLngAxis;
        int  itsLatAxis;
        int  itsSpcAxis;
        int  itsStkAxis;
    casa::Unit itsInputUnits;

    casa::TableRecord itsMiscInfo;
};

}
}

#endif

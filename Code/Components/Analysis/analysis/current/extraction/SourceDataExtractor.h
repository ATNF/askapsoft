/// @file
///
/// Base class for handling extraction of image data corresponding to a source
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
#include <boost/scoped_ptr.hpp>

using namespace askap::analysis::sourcefitting;

namespace askap {
namespace analysis {


/// @brief The base class for handling the extraction of
/// different types of image data that correspond to a source.
/// @details The types of extraction envisaged include
/// extraction of an integrated spectrum of a source (either
/// summed over a box or integrated over the entirety of an
/// extended object), extraction of a subcube ("cubelet"),
/// extraction of a moment map. Access to multiple input
/// images for different Stokes parameters is possible. This
/// base class details the basic functionality, and implements
/// constructors, input image verification, and opening of the
/// image.

class SourceDataExtractor {
    public:
        SourceDataExtractor() {};
        SourceDataExtractor(const LOFAR::ParameterSet& parset);
        virtual ~SourceDataExtractor();

        /// @brief Methods to define the source and its location in the
        /// pixel grid of the input cube.
        /// @details These functions identify the RA & Dec of the source,
        /// and transform to pixel coordinates in the frame of the input
        /// image. They also identify the source ID string, which is used
        /// in the output filename.
        /// @{
        /// @brief Define the source properties for a RadioSource object
        void setSource(RadioSource *src);
        /// @brief Define the source properties for a Component
        void setSource(CasdaComponent *src);
        /// @}

        /// @brief Method to extract the desired array - left up to the
        /// derived classes to define.
        virtual void extract() = 0;
        /// @brief Method to write the output image - left up to the
        /// derived classes to define
        virtual void writeImage() = 0;

        /// @brief Return the extracted array
        casa::Array<Float> array() {return itsArray;};
        /// @brief Return the input cube name
        std::string inputCube() {return itsInputCube;};
        /// @brief Return the list of all possible input cubes
        std::vector<std::string> inputCubeList() {return itsInputCubeList;};
        /// @brief Return the base name of the output image(s)
        std::string outputFileBase() {return itsOutputFilenameBase;};
        /// @brief Return the current output image name
        std::string outputFile() {return itsOutputFilename;};
        /// @brief Return the pointer to the provided RadioSource (if
        /// none was provided this will be null).
        RadioSource* source() {return itsSource;};
        /// @brief Return the pointer to the provided CasdaComponent
        /// (if none was provided this will be null).
        CasdaComponent* component() {return itsComponent;};
        /// @brief Return the pixel location of the source in the
        /// x-direction
        float srcXloc() {return itsXloc;};
        /// @brief Return the pixel location of the source in the
        /// y-direction
        float srcYloc() {return itsYloc;};
        /// @brief Return the source's ID string
        std::string sourceID() {return itsSourceID;};
        /// @brief Return the list of Stokes parameters
        std::vector<std::string> polarisations()
        {
            return scimath::PolConverter::toString(itsStokesList);
        };
        /// @brief Return the brightness unit for the current input
        /// image
        casa::Unit bunit();


    protected:
        /// @brief Open the input cube.
        /// @details Defines itsInputCubePtr and, assuming it is valid,
        /// sets various coordinate and unit members.
        bool openInput();

        /// @brief Close the input cube
        void closeInput();

        /// @brief Verify the set of input cubes conform
        /// @details This involves checking the list of polarisations, and
        /// ensuring there is a cube for each requested polarisation. The
        /// shape of each cube must be the same as well.
        virtual void verifyInputs();

        /// @brief Function to define the slicer used for extraction - not
        /// defined in the base class as it will differ for the different
        /// derived classes.
        virtual void defineSlicer() = 0;

        /// @brief Check whether the given image contains the given
        /// stokes parameter.
        /// @param image Name of the image to check
        /// @param stokes Stokes type to check for.
        bool checkPol(std::string image, casa::Stokes::StokesTypes stokes);

        /// @brief Return the shape of the given image
        /// @details Internal function only that uses openImage()
        /// @param image Name of image to examine.
        casa::IPosition getShape(std::string image);

        /// @brief Initialise the array for the extracted data -
        /// implementation left for derived classes
        virtual void initialiseArray() = 0;

        /// @brief Return the RA for a given object (RadioSource or
        /// CasdaComponent)
        template <class T> double getRA(T &object);
        /// @brief Return the Dec for a given object (RadioSource or
        /// CasdaComponent)
        template <class T> double getDec(T &object);
        /// @brief Return the ID string for a given object
        /// (RadioSource or CasdaComponent)
        template <class T> std::string getID(T &object);
        /// @brief Set the source's pixel location based on the RA &
        /// Dec, and the WCS of the input cube
        template <class T> void setSourceLoc(T *src);

        /// @brief Write out the restoring beam from the current input
        /// cube to the given filename
        /// @param filename The image to which the restoring beam of the
        /// current input cube is written.
        void writeBeam(std::string &filename);


        // Members

        /// @brief The RadioSource being used - if not provided,
        /// pointer remains null
        RadioSource                                     *itsSource;
        /// @brief The Component being used - if not provided, pointer
        /// remains null
        CasdaComponent                                  *itsComponent;
        /// @brief The source's ID string
        std::string                                      itsSourceID;
        /// @brief The slicer used to perform the extraction
        casa::Slicer                                     itsSlicer;
        /// @brief The input cube the array is extracted from
        std::string                                      itsInputCube;
        /// @brief The list of potential input cubes - typically one
        /// per Stokes parameter
        std::vector<std::string>                         itsInputCubeList;
        /// @brief Mapping between input cubes and Stokes parameters
        std::map<casa::Stokes::StokesTypes, std::string> itsCubeStokesMap;
        /// @brief The image interface pointer, used to access the
        /// input image on disk
        boost::shared_ptr<casa::ImageInterface<Float> >  itsInputCubePtr;
        /// @brief The list of desired Stokes parameters
        casa::Vector<casa::Stokes::StokesTypes>          itsStokesList;
        /// @brief The Stokes parameter currently being used
        casa::Stokes::StokesTypes                        itsCurrentStokes;
        /// @brief The base for the output filename, that can be added
        /// to to make the actual output filename
        std::string                                      itsOutputFilenameBase;
        /// @brief The name of the output file
        std::string                                      itsOutputFilename;
        /// @brief The array of extracted pixels
        casa::Array<Float>                               itsArray;
        /// @brief The pixel location of the source in the x-direction
        float                                            itsXloc;
        /// @brief The pixel location of the source in the y-direction
        float                                            itsYloc;
        /// @brief The coordinate system of the input cube
        casa::CoordinateSystem                           itsInputCoords;
        /// @brief The axis number for the longitude axis (0-based)
        int                                              itsLngAxis;
        /// @brief The axis number for the latitude axis (0-based)
        int                                              itsLatAxis;
        /// @brief The axis number for the spectral axis (0-based)
        int                                              itsSpcAxis;
        /// @brief The axis number for the Stokes axis (0-based)
        int                                              itsStkAxis;
        /// @brief The brightness units of the input cube
        casa::Unit                                       itsInputUnits;
        /// @brief Miscellaneous information for the output image
        casa::TableRecord                                itsMiscInfo;
};

}
}

#endif

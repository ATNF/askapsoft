/// @file
///
/// Making a curvature map for use with the Gaussian fitting in Selavy
///
/// @copyright (c) 2018 CSIRO
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
#ifndef ASKAP_ANALYSIS_CURVATURE_MAP_H_
#define ASKAP_ANALYSIS_CURVATURE_MAP_H_
#include <askapparallel/AskapParallel.h>
#include <Common/ParameterSet.h>
#include <string>
#include <casacore/casa/Arrays/MaskedArray.h>
#include <casacore/casa/Arrays/IPosition.h>
#include <outputs/ImageWriter.h>
#include <analysisparallel/SubimageDef.h>
#include <parallelanalysis/Weighter.h>
#include <duchamp/Cubes/cubes.hh>

namespace askap {

namespace analysis {

class CurvatureMapCreator {
    public:
        CurvatureMapCreator() {};
        CurvatureMapCreator(askap::askapparallel::AskapParallel &comms,
                            const LOFAR::ParameterSet &parset);
        virtual ~CurvatureMapCreator() {};

        void setCube(duchamp::Cube &cube) {itsCube = &cube;};

        /// @details Initialise the class with information from
        /// the duchamp::Cube. This is done to avoid replicating
        /// parameters and preserving the parameter
        /// hierarchy. Once the input image is known, the output
        /// image names can be set with fixName() (if they have
        /// not been defined via the parset).
        void initialise(duchamp::Cube &cube,
                        analysisutilities::SubimageDef &subdef,
                        boost::shared_ptr<Weighter> weighter);

        void calculate();
        void maskBorders();
        void write();

        float sigmaCurv() {return itsSigmaCurv;};

    protected:
        void findSigma();

        askap::askapparallel::AskapParallel *itsComms;
        LOFAR::ParameterSet itsParset;
        duchamp::Cube *itsCube;
        analysisutilities::SubimageDef *itsSubimageDef;
        boost::shared_ptr<Weighter> itsWeighter;
        std::string itsFilename;
        casa::MaskedArray<float> itsArray;
        casa::IPosition itsShape;
        casa::IPosition itsLocation;
        float itsSigmaCurv;
};

}

}

#endif

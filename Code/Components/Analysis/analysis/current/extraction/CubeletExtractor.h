/// @file
///
/// XXX Notes on program XXX
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
/// @author XXX XXX <XXX.XXX@csiro.au>
///
#ifndef ASKAP_ANALYSIS_EXTRACTION_CUBELET_EXTRACTOR_H_
#define ASKAP_ANALYSIS_EXTRACTION_CUBELET_EXTRACTOR_H_
#include <askap_analysis.h>
#include <extraction/SourceDataExtractor.h>
#include <Common/ParameterSet.h>

namespace askap {

namespace analysis {


class CubeletExtractor : public SourceDataExtractor {
    public:
        CubeletExtractor() {};
        CubeletExtractor(const CubeletExtractor& other);
        CubeletExtractor(const LOFAR::ParameterSet& parset);
        CubeletExtractor& operator= (const CubeletExtractor& other);
        virtual ~CubeletExtractor() {};

        void extract();
        void writeImage();

    protected:
        void defineSlicer();
        void initialiseArray();

        /// For the box method, how many pixels to pad around the
        /// source?
        unsigned int itsSpatialPad;
        unsigned int itsSpectralPad;

};

}

}

#endif

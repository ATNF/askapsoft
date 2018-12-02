// @file SkyModelService.ice
//
// @copyright (c) 2011 CSIRO
// Australia Telescope National Facility (ATNF)
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// PO Box 76, Epping NSW 1710, Australia
// atnf-enquiries@csiro.au
//
// This file is part of the ASKAP software distribution.
//
// The ASKAP software distribution is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#pragma once

#include <CommonTypes.ice>
#include <IService.ice>
#include <SkyModelServiceDTO.ice>
#include <SkyModelServiceCriteria.ice>

module askap
{
module interfaces
{
module skymodelservice
{
    /**
     * Extents structure. Used to specify the extents of a rectangular region of
     * interest in spatial searches.
     **/
    struct RectExtents
    {
        /**
         * Width of the bounding box
         * Units: decimal degrees
         **/
        double width;

        /**
         * Height of the bounding box
         * Units: decimal degrees
         **/
        double height;
    };

    /**
     * A Right-ascension, declination coordinate pair.
     **/
    struct Coordinate
    {
        /**
         * Right ascension in the J2000 coordinate system
         * Units: degrees
         **/
        double rightAscension;

        /**
         * Declination in the J2000 coordinate system
         * Units: degrees
         **/
        double declination;
    };

    /**
     * A rectangular region of interest in the sky.
     **/
    struct Rect
    {
        /**
         * The rectangle centre.
         **/
        Coordinate centre;

        /**
         * The rectangle extents.
         **/
        RectExtents extents;
    };

    /**
     * A sequence of component identifiers
     **/
    ["java:type:java.util.ArrayList<Long>"]
    sequence<long> ComponentIdSeq;

    /**
     * A sequence of Components
     **/
    ["java:type:java.util.ArrayList<askap.interfaces.skymodelservice.ContinuumComponent>"]
    sequence<ContinuumComponent> ComponentSeq;

    /**
     * This exception is thrown when a component id is specified but the component
     * does not exist, but is expected to.
     **/
    exception InvalidComponentIdException extends askap::interfaces::AskapIceException
    {
    };

    /**
     * Interface to the Sky Model Service.
     **/
    interface ISkyModelService extends askap::interfaces::services::IService
    {
        /**
         * Cone search
         * TODO: documentation
         * TODO: should I follow Ben's lead and return just the ID list?
         **/
        ComponentSeq coneSearch(Coordinate centre, double radius, SearchCriteria criteria);

        /**
         * Rectangular ROI search
         * TODO: documentation
         * TODO: should I follow Ben's lead and return just the ID list?
         **/
        ComponentSeq rectSearch(Rect roi, SearchCriteria criteria);
    };

};
};
};

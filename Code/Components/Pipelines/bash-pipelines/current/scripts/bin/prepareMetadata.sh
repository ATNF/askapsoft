#!/bin/bash -l
#
# A script to extract all necessary metadata from the input
# measurement set, including date & time of observation, and beam
# locations.
#
# @copyright (c) 2016 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# @author Matthew Whiting <Matthew.Whiting@csiro.au>
#

if [ "${MS_INPUT_SCIENCE}" != "" ]; then

    echo "Extracting metadata for science measurement set $MS_INPUT_SCIENCE"
    
    MS_METADATA=$parsets/mslist-science-${NOW}.txt
    mslist --full $MS_INPUT_SCIENCE 2>&1 1> ${MS_METADATA}

    # Get the observation time
    obsdate=`grep "Observed from" ${MS_METADATA} | head -1 | awk '{print $7}' | sed -e 's|/| |g' | awk '{print $1}' | sed -e 's/-/ /g'`
    obstime=`grep "Observed from" ${MS_METADATA} | head -1 | awk '{print $7}' | sed -e 's|/| |g' | awk '{print $2}'`
    DATE_OBS=`date -d "$obsdate" +"%Y-%m-%d"`
    DATE_OBS="${DATE_OBS}T${obstime}"
    
    DURATION=`grep "elapsed time" ${MS_METADATA} | head -1 | awk '{print $11}'`

    
    . ${PIPELINEDIR}/findBeamCentres.sh

fi


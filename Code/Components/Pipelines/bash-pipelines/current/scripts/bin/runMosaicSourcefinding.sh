#!/bin/bash -l
#
# Run the source-finding jobs for the mosaic images/cubes, with
# context-dependent settings that change for a FIELD-based mosaic or
# the full mosaic
#
# @copyright (c) 2017 CSIRO
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

if [ "${DO_SOURCE_FINDING_CONT}" == "true" ] || [ "${DO_SOURCE_FINDING_SPEC}" == "true" ]; then

    BEAM="all"

    if [ "${FIELD}" == "." ]; then
        # This is the full mosaic case
        
        # set the $imageBase variable to have 'linmos' in it
        imageCode=restored
        TILE="ALL"
        FIELDBEAM="Full"
        
        . "${PIPELINEDIR}/sourcefindingCont.sh"
        
        . "${PIPELINEDIR}/sourcefindingSpectral.sh"

    else
        # This is the case of source-finding on a mosaic for a single
        # FIELD - FIELDBEAM is already set to the FIELD id

        if [ "${DO_SOURCE_FINDING_FIELD_MOSAICS}" == "true" ]; then
        
            # First the continuum source-finding
            NUM_LOOPS=0
            if [ "${DO_SELFCAL}" == "true" ] && [ "${MOSAIC_SELFCAL_LOOPS}" == "true" ]; then
                NUM_LOOPS=$SELFCAL_NUM_LOOPS
            fi
            for((LOOP=0;LOOP<=NUM_LOOPS;LOOP++)); do
                . "${PIPELINEDIR}/sourcefindingCont.sh"
            done
            unset LOOP
            
            # Then the spectral-line sourcefinding
            . "${PIPELINEDIR}/sourcefindingSpectral.sh"

        fi

    fi

fi


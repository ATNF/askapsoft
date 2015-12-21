#!/usr/bin/env bash
#
# Defines the beam arrangements for predetermined named sets. This makes use
# of the ACES footprint.py tool to extract the beam offsets for a named beam
# footprint. The following variables must be defined: $BEAM_FOOTPRINT_NAME,
# $FREQ_BAND_NUMBER, $BEAM_FOOTPRINT_PA.
# Upon return, either $LINMOS_BEAM_OFFSETS will be set, or $DO_MOSAIC will have been
# set to false.
#
# @copyright (c) 2015 CSIRO
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

if [ $DO_SCIENCE_FIELD == true ] && [ $DO_MOSAIC == true ]; then

    if [ "$LINMOS_BEAM_OFFSETS" == "" ]; then
	# Beam pattern for linmos not defined. We need to use footprint.py to work it out.

	if [ "`which footprint.py`" == "" ]; then
	    # If we are here, footprint.py is not in our path. Give an
	    # error message and turn off linmos
	    
	    echo "Cannot find 'footprint.py', so cannot determine beam arrangement for linmos. Setting DO_MOSAIC=false."
	    DO_MOSAIC=false

	else

	    if [ "$BEAM_FOOTPRINT_NAME" == "diamond" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "rhombus" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "line" ] \
		   || [ "$BEAM_FOOTPRINT_NAME" == "trapezoid2" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "trapezoid3" ]\
		   || [ "$BEAM_FOOTPRINT_NAME" == "octagon" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "3x3" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "square" ]||\
                   [ "$BEAM_FOOTPRINT_NAME" == "square_3x3_4x4" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "square_5x5" ]||\
                   [ "$BEAM_FOOTPRINT_NAME" == "square_4x4" ]||\
                   [ "$BEAM_FOOTPRINT_NAME" == "hexagon19" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "closepack12" ] ||\
                   [ "$BEAM_FOOTPRINT_NAME" == "closepack30" ]; then

                # If we have an allowed footprint name, then run
                # footprint.py and extract the beam offsets

		footprintOut="$parsets/footprintOutput-${NOW}.txt"

                # Set the args for footprint: summary output, name, band, PA, pitch
                footprintArgs="-t"
                footprintArgs="$footprintArgs -n $BEAM_FOOTPRINT_NAME"
                footprintArgs="$footprintArgs -b $FREQ_BAND_NUMBER"
                footprintArgs="$footprintArgs -a $BEAM_FOOTPRINT_PA"
                footprintArgs="$footprintArgs -p $BEAM_PITCH"
                
		footprint.py $footprintArgs > ${footprintOut}

		if [ `wc -l $footprintOut | awk '{print $1}'` != 0 ]; then
		    
		# The offsets are taken from the appropriate column,
		# with the RA offset flipped in sign.

		    beamsOut="$parsets/beamOffsets.txt"
		    grep -A9 Beam ${footprintOut} | tail -n 9 | sed -e 's/(//g' | sed -e 's/)//g' | awk '{printf "linmos.feeds.beam%d = [%6.3f, %6.3f]\n",$1,-$4,$5}' > ${beamsOut}
		    LINMOS_BEAM_OFFSETS=`cat ${beamsOut}`

		else
		  
		    # Something went wrong.
		    echo "ERROR - The command 'footprint.py $footprintArgs' failed. Setting DO_MOSAIC=false."
                    echo " "
                    DO_MOSAIC=false
		
		fi

	    else

		# The footprint name is not one of the allowed values.
		echo "WARNING - Beam arrangement for ${BEAM_FOOTPRINT_NAME} footprint not defined. Setting DO_MOSAIC=false."
		DO_MOSAIC=false

	    fi

	
	    if [ "$LINMOS_BEAM_OFFSETS" == "" ]; then
		# If we get to here, we have not set the offsets for the beams, so turn off the linmos mode
		DO_MOSAIC=false
	    fi
	
	fi	
	
    fi

fi


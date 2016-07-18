#!/bin/bash -l
#
# A script to find the individual fields within the science dataset,
# and for each field obtain the beam centres given the specified beam
# footprint. 
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

needBeams=false
if [ "$DO_MOSAIC" == "true" ] || [ "$IMAGE_AT_BEAM_CENTRES" == "true" ]; then
    needBeams=true
fi

if [ "$DO_SCIENCE_FIELD" == "true" ] && [ "$needBeams" == "true" ]; then

    if [ "`which footprint.py`" == "" ]; then
	# If we are here, footprint.py is not in our path. Give an
	# error message and turn off linmos

        if [ "${DO_MOSAIC}" == "true" ]; then
	    echo "ERROR - Cannot find 'footprint.py', so cannot determine beam arrangement. 
      Setting DO_MOSAIC=false."
	    DO_MOSAIC=false
        fi

        if [ "$IMAGE_AT_BEAM_CENTRES" == "true" ]; then
            echo "ERROR - Cannot find 'footprint.py', so cannot set beam centres. 
      Not running - you may want to find footprint.py or change your config file."
            SUBMIT_JOBS=false
        fi
        
    else

        # Check to see that the footprint provided is valid.
        invalid=false
        if [ "${BEAM_FOOTPRINT_NAME}" == "" ]; then
            invalid=true
        else
            tempfile=$parsets/fpTest
            rm -f $tempfile
            footprint.py -n ${BEAM_FOOTPRINT_NAME} 2>&1 > $tempfile
            if [ "`grep invalid $tempfile`" != "" ]; then
                invalid=true
            fi
        fi
        
        if [ "$invalid" == "true" ]; then
            echo "ERROR - Your requested footprint ${BEAM_FOOTPRINT_NAME} is not valid."
            if [ ${IMAGE_AT_BEAM_CENTRES} == true ]; then
                echo "      Not running - change your config file."
                SUBMIT_JOBS=false
            else
                if [ "${DO_MOSAIC}" == "true" ]; then
                    echo "      Setting DO_MOSAIC to false."
                fi
            fi
            
        else

            # Find the number of fields in the MS
            nfields=`grep Fields ${MS_METADATA} | head -1 | cut -f 4- | cut -d' ' -f 2`
            fieldlist=$parsets/fieldlist
            rm -f $fieldlist
            grep -A${nfields} RA ${MS_METADATA} | tail -n ${nfields} | cut -f 4- >> $fieldlist

            FIELD_LIST=`sort -k2 $fieldlist | uniq | awk '{print $2}'`

            # Set the args for footprint: summary output, name, band, PA, pitch
            footprintArgs="-t"
            if [ "$BEAM_FOOTPRINT_NAME" != "" ]; then
                footprintArgs="$footprintArgs -n $BEAM_FOOTPRINT_NAME"
            fi
            if [ "$FREQ_BAND_NUMBER" != "" ]; then
                footprintArgs="$footprintArgs -b $FREQ_BAND_NUMBER"
            fi
            if [ "$BEAM_FOOTPRINT_PA" != "" ]; then
                footprintArgs="$footprintArgs -a $BEAM_FOOTPRINT_PA"
            fi
            if [ "$BEAM_PITCH" != "" ]; then
                footprintArgs="$footprintArgs -p $BEAM_PITCH"
            fi

            echo "List of fields: $FIELD_LIST"
            
            for FIELD in ${FIELD_LIST}; do
                ra=`grep $FIELD $fieldlist | awk '{print $3}'`
                dec=`grep $FIELD $fieldlist | awk '{print $4}'`
                dec=`echo $dec | awk -F'.' '{printf "%s:%s:%s.%s",$1,$2,$3,$4}'`
                # define the output file as $footprintOut
                setFootprintFile
                echo "Writing footprint for field $FIELD to file $footprintOut"
                footprint.py $footprintArgs -r "$ra,$dec" 2>&1 > ${footprintOut}

                # error handling, in case something goes wrong.
                if [ `wc -l $footprintOut | awk '{print $1}'` -eq 0 ]; then
                    # Something has failed with footprint.py
                    echo "ERROR - The footprint.py command has failed."
                    if [ "${IMAGE_AT_BEAM_CENTRES}" == "true" ]; then
                        echo "      Not running - change your config file or locate footprint.py."
                        SUBMIT_JOBS=false
                    else
                        if [ "${DO_MOSAIC}" == "true" ]; then
                            echo "      Setting DO_MOSAIC to false."
                        fi
                    fi

                fi
                
            done

        fi

    fi

fi

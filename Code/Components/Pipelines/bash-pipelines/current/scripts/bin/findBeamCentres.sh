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

    if [ "`which footprint.py 2> ${tmp}/whchftprnt`" == "" ]; then
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

        # Run schedblock to get footprint information (if present)
        sbinfo="${metadata}/schedblock-info-${SB_SCIENCE}.txt"
        module load askapcli
        if [ "`which schedblock 2> ${tmp}/whchschdblk`" == "" ]; then
            echo "WARNING - no schedblock executable found - try loading askapcli module."
        else
            if [ -e ${sbinfo} ] && [ `wc -l $sbinfo | awk '{print $1}'` -gt 1 ]; then
                echo "Reusing schedblock info file $sbinfo for SBID ${SB_SCIENCE}"
            else
                if [ -e ${sbinfo} ]; then
                    rm -f $sbinfo
                fi
                schedblock info -p ${SB_SCIENCE} > $sbinfo
            fi
            defaultFPname=`grep "%d.footprint.name" ${sbinfo} | awk '{print $3}'`
            defaultFPpitch=`grep "%d.footprint.pitch" ${sbinfo} | awk '{print $3}'`
            defaultFPangle=`grep "%d.pol_axis" ${sbinfo} | awk '{print $4}' | sed -e 's/\]//g'`
        fi
        if [ "$defaultFPname" == "" ]; then
            # Not in the schedblock information (predates footprint service?)
            # We get the default values from the config file
            defaultFPname=${BEAM_FOOTPRINT_NAME}
            defaultFPpitch=${BEAM_PITCH}
            defaultFPangle=${BEAM_FOOTPRINT_PA}
        fi
        
        # Check to see that the footprint provided is valid.
        invalid=false
        if [ "${BEAM_FOOTPRINT_NAME}" == "" ] ||
               [ "`footprint.py -n ${BEAM_FOOTPRINT_NAME} 2>&1 | grep invalid`" != "" ]; then
            invalid=true
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
            getMSname ${MS_INPUT_SCIENCE}
            fieldlist=${metadata}/fieldlist-${msname}.txt
            if [ ! -e $fieldlist ]; then
                grep -A${nfields} RA ${MS_METADATA} | tail -n ${nfields} | cut -f 4- >> $fieldlist
            fi

            FIELD_LIST=""
            TILE_LIST=""
            for FIELD in `sort -k2 $fieldlist | awk '{print $2}' | uniq `;
            do
                FIELD_LIST="$FIELD_LIST $FIELD"
                getTile
                if [ $FIELD != $TILE ]; then
                    isNew=true
                    for THETILE in $TILE_LIST; do
                        if [ $TILE == $THETILE ]; then
                            isNew=false
                        fi
                    done
                    if [ $isNew == true ]; then
                        TILE_LIST="$TILE_LIST $TILE"
                    fi
                fi
            done
            
            echo "List of fields: "
            COUNT=0
            for FIELD in ${FIELD_LIST}; do
                ID=`echo $COUNT | awk '{printf "%02d",$1}'`
                echo "${ID} - ${FIELD}"
                COUNT=`expr $COUNT + 1`
            done
            
            for FIELD in ${FIELD_LIST}; do
                echo "Finding footprint for field $FIELD"
                
                ra=`grep $FIELD $fieldlist | awk '{print $3}' | head -1`
                dec=`grep $FIELD $fieldlist | awk '{print $4}' | head -1`
                dec=`echo $dec | awk -F'.' '{printf "%s:%s:%s.%s",$1,$2,$3,$4}'`

                awkcomp="\$3==\"$FIELD\""
                srcstr=`awk $awkcomp $sbinfo | awk -F".field_name" '{print $1}'`
                FP_NAME=`grep "$srcstr.footprint.name" $sbinfo | awk '{print $3}'`
                FP_PITCH=`grep "$srcstr.footprint.pitch" $sbinfo | awk '{print $3}'`
                FP_PA=`grep "$srcstr.pol_axis" $sbinfo | awk '{print $4}' | sed -e 's/\]//g'`
                if [ "$FP_NAME" == "" ]; then
                    FP_NAME=$defaultFPname
                fi
                if [ "$FP_PITCH" == "" ]; then
                    FP_PITCH=$defaultFPpitch
                fi
                if [ "$FP_PA" == "" ]; then
                    FP_PA=$defaultFPangle
                fi
                
                # Use the function defined in utils.sh to set the arguments to footprint.py
                setFootprintArgs

                echo "Footprint arguments are:  $footprintArgs"
                
                # define the output file as $footprintOut
                setFootprintFile
                # If the footprint output file exists, we don't re-run footprint.py.
                # The only exception to that is if it exists but is empty - a previous footprint.py
                # run might have failed, so we try again
                if [ -e ${footprintOut} ] && [ `wc -l $footprintOut | awk '{print $1}'` -gt 0 ]; then
                    echo "Reusing footprint file $footprintOut for field $FIELD"
                else
                    if [ -e ${footprintOut} ]; then
                        rm -f $footprintOut
                    fi
                    echo "Writing footprint for field $FIELD to file $footprintOut"
                    module load aces
                    footprint.py $footprintArgs -r "$ra,$dec" 2>&1 > ${footprintOut}
                fi

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

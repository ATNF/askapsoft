#!/bin/bash -l
#
# A script to find, for each field, the beam centres given the
# specified beam footprint. 
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

tmpfp="${tmp}/listOfFootprints"
if [ "${USE_CLI}" == "true" ] && [ "${IS_BETA}" != "true" ]; then
    loadModule askapcli
    footprint list > "$tmpfp"
    err=$?
    unloadModule askapcli
    if [ $err -ne 0 ]; then
        echo "ERROR - the 'footprint' command failed. "
        echo "        Full command:   footprint list"
        echo "Exiting pipeline."
        exit $err
    fi
fi

NEED_BEAM_CENTRES=false
if [ "$DO_MOSAIC" == "true" ] || [ "$IMAGE_AT_BEAM_CENTRES" == "true" ]; then
    NEED_BEAM_CENTRES=true
fi

if [ "$DO_SCIENCE_FIELD" == "true" ] && [ "$NEED_BEAM_CENTRES" == "true" ]; then

    defaultFPname=""

    # For the non-BETA case, we use schedblock from the askapcli
    # module - this polls the online scheduling-block database
    if [ "${IS_BETA}" != "true" ]; then

        sbinfo="${metadata}/schedblock-info-${SB_SCIENCE}.txt"
        if [ "${USE_CLI}" != "true" ]; then

            if [ -e "${sbinfo}" ]; then
                echo "Reusing schedblock info results $sbinfo"
            else

                echo "The schedblock service is not available, and you don't have a pre-computed SB info file."
                echo "    (did not find $sbinfo)"
                echo "Halting the pipeline here as we cannot find the beam locations."
                echo "You may want to use the IS_BETA option, giving the beam footprint information directly."
                exit 1
            fi

        else
            
            # Run schedblock to get footprint information (if present)
            getSchedblock=true
            if [ -e "${sbinfo}" ]; then
                if [ "$(grep -c "${METADATA_IS_GOOD}" "${sbinfo}")" -gt 0 ]; then
                    # SB info file exists and is complete. Don't need to regenerate.
                    getSchedblock=false
                else
                    rm -f "$sbinfo"
                fi
            fi
            if [ "${getSchedblock}" == "true" ]; then
                loadModule askapcli
                schedblock info -v -p "${SB_SCIENCE}" > "$sbinfo"
                err=$?
                unloadModule askapcli
                if [ $err -ne 0 ]; then
                    echo "ERROR - the 'schedblock' command failed."
                    echo "        Full command:   schedblock info -v -p ${SB_SCIENCE}"
                    echo "Exiting pipeline."
                    exit $err
                fi
            fi
            
        fi
        defaultFPname=$(grep "%d.footprint.name" "${sbinfo}" | awk '{print $3}')
        defaultFPpitch=$(grep "%d.footprint.pitch" "${sbinfo}" | awk '{print $3}')
        defaultFPangle=$(grep "%d.pol_axis" "${sbinfo}" | awk '{print $4}' | sed -e 's/\]//g')

        # The reference rotation angle - this is what the footprint
        # was made with. Any pol_axis value is added to this reference
        # value. 
        FProtation=$(grep "%d.footprint.rotation" "${sbinfo}" | awk '{print $3}')
        if [ "$FProtation" == "" ]; then
            # footprint.rotation is not in the schedblock parset, so
            # need to set a value manually 
            if [ "$FOOTPRINT_PA_REFERENCE" != "" ]; then
                # If here, the user has provided a value for this reference
                FProtation=$FOOTPRINT_PA_REFERENCE
                echo "*Note* Using the user parameter FOOTPRINT_PA_REFERENCE=$FProtation as the reference position for footprint PA"
            else
                # if here, they haven't, so set offset to zero
                FProtation=0.
            fi
        else
            if [ "$FOOTPRINT_PA_REFERENCE" != "" ]; then
                # If here, the user has provided a value for this
                # reference. We will use their value, over-riding the
                # parset value, but we give them a nice warning.
                echo "###################"
                echo "# WARNING - you are over-riding footprint.rotation"
                echo "#  This has a value in the SB parset of ${FProtation}"
                echo "#  It will be replaced by FOOTPRINT_PA_REFERENCE=${FOOTPRINT_PA_REFERENCE}"
                echo "###################"
                FProtation=${FOOTPRINT_PA_REFERENCE}
            fi
        fi
    fi
    
    if [ "$defaultFPname" == "" ]; then
        # Not in the schedblock information (predates footprint service or BETA data)
        # We get the default values from the config file
        defaultFPname=${BEAM_FOOTPRINT_NAME}
        defaultFPpitch=${BEAM_PITCH}
        defaultFPangle=${BEAM_FOOTPRINT_PA}
        if [ "$FOOTPRINT_PA_REFERENCE" != "" ]; then
            # If here, the user has provided a value for this reference
            FProtation=$FOOTPRINT_PA_REFERENCE
            echo "*Note* Using the user parameter FOOTPRINT_PA_REFERENCE=$FProtation as the reference position for footprint PA"
        else
            # if here, they haven't, so set offset to zero
            FProtation=0.
        fi
    fi
    

    # Define the footprint for each field
    IFS="${IFS_FIELDS}"
    for FIELD in ${FIELD_LIST}; do

        # Get the centre location of each field (from the list of
        # fields in the metadata directory - the filename is recorded
        # in $FIELDLISTFILE, and set in prepareMetadata.sh)
        ra=$(grep "$FIELD" "$FIELDLISTFILE" | awk '{print $(NF-3)}' | head -1)
        dec=$(grep "$FIELD" "$FIELDLISTFILE" | awk '{print $(NF-2)}' | head -1)
        dec=$(echo "$dec" | awk -F'.' '{printf "%s:%s:%s.%s",$1,$2,$3,$4}')

        # Initialise to blank values
        FP_NAME=""
        FP_PITCH=""
        FP_PA=""
        if [ "${IS_BETA}" != "true" ]; then
            # For non-BETA data, we look for this field's footprint
            # information in the SB parset

            # split on the equals sign, and get the entire value of the parameter - allows for spaces.
            # Use this to match the field name, and then get the string that looks like
            #  "common.target.src1"
            awkcomp="\$2==\"$FIELD\""
            srcstr=$(awk -F " = " "$awkcomp" "$sbinfo" | awk -F".field_name" '{print $1}')

            # Use that to get the footprint info for that particular field
            FP_NAME=$(grep "$srcstr.footprint.name" "$sbinfo" | awk '{print $3}')
            FP_PITCH=$(grep "$srcstr.footprint.pitch" "$sbinfo" | awk '{print $3}')
            FP_PA=$(grep "$srcstr.pol_axis" "$sbinfo" | awk '{print $4}' | sed -e 's/\]//g')
        fi

        # If we didn't find them, fall back to the previously-defined
        # defaults
        
        if [ "$FP_NAME" == "" ]; then
            FP_NAME=$defaultFPname
        fi
        if [ "$FP_PITCH" == "" ]; then
            FP_PITCH=$defaultFPpitch
        fi
        if [ "$FP_PA" == "" ]; then
            FP_PA=$defaultFPangle
        fi

        # Add the footprint reference angle to the PA, so that FP_PA
        # represents the net rotation of the footprint, that can be
        # given to the footprint/footprint.py tools.
        FP_PA=$(echo "$FP_PA" "$FProtation" | awk '{print $1+$2}')

        #####
        # Define the footprint locations for this field
        #   This is done with either the footprint tool in the
        #   askapcli module, for the case of ASKAP data that makes use
        #   of the online database, or with the footprint.py tool in
        #   ACES for BETA data or old ASKAP data that doesn't have
        #   footprint information in the database.
        
        # define the output file as $footprintOut
        setFootprintFile
        
        # Check to see whether the footprint name is used by ASKAPCLI/footprint
        beamFromCLI=true
        
        if [ "${IS_BETA}" == "true" ]; then
            beamFromCLI=false
            echo "Using the ACES footprint.py tool for BETA data"
        fi

        if [ "${USE_CLI}" == "true" ] && [ "$(grep "$FP_NAME" "${tmpfp}")" == "" ]; then
            beamFromCLI=false
            echo "Footprint name $FP_NAME is not recognised by askapcli/footprint. Using the ACES footprint.py tool"
        fi

        # Determine the number of beams in the footprint - only works for the CLI option
        if [ "$beamFromCLI" == "true" ] && [ "${USE_CLI}" == "true" ]; then
            loadModule askapcli
            NUM_BEAMS_FOOTPRINT=$(footprint info ${FP_NAME} | grep n_beams | awk '{print $3}')
            unloadModule askapcli
        fi
        
        # If the footprint output file exists, we don't re-run the
        # footprint determination.
        # The only exception to that is if it exists but is empty - a previous footprint
        # run might have failed, so we try again
        if [ -e "${footprintOut}" ] && [ "$(wc -l "$footprintOut" | awk '{print $1}')" -gt 0 ]; then
            echo "Reusing footprint file $footprintOut for field $FIELD"
        else
            if [ -e "${footprintOut}" ]; then
                rm -f "$footprintOut"
            fi
            echo "Writing footprint for field $FIELD to file $footprintOut"

            if [ "$beamFromCLI" == "true" ]; then

                if [ "${USE_CLI}" != "true" ]; then
                    # If the service is not available, and we don't have beam locations, stop everything.
                    echo "The footprint service is not available, and you don't have a pre-computed footprint file."
                    echo "Halting the pipeline here as we cannot find the beam locations."
                    echo "You may want to use the IS_BETA option, giving the beam footprint information directly."
                    exit 1
                fi

                # This uses the CLI tool "footprint" to set the footprint
                loadModule askapcli
                NUM_BEAMS_FOOTPRINT=$(footprint info ${FP_NAME} | grep n_beams | awk '{print $3}')
                if [ "$FP_PA" != "" ]; then
                    footprint calculate -d $ra,$dec -r $FP_PA -p $FP_PITCH "$FP_NAME" > "${footprintOut}"
                else
                    footprint calculate -d $ra,$dec -p $FP_PITCH "$FP_NAME" > "${footprintOut}"
                fi
                err=$?
                unloadModule askapcli
                if [ $err -ne 0 ]; then
                    echo "ERROR - the 'footprint' command failed."
                    if [ "$FP_PA" != "" ]; then
                        echo "        Full command:   footprint calculate -d $ra,$dec -r $FP_PA -p $FP_PITCH $FP_NAME"
                    else
                        echo "        Full command:   footprint calculate -d $ra,$dec -p $FP_PITCH $FP_NAME"
                    fi
                    echo "Exiting pipeline."
                    exit $err
                fi
            else
                # This case uses the ACES tool "footprint.py"
                if [ "$(which footprint.py 2> "${tmp}/whchftprnt")" == "" ]; then
	            # If we are here, footprint.py is not in our path. Give an
	            # error message and turn off linmos
                    
                    if [ "${DO_MOSAIC}" == "true" ]; then
	                echo "ERROR - Cannot find 'footprint.py', so cannot determine beam arrangement. 
      Setting DO_MOSAIC=false."  
	                DO_MOSAIC=false
                    fi
                    
                    if [ "$IMAGE_AT_BEAM_CENTRES" == "true" ]; then
                        echo "ERROR - Cannot find 'footprint.py', so cannot set beam centres. 
      Not running - you may want to find footprint.py or               change your config file."
                        SUBMIT_JOBS=false
                    fi

                else
                
                    # Next, need to check that the footprint provided is
                    # valid (ie. recognised by footprint.py)
                    loadACES
                    invalidTest=$(footprint.py -n "${BEAM_FOOTPRINT_NAME}" 2>&1 | grep invalid)
                    unloadACES
                    if [ "${FP_NAME}" == "" ] || [ "${invalidTest}" != "" ]; then
                        # We don't have a valid footprint name!
                        echo "ERROR - Your requested footprint ${BEAM_FOOTPRINT_NAME} is not valid."
                        if [ "${IMAGE_AT_BEAM_CENTRES}" == "true" ]; then
                            echo "      Not running - change your config file."
                            SUBMIT_JOBS=false
                        else
                            if [ "${DO_MOSAIC}" == "true" ]; then
                                echo "      Setting DO_MOSAIC to false."
                            fi
                        fi
                        
                    else
                        # Use the function defined in utils.sh to set the arguments to footprint.py
                        setFootprintArgs
                        loadACES
                        footprint.py $footprintArgs -r "$ra,$dec" > "${footprintOut}"
                        err=$?
                        unloadACES
                        if [ $err -ne 0 ]; then
                            echo "ERROR - the 'footprint.py' command failed. "
                            echo "        Full command:   footprint.py $footprintArgs -r \"$ra,$dec\""
                            echo "Exiting pipeline."
                            exit $err
                        fi
                    fi
                fi
            fi
        fi

        # error handling, in case something goes wrong.
        if [ "$(wc -l "$footprintOut" | awk '{print $1}')" -eq 0 ]; then
            # Something has failed with footprint
            echo "ERROR - The footprint command has failed."
            if [ "${IMAGE_AT_BEAM_CENTRES}" == "true" ]; then
                echo "      Not running the pipeline."
                SUBMIT_JOBS=false
            else
                if [ "${DO_MOSAIC}" == "true" ]; then
                    echo "      Setting DO_MOSAIC to false."
                fi
            fi

        fi
        
    done
    IFS="${IFS_DEFAULT}"

fi


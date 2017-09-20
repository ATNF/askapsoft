#!/bin/bash -l
#
# A script to extract all necessary metadata from the input
# measurement set, including date & time of observation, and beam
# locations.
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

####################
# Input Measurement Sets
#  We define these based on the SB number

# 1934-638 calibration
if [ "${DO_1934_CAL}" == "true" ]; then
    # This is set to true if the SB MS is absent
    MS_CAL_MISSING=false
    if [ "${SB_1934}" != "" ]; then
        sb1934dir="$DIR_SB/$SB_1934"
        msnames=$(find "$sb1934dir" -maxdepth 1 -type d -name "*.ms")
        numMS=$(find "$sb1934dir" -maxdepth 1 -type d -name "*.ms" | wc -l)
        if [ "${numMS}" -eq 1 ]; then
            MS_INPUT_1934="$sb1934dir/${msnames##*/}"
        elif [ "${numMS}" -eq 0 ]; then
            MS_CAL_MISSING=true
            if [ "${MS_INPUT_1934}" == "" ]; then
                echo "SB directory $SB_1934 has no measurement sets. You need to give the measurement set name via 'MS_INPUT_1934'"
                echo "Exiting pipeline"
                exit 1
            else
                echo "No MS found in SB directory ${sb1934dir}. Using MS name given by 'MS_INPUT_1934=${MS_INPUT_1934}'"
                if [ "${DO_SPLIT_1934}" == "true" ]; then
                    echo "Turning off splitting for calibration dataset."
                    DO_SPLIT_1934=false
                fi
            fi
        else
            echo "SB directory $SB_1934 has more than one measurement set, and the pipeline can currently only process one at a time."
            echo "Please specify input MS with parameter 'MS_INPUT_1934'."
            exit 1
        fi
    fi
    if [ "$MS_INPUT_1934" == "" ]; then
        echo "Parameter 'MS_INPUT_1934' not defined. Turning off 1934-638 processing with DO_1934_CAL=false."
        DO_1934_CAL=false
    else
        # Set up the metadata filename - this is where the mslist information will go
        getMSname "${MS_INPUT_1934}"
        MS_METADATA_CAL=$metadata/mslist-cal-${msname}.txt
    fi

fi

# science observation - check that MS_INPUT_SCIENCE is OK:
if [ "${DO_SCIENCE_FIELD}" == "true" ]; then
    # This is set to true if the SB MS is absent
    MS_SCIENCE_MISSING=false
    if [ "${SB_SCIENCE}" != "" ]; then
        sbScienceDir=$DIR_SB/$SB_SCIENCE
        msnames=$(find "$sbScienceDir" -maxdepth 1 -type d -name "*.ms")
        numMS=$(find "$sbScienceDir" -maxdepth 1 -type d -name "*.ms" | wc -l)
        if [ "${numMS}" -eq 1 ]; then
            MS_INPUT_SCIENCE="$sbScienceDir/${msnames##*/}"
        elif [ "${numMS}" -eq 0 ]; then
            MS_SCIENCE_MISSING=true
            if [ "${MS_INPUT_SCIENCE}" == "" ]; then
                echo "SB directory $SB_SCIENCE has no measurement sets. You need to give the measurement set name via 'MS_INPUT_SCIENCE'"
                echo "Exiting pipeline"
                exit 1
            else
                echo "No MS found in SB directory ${sbScienceDir}. Using MS name given by 'MS_INPUT_SCIENCE=${MS_INPUT_SCIENCE}'"
                if [ "${DO_SPLIT_SCIENCE}" == "true" ]; then
                    echo "Turning off splitting for science dataset."
                    DO_SPLIT_SCIENCE=false
                fi
            fi
        else
            echo "SB directory $SB_SCIENCE has more than one measurement set, and the pipeline can currently only process one at a time."
            echo "Please specify with parameter 'MS_INPUT_SCIENCE'."
            exit 1
        fi
    fi

    if [ "${MS_INPUT_SCIENCE}" == "" ]; then
        echo "Parameter 'MS_INPUT_SCIENCE' not defined. Turning off science processing with DO_SCIENCE_FIELD=false."
        DO_SCIENCE_FIELD=false
    else
        # Set up the metadata filename - this is where the mslist information will go
        # define $msname
        getMSname "${MS_INPUT_SCIENCE}"
        # Extract the MS metadata into a local file ($MS_METADATA) for parsing
        MS_METADATA="$metadata/mslist-${msname}.txt"
    fi

fi

####################
# Catching old parameters

if [ "${NUM_CHAN}" != "" ]; then
    echo "You've entered NUM_CHAN=${NUM_CHAN}. This is no longer used!"
    echo "  Please use CHAN_RANGE_1934 & CHAN_RANGE_SCIENCE to specify number and range of channels."
fi

if [ "${NUM_ANT}" != "" ]; then
    echo "You've entered NUM_ANT=${NUM_ANT}. This is no longer used, as we take this info from the MS."
fi

#####
# Text to write to metadata files to indicate success
METADATA_IS_GOOD=METADATA_IS_GOOD

####################
# Once we've defined the bandpass calibrator MS name, extract metadata from it
if [ "${DO_1934_CAL}" == "true" ] && [ "${MS_INPUT_1934}" != "" ]; then

    runMSlist=true
    if [ -e "${MS_METADATA_CAL}" ]; then
        if [ "$(grep -c "${METADATA_IS_GOOD}" "${MS_METADATA_CAL}")" -gt 0 ]; then
            runMSlist=false
        else
            rm -f "${MS_METADATA_CAL}"
        fi
    fi

    if [ "${runMSlist}" == "true" ]; then
        echo "Extracting metadata for calibrator measurement set $MS_INPUT_1934"
        ${mslist} --full "${MS_INPUT_1934}" 1>& "${MS_METADATA_CAL}"
        err=$?
        if [ $err -ne 0 ]; then
            echo "ERROR - the 'mslist' command failed."
            echo "        Full command:  ${mslist} --full $MS_INPUT_1934"
            echo "Exiting pipeline."
            exit $err
        else
            cat >> "${MS_METADATA_CAL}" <<EOF
${METADATA_IS_GOOD} ${NOW}
EOF
        fi
    else
        echo "Reusing calibrator metadata file ${MS_METADATA_CAL}"
    fi

    # Number of antennas used in the calibration observation
    NUM_ANT_1934=$(grep Antennas "${MS_METADATA_CAL}" | head -1 | awk '{print $6}')
    NUM_ANT=$NUM_ANT_1934

    if [ "${NUM_ANT_1934}" == "" ]; then
        echo "ERROR - unable to determine number of antennas in calibration dataset"
        echo "        please check metadata in ${MS_METADATA_CAL}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Number of channels used in the calibration observation
    NUM_CHAN=$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="${MS_METADATA_CAL}" --val=nChan)

    if [ "${NUM_CHAN}" == "" ]; then
        echo "ERROR - unable to determine number of channels in calibration dataset"
        echo "        please check metadata in ${MS_METADATA_CAL}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Number of channels for 1934 observation
    if [ "${CHAN_RANGE_1934}" == "" ]; then
        NUM_CHAN_1934=${NUM_CHAN}
        CHAN_RANGE_1934="1-${NUM_CHAN}"
    else
        NUM_CHAN_1934=$(echo "$CHAN_RANGE_1934" | awk -F'-' '{print $2-$1+1}')
    fi

    if [ "${NUM_CHAN}" -lt "${NUM_CHAN_1934}" ]; then
        echo "ERROR! Number of channels requested for the calibration observation (${NUM_CHAN_1934}, from \"${CHAN_RANGE_1934}\") is bigger than the number in the MS (${NUM_CHAN})."
        exit 1
    fi


fi

####################
# Once we've defined the science MS name, extract metadata from it
if [ "${MS_INPUT_SCIENCE}" != "" ]; then

    runMSlist=true
    if [ -e "${MS_METADATA}" ]; then
        if [ "$(grep -c "${METADATA_IS_GOOD}" "${MS_METADATA}")" -gt 0 ]; then
            runMSlist=false
        else
            rm -f "${MS_METADATA}"
        fi
    fi

    if [ "${runMSlist}" == "true" ]; then
        echo "Extracting metadata for science measurement set $MS_INPUT_SCIENCE"
        ${mslist} --full "${MS_INPUT_SCIENCE}" 1>& "${MS_METADATA}"
        err=$?
        if [ $err -ne 0 ]; then
            echo "ERROR - the 'mslist' command failed."
            echo "        Full command:  ${mslist} --full $MS_INPUT_1934"
            echo "Exiting pipeline."
            exit $err
        else
            cat >> "${MS_METADATA}" <<EOF
${METADATA_IS_GOOD} ${NOW}
EOF
        fi
    else
        echo "Reusing science metadata file ${MS_METADATA}"
    fi

    # Get the observation time
    obsdate=$(grep "Observed from" "${MS_METADATA}" | head -1 | awk '{print $7}' | sed -e 's|/| |g' | awk '{print $1}' | sed -e 's/-/ /g')
    obstime=$(grep "Observed from" "${MS_METADATA}" | head -1 | awk '{print $7}' | sed -e 's|/| |g' | awk '{print $2}')
    DATE_OBS=$(date -d "$obsdate" +"%Y-%m-%d")
    DATE_OBS="${DATE_OBS}T${obstime}"

    if [ "${obsdate}" == "" ] || [ "${obstime}" == "" ]; then
        echo "ERROR - unable to determine observation time/date of science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Get the duration of the observation
    DURATION=$(grep "elapsed time" "${MS_METADATA}" | head -1 | awk '{print $11}')
    if [ "${DURATION}" == "" ]; then
        echo "ERROR - unable to determine observation duration for science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Get the number of antennas used in the observation
    NUM_ANT_SCIENCE=$(grep Antennas "${MS_METADATA}" | head -1 | awk '{print $6}')
    NUM_ANT=$NUM_ANT_SCIENCE

    if [ "${NUM_ANT_SCIENCE}" == "" ]; then
        echo "ERROR - unable to determine number of antennas in science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Get the number of channels used
    NUM_CHAN=$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="${MS_METADATA}" --val=nChan)
    # centre frequency - includes units
    CENTRE_FREQ="$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="${MS_METADATA}" --val=Freq)"
    # bandwidth - includes units
    BANDWIDTH="$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="${MS_METADATA}" --val=Bandwidth)"

    if [ "${NUM_CHAN}" == "" ] || [ "${CENTRE_FREQ}" == "" ] || [ "${BANDWIDTH}" == "" ]; then
        echo "ERROR - unable to determine frequency setup (# channels/freq0/bandwidth) in science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Get the requested number of channels from the config, and make sure they are the same for 1934
    # & science observations

    # Number of channels in science observation (used in applying the bandpass solution)
    if [ "$CHAN_RANGE_SCIENCE" == "" ]; then
        NUM_CHAN_SCIENCE=${NUM_CHAN}
        CHAN_RANGE_SCIENCE="1-${NUM_CHAN}"
    else
        NUM_CHAN_SCIENCE=$(echo "${CHAN_RANGE_SCIENCE}" | awk -F'-' '{print $2-$1+1}')
    fi

    if [ "${DO_1934_CAL}" == "true" ] && [ "${DO_SCIENCE_FIELD}" == "true" ]; then
        if [ "${NUM_ANT_1934}" != "${NUM_ANT_SCIENCE}" ]; then
            echo "ERROR! Number of antennas for 1934-638 observation (${NUM_ANT_1934}) is different to the science observation (${NUM_ANT_SCIENCE})."
            exit 1
        fi
        if [ "${NUM_CHAN_1934}" != "${NUM_CHAN_SCIENCE}" ]; then
            echo "ERROR! Number of channels for 1934-638 observation (${NUM_CHAN_1934}) is different to the science observation (${NUM_CHAN_SCIENCE})."
            exit 1
        fi
    fi

    if [ "${NUM_CHAN}" -lt "${NUM_CHAN_SCIENCE}" ]; then
        echo "ERROR! Number of channels requested for the science observation (${NUM_CHAN_SCIENCE}, from \"${CHAN_RANGE_SCIENCE}\") is bigger than the number in the MS (${NUM_CHAN})."
        exit 1
    fi

    ####
    # Define the list of fields and associated tiles

    # Find the number of fields in the MS
    NUM_FIELDS=$(grep Fields "${MS_METADATA}" | head -1 | cut -f 4- | cut -d' ' -f 2)
    if [ "${NUM_FIELDS}" == "" ]; then
        echo "ERROR - unable to determine number of fields in science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    FIELDLISTFILE=${metadata}/fieldlist-${msname}.txt
    if [ ! -e "$FIELDLISTFILE" ]; then
        # This works on the assumption we have something like this in
        # the mslist output:
# 2017-09-12 01:31:12	INFO		Fields: 2
# 2017-09-12 01:31:12	INFO	+	  ID   Code Name                RA               Decl           Epoch        nRows
# 2017-09-12 01:31:12	INFO	+	  0         DRAGN_T0-0A         17:05:10.000000 -24.40.00.00000 J2000      1934712
# 2017-09-12 01:31:12	INFO	+	  1         DRAGN_T0-0B         17:06:07.763000 -25.15.52.00000 J2000      1929096
        # We look for the "Fields: n" line, get the next n+1 lines,
        # and only keep the last n, which should be all the fields
        grep -A$(echo $NUM_FIELDS | awk '{print $1+1}') "Fields: ${NUM_FIELDS}" "${MS_METADATA}" | tail -n ${NUM_FIELDS} | cut -f 4- >> "$FIELDLISTFILE"
    fi

    FIELD_LIST=""
    TILE_LIST=""
    NUM_TILES=0
    for FIELD in $(sort -k2 "$FIELDLISTFILE" | awk '{print $2}' | uniq );
    do
        FIELD_LIST="$FIELD_LIST $FIELD"
        getTile
        if [ "$FIELD" != "$TILE" ]; then
            isNew=true
            for THETILE in $TILE_LIST; do
                if [ "$TILE" == "$THETILE" ]; then
                    isNew=false
                fi
            done
            if [ "$isNew" == "true" ]; then
                TILE_LIST="$TILE_LIST $TILE"
                ((NUM_TILES++))
            fi
        fi
    done

    # Print a simplified list of fields for the user
    echo "List of fields: "
    COUNT=0
    for FIELD in ${FIELD_LIST}; do
        ID=$(echo "$COUNT" | awk '{printf "%02d",$1}')
        echo "${ID} - ${FIELD}"
        ((COUNT++))
    done


    # Set the OPAL Project ID
    BACKUP_PROJECT_ID=${PROJECT_ID}

    if [ "${USE_CLI}" == "true" ] && [ "${IS_BETA}" != "true" ]; then

        # Run schedblock to get parset & variables
        sbinfo="${metadata}/schedblock-info-${SB_SCIENCE}.txt"
        getSchedblock=true
        if [ -e "${sbinfo}" ]; then
            if [ "$(grep -c "${METADATA_IS_GOOD}" "${sbinfo}")" -gt 0 ]; then
                # SB info file exists and is complete. Don't need to regenerate.
                getSchedblock=false
                echo "Reusing schedblock info file $sbinfo for SBID ${SB_SCIENCE}"
            else
                rm -f "$sbinfo"
            fi
        fi
        if [ "${getSchedblock}" == "true" ]; then
            echo "Using $sbinfo as location for SB metadata"
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
            cat >> $sbinfo <<EOF
${METADATA_IS_GOOD} ${NOW}
EOF
        fi
        awkstr="\$1==${SB_SCIENCE}"
        PROJECT_ID=$(awk "$awkstr" "${sbinfo}" | awk '{split($0,a,FS); print a[NF];}')
        if [ "$PROJECT_ID" == "" ]; then
            PROJECT_ID=$BACKUP_PROJECT_ID
        fi

        # Check for the current state of the Scheduling block.
        # If it is OBSERVED, this means the processing has not been
        # started previously. In this case only, transition the SB
        # using the command-line interface to PROCESSING. If the
        # pipeline is being run by the askapops user, then we can
        # annotate the relevant JIRA ticket that we have done so.
        #
        if [ "${DO_SCIENCE_FIELD}" == "true" ]; then

            SB_STATE_INITIAL=$(awk "$awkstr" "${sbinfo}" | awk '{split($0,a,FS); print a[NF-3];}')
            if [ "${SB_STATE_INITIAL}" == "OBSERVED" ]; then
                loadModule askapcli
                schedblock transition -s PROCESSING ${SB_SCIENCE} > "${logs}/transition-to-PROCESSING.log"
                err=$?
                unloadModule askapcli
                if [ $err -ne 0 ]; then
                    echo "$(date): ERROR - 'schedblock transition' failed for SB ${SB_SCIENCE} with error code \$err"
                fi
                if [ "$(whoami)" == "askapops" ]; then
                    if [ $err -eq 0 ]; then
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "Commencing processing. SB ${SB_SCIENCE} transitioned to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a "${ERROR_FILE}"
                        fi
                    else
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "ERROR -- Failed to transition SB ${SB_SCIENCE} to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a "${ERROR_FILE}"
                        fi
                    fi
                fi
            fi

        fi

        # Do the same for the 1934 SB
        if [ "${DO_1934_CAL}" == "true" ]; then

            # Run schedblock to get parset & variables
            sbinfoCal="${metadata}/schedblock-info-${SB_1934}.txt"
            getSchedblock=true
            if [ -e "${sbinfoCal}" ]; then
                if [ "$(grep -c "${METADATA_IS_GOOD}" "${sbinfoCal}")" -gt 0 ]; then
                    # SB info file exists and is complete. Don't need to regenerate.
                    getSchedblock=false
                    echo "Reusing schedblock info file $sbinfoCal for SBID ${SB_1934}"
                else
                    rm -f "$sbinfoCal"
                fi
            fi
            if [ "${getSchedblock}" == "true" ]; then
                echo "Using $sbinfoCal as location for bandpass SB metadata"
                loadModule askapcli
                schedblock info -v -p ${SB_1934} > $sbinfoCal
                err=$?
                unloadModule askapcli
                if [ $err -ne 0 ]; then
                    echo "ERROR - the 'schedblock' command failed."
                    echo "        Full command:   schedblock info -v -p ${SB_1934}"
                    echo "Exiting pipeline."
                    exit $err
                fi
                cat >> ${sbinfoCal} <<EOF
${METADATA_IS_GOOD} ${NOW}
EOF
            fi

            SB_STATE_INITIAL_CAL=$(awk "$awkstr" "${sbinfoCal}" | awk '{split($0,a,FS); print a[NF-3];}')
            if [ "${SB_STATE_INITIAL_CAL}" == "OBSERVED" ]; then
                loadModule askapcli
                schedblock transition -s PROCESSING ${SB_1934} >> "${logs}/transition-to-PROCESSING.log"
                err=$?
                unloadModule askapcli
                if [ $err -ne 0 ]; then
                    echo "$(date): ERROR - 'schedblock transition' failed for SB ${SB_1934} with error code \$err"
                fi
                if [ "$(whoami)" == "askapops" ]; then
                    if [ $err -eq 0 ]; then
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "Commencing processing. SB ${SB_1934} transitioned to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a "${ERROR_FILE}"
                        fi
                    else
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "ERROR -- Failed to transition SB ${SB_1934} to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a "${ERROR_FILE}"
                        fi
                    fi
                fi

            fi

        fi

    elif [ "${USE_CLI}" != "true" ]; then
        
        echo "You have set USE_CLI=false, so not contacting the schedblock service."
        echo "  SB State transitions will not be possible."
        
    fi


    # Find the beam centre locations
    . "${PIPELINEDIR}/findBeamCentres.sh"

fi

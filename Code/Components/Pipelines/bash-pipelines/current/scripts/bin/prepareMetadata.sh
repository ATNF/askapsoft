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

####################
# Input Measurement Sets
#  We define these based on the SB number

# 1934-638 calibration
if [ $DO_1934_CAL == true ]; then

    if [ "$MS_INPUT_1934" == "" ]; then
        if [ $SB_1934 != "SET_THIS" ]; then
	    sb1934dir=$DIR_SB/$SB_1934
	    if [ `\ls $sb1934dir | grep "ms" | wc -l` == 1 ]; then
	        MS_INPUT_1934=$sb1934dir/`\ls $sb1934dir | grep "ms"`
	    else
	        echo "SB directory $SB_1934 has more than one measurement set. Please specify with parameter 'MS_INPUT_1934'."
	    fi
        else
	    echo "You must set either 'SB_1934' (scheduling block number) or 'MS_INPUT_1934' (1934 measurement set)."
        fi
    fi
    if [ "$MS_INPUT_1934" == "" ]; then
	echo "Parameter 'MS_INPUT_1934' not defined. Turning off 1934-638 processing with DO_1934_CAL=false."
        DO_1934_CAL=false
    fi

fi

# science observation - check that MS_INPUT_SCIENCE is OK:
if [ $DO_SCIENCE_FIELD == true ]; then
    if [ "$MS_INPUT_SCIENCE" == "" ]; then
        if [ $SB_SCIENCE != "SET_THIS" ]; then
	    sbScienceDir=$DIR_SB/$SB_SCIENCE
	    if [ `\ls $sbScienceDir | grep "ms" | wc -l` == 1 ]; then
	        MS_INPUT_SCIENCE=$sbScienceDir/`\ls $sbScienceDir | grep "ms"`
	    else
	        echo "SB directory $SB_SCIENCE has more than one measurement set. Please specify with parameter 'MS_INPUT_SCIENCE'."
	    fi
        else
	    echo "You must set either 'SB_SCIENCE' (scheduling block number) or 'MS_INPUT_SCIENCE' (Science observation measurement set)."
        fi
    fi
    if [ "$MS_INPUT_SCIENCE" == "" ]; then
        if [ $DO_SCIENCE_FIELD == true ]; then
	    echo "Parameter 'MS_INPUT_SCIENCE' not defined. Turning off splitting/flagging with DO_FLAG_SCIENCE=false and pushing on.."
        fi
        DO_SCIENCE_FIELD=false
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
if [ $DO_1934_CAL == true ] && [ "${MS_INPUT_1934}" != "" ]; then

    getMSname ${MS_INPUT_1934}
    MS_METADATA=$metadata/mslist-cal-${msname}.txt

    runMSlist=true
    if [ -e "${MS_METADATA}" ]; then
        if [ `grep ${METADATA_IS_GOOD} ${MS_METADATA} | wc -l` -gt 0 ]; then
            runMSlist=false
        else
            rm -f ${MS_METADATA}
        fi
    fi
    
    if [ "${runMSlist}" == "true" ]; then
        echo "Extracting metadata for calibrator measurement set $MS_INPUT_1934"
        ${mslist} --full $MS_INPUT_1934 1>& ${MS_METADATA}
        err=$?
        if [ $err -ne 0 ]; then
            echo "ERROR - the 'mslist' command failed."
            echo "        Full command:  ${mslist} --full $MS_INPUT_1934" 
            echo "Exiting pipeline."
            exit $err
        else
            cat >> ${MS_METADATA} <<EOF
${METADATA_IS_GOOD} ${NOW}
EOF
        fi
    else
        echo "Reusing calibrator metadata file ${MS_METADATA}"
    fi

    # Number of antennas used in the calibration observation
    NUM_ANT_1934=`grep Antennas ${MS_METADATA} | head -1 | awk '{print $6}'`
    NUM_ANT=$NUM_ANT_1934

    if [ "${NUM_ANT_1934}" == "" ]; then
        echo "ERROR - unable to determine number of antennas in calibration dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi
    
    # Number of channels used in the calibration observation
    NUM_CHAN=`python ${PIPELINEDIR}/parseMSlistOutput.py --file=${MS_METADATA} --val=nChan`

    if [ "${NUM_CHAN}" == "" ]; then
        echo "ERROR - unable to determine number of channels in calibration dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi
        
    # Number of channels for 1934 observation
    if [ "${CHAN_RANGE_1934}" == "" ]; then
        NUM_CHAN_1934=${NUM_CHAN}
        CHAN_RANGE_1934="1-${NUM_CHAN}"
    else
        NUM_CHAN_1934=`echo $CHAN_RANGE_1934 | awk -F'-' '{print $2-$1+1}'`
    fi

    if [ "${NUM_CHAN}" -lt "${NUM_CHAN_1934}" ]; then
        echo "ERROR! Number of channels requested for the calibration observation (${NUM_CHAN_1934}, from \"${CHAN_RANGE_1934}\") is bigger than the number in the MS (${NUM_CHAN})."
        exit 1
    fi


fi
    
# Once we've defined the science MS name, extract metadata from it
if [ "${MS_INPUT_SCIENCE}" != "" ]; then

    # define $msname
    getMSname ${MS_INPUT_SCIENCE}

    # Extract the MS metadata into a local file ($MS_METADATA) for parsing
    MS_METADATA=$metadata/mslist-${msname}.txt

    runMSlist=true
    if [ -e "${MS_METADATA}" ]; then
        if [ `grep ${METADATA_IS_GOOD} ${MS_METADATA} | wc -l` -gt 0 ]; then
            runMSlist=false
        else
            rm -f ${MS_METADATA}
        fi
    fi
    
    if [ "${runMSlist}" == "true" ]; then
        echo "Extracting metadata for science measurement set $MS_INPUT_SCIENCE"
        ${mslist} --full $MS_INPUT_SCIENCE 1>& ${MS_METADATA}
        err=$?
        if [ $err -ne 0 ]; then
            echo "ERROR - the 'mslist' command failed."
            echo "        Full command:  ${mslist} --full $MS_INPUT_1934" 
            echo "Exiting pipeline."
            exit $err
        else
            cat >> ${MS_METADATA} <<EOF
${METADATA_IS_GOOD} ${NOW}
EOF
        fi
    else
        echo "Reusing science metadata file ${MS_METADATA}"
    fi

    # Get the observation time
    obsdate=`grep "Observed from" ${MS_METADATA} | head -1 | awk '{print $7}' | sed -e 's|/| |g' | awk '{print $1}' | sed -e 's/-/ /g'`
    obstime=`grep "Observed from" ${MS_METADATA} | head -1 | awk '{print $7}' | sed -e 's|/| |g' | awk '{print $2}'`
    DATE_OBS=`date -d "$obsdate" +"%Y-%m-%d"`
    DATE_OBS="${DATE_OBS}T${obstime}"

    if [ "${obsdate}" == "" ] || [ "${obstime}" == "" ]; then
        echo "ERROR - unable to determine observation time/date of science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Get the duration of the observation
    DURATION=`grep "elapsed time" ${MS_METADATA} | head -1 | awk '{print $11}'`
    if [ "${DURATION}" == "" ]; then
        echo "ERROR - unable to determine observation duration for science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi

    # Get the number of antennas used in the observation
    NUM_ANT_SCIENCE=`grep Antennas ${MS_METADATA} | head -1 | awk '{print $6}'`
    NUM_ANT=$NUM_ANT_SCIENCE

    if [ "${NUM_ANT_SCIENCE}" == "" ]; then
        echo "ERROR - unable to determine number of antennas in science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi
    
    # Get the number of channels used
    NUM_CHAN=`python ${PIPELINEDIR}/parseMSlistOutput.py --file=${MS_METADATA} --val=nChan`
    # centre frequency - includes units
    CENTRE_FREQ="`python ${PIPELINEDIR}/parseMSlistOutput.py --file=${MS_METADATA} --val=Freq`"
    # bandwidth - includes units
    BANDWIDTH="`python ${PIPELINEDIR}/parseMSlistOutput.py --file=${MS_METADATA} --val=Bandwidth`"

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
        NUM_CHAN_SCIENCE=`echo $CHAN_RANGE_SCIENCE | awk -F'-' '{print $2-$1+1}'`
    fi

    if [ ${DO_1934_CAL} == true ] && [ $DO_SCIENCE_FIELD == true ]; then
        if [ ${NUM_ANT_1934} != ${NUM_ANT_SCIENCE} ]; then
            echo "ERROR! Number of antennas for 1934-638 observation (${NUM_ANT_1934}) is different to the science observation (${NUM_ANT_SCIENCE})."
            exit 1
        fi
        if [ ${NUM_CHAN_1934} != ${NUM_CHAN_SCIENCE} ]; then
            echo "ERROR! Number of channels for 1934-638 observation (${NUM_CHAN_1934}) is different to the science observation (${NUM_CHAN_SCIENCE})."
            exit 1
        fi
    fi

    if [ ${NUM_CHAN} -lt ${NUM_CHAN_SCIENCE} ]; then
        echo "ERROR! Number of channels requested for the science observation (${NUM_CHAN_SCIENCE}, from \"${CHAN_RANGE_SCIENCE}\") is bigger than the number in the MS (${NUM_CHAN})."
        exit 1
    fi

    ####
    # Define the list of fields and associated tiles
    
    # Find the number of fields in the MS
    NUM_FIELDS=`grep Fields ${MS_METADATA} | head -1 | cut -f 4- | cut -d' ' -f 2`
    if [ "${NUM_FIELDS}" == "" ]; then
        echo "ERROR - unable to determine number of fields in science dataset"
        echo "        please check metadata in ${MS_METADATA}"
        echo "Exiting pipeline."
        exit 1
    fi
    
    FIELDLISTFILE=${metadata}/fieldlist-${msname}.txt
    if [ ! -e $FIELDLISTFILE ]; then
        grep -A${NUM_FIELDS} RA ${MS_METADATA} | tail -n ${NUM_FIELDS} | cut -f 4- >> $FIELDLISTFILE
    fi

    FIELD_LIST=""
    TILE_LIST=""
    NUM_TILES=0
    for FIELD in `sort -k2 $FIELDLISTFILE | awk '{print $2}' | uniq `;
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
                ((NUM_TILES++))
            fi
        fi
    done

    # Print a simplified list of fields for the user
    echo "List of fields: "
    COUNT=0
    for FIELD in ${FIELD_LIST}; do
        ID=`echo $COUNT | awk '{printf "%02d",$1}'`
        echo "${ID} - ${FIELD}"
        ((COUNT++))
    done

    
    # Set the OPAL Project ID
    BACKUP_PROJECT_ID=${PROJECT_ID}

    if [ "${IS_BETA}" != "true" ]; then
        
        # Run schedblock to get parset & variables
        sbinfo="${metadata}/schedblock-info-${SB_SCIENCE}.txt"
        if [ -e ${sbinfo} ] && [ `wc -l $sbinfo | awk '{print $1}'` -gt 1 ]; then
            echo "Reusing schedblock info file $sbinfo for SBID ${SB_SCIENCE}"
        else
            if [ -e ${sbinfo} ]; then
                rm -f $sbinfo
            fi
            echo "Using $sbinfo as location for SB metadata"
            module load askapcli
            schedblock info -v -p ${SB_SCIENCE} > $sbinfo
            err=$?
            module unload askapcli
            if [ $err -ne 0 ]; then
                echo "ERROR - the 'schedblock' command failed."
                echo "        Full command:   schedblock info -v -p ${SB_SCIENCE}"
                echo "Exiting pipeline."
                exit $err
            fi
        fi
        awkstr="\$1==${SB_SCIENCE}"
        PROJECT_ID=`awk $awkstr ${sbinfo} | awk '{split($0,a,FS); print a[NF];}'`
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
                module load askapcli
                schedblock transition -s PROCESSING ${SB_SCIENCE} > "${logs}/transition-to-PROCESSING.log"
                err=$?
                module unload askapcli
                if [ $err -ne 0 ]; then
                    echo "$(date): ERROR - 'schedblock transition' failed for SB ${SB_SCIENCE} with error code \$err"
                fi
                if [ "$(whoami)" == "askapops" ]; then
                    if [ $err -eq 0 ]; then
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "Commencing processing. SB ${SB_SCIENCE} transitioned to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a ${ERROR_FILE}
                        fi
                    else
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "ERROR -- Failed to transition SB ${SB_SCIENCE} to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a ${ERROR_FILE}
                        fi
                    fi
                fi
            fi

        fi

    # Do the same for the 1934 SB
        if [ "${DO_1934_CAL}" == "true" ]; then

            # Run schedblock to get parset & variables
            sbinfoCal="${metadata}/schedblock-info-${SB_1934}.txt"
            if [ -e "${sbinfoCal}" ] && [ "$(wc -l "$sbinfoCal" | awk '{print $1}')" -gt 1 ]; then
                echo "Reusing schedblock info file $sbinfoCal for SBID ${SB_1934}"
            else
                if [ -e "${sbinfoCal}" ]; then
                    rm -f "$sbinfoCal"
                fi
                echo "Using $sbinfo as location for bandpass SB metadata"
                module load askapcli
                schedblock info -v -p ${SB_1934} > $sbinfoCal
                err=$?
                module unload askapcli
                if [ $err -ne 0 ]; then
                    echo "ERROR - the 'schedblock' command failed."
                    echo "        Full command:   schedblock info -v -p ${SB_1934}"
                    echo "Exiting pipeline."
                    exit $err
                fi
            fi

            SB_STATE_INITIAL_CAL=$(awk "$awkstr" "${sbinfoCal}" | awk '{split($0,a,FS); print a[NF-3];}')
            if [ "${SB_STATE_INITIAL_CAL}" == "OBSERVED" ]; then
                module load askapcli
                schedblock transition -s PROCESSING ${SB_1934} >> "${logs}/transition-to-PROCESSING.log"
                err=$?
                module unload askapcli
                if [ $err -ne 0 ]; then
                    echo "$(date): ERROR - 'schedblock transition' failed for SB ${SB_1934} with error code \$err"
                fi
                if [ "$(whoami)" == "askapops" ]; then
                    if [ $err -eq 0 ]; then
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "Commencing processing. SB ${SB_1934} transitioned to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a ${ERROR_FILE}
                        fi
                    else
                        schedblock annotate -i ${SB_JIRA_ISSUE} -c "ERROR -- Failed to transition SB ${SB_SCIENCE} to PROCESSING." ${SB_SCIENCE}
                        annotErr=$?
                        if [ ${annotErr} -ne 0 ]; then
                            echo "$(date): ERROR - 'schedblock annotate' failed with error code ${annotErr}" | tee -a ${ERROR_FILE}
                        fi
                    fi
                fi
 
            fi
            
        fi



        
    fi


    # Find the beam centre locations    
    . ${PIPELINEDIR}/findBeamCentres.sh

fi

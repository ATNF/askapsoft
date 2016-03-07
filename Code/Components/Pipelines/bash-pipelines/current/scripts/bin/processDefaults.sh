#!/usr/bin/env bash
#
# This file takes the default values, after any modification by a
# user's config file, and creates other variables that depend upon
# them and do not require user input.
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

# Make sure we only run this file once!
PROCESS_HAS_RUN=false
if [ $PROCESS_HAS_RUN == false ]; then

    PROCESS_HAS_RUN=true

    ####################
    # Define the full path of output directory
    OUTPUT="${CWD}/${OUTPUT}"
    mkdir -p $OUTPUT
    . ${PIPELINEDIR}/utils.sh
    cd $OUTPUT
    echo $NOW >> PROCESSED_ON
    if [ ! -e ${stats} ]; then
        ln -s ${CWD}/${stats} .
    fi
    cd $CWD
    
    ####################
    # Slurm file headers

    # Reservation string
    if [ "$RESERVATION" != "" ]; then
        RESERVATION_REQUEST="#SBATCH --reservation=${RESERVATION}"
    else
        RESERVATION_REQUEST="# No reservation requested"
    fi

    # Email request
    if [ "$EMAIL" != "" ]; then
        EMAIL_REQUEST="#SBATCH --mail-user=${EMAIL}
#SBATCH --mail-type=${EMAIL_TYPE}"
    else
        EMAIL_REQUEST="# No email notifications sent"
    fi

    ####################
    # Set the number of channels, and make sure they are the same for 1934
    # & science observations

    # Number of channels for 1934 observation
    NUM_CHAN_1934=`echo $CHAN_RANGE_1934 | awk -F'-' '{print $2-$1+1}'`
    # Number of channels in science observation (used in applying the bandpass solution)
    NUM_CHAN_SCIENCE=`echo $CHAN_RANGE_SCIENCE | awk -F'-' '{print $2-$1+1}'`

    if [ ${DO_1934_CAL} == true ] && [ $DO_SCIENCE_FIELD == true ]; then
        if [ ${NUM_CHAN_1934} != ${NUM_CHAN_SCIENCE} ]; then
            echo "ERROR! Number of channels for 1934-638 observation (${NUM_CHAN_1934}) is different to the science observation (${NUM_CHAN_SCIENCE})."
            exit 1
        fi
    fi

    ####################
    # Catching old parameters

    if [ "${NUM_CHAN}" != "" ]; then
        echo "You've entered NUM_CHAN=${NUM_CHAN}. This is no longer used!"
        echo "  Please use CHAN_RANGE_1934 & CHAN_RANGE_SCIENCE to specify number and range of channels."
    fi

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

    # science observation
    # If we are splitting/flagging, check MS_INPUT_SCIENCE is OK:
    if [ ${DO_FLAG_SCIENCE} == true ]; then
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
    # Check the number of beams

    nbeam=`echo $BEAM_MAX $BEAM_MIN | awk '{print $1-$2+1}'`

    # Turn off mosaicking if there is just a single beam
    if [ $nbeam -eq 1 ]; then
        if [ $DO_MOSAIC == true ]; then
	    echo "Only have a single beam to process, so setting DO_MOSAIC=false"
        fi
        DO_MOSAIC=false
    fi

    ####################
    # Parameters required for continuum imaging
    ####

    # nchanContSci = number of channels after averaging
    nchanContSci=`echo $NUM_CHAN_SCIENCE $NUM_CHAN_TO_AVERAGE | awk '{print $1/$2}'`

    # nworkergroupsSci = number of worker groups, used for MFS imaging. 
    nworkergroupsSci=`echo $NUM_TAYLOR_TERMS | awk '{print 2*$1-1}'`

    # total number of CPUs required for MFS continuum imaging, including
    # the master
    NUM_CPUS_CONTIMG_SCI=`echo $nchanContSci $nworkergroupsSci | awk '{print $1*$2+1}'`

    # Can't have -N greater than -n in the aprun call
    if [ ${NUM_CPUS_CONTIMG_SCI} -lt ${CPUS_PER_CORE_CONT_IMAGING} ]; then
        CPUS_PER_CORE_CONT_IMAGING=${NUM_CPUS_CONTIMG_SCI}
    fi

    ####################
    # Parameters required for spectral-line imaging
    ####

    if [ "${CHAN_RANGE_SL_SCIENCE}" == "" ]; then
        CHAN_RANGE_SL_SCIENCE="1-$NUM_CHAN_SCIENCE"
    fi

    # Method used for continuum subtraction
    if [ ${CONTSUB_METHOD} != "Cmodel" ] &&
           [ ${CONTSUB_METHOD} != "Components" ] &&
           [ ${CONTSUB_METHOD} != "CleanModel" ]; then
        CONTSUB_METHOD="Cmodel"
    fi
    # Old way of choosing above
    if [ "${BUILD_MODEL_FOR_CONTSUB}" != "" ] &&
           [ "${BUILD_MODEL_FOR_CONTSUB}" != "true" ]; then
        echo "WARN - the parameter BUILD_MODEL_FOR_CONTSUB is deprecated - please use CONTSUB_METHOD instead"
        CONTSUB_METHOD="CleanModel"
    fi
    

    ####################
    # Define the beam arrangements for linmos
    . ${PIPELINEDIR}/beamArrangements.sh

    # Fix the direction string for linmos - don't need the J2000 bit
    linmosFeedCentre=`echo $DIRECTION_SCI | awk -F',' '{printf "%s,%s]",$1,$2}'`

    ####################
    # Source-finding - number of cores:
    NUM_CPUS_SELAVY=`echo $SELAVY_NSUBX $SELAVY_NSUBY | awk '{print $1*$2+1}'`
    CPUS_PER_CORE_SELAVY=${NUM_CPUS_SELAVY}
    if [ ${CPUS_PER_CORE_SELAVY} -gt 20 ]; then
        CPUS_PER_CORE_SELAVY=20
    fi

    # If the linmos sourcefinding flag has not been set, then set it to
    # true only if both source-finding and linmos are requested.
    if [ ${DO_SOURCE_FINDING_MOSAIC} == SETME ]; then
        if [ ${DO_SOURCE_FINDING} == true ] && [ ${DO_MOSAIC} == true ]; then
            DO_SOURCE_FINDING_MOSAIC=true
        fi
    fi


fi

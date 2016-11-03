#!/bin/bash -l
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
if [ "$PROCESS_DEFAULTS_HAS_RUN" != "true" ]; then

    PROCESS_DEFAULTS_HAS_RUN=true

    ####################
    # Set the overall switches for CAL & SCIENCE fields

    if [ $DO_1934_CAL != true ]; then
        # turn off all calibrator-related switches
        DO_SPLIT_1934=false
        DO_FLAG_1934=false
        DO_FIND_BANDPASS=false
    fi

    if [ $DO_SCIENCE_FIELD != true ]; then
        # turn off all science-field switches
        DO_SPLIT_SCIENCE=false
        DO_FLAG_SCIENCE=false
        DO_APPLY_BANDPASS=false
        DO_AVERAGE_CHANNELS=false
        DO_CONT_IMAGING=false
        DO_SELFCAL=false
        DO_APPLY_CAL_CONT=false
        DO_CONTCUBE_IMAGING=false
        DO_SPECTRAL_IMAGING=false
        DO_SPECTRAL_IMSUB=false
        DO_MOSAIC=true
        DO_SOURCE_FINDING=false
        DO_SOURCE_FINDING_MOSAIC=false
        DO_ALT_IMAGER=false
        #
        DO_CONVERT_TO_FITS=false
        DO_MAKE_THUMBNAILS=false
        DO_STAGE_FOR_CASDA=false
    fi



    
    ####################
    # Define the full path of output directory
    OUTPUT="${BASEDIR}/${OUTPUT}"
    ORIGINAL_OUTPUT=${OUTPUT}
    mkdir -p $OUTPUT
    . ${PIPELINEDIR}/utils.sh
    BASEDIR=${BASEDIR}
    cd $OUTPUT
    echo $NOW >> PROCESSED_ON
    if [ ! -e ${stats} ]; then
        ln -s ${BASEDIR}/${stats} .
    fi
    cd $BASEDIR
    
    ####################
    # ASKAPsoft module usage

    moduleDir="/group/askap/modulefiles"
    module use $moduleDir
    if [ "${ASKAP_ROOT}" == "" ]; then
        # Has the user asked for a specific askapsoft module?
        if [ "$ASKAPSOFT_VERSION" != "" ]; then
            # If so, ensure it exists
            if [ "`module avail -t askapsoft/${ASKAPSOFT_VERSION} 2>&1 | grep askapsoft`" == "" ]; then
                echo "WARNING - Requested askapsoft version ${ASKAPSOFT_VERSION} not available"
                ASKAPSOFT_VERSION=""
            else
                # It exists. Add a leading slash so we can append to 'askapsoft' in the module call
                ASKAPSOFT_VERSION="/${ASKAPSOFT_VERSION}"
            fi
        fi
        # load the modules correctly
        if [ "`module list -t 2>&1 | grep askapsoft`" == "" ]; then
            # askapsoft is not loaded by the .bashrc - need to specify for
            # slurm jobfiles. Use the requested one if necessary.
            if [ "${ASKAPSOFT_VERSION}" == "" ]; then
                askapsoftModuleCommands="# Loading the default askapsoft module"
                echo "Will use the default askapsoft module"
            else
                askapsoftModuleCommands="# Loading the requested askapsoft module"
                echo "Will use the askapsoft module askapsoft${ASKAPSOFT_VERSION}"
            fi
            askapsoftModuleCommands="${askapsoftModuleCommands}
module use $moduleDir
module load askapdata
module load askapsoft${ASKAPSOFT_VERSION}"
            module load askapsoft${ASKAPSOFT_VERSION}
        else
            # askapsoft is already available.
            #  If a specific version has been requested, swap to that
            #  Otherwise, do nothing
            if [ "${ASKAPSOFT_VERSION}" != "" ]; then
                askapsoftModuleCommands="# Swapping to the requested askapsoft module
module use $moduleDir
module load askapdata
module swap askapsoft askapsoft${ASKAPSOFT_VERSION}"
                echo "Will use the askapsoft module askapsoft${ASKAPSOFT_VERSION}"
                module swap askapsoft askapsoft${ASKAPSOFT_VERSION}
            else
                askapsoftModuleCommands="# Using user-defined askapsoft module
module use $moduleDir
module load askapdata"
                echo "Will use the askapsoft module defined by your environment (`module list -t 2>&1 | grep askapsoft`)"
            fi
        fi

        # askappipeline module
        askappipelineVersion=`module list -t 2>&1 | grep askappipeline | sed -e 's|askappipeline/||g'`
        askapsoftModuleCommands="$askapsoftModuleCommands
module unload askappipeline
module load askappipeline/${askappipelineVersion}"
        
        echo " "

    else
        askapsoftModuleCommands="# Using ASKAPsoft code tree directly, so no need to load modules"
    fi

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

    # Account to be used
    if [ "$ACCOUNT" != "" ]; then
        ACCOUNT_REQUEST="#SBATCH --account=${ACCOUNT}"
    else
        ACCOUNT_REQUEST="# Using the default account"
    fi

    ####################
    # Set the times for each job
    if [ "$JOB_TIME_SPLIT_1934" == "" ]; then
        JOB_TIME_SPLIT_1934=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPLIT_SCIENCE" == "" ]; then
        JOB_TIME_SPLIT_SCIENCE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FLAG_1934" == "" ]; then
        JOB_TIME_FLAG_1934=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FLAG_SCIENCE" == "" ]; then
        JOB_TIME_FLAG_SCIENCE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FIND_BANDPASS" == "" ]; then
        JOB_TIME_FIND_BANDPASS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_APPLY_BANDPASS" == "" ]; then
        JOB_TIME_APPLY_BANDPASS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_AVERAGE_MS" == "" ]; then
        JOB_TIME_AVERAGE_MS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONT_IMAGE" == "" ]; then
        JOB_TIME_CONT_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONT_APPLYCAL" == "" ]; then
        JOB_TIME_CONT_APPLYCAL=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONTCUBE_IMAGE" == "" ]; then
        JOB_TIME_CONTCUBE_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_SPLIT" == "" ]; then
        JOB_TIME_SPECTRAL_SPLIT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_APPLYCAL" == "" ]; then
        JOB_TIME_SPECTRAL_APPLYCAL=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_CONTSUB" == "" ]; then
        JOB_TIME_SPECTRAL_CONTSUB=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_IMAGE" == "" ]; then
        JOB_TIME_SPECTRAL_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_IMCONTSUB" == "" ]; then
        JOB_TIME_SPECTRAL_IMCONTSUB=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_LINMOS" == "" ]; then
        JOB_TIME_LINMOS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SOURCEFINDING" == "" ]; then
        JOB_TIME_SOURCEFINDING=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FITS_CONVERT" == "" ]; then
        JOB_TIME_FITS_CONVERT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_THUMBNAILS" == "" ]; then
        JOB_TIME_THUMBNAILS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CASDA_UPLOAD" == "" ]; then
        JOB_TIME_CASDA_UPLOAD=${JOB_TIME_DEFAULT}
    fi
    
    ####################
    # Configure the list of beams to be processed
    # Lists each beam in ${BEAMS_TO_USE}

    #    if [ "${BEAMLIST}" == "" ]; then
    # just use BEAM_MIN & BEAM_MAX
    BEAMS_TO_USE=""
    for((b=${BEAM_MIN};b<=${BEAM_MAX};b++)); do
        thisbeam=`echo $b | awk '{printf "%02d",$1}'`
        BEAMS_TO_USE="${BEAMS_TO_USE} $thisbeam"
    done
    ### NOTE - CAN'T USE THIS DUE TO THE WAY CBPCALIBRATOR DEALS WITH BEAMS!
    #    else
    #        # re-print out the provided beam list with 0-leading integers
    #        BEAMS_TO_USE=""
    #        for b in $BEAMLIST; do
    #            thisbeam=`echo $b | awk '{printf "%02d",$1}'`
    #            BEAMS_TO_USE="${BEAMS_TO_USE} $thisbeam"
    #        done
    #    fi

    # Check the number of beams, and the maximum beam number (need the
    # latter for calibration tasks)
    maxbeam=-1
    nbeams=0
    for b in ${BEAMS_TO_USE}; do
        if [ $b -gt $maxbeam ]; then
            maxbeam=$b
        fi
        nbeams=`expr $nbeams + 1`
    done
    maxbeam=`expr $maxbeam + 1`

    
    ####################
    # Parameters required for continuum imaging
    ####

    if [ $DO_SCIENCE_FIELD == true ]; then

        # Name of the MS that should be flagged by flagScience.sh
        #   This gets set differently at different stages in the scripts
        msToFlag=""
        
        # Total number of channels must be exact multiple of averaging
        # width.
        # If it isn't, report an error and exit without running anything.
        averageWidthOK=`echo $NUM_CHAN_SCIENCE $NUM_CHAN_TO_AVERAGE | awk '{if (($1 % $2)==0) print "yes"; else print "no"}'`
        if [ "${averageWidthOK}" == "no" ]; then
            echo "ERROR! Number of channels (${NUM_CHAN_SCIENCE}) must be an exact multiple of NUM_CHAN_TO_AVERAGE (${NUM_CHAN_TO_AVERAGE}). Exiting."
            exit 1
        fi

        # nchanContSci = number of channels after averaging
        nchanContSci=`echo $NUM_CHAN_SCIENCE $NUM_CHAN_TO_AVERAGE | awk '{print $1/$2}'`

        # nworkergroupsSci = number of worker groups, used for MFS imaging. 
        nworkergroupsSci=`echo $NUM_TAYLOR_TERMS | awk '{print 2*$1-1}'`

        # total number of CPUs required for MFS continuum imaging, including
        # the master
        NUM_CPUS_CONTIMG_SCI=`echo $nchanContSci $nworkergroupsSci | awk '{print $1*$2+1}'`
        # if we are using the new imager we need to tweak this
        if [ $DO_ALT_IMAGER == true ]; then
            NUM_CPUS_CONTIMG_SCI=`echo $nchanContSci $nworkergroupsSci $NCHAN_PER_CORE | awk '{print ($1/$3)*$2+1}'`
            CPUS_PER_CORE_CONT_IMAGING=8
            NUM_CPUS_SPECIMG_SCI=`echo $NUM_CHAN_SCIENCE $NCHAN_PER_CORE_SL | awk '{print ($1/$2) + 1}'`
            CPUS_PER_CORE_SPEC_IMAGING=16

        fi

        # Can't have -N greater than -n in the aprun call
        if [ ${NUM_CPUS_CONTIMG_SCI} -lt ${CPUS_PER_CORE_CONT_IMAGING} ]; then
            CPUS_PER_CORE_CONT_IMAGING=${NUM_CPUS_CONTIMG_SCI}
        fi

        ##     if [ $IMAGE_AT_BEAM_CENTRES == true ]; then
        ##         # when imaging at beam centres, we *must* use the Cmodel
        ##         # approach
        ##         if [ ${SELFCAL_METHOD} != "Cmodel" ]; then
        ##             echo "WARNING - When imaging at beam centres, must use SELFCAL_METHOD=Cmodel"
        ##         fi
        ##         SELFCAL_METHOD="Cmodel"
        ##     else
        # Method used for self-calibration - needs to be either Cmodel or Components
        if [ ${SELFCAL_METHOD} != "Cmodel" ] &&
               [ ${SELFCAL_METHOD} != "Components" ]; then
            SELFCAL_METHOD="Cmodel"
        fi
        ##     fi

        # Set the polarisation list for the continuum cubes
        if [ "${CONTCUBE_POLARISATIONS}" == "" ]; then
            if [ "$DO_CONTCUBE_IMAGING" == "true" ]; then
                echo "WARNING - No polarisation given for continuum cube imaging. Turning off DO_CONTCUBE_IMAGING"
                DO_CONTCUBE_IMAGING=false
            fi
        else
            # set the POL_LIST parameter
            getPolList
        fi

        # Set the number of CPUs for the continuum cube imaging. Either
        # set to the number of averaged channels + 1, or use that given in
        # the config file, limiting to no bigger than this number 
        maxContCubeCores=`expr $nchanContSci + 1`
        if [ "${NUM_CPUS_CONTCUBE_SCI}" == "" ]; then
            # User has not specified
            NUM_CPUS_CONTCUBE_SCI=$maxContCubeCores
        elif [ $NUM_CPUS_CONTCUBE_SCI -gt $maxContCubeCores ]; then
            # Have more cores than we need - reduce number
            echo "NOTE - Reducing NUM_CPUS_CONTCUBE_SCI to $maxContCubeCores to match the number of averaged channels"
            NUM_CPUS_CONTCUBE_SCI=$maxContCubeCores
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
            echo "WARNING - the parameter BUILD_MODEL_FOR_CONTSUB is deprecated - please use CONTSUB_METHOD instead"
            CONTSUB_METHOD="Cmodel"
        fi

        if [ "${RESTORING_BEAM_LOG}" != "" ]; then
            echo "WARNING - the parameter RESTORING_BEAM_LOG is deprecated, and is constructed from the image name instead."
        fi

        ####################
        ##    # Define the beam arrangements for linmos
        ##    . ${PIPELINEDIR}/beamArrangements.sh

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

        ####################
        # Variable inputs to Self-calibration settings
        #  This section takes the provided parameters and creates
        #  arrays that have a (potentially) different value for each
        #  loop of the self-cal operation.
        #  Parameters covered are the selfcal interval, the
        #  source-finding threshold, and whether normalise gains is on
        #  or not
        if [ "`echo $SELFCAL_INTERVAL | grep "\["`" != "" ]; then
            # Have entered a comma-separate array in square brackets
            SELFCAL_INTERVAL_ARRAY=()
            for a in `echo $SELFCAL_INTERVAL | sed -e 's/[][,]/ /g'`; do
                SELFCAL_INTERVAL_ARRAY+=($a)
            done
        else
            SELFCAL_INTERVAL_ARRAY=()
            for((i=0;i<${SELFCAL_NUM_LOOPS};i++)); do
                SELFCAL_INTERVAL_ARRAY+=($SELFCAL_INTERVAL)
            done
        fi
        if [ "`echo $SELFCAL_SELAVY_THRESHOLD | grep "\["`" != "" ]; then
            # Have entered a comma-separate array in square brackets
            SELFCAL_SELAVY_THRESHOLD_ARRAY=()
            for a in `echo $SELFCAL_SELAVY_THRESHOLD | sed -e 's/[][,]/ /g'`; do
                SELFCAL_SELAVY_THRESHOLD_ARRAY+=($a)
            done
        else
            SELFCAL_SELAVY_THRESHOLD_ARRAY=()
            for((i=0;i<${SELFCAL_NUM_LOOPS};i++)); do
                SELFCAL_SELAVY_THRESHOLD_ARRAY+=($SELFCAL_SELAVY_THRESHOLD)
            done
        fi
        if [ "`echo $SELFCAL_NORMALISE_GAINS | grep "\["`" != "" ]; then
            # Have entered a comma-separate array in square brackets
            SELFCAL_NORMALISE_GAINS_ARRAY=()
            for a in `echo $SELFCAL_NORMALISE_GAINS | sed -e 's/[][,]/ /g'`; do
                SELFCAL_NORMALISE_GAINS_ARRAY+=($a)
            done
        else
            SELFCAL_NORMALISE_GAINS_ARRAY=()
            for((i=0;i<${SELFCAL_NUM_LOOPS};i++)); do
                SELFCAL_NORMALISE_GAINS_ARRAY+=($SELFCAL_NORMALISE_GAINS)
            done
        fi
        
    fi

fi

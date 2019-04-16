#!/bin/bash -l
#
# This file takes the default values, after any modification by a
# user config file, and creates other variables that depend upon
# them and do not require user input.
#
# @copyright (c) 2019 CSIRO
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

    # Turn on profiling if requested
    PROFILE_FLAG=""
    if [ "${USE_PROFILING}" == "true" ]; then
        PROFILE_FLAG="-p"
    fi

    ####################
    # Set the overall switches for CAL & SCIENCE fields

    if [ "${DO_1934_CAL}" != "true" ]; then
        # turn off all calibrator-related switches
        DO_SPLIT_1934=false
        DO_FLAG_1934=false
        DO_FIND_BANDPASS=false
    fi

    if [ "${DO_SCIENCE_FIELD}" != "true" ]; then
        # turn off all science-field switches
        DO_SPLIT_SCIENCE=false
        DO_FLAG_SCIENCE=false
        DO_APPLY_BANDPASS=false
        DO_AVERAGE_CHANNELS=false
        DO_CONT_IMAGING=false
        DO_SELFCAL=false
        DO_APPLY_CAL_CONT=false
        DO_APPLY_CAL_SL=false
        DO_CONTCUBE_IMAGING=false
        DO_SPECTRAL_IMAGING=false
        DO_SPECTRAL_IMSUB=false
        DO_MOSAIC=true
        DO_SOURCE_FINDING_CONT=false
        DO_SOURCE_FINDING_SPEC=false
        DO_SOURCE_FINDING_BEAMWISE=false
        DO_ALT_IMAGER=false
        #
        DO_CONVERT_TO_FITS=false
        DO_MAKE_THUMBNAILS=false
        DO_STAGE_FOR_CASDA=false
    fi

    # Turn off the purging of the full-resolution MS if we need to use it.
    if [ "${DO_SPECTRAL_PROCESSING}" == "true" ]; then

        PURGE_FULL_MS=false

    fi

    ####################
    # Define the full path of output directory
    OUTPUT="${BASEDIR}/${OUTPUT}"
    ORIGINAL_OUTPUT="${OUTPUT}"
    mkdir -p "$OUTPUT"
    . "${PIPELINEDIR}/utils.sh"
    BASEDIR=${BASEDIR}
    cd "$OUTPUT"
    echo "$NOW" >> PROCESSED_ON
    if [ ! -e "${stats}" ]; then
        ln -s "${BASEDIR}/${stats}" .
    fi
    cd "$BASEDIR"

    ####################
    # ASKAPsoft module usage

    # Make use of the ASKAP module directory right now
    module use "${ASKAP_MODULE_DIR}"

    askapsoftModuleCommands="# Need to load the slurm module directly
module load slurm
# Ensure the default python module is loaded before askapsoft
module unload python
module load python"

    if [ "${ASKAP_ROOT}" == "" ]; then
        # Has the user asked for a specific askapsoft module?
        if [ "${ASKAPSOFT_VERSION}" != "" ]; then
            # If so, ensure it exists
            if [ "$(module avail -t "askapsoft/${ASKAPSOFT_VERSION}" 2>&1 | grep askapsoft)" == "" ]; then
                echo "WARNING - Requested askapsoft version ${ASKAPSOFT_VERSION} not available"
                ASKAPSOFT_VERSION=""
            else
                # It exists. Add a leading slash so we can append to "askapsoft" in the module call
                ASKAPSOFT_VERSION="/${ASKAPSOFT_VERSION}"
            fi
        fi
        # load the modules correctly
        currentASKAPsoftVersion="$(module list -t 2>&1 | grep askapsoft)"
        if [ "${currentASKAPsoftVersion}" == "" ]; then
            # askapsoft is not loaded by the .bashrc - need to specify for
            # slurm jobfiles. Use the requested one if necessary.
            if [ "${ASKAPSOFT_VERSION}" == "" ]; then
                askapsoftModuleCommands="${askapsoftModuleCommands}
                # Loading the default askapsoft module"
                echo "Will use the default askapsoft module"
            else
                askapsoftModuleCommands="${askapsoftModuleCommands}
                # Loading the requested askapsoft module"
                echo "Will use the askapsoft module askapsoft${ASKAPSOFT_VERSION}"
            fi
            askapsoftModuleCommands="${askapsoftModuleCommands}
module use ${ASKAP_MODULE_DIR}
module load askapdata
module load askapsoft${ASKAPSOFT_VERSION}"
            module load askapsoft${ASKAPSOFT_VERSION}
        else
            # askapsoft is currently available in the module list
            #  If a specific version has been requested, swap to that
            #  Otherwise, do nothing
            if [ "${ASKAPSOFT_VERSION}" != "" ]; then
                askapsoftModuleCommands="${askapsoftModuleCommands}
# Swapping to the requested askapsoft module
module use ${ASKAP_MODULE_DIR}
module load askapdata
module unload askapsoft
module load askapsoft${ASKAPSOFT_VERSION}"
                echo "Will use the askapsoft module askapsoft${ASKAPSOFT_VERSION}"
                module unload askapsoft
                module load askapsoft${ASKAPSOFT_VERSION}
            else
                askapsoftModuleCommands="${askapsoftModuleCommands}
# Using user-defined askapsoft module
module use ${ASKAP_MODULE_DIR}
module load askapdata
module unload askapsoft
module load ${currentASKAPsoftVersion}"
                echo "Will use the askapsoft module defined by your environment (${currentASKAPsoftVersion})"
            fi
        fi

        # Check for the success in loading the askapsoft module
        askapsoftModuleCommands="${askapsoftModuleCommands}
# Exit if we could not load askapsoft
if [ \"\$ASKAPSOFT_RELEASE\" == \"\" ]; then
    echo \"ERROR: \\\$ASKAPSOFT_RELEASE not available - could not load askapsoft module.\"
    exit 1
fi"
        
        # askappipeline module
        askappipelineVersion=$(module list -t 2>&1 | grep askappipeline )
        askapsoftModuleCommands="${askapsoftModuleCommands}
module unload askappipeline
module load ${askappipelineVersion}"

        # Check for the success in loading the askappipeline module
        askapsoftModuleCommands="${askapsoftModuleCommands}
# Exit if we could not load askappipeline
if [ \"\$PIPELINEDIR\" == \"\" ]; then
    echo \"ERROR: \\\$PIPELINEDIR not available - could not load askappipeline module.\"
    exit 1
fi"

    else
        askapsoftModuleCommands="${askapsoftModuleCommands}
        # Using ASKAPsoft code tree directly, so no need to load modules"
        echo "Using ASKAPsoft code direct from your code tree at ASKAP_ROOT=$ASKAP_ROOT"
        echo "ASKAPsoft modules will *not* be loaded in the slurm files."
    fi

    ####################
    # ACES usage - either ACESOPS or personal/local ACES directory
    #  Here we define two functions: loadACES and unloadACES

    #  These are used to set up the ACES environment prior to running
    #  tools out of the ACES subversion repository. Rather than use
    #  $ACES as the location, we make use of $ACES_LOCATION, which we
    #  determine once. Similarly for the revision number
    #  $ACES_VERSION_USED.
    #  If USE_ACES_OPS=true then we use the acesops module, else we
    #  point to the ACES directory defined in the user environment,
    #  setting the ACES_VERSION_USED variable to be the revision at the
    #  top-level directory.
    #  We also define loadACES() and unloadACES(), that set up the
    #  correct environment depending on the situation.
    
    if [ "${USE_ACES_OPS}" != "true" ] && [ "${ACES}" == "" ]; then
        echo "WARNING - USE_ACES_OPS is not set to \"true\", but ACES is not defined. Using the acesops module."
        USE_ACES_OPS=true
    fi
    if [ "${USE_ACES_OPS}" != "true" ] && [ ! -d "${ACES}" ]; then
        echo "WARNING - USE_ACES_OPS is not set to \"true\", but ACES=$ACES can not be found. Using the acesops module."
        USE_ACES_OPS=true
    fi
    
    if [ "${USE_ACES_OPS}" == "true" ]; then
        function loadACES()
        {
            if [ "${ACESOPS_VERSION}" == "" ]; then
                module=acesops
            else
                module="acesops/${ACESOPS_VERSION}"
            fi
            loadModule ${module}
        }
        function unloadACES()
        {
            if [ "${ACESOPS_VERSION}" == "" ]; then
                module=acesops
            else
                module="acesops/${ACESOPS_VERSION}"
            fi
            unloadModule ${module}
        }
        acesModuleCommand="module load ${module}"

        # in case $ACES is already defined, we capture it here before overwriting it
        ACES_ORIGINAL=${ACES}
        loadACES
        ACES_LOCATION=${ACES}
        ACES_VERSION_USED=$(echo ${ACES_VERSION} | sed -e 's/r//g')
        echo "Using acesops module, version ${ACES_VERSION}"
        unloadACES
        ACES=${ACES_ORIGINAL}

        
    else
        ACES_LOCATION=${ACES}
        ACES_VERSION_USED=$(cd $ACES; svn info | grep Revision | awk '{print $2}')
        echo "Using non-module ACES directory $ACES, revision $ACES_VERSION_USED"
        function loadACES()
        {
            loadModule aces
        }
        function unloadACES()
        {
            unloadModule aces
        }
        acesModuleCommand="module load aces"
    fi

    #############################
    # Other version numbers

    # CASA
    CASA_VERSION_USED=""
    if [ "\${BANDPASS_SMOOTH_TOOL}" == "plot_caltable" ] ||
           [ "$(which imageToFITS 2> "/dev/null")" == "" ] ||
           [ "${DO_SPECTRAL_IMSUB}" == "true" ]; then
        loadModule casa
        CASA_VERSION_USED="$(module list casa 2>&1 | grep casa | awk '{print $2}' | sed -e 's|casa/||g')"
        unloadModule casa
    fi

    # AOFLAGGER
    AOFLAGGER_VERSION_USED=""
    if [ "${FLAG_WITH_AOFLAGGER}" != "" ]; then
        loadModule aoflagger
        AOFLAGGER_VERSION_USED="$(aoflagger --version)"
        unloadModule aoflagger
    fi

    # BPTOOL
    BPTOOL_VERSION_USED=""
    if [ "${BANDPASS_SMOOTH_TOOL}" == "smooth_bandpass" ]; then
        module use /group/askap/raj030/modulefiles
        loadModule bptool
        BPTOOL_VERSION_USED=${BPTOOL_VERSION}
        unloadModule bptool
    fi

    echo " "

    #############################
    # HISTORY metadata string

    imageHistoryString="\"Produced with ASKAPsoft version ${ASKAPSOFT_RELEASE}\""
    imageHistoryString="${imageHistoryString}, \"Processed with ASKAP pipeline version ${PIPELINE_VERSION}\""
    imageHistoryString="${imageHistoryString}, \"Processed with ACES software revision ${ACES_VERSION_USED}\""
    imageHistoryString="${imageHistoryString}, \"Processed with ASKAP pipelines on ${NOW_FMT}\""
    if [ "${CASA_VERSION_USED}" != "" ]; then
        imageHistoryString="${imageHistoryString}, \"Processed with CASA version ${CASA_VERSION_USED}\""
    fi
    if [ "${AOFLAGGER_VERSION_USED}" != "" ]; then
        imageHistoryString="${imageHistoryString}, \"Processed with AOFlagger: ${AOFLAGGER_VERSION_USED}\""
    fi
    if [ "${BPTOOL_VERSION_USED}" != "" ]; then
        imageHistoryString="${imageHistoryString}, \"Processed with BPTOOL version ${BPTOOL_VERSION_USED}\""
    fi
    

    
    #############################
    # CONVERSION TO FITS FORMAT
    
    # This function returns a bunch of text in $fitsConvertText that can
    # be pasted into an external slurm job file.
    # The text will perform the conversion of an given CASA image to FITS
    # format, and update the headers and history appropriately.
    # For the most recent askapsoft versions, it will use the imageToFITS
    # tool to do both, otherwise it will use casa to do so.

    # We can use the imageToFITS utility to do the conversion.
    fitsConvertText="# The following converts the file in \$casaim to a FITS file, after fixing headers.
if [ -e \"\${casaim}\" ] && [ ! -e \"\${fitsim}\" ]; then
    # The FITS version of this image does not exist

    cat > \"\$parset\" << EOFINNER
ImageToFITS.casaimage = \${casaim}
ImageToFITS.fitsimage = \${fitsim}
#ImageToFITS.stokesLast = true
EOFINNER
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} imageToFITS -c \"\${parset}\" >> \"\${log}\"

fi"

    # Update the headers
    fitsConvertText="${fitsConvertText}
# Header updates
NCORES=1
NPPN=1
updateArgs=\"--fitsfile=\${fitsim}\"
updateArgs=\"\${updateArgs} --telescope=${TELESCOP_KEYWORD}\"
updateArgs=\"\${updateArgs} --project=${PROJECT_ID}\"
updateArgs=\"\${updateArgs} --sbid=${SB_SCIENCE}\"
updateArgs=\"\${updateArgs} --dateobs=${DATE_OBS}\"
updateArgs=\"\${updateArgs} --duration=${DURATION}\"
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \"${PIPELINEDIR}/updateFITSheaders.py\" \${updateArgs} ${imageHistoryString}
"

    ####################
    # Slurm file headers
    #  This adds the slurm headers for the partition, cluster,
    #  account, constraints, reservation & email notifications

    SLURM_CONFIG="#SBATCH --partition=${QUEUE}"

    if [ "${CLUSTER}" != "" ]; then
        SLURM_CONFIG="${SLURM_CONFIG}
#SBATCH --clusters=${CLUSTER}"
    fi

    if [ "$EXCLUDE_NODE_LIST" != "" ]; then
        SLURM_CONFIG="${SLURM_CONFIG}
#SBATCH --exclude=${EXCLUDE_NODE_LIST}"
    fi
    
    # Account to be used
    if [ "$ACCOUNT" != "" ]; then
        SLURM_CONFIG="${SLURM_CONFIG}
#SBATCH --account=${ACCOUNT}"
    else
        SLURM_CONFIG="${SLURM_CONFIG}
# Using the default account"
    fi

    # Slurm constraints to be applied
    if [ "${CONSTRAINT}" != "" ]; then
        SLURM_CONFIG="${SLURM_CONFIG}
#SBATCH --constraint=${CONSTRAINT}"
    else
        SLURM_CONFIG="${SLURM_CONFIG}
# No further constraints applied"
    fi

    # Reservation string
    if [ "$RESERVATION" != "" ]; then
        SLURM_CONFIG="${SLURM_CONFIG}
#SBATCH --reservation=${RESERVATION}"
    else
        SLURM_CONFIG="${SLURM_CONFIG}
# No reservation requested"
    fi

    # Email request
    if [ "$EMAIL" != "" ]; then
        SLURM_CONFIG="${SLURM_CONFIG}
#SBATCH --mail-user=${EMAIL}
#SBATCH --mail-type=${EMAIL_TYPE}"
    else
        SLURM_CONFIG="${SLURM_CONFIG}
# No email notifications sent"
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
    if [ "$JOB_TIME_MSCONCAT_SCI_AV" == "" ]; then
        JOB_TIME_MSCONCAT_SCI_AV=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_MSCONCAT_SCI_SPECTRAL" == "" ]; then
        JOB_TIME_MSCONCAT_SCI_SPECTRAL=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONT_IMAGE" == "" ]; then
        JOB_TIME_CONT_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONT_SELFCAL" == "" ]; then
        JOB_TIME_CONT_SELFCAL=${JOB_TIME_DEFAULT}
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
    if [ "$JOB_TIME_SOURCEFINDING_CONT" == "" ]; then
        JOB_TIME_SOURCEFINDING_CONT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SOURCEFINDING_SPEC" == "" ]; then
        JOB_TIME_SOURCEFINDING_SPEC=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_DIAGNOSTICS" == "" ]; then
        JOB_TIME_DIAGNOSTICS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FITS_CONVERT" == "" ]; then
        JOB_TIME_FITS_CONVERT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_THUMBNAILS" == "" ]; then
        JOB_TIME_THUMBNAILS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_VALIDATE" == "" ]; then
        JOB_TIME_VALIDATE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CASDA_UPLOAD" == "" ]; then
        JOB_TIME_CASDA_UPLOAD=${JOB_TIME_DEFAULT}
    fi

    ####################
    # Configure the list of beams to be processed
    # Lists each beam in ${BEAMS_TO_USE}

    BEAMS_TO_USE=""
    if [ "${BEAMLIST}" == "" ]; then
        # just use BEAM_MIN & BEAM_MAX
        if [ "${NUM_BEAMS_FOOTPRINT}" != "" ] && [ ${BEAM_MAX} -ge $NUM_BEAMS_FOOTPRINT ]; then
            BEAM_MAX=$(echo $NUM_BEAMS_FOOTPRINT | awk '{print $1-1}')
            echo "WARNING - SB ${SB_SCIENCE} only has ${NUM_BEAMS_FOOTPRINT} beams - setting BEAM_MAX=${BEAM_MAX}"
        fi
        if [ "${NUM_BEAMS_FOOTPRINT_CAL}" != "" ] &&
               [ ${BEAM_MAX} -ge ${NUM_BEAMS_FOOTPRINT_CAL} ] &&
               [ "${DO_1934_CAL}" == "true" ]; then
            BEAM_MAX=$(echo $NUM_BEAMS_FOOTPRINT_CAL | awk '{print $1-1}')
            echo "WARNING - Bandpass SB ${SB_1934} only has ${NUM_BEAMS_FOOTPRINT_CAL} beams - setting BEAM_MAX=${BEAM_MAX}"
        fi
        for((b=BEAM_MIN;b<=BEAM_MAX;b++)); do
            thisbeam=$(echo "$b" | awk '{printf "%02d",$1}')
            BEAMS_TO_USE="${BEAMS_TO_USE} $thisbeam"
        done
    else
        # re-print out the provided beam list with 0-leading integers
        beamAwkFile="$tmp/beamParsing.awk"
        cat > "$beamAwkFile" <<EOF
BEGIN{
  str=""
}
{
  n=split(\$1,a,",");
  for(i=1;i<=n;i++){
    n2=split(a[i],a2,"-");
    for(b=a2[1];b<=a2[n2];b++){
      str=sprintf("%s %02d",str,b);
    }
  }
}
END{
  print str
}
EOF
        haveWarnedSci=false
        haveWarnedCal=false
        BEAMS_TO_USE=""
        for b in $(echo "$BEAMLIST" | awk -f "$beamAwkFile"); do
            if [ "${NUM_BEAMS_FOOTPRINT}" != "" ] &&
                   [ ${b} -ge $NUM_BEAMS_FOOTPRINT ]; then
                if [ "${haveWarnedSci}" != "true" ]; then
                    echo "WARNING - SB ${SB_SCIENCE} only has ${NUM_BEAMS_FOOTPRINT} beams"
                    haveWarnedSci=true
                fi
            elif [ "${NUM_BEAMS_FOOTPRINT_CAL}" != "" ] &&
                     [ ${b} -ge ${NUM_BEAMS_FOOTPRINT_CAL} ] &&
                     [ "${DO_1934_CAL}" == "true" ]; then
                if [ "${haveWarnedCal}" != "true" ]; then
                    echo "WARNING - Bandpass SB ${SB_1934} only has ${NUM_BEAMS_FOOTPRINT_CAL} beams"
                    haveWarnedCal=true
                fi
            else
                BEAMS_TO_USE="${BEAMS_TO_USE} $b"
            fi
        done
    fi

    echo "Using the following beams for the science data: $BEAMS_TO_USE"

    # Check the number of beams, and the maximum beam number (need the
    # latter for calibration tasks)
    maxbeam=-1
    nbeams=0
    for b in ${BEAMS_TO_USE}; do
        thisbeam=$( echo "$b" | awk '{printf "%d",$1}')
        if [ "$thisbeam" -gt "$maxbeam" ]; then
            maxbeam=$thisbeam
        fi
        ((nbeams++))
    done
    ((maxbeam++))

    # Handle the case where the SB MS is missing.
    # First for the bandpass calibrator
    if [ "${MS_CAL_MISSING}" == "true" ]; then
        # Turn off splitting
        DO_SPLIT_1934=false
        # Check for existence of each MS
        for((IBEAM=0; IBEAM<=highestBeam; IBEAM++)); do
            BEAM=$(echo "$IBEAM" | awk '{printf "%02d",$1}')
            find1934MSnames
            if [ "${DO_1934_CAL}" == "true" ] && [ ! -e ${msCal} ]; then
                echo "The MS for the calibration SB does not exist, and at least one derived MS (${msCal}) is not present."
                echo "Turning off Bandpass Calibration processing (\"DO_1934_CAL=false\")"
                DO_1934_CAL=false
            fi
        done
    fi

    # Now for the science field
    if [ "${MS_SCIENCE_MISSING}" == "true" ]; then
        # Turn off splitting
        DO_SPLIT_SCIENCE=false
        # Check for existence of each MS
        for BEAM in ${BEAMS_TO_USE}; do
            findScienceMSnames
            if [ "${DO_SCIENCE_FIELD}" == "true" ]; then
                if [ ! -e ${msSci} ] && [ ! -e ${msSciAv} ]; then
                    echo "The MS for the science field SB does not exist, and at least one beam (${BEAM}) does not have derived MSs."
                    echo "Turning off Science Field processing (\"DO_SCIENCE_FIELD=false\")"
                    DO_SCIENCE_FIELD=false
                fi
            fi
        done
    fi

    echo " "

    # Warning about a deprecated parameter
    if [ "${USE_DCP_TO_COPY_MS}" != "" ]; then
        echo "WARNING - the parameter USE_DCP_TO_COPY_MS is deprecated. The pipeline just uses cp to copy MSs now."
    fi
    

    ####################
    # Parameters required for the aoflagger option

    if [ "${FLAG_WITH_AOFLAGGER}" != "" ]; then
        # have set the global switch, so apply to the individual
        # task-level switches
        FLAG_1934_WITH_AOFLAGGER=${FLAG_WITH_AOFLAGGER}
        FLAG_SCIENCE_WITH_AOFLAGGER=${FLAG_WITH_AOFLAGGER}
        FLAG_SCIENCE_AV_WITH_AOFLAGGER=${FLAG_WITH_AOFLAGGER}
    fi

    if [ "${AOFLAGGER_STRATEGY}" != "" ]; then

        if [ ! -e "${AOFLAGGER_STRATEGY}" ]; then
            echo "ERROR - the AOflagger strategy file ${AOFLAGGER_STRATEGY} does not exist."
            echo "        Running AOflagger but without a strategy file"
        else
            AOFLAGGER_STRATEGY_1934="${AOFLAGGER_STRATEGY}"
            AOFLAGGER_STRATEGY_SCIENCE="${AOFLAGGER_STRATEGY}"
            AOFLAGGER_STRATEGY_SCIENCE_AV="${AOFLAGGER_STRATEGY}"
        fi

    fi        

    if [ "${AOFLAGGER_STRATEGY_1934}" != "" ] && [ ! -e "${AOFLAGGER_STRATEGY_1934}" ]; then
        echo "ERROR - the AOflagger strategy file \"${AOFLAGGER_STRATEGY_1934}\" does not exist."
        echo "        Running AOflagger on bandpass data but without a strategy file"
        AOFLAGGER_STRATEGY_1934=""
    fi
    if [ "${AOFLAGGER_STRATEGY_SCIENCE}" != "" ] && [ ! -e "${AOFLAGGER_STRATEGY_SCIENCE}" ]; then
        echo "ERROR - the AOflagger strategy file \"${AOFLAGGER_STRATEGY_SCIENCE}\" does not exist."
        echo "        Running AOflagger on science data but without a strategy file"
        AOFLAGGER_STRATEGY_SCIENCE=""
    fi
    if [ "${AOFLAGGER_STRATEGY_SCIENCE_AV}" != "" ] && [ ! -e "${AOFLAGGER_STRATEGY_SCIENCE_AV}" ]; then
        echo "ERROR - the AOflagger strategy file \"${AOFLAGGER_STRATEGY_SCIENCE_AV}\" does not exist."
        echo "        Running AOflagger on averaged science data but without a strategy file"
        AOFLAGGER_STRATEGY_SCIENCE_AV=""
    fi

    # Set the generic aoflagger command line options
    AOFLAGGER_OPTIONS=""
    if [ "${AOFLAGGER_VERBOSE}" == "true" ]; then
        AOFLAGGER_OPTIONS="${AOFLAGGER_OPTIONS} -v"
    fi
    if [ "${AOFLAGGER_UVW}" == "true" ]; then
        AOFLAGGER_OPTIONS="${AOFLAGGER_OPTIONS} -uvw"
    fi
    if [ "${AOFLAGGER_READ_MODE}" == "direct" ]; then
        AOFLAGGER_OPTIONS="${AOFLAGGER_OPTIONS} -direct-read"
    elif [ "${AOFLAGGER_READ_MODE}" == "indirect" ]; then
        AOFLAGGER_OPTIONS="${AOFLAGGER_OPTIONS} -indirect-read"
    elif [ "${AOFLAGGER_READ_MODE}" == "memory" ]; then
        AOFLAGGER_OPTIONS="${AOFLAGGER_OPTIONS} -memory-read"
    else
        if [ "${AOFLAGGER_READ_MODE}" != "auto" ]; then
            echo "WARNING - unknown AOFLAGGER_READ_MODE option \"${AOFLAGGER_READ_MODE}\". Setting to \"auto\""
        fi
        AOFLAGGER_OPTIONS="${AOFLAGGER_OPTIONS} -auto-read-mode"
    fi
    

    ####################
    # Parameters required for bandpass calibration
    ####
    if [ "${DO_FIND_BANDPASS}" == "true" ] || [ "${DO_APPLY_BANDPASS}" == "true" ]; then
        
        ####################
        # Filling out wildcards for calibration table
        ####
        # Replace the %s wildcard with the SBID
        sedstr="s|%s|${SB_1934}|g"
        TABLE_BANDPASS=$(echo "${TABLE_BANDPASS}" | sed -e "$sedstr")
        
    fi

    if [ "${BANDPASS_SMOOTH_TOOL}" != "plot_caltable" ] &&
           [ "${BANDPASS_SMOOTH_TOOL}" != "smooth_bandpass" ]; then
        echo "WARNING - Invalid value for BANDPASS_SMOOTH_TOOL (${BANDPASS_SMOOTH_TOOL}). Setting to \"plot_caltable\"."
        BANDPASS_SMOOTH_TOOL="plot_caltable"
    fi

    #########
    #  Applying the bandpass to the full science dataset can be done
    #  in parallel fashion. This sets up the number of processors per
    #  node

    NPPN_CAL_APPLY=$(echo ${NUM_CORES_CAL_APPLY} | awk '{nn=int(($1-1)/20)+1; nppn=int(($1-1)/nn)+1; print nppn}')

    
    ####################
    # Parameters required for science field imaging
    ####

    if [ "${DO_SCIENCE_FIELD}" == "true" ]; then

        if [ "${NSUB_CUBES}" != "" ]; then
            echo "WARNING - the parameter NSUB_CUBES is deprecated. Using NUM_SPECTRAL_WRITERS=${NUM_SPECTRAL_WRITERS} instead."
        fi

        ####################
        # Filling out wildcards for image names
        ####
        # Replace the %s wildcard with the SBID
        sedstr="s|%s|${SB_SCIENCE}|g"
        
        IMAGE_BASE_CONT=$(echo "${IMAGE_BASE_CONT}" | sed -e "$sedstr")
        IMAGE_BASE_CONTCUBE=$(echo "${IMAGE_BASE_CONTCUBE}" | sed -e "$sedstr")
        IMAGE_BASE_SPECTRAL=$(echo "${IMAGE_BASE_SPECTRAL}" | sed -e "$sedstr")

        ####################
        # Parameters required for continuum imaging
        ####

        # Check value of IMAGETYPE - needs to be casa or fits
        if [ "${IMAGETYPE_CONT}" != "casa" ] && [ "${IMAGETYPE_CONT}" != "fits" ]; then
            echo "ERROR - Invalid image type \"${IMAGETYPE_CONT}\" - IMAGETYPE_CONT needs to be casa or fits"
            echo "   Exiting"
            exit 1
        fi
        
        # Switching on the DO_ALT_IMAGER_CONT flag - if it is not
        # defined in the config file, then set to the value of
        # DO_ALT_IMAGER
        if [ "${DO_ALT_IMAGER}" == "true" ] && [ "${DO_ALT_IMAGER_CONT}" == "" ]; then
            DO_ALT_IMAGER_CONT=${DO_ALT_IMAGER}
        fi
        
        # Total number of channels must be exact multiple of averaging width.
        # If it is not, report an error and exit without running anything.
        averageWidthOK=$(echo "${NUM_CHAN_SCIENCE}" "${NUM_CHAN_TO_AVERAGE}" | awk '{if (($1 % $2)==0) print "yes"; else print "no"}')
        if [ "${averageWidthOK}" == "no" ]; then
            echo "ERROR! Number of channels (${NUM_CHAN_SCIENCE}) must be an exact multiple of NUM_CHAN_TO_AVERAGE (${NUM_CHAN_TO_AVERAGE}). Exiting."
            exit 1
        fi
        
        # nchanContSci = number of channels after averaging
        nchanContSci=$(echo "${NUM_CHAN_SCIENCE}" "${NUM_CHAN_TO_AVERAGE}" | awk '{print $1/$2}')

        # nworkergroupsSci = number of worker groups, used for MFS imaging.
        nworkergroupsSci=$(echo "${NUM_TAYLOR_TERMS}" | awk '{print 2*$1-1}')

        # if we are using the new imager we need to tweak this
        if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
            NUM_CPUS_CONTIMG_SCI=$(echo "$nchanContSci" "$nworkergroupsSci" "${NCHAN_PER_CORE}" | awk '{print ($1/$3)*$2+1}')
        fi

        # total number of CPUs required for MFS continuum imaging, including the master
        #  Use the number given in the config file, unless it has been left blank
        # Also set the Channels selection parameter
        if [ "${NUM_CPUS_CONTIMG_SCI}" == "" ]; then
            NUM_CPUS_CONTIMG_SCI=$(echo "$nchanContSci" "$nworkergroupsSci" | awk '{print $1*$2+1}')
            CONTIMG_CHANNEL_SELECTION="# Each worker will read a single channel selection
                                                                                          Cimager.Channels                                = [1, %w]"
        else
            if [ "${CHANNEL_SELECTION_CONTIMG_SCI}" == "" ]; then
                CONTIMG_CHANNEL_SELECTION="# No Channels selection performed"
            else
                CONTIMG_CHANNEL_SELECTION="# Channel selection for each worker
Cimager.Channels                                = ${CHANNEL_SELECTION_CONTIMG_SCI}"
            fi
        fi

        # Cannot have -N greater than -n in the srun call
        if [ "${NUM_CPUS_CONTIMG_SCI}" -lt "${CPUS_PER_CORE_CONT_IMAGING}" ]; then
            CPUS_PER_CORE_CONT_IMAGING=${NUM_CPUS_CONTIMG_SCI}
        fi

        # Method used for self-calibration - needs to be one of Cmodel, Components, CleanModel
        if [ "${SELFCAL_METHOD}" != "Cmodel" ] &&
               [ "${SELFCAL_METHOD}" != "Components" ] &&
               [ "${SELFCAL_METHOD}" != "CleanModel" ]; then
            SELFCAL_METHOD="Cmodel"
        fi

        # Select correct gridding parameters, depending on snapshot status
        if [ "${GRIDDER_WMAX}" == "" ]; then
            if [ "${GRIDDER_SNAPSHOT_IMAGING}" == "true" ]; then
                GRIDDER_WMAX=${GRIDDER_WMAX_SNAPSHOT}
            else
                GRIDDER_WMAX=${GRIDDER_WMAX_NO_SNAPSHOT}
            fi
        fi
        if [ "${GRIDDER_MAXSUPPORT}" == "" ]; then
            if [ "${GRIDDER_SNAPSHOT_IMAGING}" == "true" ]; then
                GRIDDER_MAXSUPPORT=${GRIDDER_MAXSUPPORT_SNAPSHOT}
            else
                GRIDDER_MAXSUPPORT=${GRIDDER_MAXSUPPORT_NO_SNAPSHOT}
            fi
        fi
        if [ "${GRIDDER_NWPLANES}" == "" ]; then
            if [ "${GRIDDER_SNAPSHOT_IMAGING}" == "true" ]; then
                GRIDDER_NWPLANES=${GRIDDER_NWPLANES_SNAPSHOT}
            else
                GRIDDER_NWPLANES=${GRIDDER_NWPLANES_NO_SNAPSHOT}
            fi
        fi

        ####################
        # Parameters required for continuum-cube imaging
        ####

        # Check value of IMAGETYPE - needs to be casa or fits
        if [ "${IMAGETYPE_CONTCUBE}" != "casa" ] && [ "${IMAGETYPE_CONTCUBE}" != "fits" ]; then
            echo "ERROR - Invalid image type \"${IMAGETYPE_CONTCUBE}\" - IMAGETYPE_CONTCUBE needs to be casa or fits"
            echo "   Exiting"
            exit 1
        fi
        
        # Switching on the DO_ALT_IMAGER_CONTCUBE flag - if it is not
        # defined in the config file, then set to the value of
        # DO_ALT_IMAGER.
        if [ "${DO_ALT_IMAGER}" == "true" ] && [ "${DO_ALT_IMAGER_CONTCUBE}" == "" ]; then
            DO_ALT_IMAGER_CONTCUBE=${DO_ALT_IMAGER}
        fi

        # simager is not currently able to write out FITS files. So if
        # the user has requested FITS imagetype, but has not set the
        # ALT_IMAGER flag, give a warning and stop to let them fix it.
        if [ "${IMAGETYPE_CONTCUBE}" == "fits" ] && [ "${DO_ALT_IMAGER_CONTCUBE}" != "true" ]; then
            echo "ERROR - IMAGETYPE_CONTCUBE=fits can only work with DO_ALT_IMAGER_CONTCUBE=true"
            echo "   Exiting"
            exit 1
        fi

        # Set the cell size for the continuum cubes - if not provided by the user, set to that given for the continuum imaging
        if [ "${CELLSIZE_CONTCUBE}" == "" ]; then
            CELLSIZE_CONTCUBE=${CELLSIZE_CONT}
        fi

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

        # Set the number of cores for the continuum cube imaging. Either
        # set to the number of averaged channels + 1, or use that given in
        # the config file, limiting to no bigger than this number
        maxContCubeCores=$((nchanContSci + 1))
        # if we are using the new imager we need to tweak this
        if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ]; then
            maxContCubeCores=$(echo "$nchanContSci" "$NCHAN_PER_CORE_CONTCUBE" | awk '{print int($1/$2)+1}')
        fi
        if [ "${NUM_CPUS_CONTCUBE_SCI}" == "" ]; then
            # User has not specified
            NUM_CPUS_CONTCUBE_SCI=$maxContCubeCores
        elif [ "${NUM_CPUS_CONTCUBE_SCI}" -gt "$maxContCubeCores" ]; then
            # Have more cores than we need - reduce number
            echo "NOTE - Reducing NUM_CPUS_CONTCUBE_SCI to $maxContCubeCores to match the number of averaged channels"
            NUM_CPUS_CONTCUBE_SCI=$maxContCubeCores
        fi
        if [ "${CPUS_PER_CORE_CONTCUBE_IMAGING}" -gt 20 ]; then
            CPUS_PER_CORE_CONTCUBE_IMAGING=20
        fi
        if [ "${CPUS_PER_CORE_CONTCUBE_IMAGING}" -gt "${NUM_CPUS_CONTCUBE_SCI}" ]; then
            CPUS_PER_CORE_CONTCUBE_IMAGING=${NUM_CPUS_CONTCUBE_SCI}
        fi


        # Set the number of cores for the continuum cube mosaicking. Either
        # set to the number of averaged channels, or use that given in
        # the config file
        if [ "${NUM_CPUS_CONTCUBE_LINMOS}" == "" ]; then
            # User has not specified
            NUM_CPUS_CONTCUBE_LINMOS=$nchanContSci
        elif [ "${NUM_CPUS_CONTCUBE_LINMOS}" -gt "${NUM_CPUS_CONTCUBE_SCI}" ]; then
            echo "NOTE - Reducing NUM_CPUS_CONTCUBE_LINMOS to $NUM_CPUS_CONTCUBE_SCI to match the number of cores used for imaging"
            NUM_CPUS_CONTCUBE_LINMOS=$NUM_CPUS_CONTCUBE_SCI
        fi

        # Deprecation warning for the specification of
        # NUM_SPECTRAL_CUBES_CONTCUBE - this is now NUM_SPECTRAL_WRITERS_CONTCUBE
        if [ "${NUM_SPECTRAL_CUBES_CONTCUBE}" != "" ]; then
            echo "WARNING - the parameter NUM_SPECTRAL_CUBES_CONTCUBE is deprecated - please use NUM_SPECTRAL_WRITERS_CONTCUBE instead."
            echo "        - Setting NUM_SPECTRAL_WRITERS_CONTCUBE=${NUM_SPECTRAL_CUBES_CONTCUBE}"
            NUM_SPECTRAL_WRITERS_CONTCUBE=${NUM_SPECTRAL_CUBES_CONTCUBE}
        fi

        # if NUM_SPECTRAL_WRITERS_CONTCUBE not given, set to number of workers
        if [ "${NUM_SPECTRAL_WRITERS_CONTCUBE}" == "" ]; then
            NUM_SPECTRAL_WRITERS_CONTCUBE=$(echo ${NUM_CPUS_CONTCUBE_SCI} | awk '{print $1-1}')
        fi
        

        # Define the list of writer ranks used in the askap_imager
        # spectral-line output
        # Only define if we are using the askap_imager and not writing
        # to a single file. Otherwise, we define a single-value list
        # so that the loop over subbands is only done once ($subband
        # will not be referenced in that case).
        if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ] && [ "${ALT_IMAGER_SINGLE_FILE_CONTCUBE}" != "true" ]; then
            nworkers=$nchanContSci
            NUM_SPECTRAL_CUBES_CONTCUBE=${NUM_SPECTRAL_WRITERS_CONTCUBE}
            writerIncrement=$(echo "$nworkers" "${NUM_SPECTRAL_CUBES_CONTCUBE}" | awk '{print $1/$2}')
            SUBBAND_WRITER_LIST_CONTCUBE=$(seq 1 "$writerIncrement" "$nworkers")
            unset nworkers
            unset writerIncrement
        else
            SUBBAND_WRITER_LIST_CONTCUBE=1
            NUM_SPECTRAL_CUBES_CONTCUBE=1
        fi


        ####################
        # Parameters required for spectral-line imaging
        ####

        if [ "${DO_SPECTRAL_IMAGING}" == "true" ]; then
            
            # Check value of IMAGETYPE - needs to be casa or fits
            if [ "${IMAGETYPE_SPECTRAL}" != "casa" ] && [ "${IMAGETYPE_SPECTRAL}" != "fits" ]; then
                echo "ERROR - Invalid image type \"${IMAGETYPE_SPECTRAL}\" - IMAGETYPE_SPECTRAL needs to be casa or fits"
                echo "   Exiting"
                exit 1
            fi
            
            # Switching on the DO_ALT_IMAGER_SPECTRAL flag - if it is not
            # defined in the config file, then set to the value of
            # DO_ALT_IMAGER.  
            if [ "${DO_ALT_IMAGER}" == "true" ] && [ "${DO_ALT_IMAGER_SPECTRAL}" == "" ]; then
                DO_ALT_IMAGER_SPECTRAL=${DO_ALT_IMAGER}
            fi

            # simager is not currently able to write out FITS files. So if
            # the user has requested FITS imagetype, but has not set the
            # ALT_IMAGER flag, give a warning and stop to let them fix it.
            if [ "${IMAGETYPE_SPECTRAL}" == "fits" ] && [ "${DO_ALT_IMAGER_SPECTRAL}" != "true" ]; then
                echo "ERROR - IMAGETYPE_SPECTRAL=fits can only work with DO_ALT_IMAGER_SPECTRAL=true"
                echo "   Exiting"
                exit 1
            fi

            # Select correct gridding parameters, depending on snapshot status
            if [ "${GRIDDER_SPECTRAL_WMAX}" == "" ]; then
                if [ "${GRIDDER_SPECTRAL_SNAPSHOT_IMAGING}" == "true" ]; then
                    GRIDDER_SPECTRAL_WMAX=${GRIDDER_SPECTRAL_WMAX_SNAPSHOT}
                else
                    GRIDDER_SPECTRAL_WMAX=${GRIDDER_SPECTRAL_WMAX_NO_SNAPSHOT}
                fi
            fi
            if [ "${GRIDDER_SPECTRAL_MAXSUPPORT}" == "" ]; then
                if [ "${GRIDDER_SPECTRAL_SNAPSHOT_IMAGING}" == "true" ]; then
                    GRIDDER_SPECTRAL_MAXSUPPORT=${GRIDDER_SPECTRAL_MAXSUPPORT_SNAPSHOT}
                else
                    GRIDDER_SPECTRAL_MAXSUPPORT=${GRIDDER_SPECTRAL_MAXSUPPORT_NO_SNAPSHOT}
                fi
            fi
            
            # Channel range to be used for spectral-line imaging
            # If user has requested a different channel range, and the
            # DO_COPY_SL flag is set, then we define a new spectral-line
            # range of channels
            if [ "${CHAN_RANGE_SL_SCIENCE}" == "" ] || [ "${DO_COPY_SL}" != "true" ]; then
                
                # If this range has not been set, or we are not doing the
                # copy, then use the full channel range
                CHAN_RANGE_SL_SCIENCE="1-$NUM_CHAN_SCIENCE"

            fi
            
            
            # Check that we have not requested invalid channels
            minSLchan=$(echo "${CHAN_RANGE_SL_SCIENCE}" | awk -F'-' '{print $1}')
            maxSLchan=$(echo "${CHAN_RANGE_SL_SCIENCE}" | awk -F'-' '{print $2}')
            if [ "$minSLchan" -lt 0 ] || [ "$minSLchan" -gt "${NUM_CHAN_SCIENCE}" ] ||
                   [ "$maxSLchan" -lt 0 ] || [ "$maxSLchan" -gt "${NUM_CHAN_SCIENCE}" ] ||
                   [ "$minSLchan" -gt "$maxSLchan" ]; then
                echo "ERROR - Invalid selection of CHAN_RANGE_SL_SCIENCE=$CHAN_RANGE_SL_SCIENCE, for total number of channels = $NUM_CHAN_SCIENCE"
                echo "   Exiting."
                exit 1
            fi
            
            # Define the number of channels used in the spectral-line imaging
            NUM_CHAN_SCIENCE_SL=$(echo "${CHAN_RANGE_SL_SCIENCE}" | awk -F'-' '{print $2-$1+1}')
            
            # Set the output frequency frame - must be one of "topo", "bary", "lsrk"
            # If not given, default to "bary"
            if [ "${FREQ_FRAME_SL}" == "" ]; then
                FREQ_FRAME_SL="bary"
            fi
            
            if [ "${FREQ_FRAME_SL}" != "topo" ] ||
                   [ "${FREQ_FRAME_SL}" != "bary" ] ||
                   [ "${FREQ_FRAME_SL}" != "lsrk" ]; then
                echo "WARNING - FREQ_FRAME_SL (${FREQ_FRAME_SL}) is not one of \"topo\" \"bary\" \"lsrk\""
                echo "        - Setting to \"bary\""
                FREQ_FRAME_SL="bary"
            fi

            # Catch deprecated parameter DO_BARY
            if [ "${DO_BARY}" != "" ]; then
                echo "WARNING - the DO_BARY parameter is deprecated. Please use FREQ_FRAME_SL instead"
                if [ "${DO_BARY}" == "true" ]; then
                    echo "        - Setting FREQ_FRAME_SL=bary"
                    FREQ_FRAME_SL=bary
                else
                    echo "        - Setting FREQ_FRAME_SL=topo"
                    FREQ_FRAME_SL=topo
                fi
            fi
            
            # if we are using the new imager we need to tweak the number of cores
            if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ]; then
                if [ $((NUM_CHAN_SCIENCE_SL % NCHAN_PER_CORE_SL)) -ne 0 ]; then
                    echo "ERROR - NCHAN_PER_CORE_SL (${NCHAN_PER_CORE_SL}) does not evenly divide the number of channels (${NUM_CHAN_SCIENCE_SL})"
                    echo "   Exiting."
                    exit 1
                fi
                NUM_CPUS_SPECIMG_SCI=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NCHAN_PER_CORE_SL}" | awk '{print int($1/$2) + 1}')
            fi
            
            # Cannot have -N greater than -n in the srun call
            if [ "${NUM_CPUS_SPECIMG_SCI}" -lt "${CPUS_PER_CORE_SPEC_IMAGING}" ]; then
                CPUS_PER_CORE_SPEC_IMAGING=${NUM_CPUS_SPECIMG_SCI}
            fi
            

            # Deprecation warning for the specification of
            # NUM_SPECTRAL_CUBES - this is now NUM_SPECTRAL_WRITERS
            if [ "${NUM_SPECTRAL_CUBES}" != "" ]; then
                echo "WARNING - the parameter NUM_SPECTRAL_CUBES is deprecated - please use NUM_SPECTRAL_WRITERS instead."
                echo "        - Setting NUM_SPECTRAL_WRITERS=${NUM_SPECTRAL_CUBES}"
                NUM_SPECTRAL_WRITERS=${NUM_SPECTRAL_CUBES}
            fi
            
            # if NUM_SPECTRAL_WRITERS not given, set to number of workers
            if [ "${NUM_SPECTRAL_WRITERS}" == "" ]; then
                NUM_SPECTRAL_WRITERS=$(echo ${NUM_CPUS_SPECIMG_SCI} | awk '{print $1-1}')
            fi
            
            # Reduce the number of writers to no more than the number of workers
            if [ "${NUM_SPECTRAL_WRITERS}" -ge "${NUM_CPUS_SPECIMG_SCI}" ]; then
                NUM_SPECTRAL_WRITERS=$(echo ${NUM_CPUS_SPECIMG_SCI} | awk '{print $1-1}')
                echo "WARNING - Reducing NUM_SPECTRAL_WRITERS to ${NUM_SPECTRAL_WRITERS} to match number of spectral workers"
            fi

            # Method used for continuum subtraction
            if [ "${CONTSUB_METHOD}" != "Cmodel" ] &&
                   [ "${CONTSUB_METHOD}" != "Components" ] &&
                   [ "${CONTSUB_METHOD}" != "CleanModel" ]; then
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
            
            # Script for image-based continuum subtraction
            if [ "${SPECTRAL_IMSUB_SCRIPT}" != "robust_contsub.py" ] &&
                   [ "${SPECTRAL_IMSUB_SCRIPT}" != "contsub_im.py" ]; then
                SPECTRAL_IMSUB_SCRIPT="robust_contsub.py"
            fi
            
            # Define the list of writer ranks used in the askap_imager
            # spectral-line output
            # Only define if we are using the askap_imager and not writing
            # to a single file. Otherwise, we define a single-value list
            # so that the loop over subbands is only done once ($subband
            # will not be referenced in that case).
            if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ] && [ "${ALT_IMAGER_SINGLE_FILE}" != "true" ]; then
                NUM_SPECTRAL_CUBES=${NUM_SPECTRAL_WRITERS}
                nworkers=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NCHAN_PER_CORE_SL}" | awk '{print int($1/$2)}')
                writerIncrement=$(echo "$nworkers" "${NUM_SPECTRAL_CUBES}" | awk '{print int($1/$2)}')
                SUBBAND_WRITER_LIST=$(seq 1 "$writerIncrement" "$nworkers")
                unset nworkers
                unset writerIncrement
            else
                SUBBAND_WRITER_LIST=1
                NUM_SPECTRAL_CUBES=1
            fi

        fi
        
        ####################
        # Mosaicking parameters

        # Check the value of weighttype for the single-field mosaicking job
        if [ "${LINMOS_SINGLE_FIELD_WEIGHTTYPE}" != "Combined" ] &&
               [ "${LINMOS_SINGLE_FIELD_WEIGHTTYPE}" != "FromPrimaryBeamModel" ]; then
            echo "WARNING - LINMOS_SINGLE_FIELD_WEIGHTTYPE can only have value of \"Combined\" or \"FromPrimaryBeamModel\""
            echo "        - Setting to \"Combined\""
            LINMOS_SINGLE_FIELD_WEIGHTTYPE="Combined"
        fi
        
        # Fix the direction string for linmos - do not need the J2000 bit
        linmosFeedCentre=$(echo "${DIRECTION_SCI}" | awk -F',' '{printf "%s,%s]",$1,$2}')

        # Total number of channels should be exact multiple of
        # channels-per-core, else the final process will take care of
        # the rest and may run out of memory
        # If it is not, give a warning and push on
        chanPerCoreLinmosOK=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NUM_SPECTRAL_CUBES}" "${NCHAN_PER_CORE_SPECTRAL_LINMOS}" | awk '{if (($1/$2 % $3)==0) print "yes"; else print "no"}')
        if [ "${chanPerCoreLinmosOK}" == "no" ] && [ "${DO_MOSAIC}" == "true" ]; then
            echo "WARNING - Number of spectral-line channels (${NUM_CHAN_SCIENCE_SL}) is not an exact multiple of NCHAN_PER_CORE_SPECTRAL_LINMOS (${NCHAN_PER_CORE_SPECTRAL_LINMOS})."
            echo "         Pushing on, but there is the risk of memory problems with the spectral linmos task."
        fi
        # Determine the number of cores needed for spectral-line mosaicking
        if [ "$NUM_CPUS_SPECTRAL_LINMOS" == "" ]; then
            NUM_CPUS_SPECTRAL_LINMOS=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NUM_SPECTRAL_CUBES}" "${NCHAN_PER_CORE_SPECTRAL_LINMOS}" | awk '{if($1%($2*$3)==0) print $1/$2/$3; else print int($1/$2/$3)+1;}')
        fi

        ####################
        # Source finding switches

        # First check for the old DO_SOURCE_FINDING.
        #  If present, set new source-finding switches to its value
        #  and give a warning
        if [ "${DO_SOURCE_FINDING}" != "" ]; then
            DO_SOURCE_FINDING_CONT=${DO_SOURCE_FINDING}
            DO_SOURCE_FINDING_SPEC=${DO_SOURCE_FINDING}
            echo "WARNING - Parameter DO_SOURCE_FINDING has been deprecated."
            echo "        - Please use DO_SOURCE_FINDING_CONT and DO_SOURCE_FINDING_SPEC."
            echo "        - For now, these have both been set to ${DO_SOURCE_FINDING}."
        fi

        # If DO_SOURCE_FINDING_CONT has not been set, set it to be the same as the DO_CONT_IMAGING switch
        if [ "${DO_SOURCE_FINDING_CONT}" == "" ]; then
            DO_SOURCE_FINDING_CONT=${DO_CONT_IMAGING}
        fi

        # If DO_SOURCE_FINDING_SPEC has not been set, set it to be the same as the DO_SPECTRAL_IMAGING switch
        if [ "${DO_SOURCE_FINDING_SPEC}" == "" ]; then
            DO_SOURCE_FINDING_SPEC=${DO_SPECTRAL_IMAGING}
        fi

        # If there is only one field, we turn off the source-finding
        # for the individual fields, since that will be a replication
        # of the top-level source-finding.
        if [ ${NUM_FIELDS} -eq 1 ]; then
            if [ "${DO_SOURCE_FINDING_FIELD_MOSAICS}" == "true" ]; then
                echo "WARNING - Only 1 field present, so turning off DO_SOURCE_FINDING_FIELD_MOSAICS"
                DO_SOURCE_FINDING_FIELD_MOSAICS=false
            fi
        fi
        ####################
        # Continuum Source-finding

        # Number of cores:
        NUM_CPUS_SELAVY=$(echo "${SELAVY_NSUBX}" "${SELAVY_NSUBY}" | awk '{print $1*$2+1}')
        CPUS_PER_CORE_SELAVY=${NUM_CPUS_SELAVY}
        if [ "${CPUS_PER_CORE_SELAVY}" -gt 20 ]; then
            CPUS_PER_CORE_SELAVY=20
        fi
        if [ "${CPUS_PER_CORE_SELAVY}" -gt "${NUM_CPUS_SELAVY}" ]; then
            CPUS_PER_CORE_SELAVY=${NUM_CPUS_SELAVY}
        fi

        # If the sourcefinding flag has been set, but we are not
        # mosaicking, turn on the beam-wise sourcefinding flag
        if [ "${DO_SOURCE_FINDING_CONT}" == "true" ] && [ "${DO_MOSAIC}" != "true" ]; then
            DO_SOURCE_FINDING_BEAMWISE=true
        fi

        # Check that contcube imaging is turned on if we want to use them to find the spectral indices
        if [ "${USE_CONTCUBE_FOR_SPECTRAL_INDEX}" == "true" ] &&
               [ "${DO_CONTCUBE_IMAGING}" != "true" ]; then
            DO_CONTCUBE_IMAGING=true
            echo "WARNING - Turning on continuum-cube imaging, since USE_CONTCUBE_FOR_SPECTRAL_INDEX=true";
        fi

        # The cutoff level for the weights. Allow this to be
        # specified, but if it is not then we use the square of the
        # value used in the linmos
        if [ "${SELAVY_WEIGHTS_CUTOFF}" == "" ]; then
            SELAVY_WEIGHTS_CUTOFF=$(echo ${LINMOS_CUTOFF} | awk '{print $1*$1}')
        fi
        
        # We can only do RM Synthesis if we are making I,Q,U continuum
        # cubes. Need to check that the list of polarisations
        # contains these.
        if [ "${CONTCUBE_POLARISATIONS}" == "" ]; then
            if [ "${DO_RM_SYNTHESIS}" == "true" ]; then
                echo "No list of contcube polarisations provided - turning off RM Synthesis"
            fi
            DO_RM_SYNTHESIS=false
        else
            haveI=false
            haveU=false
            haveQ=false
            for p in ${POL_LIST}; do
                if [ "$p" == "I" ]; then haveI=true; fi
                if [ "$p" == "Q" ]; then haveQ=true; fi
                if [ "$p" == "U" ]; then haveU=true; fi
            done
            if [ "$haveI" != "true" ] || [ "$haveQ" != "true" ] || [ "$haveU" != "true" ]; then
                if [ "${DO_RM_SYNTHESIS}" == "true" ]; then
                    echo "Warning - List of polarisations provided (${CONTCUBE_POLARISATIONS}) does not have I,Q,U"
                    echo "        - turning off RM Synthesis"
                fi
                DO_RM_SYNTHESIS=false
            fi
        fi
        

        if [ "${SELAVY_POL_WRITE_FDF}" != "" ]; then
            echo "WARNING - the parameter SELAVY_POL_WRITE_FDF is deprecated"
            echo "        - use SELAVY_POL_WRITE_COMPLEX_FDF instead (it has been set to ${SELAVY_POL_WRITE_FDF})"
            SELAVY_POL_WRITE_COMPLEX_FDF=${SELAVY_POL_WRITE_FDF}
        fi


        ####################
        # Spectral source-finding

        # Deprecated parameters
        if [ "${SELAVY_SPEC_BASE_SPECTRUM}" ]; then
            echo "NOTE - SELAVY_SPEC_BASE_SPECTRUM is no longer used"
        fi
        if [ "${SELAVY_SPEC_BASE_NOISE}" ]; then
            echo "NOTE - SELAVY_SPEC_BASE_NOISE is no longer used"
        fi
        if [ "${SELAVY_SPEC_BASE_MOMENT}" ]; then
            echo "NOTE - SELAVY_SPEC_BASE_MOMENT is no longer used"
        fi
        if [ "${SELAVY_SPEC_BASE_CUBELET}" ]; then
            echo "NOTE - SELAVY_SPEC_BASE_CUBELET is no longer used"
        fi
        
        # Number of cores etc
        NUM_CPUS_SELAVY_SPEC=$(echo "${SELAVY_SPEC_NSUBX}" "${SELAVY_SPEC_NSUBY}" "${SELAVY_SPEC_NSUBZ}" | awk '{print $1*$2*$3+1}')
        if [ "${CPUS_PER_CORE_SELAVY_SPEC}" == "" ]; then
            CPUS_PER_CORE_SELAVY_SPEC=${NUM_CPUS_SELAVY_SPEC}
        fi
        if [ "${CPUS_PER_CORE_SELAVY_SPEC}" -gt 20 ]; then
            CPUS_PER_CORE_SELAVY_SPEC=20
        fi
        if [ "${CPUS_PER_CORE_SELAVY_SPEC}" -gt "${NUM_CPUS_SELAVY_SPEC}" ]; then
            CPUS_PER_CORE_SELAVY_SPEC=${NUM_CPUS_SELAVY_SPEC}
        fi

        # The cutoff level for the weights. Allow this to be
        # specified, but if it is not then we use the square of the
        # value used in the linmos
        if [ "${SELAVY_SPEC_WEIGHTS_CUTOFF}" == "" ]; then
            SELAVY_SPEC_WEIGHTS_CUTOFF=$(echo ${LINMOS_CUTOFF} | awk '{print $1*$1}')
        fi
        
        # Check search type
        if [ "${SELAVY_SPEC_SEARCH_TYPE}" != "spectral" ] &&
               [ "${SELAVY_SPEC_SEARCH_TYPE}" != "spatial" ]; then
            SELAVY_SPEC_SEARCH_TYPE="spectral"
            echo "WARNING - SELAVY_SPEC_SEARCH_TYPE needs to be 'spectral' or 'spatial' - Setting to 'spectral'"
        fi

        # Check smooth type
        if [ "${SELAVY_SPEC_FLAG_SMOOTH}" == "true" ]; then
            if [ "${SELAVY_SPEC_SMOOTH_TYPE}" != "spectral" ] &&
                   [ "${SELAVY_SPEC_SMOOTH_TYPE}" != "spatial" ]; then
                SELAVY_SPEC_SMOOTH_TYPE="spectral"
                echo "WARNING - SELAVY_SPEC_SMOOTH_TYPE needs to be 'spectral' or 'spatial' - Setting to 'spectral'"
            fi
        fi

        # Spatial smoothing kernel - interpret to form selavy parameters
        if [ "${SELAVY_SPEC_FLAG_SMOOTH}" == "true" ] &&
               [ "${SELAVY_SPEC_SMOOTH_TYPE}" == "spatial" ]; then
            kernelArray=()
            for a in $(echo "${SELAVY_SPEC_SPATIAL_KERNEL}" | sed -e 's/[][,]/ /g'); do
                kernelArray+=($a)
            done
            if [ "${#kernelArray[@]}" -eq 1 ]; then
                SELAVY_SPEC_KERN_MAJ=${kernelArray[0]}
                SELAVY_SPEC_KERN_MIN=${kernelArray[0]}
                SELAVY_SPEC_KERN_PA=0
            elif [ "${#kernelArray[@]}" -eq 3 ]; then
                SELAVY_SPEC_KERN_MAJ=${kernelArray[0]}
                SELAVY_SPEC_KERN_MIN=${kernelArray[1]}
                SELAVY_SPEC_KERN_PA=${kernelArray[2]}
            else
                echo "WARNING - badly formed parameter SELAVY_SPEC_SPATIAL_KERNEL=${SELAVY_SPEC_SPATIAL_KERNEL}"
                echo "        - turning off the spatial smoothing in Selavy."
                SELAVY_SPEC_FLAG_SMOOTH=false
            fi
        fi


        ####################
        # Variable inputs to Continuum imaging and Self-calibration settings
        #  This section takes the provided parameters and creates
        #  arrays that have a (potentially) different value for each
        #  loop of the self-cal operation.
        #  Parameters covered are the selfcal interval, the
        #  source-finding threshold, and whether normalise gains is on
        #  or not
        if [ "${DO_CONT_IMAGING}" == "true" ]; then

            expectedArrSize=$((SELFCAL_NUM_LOOPS + 1))

            if [ "${DO_SELFCAL}" == "true" ]; then

                if [ "$(echo "${SELFCAL_INTERVAL}" | grep "\[")" != "" ]; then
                    # Have entered a comma-separate array in square brackets
                    arrSize=$(echo "${SELFCAL_INTERVAL}" | sed -e 's/[][,]/ /g' | wc -w)
                    if [ "$arrSize" -ne "$expectedArrSize" ]; then
                        echo "ERROR - SELFCAL_INTERVAL ($SELFCAL_INTERVAL) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                        exit 1
                    fi
                    SELFCAL_INTERVAL_ARRAY=()
                    for a in $(echo "$SELFCAL_INTERVAL" | sed -e 's/[][,]/ /g'); do
                        SELFCAL_INTERVAL_ARRAY+=($a)
                    done
                else
                    SELFCAL_INTERVAL_ARRAY=()
                    for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                        SELFCAL_INTERVAL_ARRAY+=($SELFCAL_INTERVAL)
                    done
                fi

                if [ "$(echo "${SELFCAL_SELAVY_THRESHOLD}" | grep "\[")" != "" ]; then
                    # Have entered a comma-separate array in square brackets
                    arrSize=$(echo "${SELFCAL_SELAVY_THRESHOLD}" | sed -e 's/[][,]/ /g' | wc -w)
                    if [ "$arrSize" -ne "$expectedArrSize" ]; then
                        echo "ERROR - SELFCAL_SELAVY_THRESHOLD ($SELFCAL_SELAVY_THRESHOLD) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                        exit 1
                    fi
                    SELFCAL_SELAVY_THRESHOLD_ARRAY=()
                    for a in $(echo "${SELFCAL_SELAVY_THRESHOLD}" | sed -e 's/[][,]/ /g'); do
                        SELFCAL_SELAVY_THRESHOLD_ARRAY+=($a)
                    done
                else
                    SELFCAL_SELAVY_THRESHOLD_ARRAY=()
                    for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                        SELFCAL_SELAVY_THRESHOLD_ARRAY+=($SELFCAL_SELAVY_THRESHOLD)
                    done
                fi

                if [ "$(echo "${SELFCAL_NORMALISE_GAINS}" | grep "\[")" != "" ]; then
                    # Have entered a comma-separate array in square brackets
                    arrSize=$(echo "${SELFCAL_NORMALISE_GAINS}" | sed -e 's/[][,]/ /g' | wc -w)
                    if [ "$arrSize" -ne "$expectedArrSize" ]; then
                        echo "ERROR - SELFCAL_NORMALISE_GAINS ($SELFCAL_NORMALISE_GAINS) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                        exit 1
                    fi
                    SELFCAL_NORMALISE_GAINS_ARRAY=()
                    for a in $(echo "${SELFCAL_NORMALISE_GAINS}" | sed -e 's/[][,]/ /g'); do
                        SELFCAL_NORMALISE_GAINS_ARRAY+=($a)
                    done
                else
                    SELFCAL_NORMALISE_GAINS_ARRAY=()
                    for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                        SELFCAL_NORMALISE_GAINS_ARRAY+=($SELFCAL_NORMALISE_GAINS)
                    done
                fi

            fi

            if [ "$(echo "${CLEAN_NUM_MAJORCYCLES}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_NUM_MAJORCYCLES}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_NUM_MAJORCYCLES ($CLEAN_NUM_MAJORCYCLES) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_NUM_MAJORCYCLES_ARRAY=()
                for a in $(echo "${CLEAN_NUM_MAJORCYCLES}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_NUM_MAJORCYCLES_ARRAY+=($a)
                done
            else
                CLEAN_NUM_MAJORCYCLES_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_NUM_MAJORCYCLES_ARRAY+=($CLEAN_NUM_MAJORCYCLES)
                done
            fi

            if [ "$(echo "${CLEAN_THRESHOLD_MAJORCYCLE}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_THRESHOLD_MAJORCYCLE}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_THRESHOLD_MAJORCYCLE ($CLEAN_THRESHOLD_MAJORCYCLE) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_THRESHOLD_MAJORCYCLE_ARRAY=()
                for a in $(echo "${CLEAN_THRESHOLD_MAJORCYCLE}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_THRESHOLD_MAJORCYCLE_ARRAY+=($a)
                done
            else
                CLEAN_THRESHOLD_MAJORCYCLE_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_THRESHOLD_MAJORCYCLE_ARRAY+=($CLEAN_THRESHOLD_MAJORCYCLE)
                done
            fi

            if [ "$(echo "${CIMAGER_MINUV}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CIMAGER_MINUV}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CIMAGER_MINUV ($CIMAGER_MINUV) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CIMAGER_MINUV_ARRAY=()
                for a in $(echo "${CIMAGER_MINUV}" | sed -e 's/[][,]/ /g'); do
                    CIMAGER_MINUV_ARRAY+=($a)
                done
            else
                CIMAGER_MINUV_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CIMAGER_MINUV_ARRAY+=($CIMAGER_MINUV)
                done
            fi

            if [ "$(echo "${CIMAGER_MAXUV}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CIMAGER_MAXUV}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CIMAGER_MAXUV ($CIMAGER_MAXUV) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CIMAGER_MAXUV_ARRAY=()
                for a in $(echo "${CIMAGER_MAXUV}" | sed -e 's/[][,]/ /g'); do
                    CIMAGER_MAXUV_ARRAY+=($a)
                done
            else
                CIMAGER_MAXUV_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CIMAGER_MAXUV_ARRAY+=($CIMAGER_MAXUV)
                done
            fi

            if [ "$(echo "${CCALIBRATOR_MINUV}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CCALIBRATOR_MINUV}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CCALIBRATOR_MINUV ($CCALIBRATOR_MINUV) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CCALIBRATOR_MINUV_ARRAY=()
                for a in $(echo "${CCALIBRATOR_MINUV}" | sed -e 's/[][,]/ /g'); do
                    CCALIBRATOR_MINUV_ARRAY+=($a)
                done
            else
                CCALIBRATOR_MINUV_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CCALIBRATOR_MINUV_ARRAY+=($CCALIBRATOR_MINUV)
                done
            fi

            if [ "$(echo "${CCALIBRATOR_MAXUV}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CCALIBRATOR_MAXUV}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CCALIBRATOR_MAXUV ($CCALIBRATOR_MAXUV) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CCALIBRATOR_MAXUV_ARRAY=()
                for a in $(echo "${CCALIBRATOR_MAXUV}" | sed -e 's/[][,]/ /g'); do
                    CCALIBRATOR_MAXUV_ARRAY+=($a)
                done
            else
                CCALIBRATOR_MAXUV_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CCALIBRATOR_MAXUV_ARRAY+=($CCALIBRATOR_MAXUV)
                done
            fi
	    # 1. CLEAN_ALGORITHM 
	    # expected input: CLEAN_ALGORITHM="[Hogbom,BasisFunctionMFS]"
            if [ "$(echo "${CLEAN_ALGORITHM}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_ALGORITHM}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_ALGORITHM ($CLEAN_ALGORITHM) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_ALGORITHM_ARRAY=()
                for a in $(echo "${CLEAN_ALGORITHM}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_ALGORITHM_ARRAY+=($a)
                done
            else
                CLEAN_ALGORITHM_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_ALGORITHM_ARRAY+=($CLEAN_ALGORITHM)
                done
            fi

	    # 2. CLEAN_MINORCYCLE_NITER  
	    # expected input: CLEAN_MINORCYCLE_NITER="[200,800]"
            if [ "$(echo "${CLEAN_MINORCYCLE_NITER}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_MINORCYCLE_NITER}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_MINORCYCLE_NITER ($CLEAN_MINORCYCLE_NITER) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_MINORCYCLE_NITER_ARRAY=()
                for a in $(echo "${CLEAN_MINORCYCLE_NITER}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_MINORCYCLE_NITER_ARRAY+=($a)
                done
            else
                CLEAN_MINORCYCLE_NITER_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_MINORCYCLE_NITER_ARRAY+=($CLEAN_MINORCYCLE_NITER)
                done
            fi
	    # 3. CLEAN_GAIN
	    # expected input: CLEAN_GAIN="[0.1,0.2]"
            if [ "$(echo "${CLEAN_GAIN}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_GAIN}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_GAIN ($CLEAN_GAIN) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_GAIN_ARRAY=()
                for a in $(echo "${CLEAN_GAIN}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_GAIN_ARRAY+=($a)
                done
            else
                CLEAN_GAIN_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_GAIN_ARRAY+=($CLEAN_GAIN)
                done
            fi
	    # 4. CLEAN_PSFWIDTH 
	    # expected input: CLEAN_PSFWIDTH="[256,6144]"
            if [ "$(echo "${CLEAN_PSFWIDTH}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_PSFWIDTH}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_PSFWIDTH ($CLEAN_PSFWIDTH) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_PSFWIDTH_ARRAY=()
                for a in $(echo "${CLEAN_PSFWIDTH}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_PSFWIDTH_ARRAY+=($a)
                done
            else
                CLEAN_PSFWIDTH_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_PSFWIDTH_ARRAY+=($CLEAN_PSFWIDTH)
                done
            fi
	    # 5. CLEAN_SCALES 
	    IFS=';'
	    # expected input: CLEAN_SCALES="[0] ; [0,3,10] ; [0,3,10,480,960]"
	    # Note the "space" characters around ";" are crucial when you want to specify 
	    # an array of scales. This scheme is backward compatible with older pipeline 
	    # versions where one could not use different scales for different selfcal loops.
	    declare -a CLEAN_SCALES_ARRAY=(${CLEAN_SCALES})
	    unset IFS
	    arrSize=${#CLEAN_SCALES_ARRAY[@]}
	    if [ "$arrSize" -le "0" ]; then 
		echo "ERROR - CLEAN_SCALES_ARRAY ($CLEAN_SCALES_ARRAY) needs to be of size > 0"
		exit 1
	    fi
            if [ "$arrSize" -eq 1 ]; then
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_SCALES_ARRAY[$i]=${CLEAN_SCALES_ARRAY[0]}
                done
	    elif [ "$arrSize" -ne "$expectedArrSize" ]; then 
                echo "ERROR - CLEAN_SCALES ($CLEAN_SCALES) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                exit 1
            fi
	    # 6. CLEAN_THRESHOLD_MINORCYCLE 
	    # expected input: CLEAN_THRESHOLD_MINORCYCLE="[45%,2mJy] ; [25%,1mJy,0.03mJy]"
	    IFS=';'
	    declare -a CLEAN_THRESHOLD_MINORCYCLE_ARRAY=(${CLEAN_THRESHOLD_MINORCYCLE})
	    arrSize=${#CLEAN_THRESHOLD_MINORCYCLE_ARRAY[@]}
	    unset IFS
	    if [ "$arrSize" -le "0" ]; then 
		echo "ERROR - CLEAN_THRESHOLD_MINORCYCLE_ARRAY ($CLEAN_THRESHOLD_MINORCYCLE_ARRAY) needs to be of size > 0"
		exit 1
	    fi
	    if [ "$arrSize" -eq 1 ]; then
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_THRESHOLD_MINORCYCLE_ARRAY[$i]=${CLEAN_THRESHOLD_MINORCYCLE_ARRAY[0]}
                done
	    elif [ "$arrSize" -lt "$expectedArrSize" ]; then 
                echo "ERROR - CLEAN_THRESHOLD_MINORCYCLE ($CLEAN_THRESHOLD_MINORCYCLE) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                exit 1
            fi

	    #+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

            # Validate that all these arrays are the same length as
            # SELFCAL_NUM_LOOPS, as long as the latter is >0
            if [ "${SELFCAL_NUM_LOOPS}" -gt 0 ]; then
                arraySize=$((SELFCAL_NUM_LOOPS + 1))

                if [ "${DO_SELFCAL}" == "true" ]; then
                    if [ "${#SELFCAL_INTERVAL_ARRAY[@]}" -ne "$arraySize" ]; then
                        echo "ERROR! Size of SELFCAL_INTERVAL (${SELFCAL_INTERVAL}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                        exit 1
                    fi
                    if [ "${#SELFCAL_SELAVY_THRESHOLD_ARRAY[@]}" -ne "$arraySize" ]; then
                        echo "ERROR! Size of SELFCAL_SELAVY_THRESHOLD (${SELFCAL_SELAVY_THRESHOLD}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                        exit 1
                    fi
                    if [ "${#SELFCAL_NORMALISE_GAINS_ARRAY[@]}" -ne "$arraySize" ]; then
                        echo "ERROR! Size of SELFCAL_NORMALISE_GAINS (${SELFCAL_NORMALISE_GAINS}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                        exit 1
                    fi
                fi

                if [ "${#CLEAN_NUM_MAJORCYCLES_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_NUM_MAJORCYCLES (${CLEAN_NUM_MAJORCYCLES}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_THRESHOLD_MAJORCYCLE_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_THRESHOLD_MAJORCYCLE (${CLEAN_THRESHOLD_MAJORCYCLE}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CIMAGER_MINUV_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CIMAGER_MINUV (${CIMAGER_MINUV}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CCALIBRATOR_MINUV_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CCALIBRATOR_MINUV (${CCALIBRATOR_MINUV}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CIMAGER_MAXUV_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CIMAGER_MAXUV (${CIMAGER_MAXUV}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CCALIBRATOR_MAXUV_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CCALIBRATOR_MAXUV (${CCALIBRATOR_MAXUV}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_ALGORITHM_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_ALGORITHM (${CLEAN_ALGORITHM}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_MINORCYCLE_NITER_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_MINORCYCLE_NITER (${CLEAN_MINORCYCLE_NITER}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_GAIN_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_GAIN (${CLEAN_GAIN}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_PSFWIDTH_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_PSFWIDTH (${CLEAN_PSFWIDTH}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_SCALES_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_SCALES (${CLEAN_SCALES}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_THRESHOLD_MINORCYCLE_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_THRESHOLD_MINORCYCLE (${CLEAN_THRESHOLD_MINORCYCLE}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi

            fi

        fi

    fi
    
fi

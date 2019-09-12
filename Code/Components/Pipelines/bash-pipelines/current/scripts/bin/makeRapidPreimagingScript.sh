#!/bin/bash -l
#
# Creates a script that runs the pre-imaging tasks in a single
# go. This is particularly useful for rapid surveys where individual
# datasets are small and so don't need separate slurm jobs for each
# task.
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

if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then
    echo "The rapid processing mode assumes no time splitting."
    exit 1
fi

scriptName=${tools}/rapidPreimagingScript-${NOW}.sh
cat > $scriptName <<EOFOUTER
#!/bin/bash -l
#
# Runs a number of pre-imaging tasks for a given Field/Beam
# combination, where both field and beam are given as arguments.
#
# The assumptions are that the datasets are small (it is designed for
# "rapid survey" observations, where short observations are used to
# cover a wide area). In particular, the raw data is not split into
# spectral chunks - there is either a single raw MS or one MS per beam.
# 
# This is designed to be run either in a stand-alone way or called
# from an imaging job, where it is run prior to the imaging.
#
# Note that srun is *not* used to execute any of the tasks. It is
# assumed this script is called with srun if it is needed to be run
# within a slurm job.
#
# The tasks performed are:
#   - Splitting of the field/beam data into a local MS
#   - Application of the bandpass calibration
#   - Flagging of the dataset
#   - Averaging to continuum resolution
#   - Flagging of the averaged dataset
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

USAGE="\$1 - run pre-imaging tasks for a given beam/field combination.
Arguments:
  -b BEAM - which beam number (00, 01, 02, etc)
  -f FIELDID - numerical ID for the field"

while getopts 'b:f:' opt
do
    case \$opt in
	b) BEAM=\$OPTARG;;
        f) FIELD_ID=\$OPTARG;;
        h) echo "Usage: \$USAGE"
           exit 0;;
	\?) echo "ERROR: Invalid option: \$USAGE"
	    exit 1;;
    esac
done

echo "Running rapid pre-imaging script with BEAM=\$BEAM and FIELD_ID=\$FIELD_ID"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
OUTPUT=${OUTPUT}
cd \$OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Define the field name based on the field ID
FIELD_LIST="${FIELD_LIST}"
ID=0
IFS="${IFS_FIELDS}"
for FIELD in \${FIELD_LIST}; do
    if [ \$ID -eq \$FIELD_ID ]; then
        break;
    fi
    ((ID++))
done
IFS="${IFS_DEFAULT}"
# Have defined FIELD to match FIELD_ID
echo "FIELD_ID \${FIELD_ID} equates to FIELD \${FIELD}"

FIELDBEAM=\$(echo "\$FIELD_ID" "\$BEAM" | awk '{printf "F%02d_B%s",\$1,\$2}')

# Input parameters set at pipeline start
MS_INPUT_SCIENCE="${MS_INPUT_SCIENCE}"
MS_BASE_SCIENCE=${MS_BASE_SCIENCE}
MS_SCIENCE_AVERAGE="${MS_SCIENCE_AVERAGE}"
SB_SCIENCE=${SB_SCIENCE}
DO_SPLIT_TIMEWISE=${DO_SPLIT_TIMEWISE}
DO_COPY_SL=${DO_COPY_SL}
KEEP_RAW_AV_MS=${KEEP_RAW_AV_MS}
CHAN_RANGE_SCIENCE="${CHAN_RANGE_SCIENCE}"
NUM_CHAN_SCIENCE="${NUM_CHAN_SCIENCE}"
BUCKET_SIZE="${BUCKET_SIZE}"
TILE_NCHAN_SCIENCE="${TILE_NCHAN_SCIENCE}"

# set MS names and Check files
findScienceMSnames
setCheckfiles

############################
# Split MS - assume no time-splitting or channel selection, just by field
# This means that there is either 1 MS or 36 (one for each beam).
msnamesSci="${msnamesSci}"
numMSsci="${numMSsci}"

if [ \$numMSsci -eq 1 ]; then
    inputMS=\${MS_INPUT_SCIENCE}
else
    msroot=\$(for ms in \${msnamesSci}; do echo \$ms; done | sed -e 's/[0-9]*[0-9].ms//g' | uniq)
    inputMS=\$(echo \$msroot \$BEAM | awk '{printf "%s%d.ms\n",\$1,\$2}')
fi

if [ "\${CHAN_RANGE_SCIENCE}" == "" ]; then
    channelParam="channel     = 1-\${NUM_CHAN_SCIENCE}"
else
    channelParam="channel     = \${CHAN_RANGE_SCIENCE}"
fi

if [ ! -e "\${msSci}" ]; then
    echo "Splitting from \$inputMS to form \$msSci"
    parset="${parsets}/split_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.in"
    cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = \${inputMS}

# Output measurement set
# Default: <no default>
outputvis   = \${msSci}

# Beam selection via beam ID
# Select an individual beam
beams        = [\${BEAM}]

# Field selection for the science observation
# Select only the current field
fieldnames   = "\${FIELD}"

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
\$channelParam

# Set a larger bucketsize
stman.bucketsize  = \${BUCKET_SIZE}
# Make the tile size 54 channels, as that is what we will average over
stman.tilenchan   = \${TILE_NCHAN_SCIENCE}
EOFINNER

    log="${logs}/split_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
    echo "Splitting log file is \$log"
    
    NCORES=1
    NPPN=1
    ${mssplit} -c "\${parset}" > "\${log}"
    err=\$?
    rejuvenate \${msSci}
    extractStats "\${log}" "\${NCORES}" "\${SLURM_JOB_ID}" "\${err}" "split" "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi
    echo " "
fi

###########################
# Apply bandpass

DO_IT=$DO_APPLY_BANDPASS
if [ "\${DO_IT}" == true ] && [ ! -e "\${BANDPASS_CHECK_FILE}" ]; then

    RAW_TABLE=${TABLE_BANDPASS}
    SMOOTH_TABLE=${TABLE_BANDPASS}.smooth
    DO_SMOOTH=${DO_BANDPASS_SMOOTH}
    if [ \${DO_SMOOTH} ] && [ -e \${SMOOTH_TABLE} ]; then
        TABLE=\${SMOOTH_TABLE}
    else
        TABLE=\${RAW_TABLE}
    fi

    echo "Applying bandpass table \$TABLE"

    # Determine the number of channels from the MS metadata file (generated by mslist)
    msMetadata="${MS_METADATA}"
    #nChan=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\${msMetadata}" --val=nChan)
    nChan=${NUM_CHAN_SCIENCE}

    parset=${parsets}/ccalapply_bp_\${FIELDBEAM}_\${SLURM_JOB_ID}.in
    cat > "\$parset" << EOFINNER
Ccalapply.dataset                         = \${msSci}
#
# Allow flagging of vis if inversion of Mueller matrix fails
Ccalapply.calibrate.allowflag             = true
Ccalapply.calibrate.scalenoise            = ${BANDPASS_SCALENOISE}
# Ignore the leakage
Ccalapply.calibrate.ignoreleakage         = true
#
Ccalapply.calibaccess                     = table
Ccalapply.calibaccess.table.maxant        = ${NUM_ANT}
Ccalapply.calibaccess.table.maxbeam       = ${maxbeam}
Ccalapply.calibaccess.table.maxchan       = \${nChan}
Ccalapply.calibaccess.table               = \${TABLE}

EOFINNER
    log=${logs}/ccalapply_bp_\${FIELDBEAM}_\${SLURM_JOB_ID}.log
    echo "Bandpass application log file is \$log"

    NCORES=1
    NPPN=1
    ${ccalapply} -c "\$parset" > "\$log"
    err=\$?
    rejuvenate \${msSci}
    rejuvenate \${TABLE}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "applyBP" "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch "\$BANDPASS_CHECK_FILE"
    fi
    echo " "

fi


###########################
# Flag full dataset

DO_IT=$DO_APPLY_BANDPASS
if [ "\${DO_IT}" == true ] && [ ! -e "\${FLAG_CHECK_FILE}" ]; then

    echo "Flagging the full dataset"

    # Input parameters
    ANTENNA_FLAG_SCIENCE="${ANTENNA_FLAG_SCIENCE}"
    CHANNEL_FLAG_SCIENCE="${CHANNEL_FLAG_SCIENCE}"
    TIME_FLAG_SCIENCE="${TIME_FLAG_SCIENCE}"
    FLAG_AUTOCORRELATION_SCIENCE="${FLAG_AUTOCORRELATION_SCIENCE}"
    FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW="${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}"
    FLAG_DO_FLAT_AMPLITUDE_SCIENCE="${FLAG_DO_FLAT_AMPLITUDE_SCIENCE}"
    FLAG_THRESHOLD_AMPLITUDE_SCIENCE="${FLAG_THRESHOLD_AMPLITUDE_SCIENCE}"
    ELEVATION_FLAG_SCIENCE_LOW="${ELEVATION_FLAG_SCIENCE_LOW}"
    ELEVATION_FLAG_SCIENCE_HIGH="${ELEVATION_FLAG_SCIENCE_HIGH}"
    USE_AOFLAGGER="${FLAG_SCIENCE_WITH_AOFLAGGER}"
    USE_PREFLAGS="${DO_PREFLAG_SCIENCE}"
    DO_DYNAMIC=${FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE}
    DO_STOKESV=${FLAG_DO_STOKESV_SCIENCE}
    
    DO_AMP_FLAG=false
    ruleList=""
    
    # Rule 1 relates to antenna-based flagging
    if [ "\${ANTENNA_FLAG_SCIENCE}" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rule1.antenna   = \${ANTENNA_FLAG_SCIENCE}"
        if [ "\${ruleList}" == "" ]; then
            ruleList="rule1"
        else
            ruleList="\${ruleList},rule1"
        fi
        DO_AMP_FLAG=true
    fi
    
        # Rule 2 relates to channel range flagging
    if [ "\${CHANNEL_FLAG_SCIENCE}" == "" ]; then
        channelFlagging="# Not flagging any specific channel range"
    else
        channelFlagging="# The following flags out specific channels:
Cflag.selection_flagger.rule2.spw = \${CHANNEL_FLAG_SCIENCE}"
        if [ "\${ruleList}" == "" ]; then
            ruleList="rule2"
        else
            ruleList="\${ruleList},rule2"
        fi
        DO_AMP_FLAG=true
    fi
    
    # Rule 3 relates to time range flagging
    if [ "\${TIME_FLAG_SCIENCE}" == "" ]; then
        timeFlagging="# Not flagging any specific time range"
    else
        timeFlagging="# The following flags out specific time range(s):
Cflag.selection_flagger.rule3.timerange = \${TIME_FLAG_SCIENCE}"
        if [ "\${ruleList}" == "" ]; then
            ruleList="rule3"
        else
            ruleList="\${ruleList},rule3"
        fi
        DO_AMP_FLAG=true
    fi
    
        # Rule 4 relates to flagging of autocorrelations
    if [ "\${FLAG_AUTOCORRELATION_SCIENCE}" == "true" ]; then
        autocorrFlagging="# The following flags out the autocorrelations, if set to true:
Cflag.selection_flagger.rule4.autocorr  = \${FLAG_AUTOCORRELATION_SCIENCE}"
        if [ "\${ruleList}" == "" ]; then
            ruleList="rule4"
        else
            ruleList="\${ruleList},rule4"
        fi
        DO_AMP_FLAG=true
    else
        autocorrFlagging="# No flagging of autocorrelations"
    fi
    
    # Define a lower bound to the amplitudes, for use in either
    # flagging task
    amplitudeLow="# No absolute lower bound to visibility amplitudes"
    if [ "\${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}" != "" ]; then
        # Only add the low threshold if it has been given
        amplitudeLow="Cflag.amplitude_flagger.low             = \${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}"
    fi
    
    # The flat amplitude cut to be applied
    if [ "\${FLAG_DO_FLAT_AMPLITUDE_SCIENCE}" == "true" ]; then
        amplitudeCut="# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = \${FLAG_THRESHOLD_AMPLITUDE_SCIENCE}
${amplitudeLow}"
        DO_AMP_FLAG=true
    else
        amplitudeCut="# No flat amplitude flagging applied"
    fi
    # Define a lower limit for Elevation (deg) below which all vis are to be flagged:
    elevationLowFlagging="# No flagging based on lower bound for Elevation"
    elevationFlaggerEnable=false
    if [ "\${ELEVATION_FLAG_SCIENCE_LOW}" != "" ]; then
        # Only add the low Elevation limit if it has been given
        elevationLowFlagging="Cflag.elevation_flagger.low       =  \${ELEVATION_FLAG_SCIENCE_LOW}"
        elevationFlaggerEnable=true
    fi
    # Define an upper limit for Elevation (deg) above which all vis are to be flagged:
    elevationHighFlagging="# No flagging based on upper bound for Elevation"
    if [ "\${ELEVATION_FLAG_SCIENCE_HIGH}" != "" ]; then
        # Only add the upper Elevation limit if it has been given
        elevationHighFlagging="Cflag.elevation_flagger.high       =  \${ELEVATION_FLAG_SCIENCE_HIGH}"
        elevationFlaggerEnable=true
    fi
    
    
    if [ "\${USE_AOFLAGGER}" == "true" ]; then
    
        echo "Flagging data with aoflagger"
    
        AOFLAGGER_OPTIONS_SCIENCE="${AOFLAGGER_OPTIONS}"
        AOFLAGGER_STRATEGY_SCIENCE="${AOFLAGGER_STRATEGY_SCIENCE}"
        if [ "${AOFLAGGER_STRATEGY_SCIENCE}" != "" ]; then
            AOFLAGGER_OPTIONS_SCIENCE="\${AOFLAGGER_OPTIONS_SCIENCE} -strategy \"${AOFLAGGER_STRATEGY_SCIENCE}\""
        fi
    
        log="${logs}/aoflag_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
        echo "AOflagger log file is \$log"
    
        # Need to use the gnu PrgEnv, not the cray, to be able to load aoflagger
        if [ "\$(module list -t 2>&1 | grep PrgEnv-cray)" != "" ]; then
          module swap PrgEnv-cray PrgEnv-gnu
        fi
        loadModule aoflagger
        NCORES=1
        NPPN=1
        aoflagger \${AOFLAGGER_OPTIONS_SCIENCE} \${msSci} > "\${log}"
        err=\$?
        unloadModule aoflagger
        rejuvenate \${msSci}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "flag_aoflag" "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "\$FLAG_CHECK_FILE"
        fi
    
    else
    
        echo "Flagging data with cflag"
    
        if [ "\${USE_PREFLAGS}" == "true" ]; then
            loadModule bptool
            NCORES=1
            NPPN=1
            tmpstr=\$(flagInfo_bp.py -t \${TABLE} -npol 2 -b \${BEAM})
            badAnteList=\$(echo \${tmpstr} |awk '{print \$2}')
            addRule1=\$(echo \${tmpstr} |awk '{print \$1}')
            if [ "\${badAnteList}" == "" ]; then 
                addRule1=""
                antennaPreFlagging="# Not pre-flagging any antennas "
            else
        	if [ "\${addRule1}" == "" ];then 
        	    addRule1="BADANTE_B\${BEAM}"
        	fi
                antennaPreFlagging="# The following flags out the bad antennas using Pre-flags:
Cflag.selection_flagger.\${addRule1}.antenna   = \${badAnteList}"
                if [ "\${ruleList}" == "" ]; then
                    ruleList="\${addRule1}"
                else
                    ruleList="\${ruleList},\${addRule1}"
                fi
    	        DO_AMP_FLAG=true
            fi
            unloadModule bptool
        fi

        if [ "\${ruleList}" == "" ]; then
            selectionRule="# No selection rules used"
        else
            selectionRule="Cflag.selection_flagger.rules           = [\${ruleList}]"
        fi
    
        if [ "\${DO_AMP_FLAG}" == "true" ]; then
    
            parset="${parsets}/cflag_amp_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.in"
            cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = \${msSci}

\${amplitudeCut}

\${selectionRule}

\${antennaFlagging}
\${antennaPreFlagging}

\${channelFlagging}

\${timeFlagging}

\${autocorrFlagging}
Cflag.elevation_flagger.enable          = \${elevationFlaggerEnable}
\${elevationLowFlagging}
\${elevationHighFlagging}
EOFINNER

            log="${logs}/cflag_amp_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
            echo "Cflag log for the amplitude flagger file is \$log"

            ${cflag} -c "\${parset}" > "\${log}"
            err=\$?
            rejuvenate ${msSci}
            extractStats "\${log}" "\${NCORES}" "\${SLURM_JOB_ID}" "\${err}" "flag_Amp" "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
            else
                touch "\$FLAG_CHECK_FILE"
            fi
        fi
    
        if [ "\${DO_DYNAMIC}" == "true" ] || [ "\${DO_STOKESV}" == "true" ]; then
    
            parset="${parsets}/cflag_dynamic_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.in"
            cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = \${msSci}

# Amplitude based flagging with dynamic thresholds
#  This finds a statistical threshold in the spectrum of each
#  time-step, then applies the same threshold level to the integrated
#  spectrum at the end.
Cflag.amplitude_flagger.enable           = \${DO_DYNAMIC}
Cflag.amplitude_flagger.dynamicBounds    = true
Cflag.amplitude_flagger.threshold        = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE}
Cflag.amplitude_flagger.integrateSpectra = ${FLAG_DYNAMIC_INTEGRATE_SPECTRA}
Cflag.amplitude_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE_SPECTRA}
Cflag.amplitude_flagger.integrateTimes = ${FLAG_DYNAMIC_INTEGRATE_TIMES}
Cflag.amplitude_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE_TIMES}

\${amplitudeLow}
Cflag.elevation_flagger.enable          = \${elevationFlaggerEnable}
\${elevationLowFlagging}
\${elevationHighFlagging}

# Stokes-V flagging
Cflag.stokesv_flagger.enable           = \${DO_STOKESV}
Cflag.stokesv_flagger.useRobustStatistics = ${FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE}
Cflag.stokesv_flagger.threshold        = ${FLAG_THRESHOLD_STOKESV_SCIENCE}
Cflag.stokesv_flagger.integrateSpectra = ${FLAG_STOKESV_INTEGRATE_SPECTRA}
Cflag.stokesv_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA}
Cflag.stokesv_flagger.integrateTimes = ${FLAG_STOKESV_INTEGRATE_TIMES}
Cflag.stokesv_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES}

EOFINNER

            log="${logs}/cflag_dynamic_science_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
            echo "Cflag log for the dynamic flagger file is \$log"

            ${cflag} -c "\${parset}" > "\${log}"
            err=\$?
            rejuvenate ${msSci}
            extractStats "\${log}" "\${NCORES}" "\${SLURM_JOB_ID}" "\${err}" "flag_Dyn" "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
            else
                touch "\$FLAG_CHECK_FILE"
            fi
    
        fi
    
    fi

echo " "

fi

###########################
# Averaging

DO_IT=$DO_AVERAGE_CHANNELS
if [ "\${DO_IT}" == "true" ] && [ ! -e "\${msSciAv}" ]; then

    echo "Averaging the MS to continuum resolution"

    nChan=${NUM_CHAN_SCIENCE}
    width=${NUM_CHAN_TO_AVERAGE}
    tilenchan=${NUM_CHAN_TO_AVERAGE}
    bucketsize=${BUCKET_SIZE}
    
    parset="${parsets}/science_average_\${FIELDBEAM}_\${SLURM_JOB_ID}.in"
    cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = \${msSci}

# Output measurement set
# Default: <no default>
outputvis   = \${msSciAv}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
# Note that we don't use CHAN_RANGE_SCIENCE, since the splitting out
# has already been done. We instead set this to 1-NUM_CHAN
channel     = "1-\${nChan}"

# Defines the number of channel to average to form the one output channel
# Default: 1
width       = \${width}

# Make the tile size the number of averaged channels, to improve I/O
stman.tilenchan   = \${tilenchan}
# Set a larger bucketsize
stman.bucketsize  = \${bucketsize}

EOFINNER
    
    log="${logs}/science_average_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
    echo "Log file for the averaging is \$log"

    $mssplit -c "\${parset}" > "\${log}"
    err=\$?
    rejuvenate ${msSci}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "avg" "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        # If the averaging succeeded, we can delete the full-resolution MS
        # if so desired.
        purgeFullMS=${PURGE_FULL_MS}
        if [ "\${purgeFullMS}" == "true" ]; then
            echo "Removing the input MS since the averaging succeeded"
            lfs find \$msSci -type f -print0 | xargs -0 munlink
            find \$msSci -type l -delete
            find \$msSci -depth -type d -empty -delete
        fi
    fi

    echo " "
fi

###########################
# Flagging of the averaged data

DO_IT=$FLAG_AFTER_AVERAGING

if [ "\${DO_IT}" == "true" ] && [ ! -e "\${FLAG_AV_CHECK_FILE}" ]; then

    echo "Flagging the averaged dataset"

    FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV=${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV}
    FLAG_DO_FLAT_AMPLITUDE_SCIENCE_AV=${FLAG_DO_FLAT_AMPLITUDE_SCIENCE_AV}
    FLAG_THRESHOLD_AMPLITUDE_SCIENCE_AV=${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_AV}
    CHANNEL_FLAG_SCIENCE_AV=${CHANNEL_FLAG_SCIENCE_AV}
    TIME_FLAG_SCIENCE_AV=${TIME_FLAG_SCIENCE_AV}
    DO_DYNAMIC=${FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE_AV}
    DO_STOKESV=${FLAG_DO_STOKESV_SCIENCE_AV}
    USE_AOFLAGGER=${FLAG_SCIENCE_AV_WITH_AOFLAGGER}
    
    DO_AMP_FLAG=false
    ruleList=""

    # Define a lower bound to the amplitudes
    amplitudeLow="# No absolute lower bound to visibility amplitudes"
    if [ "\${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV}" != "" ]; then
        # Only add the low threshold if it has been given
        amplitudeLow="Cflag.amplitude_flagger.low             = \${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV}"
    fi

    # The flat amplitude cut to be applied
    if [ "\${FLAG_DO_FLAT_AMPLITUDE_SCIENCE_AV}" == "true" ]; then
        amplitudeCut="# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = \${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_AV}
${amplitudeLow}"
        DO_AMP_FLAG=true
    else
        amplitudeCut="# No flat amplitude flagging applied"
    fi

    # Rule 1 relates to channel range flagging
    if [ "\${CHANNEL_FLAG_SCIENCE_AV}" == "" ]; then
        channelFlagging="# Not flagging any specific channel range"
    else
        channelFlagging="# The following flags out specific channels:
Cflag.selection_flagger.rule1.spw = \${CHANNEL_FLAG_SCIENCE_AV}"
        if [ "\${ruleList}" == "" ]; then
            ruleList="rule1"
        else
            ruleList="\${ruleList},rule1"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 2 relates to time range flagging
    if [ "\${TIME_FLAG_SCIENCE_AV}" == "" ]; then
        timeFlagging="# Not flagging any specific time range"
    else
        timeFlagging="# The following flags out specific time range(s):
Cflag.selection_flagger.rule2.timerange = \${TIME_FLAG_SCIENCE_AV}"
        if [ "\${ruleList}" == "" ]; then
            ruleList="rule2"
        else
            ruleList="\${ruleList},rule2"
        fi
        DO_AMP_FLAG=true
    fi

    if [ "\${ruleList}" == "" ]; then
        selectionRule="# No selection rules used"
    else
        selectionRule="Cflag.selection_flagger.rules           = [\${ruleList}]"
    fi

    if [ "\${USE_AOFLAGGER}" == "true" ]; then
    
        AOFLAGGER_OPTIONS_SCIENCE_AV="${AOFLAGGER_OPTIONS}"
        if [ "\${AOFLAGGER_STRATEGY_SCIENCE_AV}" != "" ]; then
            AOFLAGGER_OPTIONS_SCIENCE_AV="${AOFLAGGER_OPTIONS_SCIENCE_AV} -strategy \"\${AOFLAGGER_STRATEGY_SCIENCE_AV}\""
        fi
    
        log="${logs}/aoflag_science_ave_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
        echo "Flagging with AOflagger - log file is \$log"

        # Need to use the gnu PrgEnv, not the cray, to be able to load aoflagger
        if [ "\$(module list -t 2>&1 | grep PrgEnv-cray)" != "" ]; then
          module swap PrgEnv-cray PrgEnv-gnu
        fi
        loadModule aoflagger
        NCORES=1
        aoflagger \${AOFLAGGER_OPTIONS_SCIENCE_AV} \${msSciAv} > "\${log}"
        err=\$?
        unloadModule aoflagger
        rejuvenate \${msSciAv}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} flag_av_aoflag "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "\$FLAG_AV_CHECK_FILE"
        fi
    
    else
    
        if [ "\$DO_AMP_FLAG" == "true" ]; then
    
            parset="${parsets}/cflag_amp_science_ave_\${FIELDBEAM}_\${SLURM_JOB_ID}.in"
            cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = \${msSciAv}

\${amplitudeCut}

\${selectionRule}

\${channelFlagging}

\${timeFlagging}

EOFINNER

            log="${logs}/cflag_amp_science_ave_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
            echo "Flagging amplitudes with Cflag - log file is \$log"

            NCORES=1
            NPPN=1
            ${cflag} -c "\${parset}" > "\${log}"
            err=\$?
            rejuvenate \${msSciAv}
            extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} flag_av_Amp "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
            else
                touch "\$FLAG_AV_CHECK_FILE"
            fi
        fi
    
        if [ "\${DO_DYNAMIC}" == "true" ] || [ "\${DO_STOKESV}" == "true" ]; then
    
            parset="${parsets}/cflag_dynamic_science_ave_\${FIELDBEAM}_\${SLURM_JOB_ID}.in"
            cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = \${msSciAv}

# Amplitude based flagging with dynamic thresholds
#  This finds a statistical threshold in the spectrum of each
#  time-step, then applies the same threshold level to the integrated
#  spectrum at the end.
Cflag.amplitude_flagger.enable           = \${DO_DYNAMIC}
Cflag.amplitude_flagger.dynamicBounds    = true
Cflag.amplitude_flagger.threshold        = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE_AV}
Cflag.amplitude_flagger.integrateSpectra = ${FLAG_DYNAMIC_INTEGRATE_SPECTRA_AV}
Cflag.amplitude_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE_SPECTRA_AV}
Cflag.amplitude_flagger.integrateTimes = ${FLAG_DYNAMIC_INTEGRATE_TIMES_AV}
Cflag.amplitude_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE_TIMES_AV}

${amplitudeLow}

# Stokes-V flagging
Cflag.stokesv_flagger.enable           = \${DO_STOKESV}
Cflag.stokesv_flagger.useRobustStatistics = ${FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE_AV}
Cflag.stokesv_flagger.threshold        = ${FLAG_THRESHOLD_STOKESV_SCIENCE_AV}
Cflag.stokesv_flagger.integrateSpectra = ${FLAG_STOKESV_INTEGRATE_SPECTRA_AV}
Cflag.stokesv_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA_AV}
Cflag.stokesv_flagger.integrateTimes = ${FLAG_STOKESV_INTEGRATE_TIMES_AV}
Cflag.stokesv_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES_AV}

EOFINNER

            log="${logs}/cflag_dynamic_science_ave_\${FIELDBEAM}_\${SLURM_JOB_ID}.log"
            echo "Dynamic flagging with Cflag - log file is \$log"

            NCORES=1
            NPPN=1
            ${cflag} -c "\${parset}" > "\${log}"
            err=\$?
            rejuvenate \${msSciAv}
            extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} flag_av_Dyn "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
            else
                touch "\$FLAG_AV_CHECK_FILE"
            fi
    
        fi
    
    fi

echo " "
fi

echo "Preimaging for FIELD \$FIELD and BEAM \$BEAM is complete"
echo " "

EOFOUTER

chmod a+x ${scriptName}

# This defines some text to go in the sbatch files for the continuum imaging / self-cal
PREIMAGING_TEXT="# The following runs the pre-imaging script to set up the MS ready for imaging
NCORES=1   
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${scriptName} -b \${BEAM} -f \${FIELD_ID}

"

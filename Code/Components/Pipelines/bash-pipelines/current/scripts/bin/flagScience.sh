#!/bin/bash -l
#
# Launches a job to flag the science data in two passes, one with a
# flat amplitude cut to remove any bright spikes, plus antenna or
# autocorrelation-based flagging, and a second with a dynamic
# amplitude threshold
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

ID_FLAG_SCI=""

DO_IT=$DO_FLAG_SCIENCE

if [ -e "${FLAG_CHECK_FILE}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Flagging for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ "${DO_SPLIT_SCIENCE}" != "true" ]; then
    # If we aren't doing splitting, we need the relevant MS to already exist
    if [ "${DO_IT}" == "true" ] && [ ! -e "${OUTPUT}/${msSci}" ]; then
        echo "Single-beam MS $msSci does not exist, so cannot run flagging"
        DO_IT=false
    fi
fi

if [ "${DO_IT}" == "true" ]; then

    AOFLAGGER_OPTIONS_SCIENCE="${AOFLAGGER_OPTIONS}"
    if [ "${AOFLAGGER_STRATEGY_SCIENCE}" != "" ]; then
        AOFLAGGER_OPTIONS_SCIENCE="${AOFLAGGER_OPTIONS_SCIENCE} -strategy \"${AOFLAGGER_STRATEGY_SCIENCE}\""
    fi

    DO_AMP_FLAG=false
    ruleList=""

    # Rule 1 relates to antenna-based flagging
    if [ "${ANTENNA_FLAG_SCIENCE}" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
    else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rule1.antenna   = ${ANTENNA_FLAG_SCIENCE}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule1"
        else
            ruleList="${ruleList},rule1"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 2 relates to channel range flagging
    if [ "${CHANNEL_FLAG_SCIENCE}" == "" ]; then
        channelFlagging="# Not flagging any specific channel range"
    else
        channelFlagging="# The following flags out specific channels:
Cflag.selection_flagger.rule2.spw = ${CHANNEL_FLAG_SCIENCE}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule2"
        else
            ruleList="${ruleList},rule2"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 3 relates to time range flagging
    if [ "${TIME_FLAG_SCIENCE}" == "" ]; then
        timeFlagging="# Not flagging any specific time range"
    else
        timeFlagging="# The following flags out specific time range(s):
Cflag.selection_flagger.rule3.timerange = ${TIME_FLAG_SCIENCE}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule3"
        else
            ruleList="${ruleList},rule3"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 4 relates to flagging of autocorrelations
    if [ "${FLAG_AUTOCORRELATION_SCIENCE}" == "true" ]; then
        autocorrFlagging="# The following flags out the autocorrelations, if set to true:
Cflag.selection_flagger.rule4.autocorr  = ${FLAG_AUTOCORRELATION_SCIENCE}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule4"
        else
            ruleList="${ruleList},rule4"
        fi
        DO_AMP_FLAG=true
    else
        autocorrFlagging="# No flagging of autocorrelations"
    fi

    if [ "${ruleList}" == "" ]; then
        selectionRule="# No selection rules used"
    else
        selectionRule="Cflag.selection_flagger.rules           = [${ruleList}]"
    fi

    # Define a lower bound to the amplitudes, for use in either
    # flagging task
    amplitudeLow="# No absolute lower bound to visibility amplitudes"
    if [ "${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}" != "" ]; then
        # Only add the low threshold if it has been given
        amplitudeLow="Cflag.amplitude_flagger.low             = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}"
    fi

    # The flat amplitude cut to be applied
    if [ "${FLAG_DO_FLAT_AMPLITUDE_SCIENCE}" == "true" ]; then
        amplitudeCut="# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE}
${amplitudeLow}"
        DO_AMP_FLAG=true
    else
        amplitudeCut="# No flat amplitude flagging applied"
    fi
    # Define a lower limit for Elevation (deg) below which all vis are to be flagged:
    elevationLowFlagging="# No flagging based on lower bound for Elevation"
    elevationFlaggerEnable=false
    if [ "${ELEVATION_FLAG_SCIENCE_LOW}" != "" ]; then
        # Only add the low Elevation limit if it has been given
        elevationLowFlagging="Cflag.elevation_flagger.low       =  ${ELEVATION_FLAG_SCIENCE_LOW}"
        elevationFlaggerEnable=true
    fi
    # Define an upper limit for Elevation (deg) above which all vis are to be flagged:
    elevationHighFlagging="# No flagging based on upper bound for Elevation"
    if [ "${ELEVATION_FLAG_SCIENCE_HIGH}" != "" ]; then
        # Only add the upper Elevation limit if it has been given
        elevationHighFlagging="Cflag.elevation_flagger.high       =  ${ELEVATION_FLAG_SCIENCE_HIGH}"
        elevationFlaggerEnable=true
    fi

    setJob flag_science flag
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_FLAG_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-flagSci-b${BEAM}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

USE_AOFLAGGER=${FLAG_SCIENCE_WITH_AOFLAGGER}

if [ "\${USE_AOFLAGGER}" == "true" ]; then

    log="${logs}/aoflag_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

    # Need to use the gnu PrgEnv, not the cray, to be able to load aoflagger
    if [ "\$(module list -t 2>&1 | grep PrgEnv-cray)" != "" ]; then
      module swap PrgEnv-cray PrgEnv-gnu
    fi
    loadModule aoflagger
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} aoflagger ${AOFLAGGER_OPTIONS_SCIENCE} ${msSci} > "\${log}"
    err=\$?
    unloadModule aoflagger
    rejuvenate ${msSci}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_aoflag "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch "$FLAG_CHECK_FILE"
    fi

else

    DO_AMP_FLAG=${DO_AMP_FLAG}
    if [ \$DO_AMP_FLAG == true ]; then

        parset="${parsets}/cflag_amp_science_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
        cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSci}

${amplitudeCut}

${selectionRule}

${antennaFlagging}

${channelFlagging}

${timeFlagging}

${autocorrFlagging}
Cflag.elevation_flagger.enable          = ${elevationFlaggerEnable}
${elevationLowFlagging}
${elevationHighFlagging}
EOFINNER

        log="${logs}/cflag_amp_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cflag} -c "\${parset}" > "\${log}"
        err=\$?
        rejuvenate ${msSci}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_Amp "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "$FLAG_CHECK_FILE"
        fi
    fi

    DO_DYNAMIC=${FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE}
    DO_STOKESV=${FLAG_DO_STOKESV_SCIENCE}
    if [ "\${DO_DYNAMIC}" == "true" ] || [ "\${DO_STOKESV}" == "true" ]; then

        parset="${parsets}/cflag_dynamic_science_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
        cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSci}

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

${amplitudeLow}
Cflag.elevation_flagger.enable          = ${elevationFlaggerEnable}
${elevationLowFlagging}
${elevationHighFlagging}

# Stokes-V flagging
Cflag.stokesv_flagger.enable           = \${DO_STOKESV}
Cflag.stokesv_flagger.useRobustStatistics = ${FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE}
Cflag.stokesv_flagger.threshold        = ${FLAG_THRESHOLD_STOKESV_SCIENCE}
Cflag.stokesv_flagger.integrateSpectra = ${FLAG_STOKESV_INTEGRATE_SPECTRA}
Cflag.stokesv_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA}
Cflag.stokesv_flagger.integrateTimes = ${FLAG_STOKESV_INTEGRATE_TIMES}
Cflag.stokesv_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES}

EOFINNER

        log="${logs}/cflag_dynamic_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cflag} -c "\${parset}" > "\${log}"
        err=\$?
        rejuvenate ${msSci}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_Dyn "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "$FLAG_CHECK_FILE"
        fi

    fi

fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI")
	ID_FLAG_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_FLAG_SCI}" "Flagging beam ${BEAM} of science observation, with flags \"$DEP\""
	ID_FLAG_SCI_LIST=$(addDep "$ID_FLAG_SCI" "$ID_FLAG_SCI_LIST")
    else
	echo "Would run flagging beam ${BEAM} for science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

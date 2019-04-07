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

ID_FLAG_SCI_AV=""

DO_IT=$DO_FLAG_SCIENCE

if [ -e "${FLAG_AV_CHECK_FILE}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Flagging of averaged data for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ ! -e "${msSciAv}" ] && [ "${DO_AVERAGE_CHANNELS}" != "true" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Flagging of averaged data will be turned off, as we are not creating the averaged data!"
    fi
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    AOFLAGGER_OPTIONS_SCIENCE_AV="${AOFLAGGER_OPTIONS}"
    if [ "${AOFLAGGER_STRATEGY_SCIENCE_AV}" != "" ]; then
        AOFLAGGER_OPTIONS_SCIENCE_AV="${AOFLAGGER_OPTIONS_SCIENCE_AV} -strategy \"${AOFLAGGER_STRATEGY_SCIENCE_AV}\""
    fi

    DO_AMP_FLAG=false
    ruleList=""

    # Define a lower bound to the amplitudes, for use in either
    # flagging task
    amplitudeLow="# No absolute lower bound to visibility amplitudes"
    if [ "${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV}" != "" ]; then
        # Only add the low threshold if it has been given
        amplitudeLow="Cflag.amplitude_flagger.low             = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV}"
    fi

    # The flat amplitude cut to be applied
    if [ "${FLAG_DO_FLAT_AMPLITUDE_SCIENCE_AV}" == "true" ]; then
        amplitudeCut="# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_AV}
${amplitudeLow}"
        DO_AMP_FLAG=true
    else
        amplitudeCut="# No flat amplitude flagging applied"
    fi

    # Rule 1 relates to channel range flagging
    if [ "${CHANNEL_FLAG_SCIENCE_AV}" == "" ]; then
        channelFlagging="# Not flagging any specific channel range"
    else
        channelFlagging="# The following flags out specific channels:
Cflag.selection_flagger.rule1.spw = ${CHANNEL_FLAG_SCIENCE_AV}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule1"
        else
            ruleList="${ruleList},rule1"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 2 relates to time range flagging
    if [ "${TIME_FLAG_SCIENCE_AV}" == "" ]; then
        timeFlagging="# Not flagging any specific time range"
    else
        timeFlagging="# The following flags out specific time range(s):
Cflag.selection_flagger.rule2.timerange = ${TIME_FLAG_SCIENCE_AV}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule2"
        else
            ruleList="${ruleList},rule2"
        fi
        DO_AMP_FLAG=true
    fi

    if [ "${ruleList}" == "" ]; then
        selectionRule="# No selection rules used"
    else
        selectionRule="Cflag.selection_flagger.rules           = [${ruleList}]"
    fi

    setJob flag_ave_science flagAv
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

USE_AOFLAGGER=${FLAG_SCIENCE_AV_WITH_AOFLAGGER}

if [ "\${USE_AOFLAGGER}" == "true" ]; then

    log="${logs}/aoflag_science_ave_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

    # Need to use the gnu PrgEnv, not the cray, to be able to load aoflagger
    if [ "\$(module list -t 2>&1 | grep PrgEnv-cray)" != "" ]; then
      module swap PrgEnv-cray PrgEnv-gnu
    fi
    loadModule aoflagger
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} aoflagger ${AOFLAGGER_OPTIONS_SCIENCE_AV} ${msSciAv} > "\${log}"
    err=\$?
    unloadModule aoflagger
    rejuvenate ${msSciAv}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_aoflag "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch "$FLAG_CHECK_FILE"
    fi

else

    DO_AMP_FLAG=${DO_AMP_FLAG}
    if [ \$DO_AMP_FLAG == true ]; then

        parset="${parsets}/cflag_amp_science_ave_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
        cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSciAv}

${amplitudeCut}

${selectionRule}

${channelFlagging}

${timeFlagging}

EOFINNER

        log="${logs}/cflag_amp_science_ave_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cflag} -c "\${parset}" > "\${log}"
        err=\$?
        rejuvenate ${msSciAv}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_Amp "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "$FLAG_AV_CHECK_FILE"
        fi
    fi

    DO_DYNAMIC=${FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE_AV}
    DO_STOKESV=${FLAG_DO_STOKESV_SCIENCE_AV}
    if [ "\${DO_DYNAMIC}" == "true" ] || [ "\${DO_STOKESV}" == "true" ]; then

        parset="${parsets}/cflag_dynamic_science_ave_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
        cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSciAv}

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

        log="${logs}/cflag_dynamic_science_ave_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cflag} -c "\${parset}" > "\${log}"
        err=\$?
        rejuvenate ${msSciAv}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_Dyn "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "$FLAG_AV_CHECK_FILE"
        fi

    fi

fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI")
        DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI")
	ID_FLAG_SCI_AV=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_FLAG_SCI_AV}" "Flagging beam ${BEAM} of averaged science observation, with flags \"$DEP\""
	ID_FLAG_SCI_AV_LIST=$(addDep "$ID_FLAG_SCI_AV" "$ID_FLAG_SCI_AV_LIST")
    else
	echo "Would run flagging beam ${BEAM} for the averaged science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

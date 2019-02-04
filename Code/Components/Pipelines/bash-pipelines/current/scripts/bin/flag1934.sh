#!/bin/bash -l
#
# Launches a job to flag the 1934-638 data in two passes, one with a
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

ID_FLAG_1934=""

DO_IT=$DO_FLAG_1934
if [ -e "${FLAG_1934_CHECK_FILE}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Flagging for beam $BEAM of calibrator observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ "${DO_SPLIT_1934}" != "true" ]; then
    # If we aren't doing splitting, we need the relevant MS to already exist
    if [ "${DO_IT}" == "true" ] && [ ! -e "${OUTPUT}/${msCal}" ]; then
        echo "Single-beam MS $msCal does not exist, so cannot run flagging"
        DO_IT=false
    fi
fi

if [ "${DO_IT}" == "true" ]; then

    AOFLAGGER_OPTIONS_1934="${AOFLAGGER_OPTIONS}"
    if [ "${AOFLAGGER_STRATEGY_1934}" != "" ]; then
        AOFLAGGER_OPTIONS_1934="${AOFLAGGER_OPTIONS_1934} -strategy \"${AOFLAGGER_STRATEGY_1934}\""
    fi

    DO_AMP_FLAG=false
    ruleList=""

    # Rule 1 relates to antenna-based flagging
    if [ "${ANTENNA_FLAG_1934}" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
    else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rule1.antenna   = ${ANTENNA_FLAG_1934}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule1"
        else
            ruleList="${ruleList},rule1"
        fi
        DO_AMP_FLAG=true
    fi
    
    # Rule 2 relates to channel range flagging
    if [ "${CHANNEL_FLAG_1934}" == "" ]; then
        channelFlagging="# Not flagging any specific channel range"
    else
        channelFlagging="# The following flags out specific channels:
Cflag.selection_flagger.rule2.spw = ${CHANNEL_FLAG_1934}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule2"
        else
            ruleList="${ruleList},rule2"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 3 relates to time range flagging
    if [ "${TIME_FLAG_1934}" == "" ]; then
        timeFlagging="# Not flagging any specific time range"
    else
        timeFlagging="# The following flags out specific time range(s):
Cflag.selection_flagger.rule3.timerange = ${TIME_FLAG_1934}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule3"
        else
            ruleList="${ruleList},rule3"
        fi
        DO_AMP_FLAG=true
    fi

    # Rule 4 relates to flagging of the autocorrelations.
    if [ "${FLAG_AUTOCORRELATION_1934}" == "true" ]; then
        autocorrFlagging="# The following flags out the autocorrelations, if set to true:
Cflag.selection_flagger.rule4.autocorr  = ${FLAG_AUTOCORRELATION_1934}"
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

    # The flat amplitude cut to be applied
    if [ "${FLAG_DO_FLAT_AMPLITUDE_1934}" == "true" ]; then
        amplitudeCut="# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = ${FLAG_THRESHOLD_AMPLITUDE_1934}"
        if [ "${FLAG_THRESHOLD_AMPLITUDE_1934_LOW}" != "" ]; then
            # Only add the low threshold if it has been given
            amplitudeCut="${amplitudeCut}
Cflag.amplitude_flagger.low             = ${FLAG_THRESHOLD_AMPLITUDE_1934_LOW}"
        fi
        DO_AMP_FLAG=true
    fi


    setJob flag_1934 flag
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_FLAG_1934}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-flag1934-b${BEAM}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

USE_AOFLAGGER=${FLAG_1934_WITH_AOFLAGGER}

if [ "\${USE_AOFLAGGER}" == "true" ]; then

    log="${logs}/aoflag_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

    # Need to use the gnu PrgEnv, not the cray, to be able to load aoflagger
    if [ "\$(module list -t 2>&1 | grep PrgEnv-cray)" != "" ]; then
      module swap PrgEnv-cray PrgEnv-gnu
    fi
    loadModule aoflagger
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} aoflagger ${AOFLAGGER_OPTIONS_1934} ${msCal} > "\${log}"
    err=\$?
    unloadModule aoflagger
    rejuvenate ${msCal}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_aoflag "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch "$FLAG_1934_CHECK_FILE"
    fi

else

    DO_AMP_FLAG=${DO_AMP_FLAG}
    if [ "\${DO_AMP_FLAG}" == "true" ]; then

        parset="${parsets}/cflag_amp_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
        cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msCal}

${amplitudeCut}

${selectionRule}

${antennaFlagging}

${channelFlagging}

${timeFlagging}

${autocorrFlagging}

EOFINNER

        log="${logs}/cflag_amp_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cflag} -c "\${parset}" > "\${log}"
        err=\$?
        rejuvenate ${msCal}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_Amp "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "$FLAG_1934_CHECK_FILE"
        fi
    fi

    DO_DYNAMIC=${FLAG_DO_DYNAMIC_AMPLITUDE_1934}
    DO_STOKESV=${FLAG_DO_STOKESV_1934}
    if [ "\${DO_DYNAMIC}" == "true" ] || [ "\${DO_STOKESV}" == "true" ]; then

        parset="${parsets}/cflag_dynamic_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
        cat > "\$parset" <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msCal}

# Amplitude based flagging
Cflag.amplitude_flagger.enable           = \${DO_DYNAMIC}
Cflag.amplitude_flagger.dynamicBounds    = true
Cflag.amplitude_flagger.threshold        = ${FLAG_THRESHOLD_DYNAMIC_1934}
Cflag.amplitude_flagger.integrateSpectra = ${FLAG_DYNAMIC_1934_INTEGRATE_SPECTRA}
Cflag.amplitude_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_DYNAMIC_1934_SPECTRA}
Cflag.amplitude_flagger.integrateTimes = ${FLAG_DYNAMIC_1934_INTEGRATE_TIMES}
Cflag.amplitude_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_DYNAMIC_1934_TIMES}

# Stokes-V based flagging
Cflag.stokesv_flagger.enable           = \${DO_STOKESV}
Cflag.stokesv_flagger.useRobustStatistics = ${FLAG_USE_ROBUST_STATS_STOKESV_1934}
Cflag.stokesv_flagger.threshold        = ${FLAG_THRESHOLD_STOKESV_1934}
Cflag.stokesv_flagger.integrateSpectra = ${FLAG_STOKESV_1934_INTEGRATE_SPECTRA}
Cflag.stokesv_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_STOKESV_1934_SPECTRA}
Cflag.stokesv_flagger.integrateTimes = ${FLAG_STOKESV_1934_INTEGRATE_TIMES}
Cflag.stokesv_flagger.integrateTimes.threshold = ${FLAG_THRESHOLD_STOKESV_1934_TIMES}

${antennaFlagging}
EOFINNER

        log="${logs}/cflag_dynamic_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cflag} -c "\${parset}" > "\${log}"
        err=\$?
        rejuvenate ${msCal}
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_Dyn "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        else
            touch "$FLAG_1934_CHECK_FILE"
        fi
    fi

fi
EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_1934")
        ID_FLAG_1934=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_FLAG_1934}" "Flagging 1934-638, beam $BEAM"
        FLAG_CBPCAL_DEP=$(addDep "$FLAG_CBPCAL_DEP" "$ID_FLAG_1934")
    else
        echo "Would run flagging of 1934-638, beam $BEAM, with slurm file $sbatchfile"
    fi

    echo " "

fi

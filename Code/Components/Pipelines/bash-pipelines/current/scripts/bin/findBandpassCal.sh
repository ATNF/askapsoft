#!/bin/bash -l
#
# Launches a job to solve for the bandpass calibration. This uses all
# 1934-638 measurement sets after the splitFlag1934.sh jobs have
# completed. The bandpass calibration is done assuming the special
# '1934-638' component.
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

ID_CBPCAL=""

DO_IT=$DO_FIND_BANDPASS

if [ "${CLOBBER}" != "true" ] && [ -e "${TABLE_BANDPASS}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Bandpass table ${TABLE_BANDPASS} exists, so not running cbpcalibrator"
    fi
    DO_IT=false
fi

if [ "${ms1934list}" == "" ]; then
    echo "ERROR (findBandpass) - don't have any bandpass MS files to use"
    exit 1
fi

if [ "${DO_IT}" == "true" ]; then
    # If we aren't splitting, check for the existence of each MS in our list.
    # They all need to be there for the bandpass cal job to work
    if [ "${DO_SPLIT_1934}" != "true" ]; then
        for ms in $(echo $ms1934list | sed -e 's/,/ /g'); do
            if [ "${DO_IT}" == "true" ] && [ ! -e "${ms}" ]; then
                echo "Calibrator MS ${ms} does not exist, so turning off bandpass calibration."
                DO_IT=false
            fi
        done
    fi
fi

if [ "${DO_IT}" == "true" ]; then

    # Optional data selection
    dataSelectionPars="# Using all the data"
    if [ "${BANDPASS_MINUV}" -gt 0 ]; then
        dataSelectionPars="# Minimum UV distance for bandpass calibration:
Cbpcalibrator.MinUV = ${BANDPASS_MINUV}"
    fi
    referencePars="# Referencing for cbpcalibrator"
    if [ "${BANDPASS_REFANTENNA}" != "" ]; then
        referencePars="${referencePars}
Cbpcalibrator.refantenna                      = ${BANDPASS_REFANTENNA}"
    fi


    # Check for bandpass smoothing options
    DO_RUN_VALIDATION=false
    if [ "${DO_BANDPASS_SMOOTH}" != "true" ]; then
        BANDPASS_SMOOTH_TOOL=""
    fi

    script_location="${ACES_LOCATION}/tools"

    if [ "${BANDPASS_SMOOTH_TOOL}" == "plot_caltable" ]; then

        DO_RUN_VALIDATION=true

        script_name="plot_caltable.py"
        if [ ! -e "${script_location}/${script_name}" ]; then
            echo "WARNING - ${script_name} not found in $script_location - not running bandpass smoothing/plotting."
            DO_RUN_PLOT_CALTABLE=false
        fi
        script_args="-t ${TABLE_BANDPASS} -s B "
	script_args_bare_minimum=${script_args}
        if [ "${DO_BANDPASS_SMOOTH}" == "true" ]; then
            script_args="${script_args} -sm"
            if [ "${DO_BANDPASS_PLOT}" != "true" ]; then
                script_args="${script_args} --no_plot"
            fi
        fi
        if [ "${BANDPASS_SMOOTH_AMP}" == "true" ]; then
            script_args="${script_args} -sa"
        fi
        if [ "${BANDPASS_SMOOTH_OUTLIER}" == "true" ]; then
            script_args="${script_args} -o"
        fi

        if [ "${BANDPASS_SMOOTH_F54}" != "" ]; then
            numberOfChannels=$(echo ${BANDPASS_SMOOTH_F54} | awk '{print $1*54}')
            script_args="${script_args} -bf ${numberOfChannels}"
        fi
        
        script_args="${script_args} -fit ${BANDPASS_SMOOTH_FIT} -th ${BANDPASS_SMOOTH_THRESHOLD}"

    elif [ "${BANDPASS_SMOOTH_TOOL}" == "smooth_bandpass" ]; then

        DO_RUN_VALIDATION=true
        script_module_commands="module use /group/askap/raj030/modulefiles
loadModule bptool"
        unload_script="unloadModule bptool"
        script_name="smooth_bandpass.py"
        script_args="-t ${TABLE_BANDPASS} -wp "
	script_args_bare_minimum=${script_args}

        if [ "${BANDPASS_REFANTENNA}" != "" ]; then
            script_args="${script_args} -r ${BANDPASS_REFANTENNA}"
        fi
        if [ "${BANDPASS_SMOOTH_POLY_ORDER}" != "" ]; then
            script_args="${script_args} -np ${BANDPASS_SMOOTH_POLY_ORDER}"
        fi
        if [ "${BANDPASS_SMOOTH_HARM_ORDER}" != "" ]; then
            script_args="${script_args} -nh ${BANDPASS_SMOOTH_HARM_ORDER}"
        fi
        if [ "${BANDPASS_SMOOTH_N_WIN}" != "" ]; then
            script_args="${script_args} -nwin ${BANDPASS_SMOOTH_N_WIN}"
        fi
        if [ "${BANDPASS_SMOOTH_N_TAPER}" != "" ]; then
            script_args="${script_args} -nT ${BANDPASS_SMOOTH_N_TAPER}"
        fi
        if [ "${BANDPASS_SMOOTH_N_ITER}" != "" ]; then
            script_args="${script_args} -nI ${BANDPASS_SMOOTH_N_ITER}"
        fi
        if [ "${BANDPASS_SMOOTH_F54}" != "" ]; then
            script_args="${script_args} -f54 ${BANDPASS_SMOOTH_F54}"
        fi


    fi

    # Overwrite argument string if the user supplies the arguments in a string: 
    if [ "${BANDPASS_SMOOTH_ARG_STRING}" != "" ]; then 
	    script_args="${script_args_bare_minimum} ${BANDPASS_SMOOTH_ARG_STRING}"
    fi

    validation_script="bandpassValidation.py"
    if [ ! -e "${script_location}/${validation_script}" ]; then
        echo "WARNING - ${validation_script} not found in $script_location - not running bandpass validation."
        DO_RUN_VALIDATION=false
    fi
    validation_args="-d ${BASEDIR} -o $(basename ${ORIGINAL_OUTPUT}) -s ${SB_1934}"

    sbatchfile="$slurms/cbpcalibrator_1934.sbatch"
    cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_FIND_BANDPASS}
#SBATCH --ntasks=${NUM_CPUS_CBPCAL}
#SBATCH --ntasks-per-node=20
#SBATCH --job-name=cbpcal
${exportDirective}
#SBATCH --output="$slurmOut/slurm-findBandpass-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

# Determine the number of channels from the MS metadata file (generated by mslist)
msMetadata="${MS_METADATA_CAL}"
#nChan=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\${msMetadata}" --val=nChan)
nChan=${NUM_CHAN_1934}

parset="${parsets}/cbpcalibrator_1934_\${SLURM_JOB_ID}.in"
cat > "\$parset" <<EOFINNER
Cbpcalibrator.dataset                         = [${ms1934list}]
${dataSelectionPars}
Cbpcalibrator.nAnt                            = ${NUM_ANT}
Cbpcalibrator.nBeam                           = ${maxbeam}
Cbpcalibrator.nChan                           = \${nChan}
${referencePars}
#
Cbpcalibrator.calibaccess                     = table
Cbpcalibrator.calibaccess.table.maxant        = ${NUM_ANT}
Cbpcalibrator.calibaccess.table.maxbeam       = ${maxbeam}
Cbpcalibrator.calibaccess.table.maxchan       = \${nChan}
Cbpcalibrator.calibaccess.table               = ${TABLE_BANDPASS}
#
Cbpcalibrator.sources.names                   = [field1]
Cbpcalibrator.sources.field1.direction        = ${DIRECTION_1934}
Cbpcalibrator.sources.field1.components       = src
Cbpcalibrator.sources.src.calibrator          = 1934-638
#
Cbpcalibrator.gridder                         = SphFunc
#
Cbpcalibrator.ncycles                         = ${NCYCLES_BANDPASS_CAL}

EOFINNER

log="${logs}/cbpcalibrator_1934_\${SLURM_JOB_ID}.log"

NCORES=${NUM_CPUS_CBPCAL}
NPPN=20
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $cbpcalibrator -c "\$parset" > "\$log"
err=\$?
for ms in \$(echo $ms1934list | sed -e 's/,/ /g'); do
    rejuvenate "\$ms";
done
rejuvenate ${TABLE_BANDPASS}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} findBandpass "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

BANDPASS_SMOOTH_TOOL=${BANDPASS_SMOOTH_TOOL}
if [ "\${BANDPASS_SMOOTH_TOOL}" == "plot_caltable" ]; then

    log="${logs}/plot_caltable_\${SLURM_JOB_ID}.log"
    script="${script_location}/${script_name}"

    loadModule casa
    STARTTIME=\$(date +%FT%T)
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" casa --nogui --nologger --log2term --agg -c "\${script}" ${script_args} > "\${log}"
    err=\$?
    unloadModule casa
    for tab in ${TABLE_BANDPASS}*; do
        rejuvenate "\${tab}"
    done
    echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
    extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "smoothBandpass" "txt,csv"

    if [ \$err != 0 ]; then
        exit \$err
    fi

elif [ "\${BANDPASS_SMOOTH_TOOL}" == "smooth_bandpass" ]; then

    log="${logs}/smooth_bandpass_\${SLURM_JOB_ID}.log"
    ${script_module_commands}

    STARTTIME=\$(date +%FT%T)
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" "${script_name}" ${script_args} > "\${log}"
    err=\$?
    ${unload_script}
    for tab in ${TABLE_BANDPASS}*; do
        rejuvenate "\${tab}"
    done
    echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
    extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "smoothBandpass" "txt,csv"

    if [ \$err != 0 ]; then
        exit \$err
    fi

fi

EOF

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        ID_CBPCAL=$(sbatch ${FLAG_CBPCAL_DEP} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_CBPCAL}" "Finding bandpass calibration with 1934-638, with flags \"$FLAG_CBPCAL_DEP\""
    else
        echo "Would find bandpass calibration with slurm file $sbatchfile"
    fi

    echo " "


    sbatchfile="$slurms/bandpass_validation.sbatch"
    cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_FIND_BANDPASS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=bpvalidate
${exportDirective}
#SBATCH --output="$slurmOut/slurm-findBandpass-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"


runValidation=${DO_RUN_VALIDATION}
if [ "\${runValidation}" == "true" ]; then

    log="${logs}/bandpass_validation_\${SLURM_JOB_ID}.log"
    script="${script_location}/${validation_script}"
    STARTTIME=\$(date +%FT%T)
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" "\${script}" ${validation_args} > "\${log}"
    err=\$?
    echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
    extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "bandpassValidation" "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        # Copy the log from the valiation to the diagnostics directory 
        cp \${log} \${diagnostics}
    fi

fi
EOF

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=${FLAG_CBPCAL_DEP}
        DEP=$(addDep "$DEP" "$ID_CBPCAL")
        ID_BPVAL=$(sbatch ${DEP} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_BPVAL}" "Running validation of bandpass calibration, with flags \"$FLAG_CBPCAL_DEP\""
    else
        echo "Would run validation of bandpass calibration with slurm file $sbatchfile"
    fi

    echo " "


    

fi

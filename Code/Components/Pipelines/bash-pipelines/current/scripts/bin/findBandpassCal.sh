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

if [ "${DO_IT}" == "true" ]; then

    # Optional data selection
    dataSelectionPars="# Using all the data"
    if [ "${BANDPASS_MINUV}" -gt 0 ]; then
        dataSelectionPars="# Minimum UV distance for bandpass calibration:
Cbpcalibrator.MinUV = ${BANDPASS_MINUV}"
    fi

    # Check for bandpass smoothing options
    DO_RUN_PLOT_CALTABLE=false
    if [ "${DO_BANDPASS_PLOT}" == "true" ] || [ "${DO_BANDPASS_SMOOTH}" == "true" ]; then
        DO_RUN_PLOT_CALTABLE=true
    fi
    if [ "${DO_RUN_PLOT_CALTABLE}" == "true" ]; then
        script_location="$ACES/tools"
        script_name="plot_caltable"
        if [ ! -e "${script_location}/${script_name}.py" ]; then
            echo "WARNING - ${script_name}.py not found in $script_location - not running bandpass smoothing/plotting."
            DO_RUN_PLOT_CALTABLE=false
        fi
        script_args="-t ${TABLE_BANDPASS} -s B "
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
        script_args="${script_args} -fit ${BANDPASS_SMOOTH_FIT} -th ${BANDPASS_SMOOTH_THRESHOLD}"

    fi

    sbatchfile="$slurms/cbpcalibrator_1934.sbatch"
    cat > "$sbatchfile" <<EOF
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_FIND_BANDPASS}
#SBATCH --ntasks=${NUM_CPUS_CBPCAL}
#SBATCH --ntasks-per-node=20
#SBATCH --job-name=cbpcal
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-findBandpass-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

parset=${parsets}/cbpcalibrator_1934_\${SLURM_JOB_ID}.in
cat > "\$parset" <<EOFINNER
Cbpcalibrator.dataset                         = [${ms1934list}]
${dataSelectionPars}
Cbpcalibrator.nAnt                            = ${NUM_ANT}
Cbpcalibrator.nBeam                           = ${maxbeam}
Cbpcalibrator.nChan                           = ${NUM_CHAN_1934}
Cbpcalibrator.refantenna                      = 1
#
Cbpcalibrator.calibaccess                     = table
Cbpcalibrator.calibaccess.table.maxant        = ${NUM_ANT}
Cbpcalibrator.calibaccess.table.maxbeam       = ${maxbeam}
Cbpcalibrator.calibaccess.table.maxchan       = ${NUM_CHAN_1934}
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

log=${logs}/cbpcalibrator_1934_\${SLURM_JOB_ID}.log

NCORES=${NUM_CPUS_CBPCAL}
NPPN=20
aprun -n \${NCORES} -N \${NPPN} $cbpcalibrator -c "\$parset" > "\$log"
err=\$?
for ms in \$(echo $ms1934list | sed -e 's/,/ /g'); do
    rejuvenate "\$ms";
done
rejuvenate ${TABLE_BANDPASS}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} findBandpass "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

PLOT_CALTABLE=${DO_RUN_PLOT_CALTABLE}
if [ \${PLOT_CALTABLE} == true ]; then

    log=${logs}/plot_caltable_\${SLURM_JOB_ID}.log
    scriptCommand="${script_location}/${script_name}.py ${script_args}"

    module load casa
    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} -b casa --nogui --nologger --log2term -c "\${scriptCommand}" > "\${log}"
    module unload casa
    err=\$?
    for tab in ${TABLE_BANDPASS}*; do
        rejuvenate "\${tab}"
    done
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} smoothBandpass "txt,csv"
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

fi

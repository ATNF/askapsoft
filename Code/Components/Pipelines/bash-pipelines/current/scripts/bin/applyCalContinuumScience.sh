#!/bin/bash -l
#
# Applies the gains calibration determined from self-calibration on
# the continuum dataset to that continuum (averaged) MS. The MS can
# optionally be copied first, to preserve the raw averaged MS.
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

DO_IT=$DO_APPLY_CAL_CONT
if [ -e "$CONT_GAINS_CHECK_FILE" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Application of gains solution to continuum data for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    setJob apply_gains_cal_cont applyCalC
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CONT_APPLYCAL}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-calappCont${BEAM}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

keepRaw=${KEEP_RAW_AV_MS}
if [ "\${keepRaw}" == "true" ]; then
  # we need to make a copy that we will then apply the cal to
  NCORES=1
  NPPN=1
  srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} cp -r ${msSciAv} ${msSciAvCal}
fi

parset="${parsets}/apply_gains_cal_cont_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
log="${logs}/apply_gains_cal_cont_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
cat > "\$parset" <<EOFINNER
Ccalapply.dataset                         = ${msSciAvCal}
#
# Allow flagging of vis if inversion of Mueller matrix fails
Ccalapply.calibrate.allowflag             = true
#
# Ignore the beam, since we calibrate with antennagains
Ccalapply.calibrate.ignorebeam            = true
# Ignore the channel dependence
Ccalapply.calibrate.ignorechannel         = true
# Ignore the leakage
Ccalapply.calibrate.ignoreleakage         = true
#
Ccalapply.calibaccess                     = table
Ccalapply.calibaccess.table.maxant        = ${NUM_ANT}
Ccalapply.calibaccess.table.maxbeam       = ${maxbeam}
Ccalapply.calibaccess.table.maxchan       = ${nchanContSci}
Ccalapply.calibaccess.table               = ${gainscaltab}
EOFINNER

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${ccalapply} -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msSciAvCal}
rejuvenate ${gainscaltab}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch "$CONT_GAINS_CHECK_FILE"
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        submitIt=true
        if [ "${DO_CONT_IMAGING}" != "true" ] || [ "${DO_SELFCAL}" != "true" ]; then
            # If we aren't creating the gains cal table with a self-cal job, then check to see if it exists.
            # If it doesn't, we can't run this job.
            if [ ! -e "${OUTPUT}/${gainscaltab}" ]; then
                submitIt=false
                echo "Not submitting gains calibration of continuum dataset as gains table ${gainscaltab} doesn't exist"
            fi
        fi
        if [ "$submitIt" == "true" ]; then
            DEP=""
            DEP=$(addDep "$DEP" "$DEP_START")
            DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV_LIST")
            DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
            ID_CAL_APPLY_CONT_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
            recordJob "${ID_CAL_APPLY_CONT_SCI}" "Apply gains calibration to the continuum dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
        fi
    else
        echo "Would apply gains calibration to the continuum dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

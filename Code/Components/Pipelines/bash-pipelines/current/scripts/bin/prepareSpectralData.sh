#!/bin/bash -l
#
# Prepares the dataset to be used for spectral-line imaging. Includes
# copying, allowing a selection of a subset of channels, application
# of the gains calibration table from the continuum self-calibration,
# and subtraction of the continuum flux
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

########
# Use mssplit to make a copy of the dataset prior to applying the
# calibration & subtracting the continuum

# Define a few parameters related to the continuum imaging
. "${PIPELINEDIR}/getContinuumCimagerParams.sh"

ID_SPLIT_SL_SCI=""
ID_CAL_APPLY_SL_SCI=""
ID_CONT_SUB_SL_SCI=""

DO_IT=${DO_COPY_SL}
if [ -e "${OUTPUT}/${msSciSL}" ]; then
    if [ "${CLOBBER}" != "true" ]; then
        # If we aren't clobbering files, don't run anything
        if [ "${DO_IT}" == "true" ]; then
            echo "MS ${msSciSL} exists, so not making spectral-line dataset for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ "${DO_IT}" == "true" ]; then
            rm -rf "${OUTPUT}/${msSciSL}"
            rm -f "${SL_GAINS_CHECK_FILE}"
            rm -f "${CONT_SUB_CHECK_FILE}"
        fi
    fi
fi


if [ "${DO_IT}" == "true" ]; then

    setJob split_spectralline_science splitSL
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_SPLIT}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-splitSLsci-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

parset="${parsets}/split_spectralline_science_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${msSci}

# Output measurement set
# Default: <no default>
outputvis   = ${msSciSL}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = ${CHAN_RANGE_SL_SCIENCE}

# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}
# Allow a different tile size for the spectral-line imaging
stman.tilenchan   = ${TILENCHAN_SL}
EOFINNER

log="${logs}/split_spectralline_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${mssplit} -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msSciSL}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER


    if [ "${SUBMIT_JOBS}" == "true" ]; then
	DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
        ID_SPLIT_SL_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_SPLIT_SL_SCI}" "Copy the required spectral-line dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
	ID_SPLIT_SL_SCI_LIST=$(addDep "$ID_SPLIT_SL_SCI" "$ID_SPLIT_SL_SCI_LIST")

    else
        echo "Would copy the required spectral-line dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi


##################
# Apply gains calibration

DO_IT=$DO_APPLY_CAL_SL
if [ -e "${SL_GAINS_CHECK_FILE}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Application of gains solution to spectral-line data for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    setJob apply_gains_cal_spectralline applyCalS
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_APPLYCAL}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-calappSLsci-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

parset="${parsets}/apply_gains_cal_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
log="${logs}/apply_gains_cal_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
cat > "\$parset" <<EOFINNER
Ccalapply.dataset                         = ${msSciSL}
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
rejuvenate ${msSciSL}
rejuvenate ${gainscaltab}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch "$SL_GAINS_CHECK_FILE"
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        submitIt=true
        if [ "${DO_CONT_IMAGING}" != "true" ] || [ "${DO_SELFCAL}" != "true" ]; then
            # If we aren't creating the gains cal table with a self-cal job, then check to see if it exists.
            # If it doesn't, we can't run this job.
            if [ ! -e "${gainscaltab}" ]; then
                submitIt=false
                echo "Not submitting gains calibration of spectral-line dataset as gains table ${gainscaltab} doesn't exist"
            fi
        fi
        if [ "${submitIt}" == "true" ]; then
            DEP=""
            DEP=$(addDep "$DEP" "$DEP_START")
            DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV_LIST")
            DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
            DEP=$(addDep "$DEP" "$ID_SPLIT_SL_SCI")
            ID_CAL_APPLY_SL_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
            recordJob "${ID_CAL_APPLY_SL_SCI}" "Apply gains calibration to the spectral-line dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
	    ID_CAL_APPLY_SL_SCI_LIST=$(addDep "$ID_CAL_APPLY_SL_SCI" "$ID_CAL_APPLY_SL_SCI_LIST")
        fi
    else
        echo "Would apply gains calibration to the spectral-line dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

##################
# Subtract continuum model

DO_IT=$DO_CONT_SUB_SL
if [ -e "${CONT_SUB_CHECK_FILE}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Continuum subtraction from spectral-line data for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    . "${PIPELINEDIR}/spectralContinuumSubtraction.sh"
        
fi



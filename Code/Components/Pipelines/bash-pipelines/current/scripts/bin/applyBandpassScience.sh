#!/bin/bash -l
#
# Launches a job to apply the bandpass solution to the measurement set 
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

ID_CCALAPPLY_SCI=""

DO_IT=$DO_APPLY_BANDPASS
if [ "$DO_IT" == "true" ] && [ -e "$BANDPASS_CHECK_FILE" ]; then
    echo "Bandpass has already been applied to beam $BEAM of the science observation - not re-doing"
    DO_IT=false
fi

if [ "$DO_IT" == "true" ]; then

    setJob apply_bandpass applyBP
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_APPLY_BANDPASS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-applyBandpass-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

RAW_TABLE=${TABLE_BANDPASS}
SMOOTH_TABLE=${TABLE_BANDPASS}.smooth
DO_SMOOTH=${DO_BANDPASS_SMOOTH}
if [ \${DO_SMOOTH} ] && [ -e \${SMOOTH_TABLE} ]; then
    TABLE=\${SMOOTH_TABLE}
else
    TABLE=\${RAW_TABLE}
fi

parset=${parsets}/ccalapply_bp_b${BEAM}_\${SLURM_JOB_ID}.in
cat > "\$parset" << EOFINNER
Ccalapply.dataset                         = ${msSci}
#
# Allow flagging of vis if inversion of Mueller matrix fails
Ccalapply.calibrate.allowflag             = true
Ccalapply.calibrate.scalenoise            = ${BANDPASS_SCALENOISE}
#
Ccalapply.calibaccess                     = table
Ccalapply.calibaccess.table.maxant        = ${NUM_ANT}
Ccalapply.calibaccess.table.maxbeam       = ${maxbeam}
Ccalapply.calibaccess.table.maxchan       = ${NUM_CHAN_SCIENCE}
Ccalapply.calibaccess.table               = \${TABLE}

EOFINNER

log=${logs}/ccalapply_bp_b${BEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${ccalapply} -c "\$parset" > "\$log"
err=\$?
rejuvenate ${msSci}
rejuvenate \${TABLE}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $BANDPASS_CHECK_FILE
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
	DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI")
        DEP=$(addDep "$DEP" "$ID_CBPCAL")
	ID_CCALAPPLY_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_CCALAPPLY_SCI}" "Applying bandpass calibration to science observation, with flags \"$DEP\""
    else
	echo "Would apply bandpass calibration to science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

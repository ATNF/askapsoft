#!/bin/bash -l
#
# Launches a job to average the measurement set for the current beam
# of the science observation so that it can be imaged by the continuum
# imager. 
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

ID_AVERAGE_SCI=""

DO_IT=$DO_AVERAGE_CHANNELS

if [ "${DO_IT}" == "true" ] && [ -e "${OUTPUT}/${msSciAv}" ]; then
    if [ "${CLOBBER}" != "true" ]; then
        # If we aren't clobbering files, don't run anything
        if [ "${DO_IT}" == "true" ]; then
            echo "MS ${msSciAv} exists, so not running averaging for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ "${DO_IT}" == "true" ]; then
            rm -rf "${OUTPUT}/${msSciAv}"
        fi
    fi
fi

if [ "${DO_IT}" == "true" ]; then

    setJob science_average avg
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_AVERAGE_MS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-averageSci-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

parset=${parsets}/science_average_${FIELDBEAM}_\${SLURM_JOB_ID}.in
cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${msSci}

# Output measurement set
# Default: <no default>
outputvis   = ${msSciAv}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
# Note that we don't use CHAN_RANGE_SCIENCE, since the splitting out
# has already been done. We instead set this to 1-NUM_CHAN_SCIENCE
channel     = "1-${NUM_CHAN_SCIENCE}"

# Defines the number of channel to average to form the one output channel
# Default: 1
width       = ${NUM_CHAN_TO_AVERAGE}


# Make the tile size the number of averaged channels, to improve I/O
stman.tilenchan   = ${nchanContSci}
# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}

EOFINNER

log=${logs}/science_average_${FIELDBEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} $mssplit -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msSci}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    # If the averaging succeeded, we can delete the full-resolution MS
    # if so desired.
    purgeFullMS=${PURGE_FULL_MS}
    if [ \${purgeFullMS} == true ]; then
        lfs find $msSci -type f -print0 | xargs -0 munlink
        find $msSci -type l -delete
        find $msSci -depth -type d -empty -delete
    fi
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
	DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI")
	ID_AVERAGE_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_AVERAGE_SCI}" "Averaging beam ${BEAM} of the science observation, with flags \"$DEP\""
    else
	echo "Would average beam ${BEAM} of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

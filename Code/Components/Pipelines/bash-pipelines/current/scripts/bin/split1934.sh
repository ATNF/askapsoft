#!/bin/bash -l
#
# Launches a job to extract the appropriate beam & scan combination
# from the 1934-638 observation
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

ID_SPLIT_1934=""

# Get the name of the 1934 dataset, replacing any %b with the beam
# number if necessary
find1934MSnames

if [ "$ms1934list" == "" ]; then 
    ms1934list="$msCal"
else
    ms1934list="$ms1934list,$msCal"
fi

DO_IT=$DO_SPLIT_1934

if [ -e "${OUTPUT}/${msCal}" ]; then
    if [ "${CLOBBER}" != "true" ]; then
        # If we aren't clobbering files, don't run anything
        if [ "${DO_IT}" == "true" ]; then
            echo "MS ${msCal} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ "${DO_IT}" == "true" ]; then
            rm -rf "${OUTPUT}/${msCal}"
            rm -f "${FLAG_1934_CHECK_FILE}"
        fi
    fi
fi

if [ "${DO_IT}" == "true" ]; then

    # Define the input MS. If there is a single MS in the archive
    # directory, just use MS_INPUT_1934. If there is more than one, we
    # assume it is split per beam. Unlike the science case, we don't
    # compare the number of beams with the number of MSs, we just use
    # the list as presented. We add the beam number to the root MS
    # name.

    if [ $numMS1934 -eq 1 ]; then
        inputMS=${MS_INPUT_1934}
    else
        msroot=$(for ms in ${msnames1934}; do echo $ms; done | sed -e 's/[0-9]*[0-9].ms//g' | uniq)
        inputMS=$(echo $msroot $BEAM | awk '{printf "%s%d.ms\n",$1,$2}')
        if [ ! -e "${inputMS}" ]; then
            echo "ERROR - wanted to use $inputMS as input for beam $BEAM"
            echo "      -  but it does not exist! Exiting."
            exit 1
        fi
    fi
        
    if [ "${DO_FIND_BANDPASS}" == "true" ] && [ -e "${TABLE_BANDPASS}" ]; then
        # If we are splitting and the user wants to find the bandpass,
        # remove any existing bandpass table so that we will be able
        # to create a new one
        echo "Removing the bandpass table so we can recompute"
        rm -rf "${TABLE_BANDPASS}" "${TABLE_BANDPASS}".smooth
    fi

    if [ -e "${FLAG_1934_CHECK_FILE}" ]; then
        echo "Removing check file for flagging"
        rm -f "${FLAG_1934_CHECK_FILE}"
    fi
    
    setJob split_1934 split
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPLIT_1934}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-split1934-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

parset=${parsets}/split_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.in
cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${inputMS}

# Output measurement set
# Default: <no default>
outputvis   = ${msCal}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = ${CHAN_RANGE_1934}

# Beam selection via beam ID
# Select just a single beam for this obs
beams        = [${BEAM}]

# Scan selection for the 1934-638 observation. Assume the scan ID matches the beam ID
scans        = [${BEAM}]

# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}
stman.tilenchan   = ${NUM_CHAN_TO_AVERAGE}
EOFINNER

log=${logs}/split_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${mssplit} -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msCal}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    
    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
	ID_SPLIT_1934=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_SPLIT_1934}" "Splitting beam ${BEAM} of 1934-638 observation"
    else
	echo "Would run splitting ${BEAM} of 1934-638 observation with slurm file $sbatchfile"
    fi

fi



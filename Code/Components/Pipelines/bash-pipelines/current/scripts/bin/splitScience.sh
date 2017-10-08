#!/bin/bash -l
#
# Launches a job to extract the science observation from the
# appropriate measurement set, selecting the required beam and,
# optionally, a given scan or field
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

ID_SPLIT_SCI=""

DO_IT=$DO_SPLIT_SCIENCE

if [ -e "${OUTPUT}/${msSci}" ]; then
    if [ "${CLOBBER}" != "true" ]; then
        # If we aren't clobbering files, don't run anything
        if [ "${DO_IT}" == "true" ]; then
            echo "MS ${msSci} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ "${DO_IT}" == "true" ]; then
            rm -rf "${OUTPUT}/${msSci}"
            rm -f "${FLAG_CHECK_FILE}"
            rm -f "${BANDPASS_CHECK_FILE}"
        fi
    fi
fi

if [ -e "${OUTPUT}/${msSciAv}" ] && [ "${PURGE_FULL_MS}" == "true" ]; then
    # If we are purging the full MS, that means we don't need it.
    # If the averaged one works, we don't need to run the splitting
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    # Define the input MS. If there is a single MS in the archive
    # directory, just use MS_INPUT_SCIENCE. If there is more than one,
    # we assume it is split per beam. We compare the number of beams
    # with the number of MSs, giving an error if they are
    # different. If OK, then we add the beam number to the root MS
    # name.

    if [ $numMSsci -eq 1 ]; then
        inputMS=${MS_INPUT_SCIENCE}
    elif [ $numMSsci -eq ${NUM_BEAMS_FOOTPRINT} ]; then
        msroot=$(for ms in ${msnamesSci}; do echo $ms; done | sed -e 's/[0-9]*[0-9].ms//g' | uniq)
        inputMS=$(echo $msroot $BEAM | awk '{printf "%s%d.ms\n",$1,$2}')
        if [ ! -e "${inputMS}" ]; then
            echo "ERROR - wanted to use $inputMS as input for beam $BEAM"
            echo "      -  but it does not exist! Exiting."
            exit 1
        fi
    else
        echo "ERROR - have more than one MS, but not the same number as the number of beams."
        echo "      - this is unexpected. Exiting."
        exit 1
    fi
        
    if [ "${SCAN_SELECTION_SCIENCE}" == "" ]; then
	scanParam="# No scan selection done"
    else
        if [ "$(echo "${SCAN_SELECTION_SCIENCE}" | awk -F'[' '{print NF}')" -gt 1 ]; then
	    scanParam="scans        = ${SCAN_SELECTION_SCIENCE}"
        else
            scanParam="scans        = [${SCAN_SELECTION_SCIENCE}]"
        fi
    fi

    if [ "${CHAN_RANGE_SCIENCE}" == "" ]; then
        channelParam="channel     = 1-${NUM_CHAN_SCIENCE}"
    else
        channelParam="channel     = ${CHAN_RANGE_SCIENCE}"
    fi

    # Select only the current field
    fieldParam="fieldnames   = ${FIELD}"


    # If only a single field, and no channel or scan selection, and we are in the MS-per-beam mode, then we can just copy the MS without needing a split

    if [ ${NUM_FIELDS} -eq 1 ] &&
           [ "${CHAN_RANGE_SCIENCE}" == "" ] &&
           [ "${SCAN_SELECTION_SCIENCE}" == "" ] &&
           [ $numMSsci -eq ${NUM_BEAMS_FOOTPRINT} ]; then

        # copy $inputMS to $msSci

        setJob copy_science copy
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPLIT_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-copySci-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

STARTTIME=$(date +%FT%T)
log=${logs}/copyMS_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log
cat > \$log <<EOFINNER
Log file for copying input measurement set :
inputMS=${inputMS}
outputMS=${OUTPUT}/${msSci}

EOFINNER

/usr/bin/time -p -o \$log ssh hpc-data "module load mpifileutils; mpirun -np 4 dcp $inputMS ${OUTPUT}/$msSci"

err=\$?
if [ "\$?" -eq 0 ]; then
    RESULT_TXT="OK"
else
    RESULT_TXT="FAIL"
fi

REALTIME=\$(grep real \$log | awk '{print \$2}')
USERTIME=\$(grep user \$log | awk '{print \$2}')
SYSTIME=\$(grep sys \$log | awk '{print \$2}')

for format in csv txt; do
    if [ "\$format" == "txt" ]; then
        output="${stats}/stats-\${SLURM_JOB_ID}-${jobname}.txt"
    elif [ "\$format" == "csv" ]; then
        output="${stats}/stats-\${SLURM_JOB_ID}-${jobname}.csv"
    fi
    writeStats "\$SLURM_JOB_ID" 1 "${jobname}"  "\$RESULT_TXT" "\$REALTIME" "\$USERTIME" "\$SYSTIME" 0. 0.  "\$STARTTIME" "\$format" >> "\$output"
done


EOFOUTER

        if [ "${SUBMIT_JOBS}" == "true" ]; then
            DEP=""
            DEP=$(addDep "$DEP" "$DEP_START")
	    ID_SPLIT_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	    recordJob "${ID_SPLIT_SCI}" "Copying beam ${BEAM} of science observation"
        else
	    echo "Would run copying for ${BEAM} of science observation with slurm file $sbatchfile"
        fi

       
    else

        # need to run mssplit
    
        setJob split_science split
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPLIT_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-splitSci-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

parset=${parsets}/split_science_${FIELDBEAM}_\${SLURM_JOB_ID}.in
cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${inputMS}

# Output measurement set
# Default: <no default>
outputvis   = ${msSci}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
$channelParam

# Beam selection via beam ID
# Select an individual beam
beams        = [${BEAM}]

# Scan selection for the science observation
$scanParam

# Field selection for the science observation
$fieldParam

# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}
# Make the tile size 54 channels, as that is what we will average over
stman.tilenchan   = ${NUM_CHAN_TO_AVERAGE}
EOFINNER

log=${logs}/split_science_${FIELDBEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${mssplit} -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msSci}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    
        if [ "${SUBMIT_JOBS}" == "true" ]; then
            DEP=""
            DEP=$(addDep "$DEP" "$DEP_START")
	    ID_SPLIT_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	    recordJob "${ID_SPLIT_SCI}" "Splitting beam ${BEAM} of science observation"
        else
	    echo "Would run splitting ${BEAM} of science observation with slurm file $sbatchfile"
        fi

    fi
        
fi

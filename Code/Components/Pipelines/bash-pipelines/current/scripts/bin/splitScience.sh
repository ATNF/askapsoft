#!/bin/bash -l
#
# Launches a job to extract the science observation from the
# appropriate measurement set, selecting the required beam and,
# optionally, a given scan or field
#
# @copyright (c) 2016 CSIRO
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

if [ -e ${OUTPUT}/${msSci} ]; then
    if [ $CLOBBER == false ]; then
        # If we aren't clobbering files, don't run anything
        if [ $DO_IT == true ]; then
            echo "MS ${msSci} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ $DO_IT == true ]; then
            rm -rf ${OUTPUT}/${msSci}
            rm -f ${FLAG_CHECK_FILE}
            rm -f ${BANDPASS_CHECK_FILE}
        fi
    fi
fi

if [ $DO_IT == true ]; then

    if [ "$SCAN_SELECTION_SCIENCE" == "" ]; then
	scanParam="# No scan selection done"
    else
        if [ `echo ${SCAN_SELECTION_SCIENCE} | awk -F'[' '{print NF}'` -gt 1 ]; then
	    scanParam="scans        = ${SCAN_SELECTION_SCIENCE}"
        else
            scanParam="scans        = [${SCAN_SELECTION_SCIENCE}]"
        fi
    fi

    # Select only the current field
    fieldParam="fieldnames   = ${FIELD}"
    
    sbatchfile=$slurms/split_science_${FIELDBEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPLIT_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=split_${FIELDBEAMJOB}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-splitSci-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/split_science_${FIELDBEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${MS_INPUT_SCIENCE}

# Output measurement set
# Default: <no default>
outputvis   = ${msSci}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = ${CHAN_RANGE_SCIENCE}

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
aprun -n \${NCORES} -N \${NPPN} ${mssplit} -c \${parset} > \${log}
err=\$?
rejuvenate ${msSci}
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} splitScience_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    
    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
	ID_SPLIT_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SPLIT_SCI} "Splitting beam ${BEAM} of science observation"
    else
	echo "Would run splitting ${BEAM} of science observation with slurm file $sbatchfile"
    fi

fi

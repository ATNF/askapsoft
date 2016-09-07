#!/bin/bash -l
#
# Launches a job to extract the appropriate beam & scan combination
# from the 1934-638 observation
#
# @copyright (c) 2015 CSIRO
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

if [ -e ${OUTPUT}/${msCal} ]; then
    if [ $CLOBBER == false ]; then
        # If we aren't clobbering files, don't run anything
        if [ $DO_IT == true ]; then
            echo "MS ${msCal} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ $DO_IT == true ]; then
            rm -rf ${OUTPUT}/${msCal}
            rm -f ${FLAG_1934_CHECK_FILE}
        fi
    fi
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/split_1934_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPLIT_1934}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=splitCal${BEAM}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-split1934-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/split_1934_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${MS_INPUT_1934}

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

log=${logs}/split_1934_beam${BEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${mssplit} -c \${parset} > \${log}
err=\$?
rejuvenate ${msCal}
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} split1934_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    
    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
	ID_SPLIT_1934=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SPLIT_1934} "Splitting beam ${BEAM} of 1934-638 observation"
    else
	echo "Would run splitting ${BEAM} of 1934-638 observation with slurm file $sbatchfile"
    fi

fi



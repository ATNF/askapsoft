#!/bin/bash -l
#
# This script submits a job to gather all statistics files for the
# processing jobs into a single file (this is done at the end to avoid
# race conditions). 
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

##############################

if [ $SUBMIT_JOBS == true ] && [ "${ALL_JOB_IDS}" != "" ]; then

    # Gather stats on all running jobs

    sbatchfile=$slurms/gatherAll.sbatch
    cat > $sbatchfile <<EOF
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=0:10:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=gatherStats
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-gatherAll-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

statsTXT=stats-all-${NOW}.txt
statsCSV=stats-all-${NOW}.csv
writeStatsHeader txt > \$statsTXT
writeStatsHeader csv > \$statsCSV
for i in `echo $ALL_JOB_IDS | sed -e 's/,/ /g'`; do
    for file in \`\ls $stats/stats-\$i*.txt\`; do
        grep -v JobID \$file >> \$statsTXT
    done
    for file in \`\ls $stats/stats-\$i*.csv\`; do
        grep -v JobID \$file >> \$statsCSV
    done
done
EOF

    if [ $SUBMIT_JOBS == true ]; then    
        dep="-d afterany:`echo $ALL_JOB_IDS | sed -e 's/,/:/g'`"
        ID_STATS=`sbatch ${dep} $sbatchfile | awk '{print $4}'`
        recordJob ${ID_STATS} "Final job to gather statistics on all jobs, with flags \"${dep}\""
    else
        echo "Would submit job to gather statistics based on all jobs, with slurm file $sbatchfile"
    fi

fi


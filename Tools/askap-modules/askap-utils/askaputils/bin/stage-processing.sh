#!/bin/bash -l
#
# Script to start processing on the completion of a "restore-sbid"
# job.
# The assumption is that an askapops user has started a restore-sbid
# job that appears on the slurm queue for zeus with the job name
# "restoreXXXX" where XXXX is the SB id.
#
# This script is run with the job ID of the restore job and the name
# of the pipeline config file, and it will launch a zeus job to
# commence the pipeline once the restore job has finished.
#
# ----
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

if [ $# -ne 2 ]; then

    echo " "
    echo "Usage: stage-processing.sh <configFile> <jobID>"
    echo "     configFile = pipeline configuration file to be passed to"
    echo "          the ASKAP pipeline script processASKAP.sh"
    echo "     jobID = slurm job ID(s) of a restoreXXXX job running on"
    echo "          zeus. If you are waiting on more than one job,"
    echo "          separate the individual job ids by colons"
    echo "          (eg. \"1234:1235\") "

    echo "This script launches a zeus job to start pipeline processing upon the"
    echo "successful completion of one or more RESTORE jobs (these are jobs to"
    echo "restore the measurement set of a scheduling block from the"
    echo "commissioning archive). "
    echo " "
    echo "The command-line arguments should be the name of the configuration"
    echo "file to be used in starting the pipeline, and the job ID(s) of the"
    echo "restore job."
    echo " "
    echo "Example:   stage-processing.sh myconfig.sh 203456:203567"
    echo " "
    echo "The script will also be able to detect if the restore job fails, by"
    echo "launching a second zeus job. If it does fail, a file will be written"
    echo "called RESTORE_FAILED_<jobID>_ID (where the second ID is the slurm ID"
    echo "of the second job)."
    echo " "
    echo "Note that any parsing of the configuration file is left up to"
    echo "processASKAP.sh, so running this is no guarantee that your processing"
    echo "will run!."
    echo " "

    exit 0

else

    #########
    # Pipeline configuration file that is passed to processASKAP.sh
    configfile="$1"

    if [ ! -e ${configfile} ]; then

        echo "ERROR - configFile ${configfile} does not exist."
        echo "   Please try again."
        exit 1

    fi

    #########
    # Job ID(s) of the restore jobs currently running
    jobid="$2"

    #########
    # Define the dependency string for checking if one or more of the
    # restore jobs have failed.

    faildep=""
    for id in `echo ${jobid} | sed -e 's/:/ /g'`; do
        if [ "${faildep}" == "" ]; then
            faildep="afternotok:${id}"
        else
            faildep="${faildep}?afternotok:${id}"
        fi
    done

    #########
    # Define the modules for launching the pipeline. Use the default
    # askappipeline module, unless the user has a different one
    # defined in their current environment
    
    pipelinemodule=`module list -t 2>&1 | grep askappipeline`
    if [ "${pipelinemodule}" == "" ]; then
        modulesetup="module load askappipeline"
    else
        modulesetup="# Loading the correct pipeline module
if [ \"\`module list -t 2>&1 | grep askappipeline\`\" == \"\" ]; then
    module load $pipelinemodule
else
    module swap askappipeline $pipelinemodule
fi"
    fi

    #########
    #  Define and run the slurm job that checks for failures of any of
    #  the restore jobs provided
    
    failureSlurmfile="checkRestoreFailed-${jobid}.slurm"
    cat > $failureSlurmfile <<EOF
#!/bin/bash -l
#SBATCH --cluster=zeus
#SBATCH --partition=copyq
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --time=0:10:00
#SBATCH --job-name=checkRestore
#SBATCH --output=RESTORE_FAILED_${jobid}_%j
#SBATCH --export=NONE

now=\`date +%F-%H%M\`
echo "\${now}"
echo "Restore job ${jobid} failed, so have not started the pipeline."
EOFINNER

EOF
    failjobid=`sbatch -M zeus -d ${faildep} ${failureSlurmfile} | awk '{print $4}'`
    
    #########
    #  Define and run the slurm job that runs processASKAP.sh upon
    #  completion of the restore job(s).

    slurmfile="launchPipeline-${configfile}-${jobid}.slurm"
    cat > $slurmfile <<EOF
#!/bin/bash -l
#SBATCH --cluster=zeus
#SBATCH --partition=copyq
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --time=0:10:00
#SBATCH --job-name=launchPipeline
#SBATCH --output=slurm-launchPipeline-%j.out
#SBATCH --export=NONE

module use /group/askap/modulefiles
${modulesetup}

processASKAP.sh -c ${configfile}

EOF

    myjobid=`sbatch -M zeus -d afterok:${jobid} ${slurmfile} | awk '{print $4}'`
    echo "Submitted pipeline-launching job on zeus as job ID ${myjobid}"

fi

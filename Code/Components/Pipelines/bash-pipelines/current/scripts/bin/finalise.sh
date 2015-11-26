#!/usr/bin/env bash
#
# This script takes care of tasks that need to be run either upon
# completion of all jobs from a single processBETA.sh call, or can be
# run once all jobs have been submitted to the queue (and hence their
# job IDs are known). 
# Included here are:
#  * Creation of a user script to indicate progress of queued jobs.
#  * Creation of a user script to kill all launched jobs
#  * Submission of a job to gather all statistics files into a single
#  file (this is done at the end to avoid race conditions).
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

##############################

if [ $SUBMIT_JOBS == true ]; then

    # Gather stats on all running jobs

    sbatchfile=$slurms/gatherAll.sbatch
    cat > $sbatchfile <<EOF
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=0:10:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=gatherStats
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-gatherAll-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

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

    ##############################

    # Create tool to report progress

    reportscript=${tools}/reportProgress-${NOW}.sh
    cat > $reportscript <<EOF
#!/usr/bin/env bash
#
# A script to report progress on all jobs associated with a given call of processBETA.sh.
# It will run squeue just on the appropriate job IDs, showing those that have not yet completed.
# Providing the '-v' option will also show a list of jobs with detailed descriptions.
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

verbose="false"
while getopts ':v' opt
do
    case \$opt in
	v) verbose="true";;
	\?) echo "ERROR: Invalid option"
	    echo "   Usage: \$0 [-v]"
	    echo "     -v : verbose mode, shows list of jobs with descriptions"
	    exit 1;;
    esac
done

if [ \$verbose == "true" ]; then

    echo "SUMMARY OF JOBS:"
    cat $JOBLIST
    echo " "

fi

squeue --jobs=${ALL_JOB_IDS}

EOF
    # make it executable
    chmod a+x $reportscript

    ##############################

    # Create tool to kill all jobs with this datestamp
    killscript=${tools}/killAll-${NOW}.sh
    cat > $killscript <<EOF
#!/usr/bin/env bash
#
# A simple script to run scancel on all jobs associated with a 
# given call of processBETA.sh
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

echo "This will run scancel on all remaining jobs (run 'reportProgress.sh' to see them)."
read -p "Are you sure? (type yes to continue) : " ANS

if [ "\$ANS" == "yes" ]; then

    scancel `echo $ALL_JOB_IDS | sed -e 's/,/ /g'`

else

    echo "OK, nothing cancelled"

fi
EOF
    # make it executable
    chmod a+x $killscript

    ##############################

    # Make symbolic links that don't have the timestamp
    function makeLink()
    {
        # first, remove the timestamp
        sedstr="s/-${NOW}//g"
        link=`echo $1 | sed -e $sedstr`
        # remove trailing suffix
        link=`echo ${link%.*}`
        # remove all leading paths
        link=`echo ${link##*/}`
        # clobber any existing link
        if [ -e $link ]; then
	    rm -f $link
        fi
        # make the link
        ln -s $1 $link
    }

    # make links to the current scripts
    makeLink $reportscript 
    makeLink $killscript

    # make links to the current job list, both at the top level *and*
    # in the output directory
    makeLink $JOBLIST
    cd $OUTPUT
    makeLink $JOBLIST
    cd ..


fi

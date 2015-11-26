#!/usr/bin/env bash
#
# Launches a job to average the measurement set for the current beam
# of the science observation so that it can be imaged by the continuum
# imager. 
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

ID_AVERAGE_SCI=""

DO_IT=$DO_AVERAGE_CHANNELS

cd $OUTPUT
if [ $DO_IT == true ] && [ -e ${msSciAv} ]; then
    if [ $CLOBBER == false ]; then
        # If we aren't clobbering files, don't run anything
        if [ $DO_IT == true ]; then
            echo "MS ${msSciAv} exists, so not running averaging for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ $DO_IT == true ]; then
            rm -rf ${msSciAv}
        fi
    fi
fi
cd $CWD

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/science_average_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=avSci${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-averageSci-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/science_average_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
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

EOFINNER

log=${logs}/science_average_beam${BEAM}_\${SLURM_JOB_ID}.log

aprun -n 1 -N 1 $mssplit -c \${parset} > \${log}
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} avScience_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
	DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
	if [ "$ID_CCALAPPLY_SCI" != "" ]; then
	    DEP="-d afterok:${ID_CCALAPPLY_SCI}"
	fi	
	ID_AVERAGE_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_AVERAGE_SCI} "Averaging beam ${BEAM} of the science observation, with flags \"$DEP\""
    else
	echo "Would average beam ${BEAM} of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

#!/usr/bin/env bash
#
# Launches a job to apply the bandpass solution to the measurement set 
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

ID_CCALAPPLY_SCI=""

DO_IT=$DO_APPLY_BANDPASS
if [ $DO_IT == true ] && [ -e $BANDPASS_CHECK_FILE ]; then
    echo "Bandpass has already been applied to beam $BEAM of the science observation - not re-doing"
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/ccalapply_science_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=calapply${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-applyBandpass-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/ccalapply_bp_\${SLURM_JOB_ID}.in
cat > \$parset << EOFINNER
Ccalapply.dataset                             = ${msSci}
#
# Allow flagging of vis if inversion of Mueller matrix fails
Ccalapply.calibrate.allowflag                 = true
#
Ccalapply.calibaccess                     = table
Ccalapply.calibaccess.table.maxant        = 6
Ccalapply.calibaccess.table.maxbeam       = ${nbeam}
Ccalapply.calibaccess.table.maxchan       = ${NUM_CHAN_SCIENCE}
Ccalapply.calibaccess.table               = ${TABLE_BANDPASS}

EOFINNER

log=${logs}/ccalapply_bp_\${SLURM_JOB_ID}.log

aprun -n 1 -N 1 ${ccalapply} -c \$parset > \$log
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} calapply_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $BANDPASS_CHECK_FILE
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
	DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_CBPCAL"`
	ID_CCALAPPLY_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_CCALAPPLY_SCI} "Applying bandpass calibration to science observation, with flags \"$DEP\""
    else
	echo "Would apply bandpass calibration to science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

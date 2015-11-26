#!/usr/bin/env bash
#
# Launches a job to solve for the bandpass calibration. This uses all
# 1934-638 measurement sets after the splitFlag1934.sh jobs have
# completed. The bandpass calibration is done assuming the special
# '1934-638' component.
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

ID_CBPCAL=""

DO_IT=$DO_FIND_BANDPASS

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${TABLE_BANDPASS} ]; then
    if [ $DO_IT == true ]; then
        echo "Bandpass table ${TABLE_BANDPASS} exists, so not running cbpcalibrator"
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/cbpcalibrator_1934.sbatch
    cat > $sbatchfile <<EOF
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=${NUM_CPUS_CBPCAL}
#SBATCH --ntasks-per-node=20
#SBATCH --job-name=cbpcal
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-findBandpass-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/cbpcalibrator_1934_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
Cbpcalibrator.dataset                         = [${ms1934list}]
Cbpcalibrator.nAnt                            = 6
Cbpcalibrator.nBeam                           = ${nbeam}
Cbpcalibrator.nChan                           = ${NUM_CHAN_1934}
Cbpcalibrator.refantenna                      = 1
#
Cbpcalibrator.calibaccess                     = table
Cbpcalibrator.calibaccess.table.maxant        = 6
Cbpcalibrator.calibaccess.table.maxbeam       = ${nbeam}
Cbpcalibrator.calibaccess.table.maxchan       = ${NUM_CHAN_1934}
Cbpcalibrator.calibaccess.table               = ${TABLE_BANDPASS}
#
Cbpcalibrator.sources.names                   = [field1]
Cbpcalibrator.sources.field1.direction        = ${DIRECTION_1934}
Cbpcalibrator.sources.field1.components       = src
Cbpcalibrator.sources.src.calibrator          = 1934-638
#
Cbpcalibrator.gridder                         = SphFunc
#
Cbpcalibrator.ncycles                         = ${NCYCLES_BANDPASS_CAL}

EOFINNER

log=${logs}/cbpcalibrator_1934_\${SLURM_JOB_ID}.log

aprun -n 343 -N 20 ${cbpcalibrator} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} findBandpass "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOF

    if [ $SUBMIT_JOBS == true ]; then
        ID_CBPCAL=`sbatch ${FLAG_CBPCAL_DEP} $sbatchfile | awk '{print $4}'`
        recordJob ${ID_CBPCAL} "Finding bandpass calibration with 1934-638, with flags \"$FLAG_CBPCAL_DEP\""
    else
        echo "Would find bandpass calibration with slurm file $sbatchfile"
    fi

    echo " "

fi

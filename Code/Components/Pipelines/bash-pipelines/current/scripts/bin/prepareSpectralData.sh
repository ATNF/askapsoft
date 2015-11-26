#!/usr/bin/env bash
#
# Prepares the dataset to be used for spectral-line imaging. Includes
# copying, allowing a selection of a subset of channels, application
# of the gains calibration table from the continuum self-calibration,
# and subtraction of the continuum flux
#
# (c) Matthew Whiting, CSIRO ATNF, 2015

########
# Use mssplit to make a copy of the dataset prior to applying the
# calibration & subtracting the continuum

# Define a few parameters related to the continuum imaging
. ${SCRIPTDIR}/getContinuumCimagerParams.sh

ID_SPLIT_SL_SCI=""
ID_CAL_APPLY_SL_SCI=""
ID_CONT_SUB_SL_SCI=""

DO_IT=${DO_COPY_SL}
if [ -e ${OUTPUT}/${msSciSL} ]; then
    if [ $CLOBBER == false ]; then
        # If we aren't clobbering files, don't run anything
        if [ $DO_IT == true ]; then
            echo "MS ${msSciSL} exists, so not making spectral-line dataset for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ $DO_IT == true ]; then
            rm -rf ${OUTPUT}/${msSciSL}
            rm -f ${SL_GAINS_CHECK_FILE}
            rm -f ${CONT_SUB_CHECK_FILE}
        fi
    fi
fi


if [ $DO_IT == true ]; then

    sbatchfile=$slurms/split_spectralline_science_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=splitSLsci${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-splitSLsci-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/split_spectralline_science_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${msSci}

# Output measurement set
# Default: <no default>
outputvis   = ${msSciSL}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = ${CHAN_RANGE_SL_SCIENCE}

# Set a larger bucketsize
stman.bucketsize  = 65536
# Allow a different tile size for the spectral-line imaging
stman.tilenchan   = ${TILENCHAN_SL}
EOFINNER

log=${logs}/split_spectralline_science_beam${BEAM}_\${SLURM_JOB_ID}.log

aprun -n 1 -N 1 ${mssplit} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} splitScience_B${BEAM} "txt,csv"
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
        ID_SPLIT_SL_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
        recordJob ${ID_SPLIT_SL_SCI} "Copy the required spectral-line dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
    else
        echo "Would copy the required spectral-line dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi


##################
# Apply gains calibration

DO_IT=$DO_APPLY_CAL_SL
if [ -e $SL_GAINS_CHECK_FILE ]; then
    if [ $DO_IT == true ]; then
        echo "Application of gains solution to spectral-line data for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/apply_cal_spectralline_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=calappSLsci${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-calappSLsci-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/apply_cal_spectralline_beam${BEAM}_\${SLURM_JOB_ID}.in
log=${logs}/apply_cal_spectralline_beam${BEAM}_\${SLURM_JOB_ID}.log
cat > \$parset <<EOFINNER
Ccalapply.dataset                             = ${msSciSL}
#
# Allow flagging of vis if inversion of Mueller matrix fails
Ccalapply.calibrate.allowflag                 = true
#
Ccalapply.calibaccess                     = table
Ccalapply.calibaccess.table.maxant        = 6
Ccalapply.calibaccess.table.maxbeam       = ${nbeam}
Ccalapply.calibaccess.table.maxchan       = ${nchanContSci}
Ccalapply.calibaccess.table               = ${gainscaltab}
EOFINNER

aprun -n 1 -N 1 ${ccalapply} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} calapply_spectral_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $SL_GAINS_CHECK_FILE
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        submitIt=true
        if [ $DO_CONT_IMAGING != true ] || [ $DO_SELFCAL != true ]; then
            # If we aren't creating the gains cal table with a self-cal job, then check to see if it exists.
            # If it doesn't, we can't run this job.
            if [ ! -e "${gainscaltab}" ]; then
                submitIt=false
                echo "Not submitting gains calibration of spectral-line dataset as gains table ${gainscaltab} doesn't exist"
            fi
        fi
        if [ $submitIt == true ]; then
            DEP=""
            DEP=`addDep "$DEP" "$DEP_START"`
            DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
            DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
            DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
            DEP=`addDep "$DEP" "$ID_SPLIT_SL_SCI"`
            ID_CAL_APPLY_SL_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
            recordJob ${ID_CAL_APPLY_SL_SCI} "Apply gains calibration to the spectral-line dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
        fi
    else
        echo "Would apply gains calibration to the spectral-line dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

##################
# Subtract continuum model

DO_IT=$DO_CONT_SUB_SL
if [ -e $CONT_SUB_CHECK_FILE ]; then
    if [ $DO_IT == true ]; then
        echo "Continuum subtraction from spectral-line data for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ ${DO_CONT_SUB_SL} == "true" ]; then

    modelImage=image.${imageBase}

    sbatchfile=$slurms/contsub_spectralline_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=contsubSLsci${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-contsubSLsci-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

if [ "${DIRECTION_SCI}" != "" ]; then
    modelDirection="${DIRECTION_SCI}"
else
    log=${logs}/mslist_for_ccontsub_\${SLURM_JOB_ID}.log
    aprun -n 1 -N 1 $mslist --full ${msSciSL} 1>& \${log}
    ra=\`grep -A1 RA \$log | tail -1 | awk '{print \$7}'\`
    dec=\`grep -A1 RA \$log | tail -1 | awk '{print \$8}'\`
    eq=\`grep -A1 RA \$log | tail -1 | awk '{print \$9}'\`
    modelDirection="[\${ra}, \${dec}, \${eq}]"
fi

parset=${parsets}/contsub_spectralline_beam${BEAM}_\${SLURM_JOB_ID}.in
log=${logs}/contsub_spectralline_beam${BEAM}_\${SLURM_JOB_ID}.log
cat > \$parset <<EOFINNER
# The measurement set name - this will be overwritten
CContSubtract.dataset                             = ${msSciSL}
# The model definition
CContSubtract.sources.names                       = [lsm]
CContSubtract.sources.lsm.direction               = \${modelDirection}
CContSubtract.sources.lsm.model                   = ${modelImage}
CContSubtract.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}
# The gridding parameters
CContSubtract.gridder.snapshotimaging             = ${GRIDDER_SNAPSHOT_IMAGING}
CContSubtract.gridder.snapshotimaging.wtolerance  = ${GRIDDER_SNAPSHOT_WTOL}
CContSubtract.gridder                             = WProject
CContSubtract.gridder.WProject.wmax               = ${GRIDDER_WMAX}
CContSubtract.gridder.WProject.nwplanes           = ${GRIDDER_NWPLANES}
CContSubtract.gridder.WProject.oversample         = ${GRIDDER_OVERSAMPLE}
CContSubtract.gridder.WProject.maxfeeds           = 1
CContSubtract.gridder.WProject.maxsupport         = ${GRIDDER_MAXSUPPORT}
CContSubtract.gridder.WProject.frequencydependent = true
CContSubtract.gridder.WProject.variablesupport    = true
CContSubtract.gridder.WProject.offsetsupport      = true
EOFINNER

aprun -n 1 -N 1 ${ccontsubtract} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} contsub_spectral_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $CONT_SUB_CHECK_FILE
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SL_SCI"`
        DEP=`addDep "$DEP" "$ID_CAL_APPLY_SL_SCI"`
        ID_CONT_SUB_SL_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
        recordJob ${ID_CONT_SUB_SL_SCI} "Subtract the continuum model from the spectral-line dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
    else
        echo "Would subtract the continuum model from the spectral-line dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "
    
fi



#!/usr/bin/env bash
#
# Launches a job to image the current beam of the science
# observation, using the BasisfunctionMFS solver. This imaging
# involves self-calibration, whereby the image is searched with Selavy
# to produce a component catalogue, which is then used by Ccalibrator
# to calibrate the gains, before running Cimager again. This is done a
# number of times to hopefully converge to a sensible image.
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

# Define the Cimager parset and associated parameters
. ${SCRIPTDIR}/getContinuumCimagerParams.sh

ID_CONTIMG_SCI=""

DO_IT=$DO_CONT_IMAGING

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${outputImage} ]; then
    if [ $DO_IT == true ]; then
        echo "Image ${outputImage} exists, so not running continuum imaging for beam ${BEAM}"
    fi
    DO_IT=false
fi

if [ $DO_IT == true ] && [ $DO_SELFCAL == true ]; then

    if [ $NUM_CPUS_CONTIMG_SCI -lt 19 ]; then
	NUM_CPUS_SELFCAL=19
    else
	NUM_CPUS_SELFCAL=$NUM_CPUS_CONTIMG_SCI
    fi

    NPROCS_SELAVY=`echo $SELFCAL_SELAVY_NSUBX $SELFCAL_SELAVY_NSUBY | awk '{print $1*$2+1}'`
    if [ ${CPUS_PER_CORE_CONT_IMAGING} -lt ${NPROCS_SELAVY} ]; then
	CPUS_PER_CORE_SELFCAL=${CPUS_PER_CORE_CONT_IMAGING}
    else
	CPUS_PER_CORE_SELFCAL=${NPROCS_SELAVY}
    fi

    if [ $NUM_TAYLOR_TERMS == 1 ]; then
        selavyImage=${OUTPUT}/image.${imageBase}.restored
    else
        selavyImage=${OUTPUT}/image.${imageBase}.taylor.0.restored
    fi

    sbatchfile=$slurms/science_continuumImageSelfcal_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=${NUM_CPUS_SELFCAL}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONT_IMAGING}
#SBATCH --job-name=cleanSC${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-contImagingSelfcal-%j.out

cd $OUTPUT
. ${SCRIPTDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

caldir=selfCal_${imageBase}
mkdir -p \$caldir

copyImages=${SELFCAL_KEEP_IMAGES}

for((LOOP=0;LOOP<=${SELFCAL_NUM_LOOPS};LOOP++)); do

    loopdir=\${caldir}/Loop\${LOOP}
    sources=sources_loop\${LOOP}.in
    caldata=caldata_loop\${LOOP}.tab

    if [ \${LOOP} -gt 0 ]; then
        mkdir -p \${loopdir}
        calparams="# Self-calibration using the recently-generated cal table
Cimager.calibrate                           = true
Cimager.calibaccess                         = table
Cimager.calibaccess.table                   = \${loopdir}/\${caldata}
Cimager.calibaccess.table.maxant            = 6
Cimager.calibaccess.table.maxbeam           = ${nbeam}
Cimager.calibaccess.table.maxchan           = ${nchanContSci}
Cimager.calibaccess.table.reuse             = false
"
    else
        calparams="# No self-calibration as it is the first time around the loop
Cimager.calibrate                               = false
"
    fi

    parset=${parsets}/science_imagingSelfcal_beam${BEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}.in
    log=${logs}/science_imagingSelfcal_beam${BEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}.log

    cat > \$parset <<EOFINNER
##########
## Continuum imaging with cimager
##
${cimagerParams}
#
\${calparams}
#
EOFINNER
    if [ \${LOOP} -gt 0 ]; then
            cat >> \$parset <<EOFINNER
##########
## Shallow source-finding with selavy
##
# The image to be searched
Selavy.image                                    = ${selavyImage}
#
# This is how we divide it up for distributed processing, with the
#  number of subdivisions in each direction, and the size of the
#  overlap region in pixels
Selavy.nsubx                                    = ${SELFCAL_SELAVY_NSUBX}
Selavy.nsuby                                    = ${SELFCAL_SELAVY_NSUBY}
Selavy.overlapx                                 = 50
Selavy.overlapy                                 = 50
#
# The search threshold, in units of sigma
Selavy.snrCut                                   = ${SELFCAL_SELAVY_THRESHOLD}
# Grow the detections to a secondary threshold
Selavy.flagGrowth                               = true
Selavy.growthCut                                = 5
#
# Turn on the variable threshold option
Selavy.VariableThreshold                        = true
Selavy.VariableThreshold.boxSize                = 50
Selavy.VariableThreshold.ThresholdImageName     = detThresh.img
Selavy.VariableThreshold.NoiseImageName         = noiseMap.img
Selavy.VariableThreshold.AverageImageName       = meanMap.img
Selavy.VariableThreshold.SNRimageName           = snrMap.img
#
# Parameters to switch on and control the Gaussian fitting
Selavy.Fitter.doFit                             = true
# Fit all 6 parameters of the Gaussian
Selavy.Fitter.fitTypes                          = [full]
# Limit the number of Gaussians to 1
Selavy.Fitter.maxNumGauss = 1
# Do not use the number of initial estimates to determine how many Gaussians to fit
Selavy.Fitter.numGaussFromGuess = false
# The fit may be a bit poor, so increase the reduced-chisq threshold
Selavy.Fitter.maxReducedChisq = 15.
#
# Allow islands that are slightly separated to be considered a single 'source'
Selavy.flagAdjacent = false
# The separation in pixels for islands to be considered 'joined'
Selavy.threshSpatial = 7
#
# Saving the fitted components to a parset for use by ccalibrator
Selavy.outputComponentParset                    = \${sources}
# Only use the brightest components in the parset
Selavy.outputComponentParset.maxNumComponents   = 10
#
# Size criteria for the final list of detected islands
Selavy.minPix                                   = 3
Selavy.minVoxels                                = 3
Selavy.minChannels                              = 1
#
# How the islands are sorted in the final catalogue - by
#  integrated flux in this case
Selavy.sortingParam                             = -iflux
#
##########
## Calibration using selavy's component parset
##
# parameters for calibrator
Ccalibrator.dataset                             = ${OUTPUT}/${msSciAv}
Ccalibrator.nAnt                                = 6
Ccalibrator.nBeam                               = 1
Ccalibrator.solve                               = antennagains
Ccalibrator.interval                            = ${SELFCAL_INTERVAL}
#
Ccalibrator.calibaccess                         = table
Ccalibrator.calibaccess.table                   = \${caldata}
Ccalibrator.calibaccess.table.maxant            = 6
Ccalibrator.calibaccess.table.maxbeam           = ${nbeam}
Ccalibrator.calibaccess.table.maxchan           = ${nchanContSci}
Ccalibrator.calibaccess.table.reuse             = false
#
Ccalibrator.sources.definition                  = \${sources}
#
Ccalibrator.gridder.snapshotimaging             = true
Ccalibrator.gridder.snapshotimaging.wtolerance  = 2600
Ccalibrator.gridder                             = WProject
#
Ccalibrator.ncycles                             = 25

EOFINNER
    fi

    echo "=== Continuum imaging with Self-calibration, for beam ${BEAM}, self-cal loop \${LOOP} ===" > \$log

    # Other than for the first loop, run selavy to extract the
    #  component parset and use it to calibrate
    if [ \${LOOP} -gt 0 ]; then
        cd \${loopdir}
        ln -s ${logs} .
        ln -s ${parsets} .

        echo "--- Source finding with $selavy ---" >> \$log
        aprun -n ${NPROCS_SELAVY} -N ${CPUS_PER_CORE_SELFCAL} $selavy -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${SLURM_JOB_ID} \${err} selavySC_L\${LOOP}_B${BEAM} "txt,csv"

        if [ \$err != 0 ]; then
            exit \$err
        fi

        echo "--- Calibration with $ccalibrator ---" >> \$log
        aprun -n 1 -N 1 $ccalibrator -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${SLURM_JOB_ID} \${err} ccalSC_L\${LOOP}_B${BEAM} "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        fi
        cp -r \${caldata} ${OUTPUT}/${gainscaltab}

        # Keep a backup of the intermediate images, prior to re-imaging.
        if [ \${copyImages} == true ]; then
            mv ${OUTPUT}/*${imageBase}* .
        fi

        cd $OUTPUT
    fi

    # Run the imager, calibrating if not the first time.
    echo "--- Imaging with $cimager ---" >> \$log
    aprun -n ${NUM_CPUS_CONTIMG_SCI} -N ${CPUS_PER_CORE_CONT_IMAGING} $cimager -c \$parset >> \$log
    err=\$?
    extractStats \${log} \${SLURM_JOB_ID} \${err} contImagingSC_L\${LOOP}_B${BEAM} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

done

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
	DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
        DEP=`addDep "$DEP" "$ID_AVERAGE_SCI"`
	ID_CONTIMG_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_CONTIMG_SCI} "Make a self-calibrated continuum image for beam $BEAM of the science observation, with flags \"$DEP\""
        FLAG_IMAGING_DEP=`addDep "$FLAG_IMAGING_DEP" "$ID_CONTIMG_SCI"`
    else
	echo "Would make a self-calibrated continuum image for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

#!/bin/bash -l
#
# Launches a job to image the current beam of the science
# observation, using the BasisfunctionMFS solver. This imaging
# involves self-calibration, whereby the image is searched with Selavy
# to produce a component catalogue, which is then used by Ccalibrator
# to calibrate the gains, before running Cimager again. This is done a
# number of times to hopefully converge to a sensible image.
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

# Define the Cimager parset and associated parameters
. ${PIPELINEDIR}/getContinuumCimagerParams.sh

ID_CONTIMG_SCI_SC=""

DO_IT=$DO_CONT_IMAGING

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${imageName} ]; then
    if [ $DO_IT == true ]; then
        echo "Image ${imageName} exists, so not running continuum imaging for beam ${BEAM}"
    fi
    DO_IT=false
fi

if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
    theimager=$altimager
else
    theimager=$cimager
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

    imageCode=restored
    setImageProperties cont
    selavyImage="${OUTPUT}/${imageName}"
    selavyWeights="${OUTPUT}/${weightsImage}"

    cutWeights=`echo ${SELFCAL_SELAVY_WEIGHTSCUT} | awk '{if (($1>0.)&&($1<1.)) print "true"; else print "false";}'`
    if [ ${cutWeights} == "true" ]; then
        selavyWeights="# Use the weights image, with a high cutoff - since we are using
# WProject, everything should be flat. This will reject areas where
# the snapshot warp is reducing the weight.
Selavy.Weights.weightsImage                     = ${selavyWeights}
Selavy.Weights.weightsCutoff                    = ${SELFCAL_SELAVY_WEIGHTSCUT}"
    else
        selavyWeights="# No weights scaling applied in Selavy"
    fi

    modelImage=contmodel.${imageBase}

    if [ "${SELFCAL_METHOD}" == "Cmodel" ]; then

        CmodelParset="##########
## Creation of the model image
##
# The below specifies the GSM source is a selavy output file
Cmodel.gsm.database       = votable
Cmodel.gsm.file           = selavy-results.components.xml
Cmodel.gsm.ref_freq       = ${CENTRE_FREQ}Hz

# General parameters
Cmodel.bunit              = Jy/pixel
Cmodel.frequency          = ${CENTRE_FREQ}Hz
Cmodel.increment          = ${BANDWIDTH}Hz
Cmodel.flux_limit         = ${SELFCAL_MODEL_FLUX_LIMIT}
Cmodel.shape              = [${NUM_PIXELS_CONT},${NUM_PIXELS_CONT}]
Cmodel.cellsize           = [${CELLSIZE_CONT}arcsec, ${CELLSIZE_CONT}arcsec]
Cmodel.direction          = \${modelDirection}
Cmodel.stokes             = [I]
Cmodel.nterms             = ${NUM_TAYLOR_TERMS}

# Output specific parameters
Cmodel.output             = casa
Cmodel.filename           = ${modelImage}"

        SelavyComponentParset="# Not saving the component parset"

        CalibratorModelDefinition="# The model definition
Ccalibrator.sources.names                       = [lsm]
Ccalibrator.sources.lsm.direction               = \${modelDirection}
Ccalibrator.sources.lsm.model                   = ${modelImage}
Ccalibrator.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}"
        if [ ${NUM_TAYLOR_TERMS} -gt 1 ]; then
            if [ "$MFS_REF_FREQ" == "" ]; then
                freq=$CENTRE_FREQ
            else
                freq=${MFS_REF_FREQ}
            fi
            CalibratorModelDefinition="$CalibratorModelDefinition
Ccalibrator.visweights                          = MFS
Ccalibrator.visweights.MFS.reffreq              = ${freq}"
        fi
    else

        CmodelParset="##########
        ## Creation of the model image is not done here"

        SelavyComponentParset="# Saving the fitted components to a parset for use by ccalibrator
Selavy.outputComponentParset                    = true
Selavy.outputComponentParset.filename           = \${sources}
# Reference direction for which component positions should be measured
#  relative to.
Selavy.outputComponentParset.referenceDirection = \${refDirection}
# Only use the brightest components in the parset
Selavy.outputComponentParset.maxNumComponents   = 10"

        CalibratorModelDefinition="# The model definition
Ccalibrator.sources.definition                  = \${sources}"

    fi

    # Optional referencing of ccalibrator
    CcalibratorReference="# Referencing for ccalibrator"
    if [ "${SELFCAL_REF_ANTENNA}" != "" ]; then
        CcalibratorReference="${CcalibratorReference}
Ccalibrator.refantenna                          = ${SELFCAL_REF_ANTENNA}"
    fi
    if [ "${SELFCAL_REF_GAINS}" != "" ]; then
        CcalibratorReference="${CcalibratorReference}
Ccalibrator.refgain                             = ${SELFCAL_REF_GAINS}"
    fi
    if [ "${SELFCAL_REF_ANTENNA}" == "" ] && [ "${SELFCAL_REF_GAINS}" == "" ]; then
        CcalibratorReference="${CcalibratorReference} is not done in this job"
    fi
    
    setJob science_continuumImageSelfcal contSC
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_CONT_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_SELFCAL}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONT_IMAGING}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-contImagingSelfcal-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

selfcalMethod=${SELFCAL_METHOD}

log=${logs}/mslist_for_selfcal_\${SLURM_JOB_ID}.log
NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} $mslist --full ${msSci} 2>&1 1> \${log}
ra=\`python ${PIPELINEDIR}/parseMSlistOutput.py --file=\$log --val=RA\`
dec=\`python ${PIPELINEDIR}/parseMSlistOutput.py --file=\$log --val=Dec\`
epoch=\`python ${PIPELINEDIR}/parseMSlistOutput.py --file=\$log --val=Epoch\`
if [ "${DIRECTION}" != "" ]; then
    modelDirection="${DIRECTION}"
else
    modelDirection="[\${ra}, \${dec}, \${epoch}]"
fi
# Reformat for Selavy's referenceDirection
ra=\`echo \$ra | awk -F':' '{printf "%sh%sm%s",\$1,\$2,\$3}'\` 
refDirection="[\${ra}, \${dec}, \${epoch}]"

caldir=selfCal_${imageBase}
mkdir -p \$caldir

copyImages=${SELFCAL_KEEP_IMAGES}

# Parameters that can vary with self-calibration loop number
SELFCAL_INTERVAL_ARRAY=(${SELFCAL_INTERVAL_ARRAY[@]})
SELFCAL_SELAVY_THRESHOLD_ARRAY=(${SELFCAL_SELAVY_THRESHOLD_ARRAY[@]})
SELFCAL_NORMALISE_GAINS_ARRAY=(${SELFCAL_NORMALISE_GAINS_ARRAY[@]})
CLEAN_THRESHOLD_MAJORCYCLE_ARRAY=(${CLEAN_THRESHOLD_MAJORCYCLE_ARRAY[@]})
CLEAN_NUM_MAJORCYCLES_ARRAY=(${CLEAN_NUM_MAJORCYCLES_ARRAY[@]})
CIMAGER_MINUV_ARRAY=(${CIMAGER_MINUV_ARRAY[@]})
CCALIBRATOR_MINUV_ARRAY=(${CCALIBRATOR_MINUV_ARRAY[@]})

for((LOOP=0;LOOP<=${SELFCAL_NUM_LOOPS};LOOP++)); do

    loopdir=\${caldir}/Loop\${LOOP}
    sources=sources_loop\${LOOP}.in
    caldata=caldata_loop\${LOOP}.tab

    if [ \${LOOP} -gt 0 ]; then
        mkdir -p \${loopdir}
        calparams="# Self-calibration using the recently-generated cal table
Cimager.calibrate                           = true
Cimager.calibrate.ignorebeam                = true
# Allow flagging of vis if inversion of Mueller matrix fails
Cimager.calibrate.allowflag                 = true
# Scale the noise to get correct weighting
Cimager.calibrate.scalenoise                = ${SELFCAL_SCALENOISE}
#
Cimager.calibaccess                         = table
Cimager.calibaccess.table                   = \${loopdir}/\${caldata}
Cimager.calibaccess.table.maxant            = ${NUM_ANT}
Cimager.calibaccess.table.maxbeam           = ${maxbeam}
Cimager.calibaccess.table.maxchan           = ${nchanContSci}
Cimager.calibaccess.table.reuse             = false
"
    else
        calparams="# No self-calibration as it is the first time around the loop
Cimager.calibrate                               = false
"
    fi

    parset=${parsets}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}.in

    # generate the loop-dependant cimager parameters --> loopParams & dataSelectionParams
    cimagerSelfcalLoopParams
    dataSelectionSelfcalLoop Cimager

    cat > \$parset <<EOFINNER
##########
## Continuum imaging with cimager
##
${cimagerParams}
#
\${loopParams}
\${dataSelectionParams}
#
\${calparams}
#
EOFINNER
    if [ \${LOOP} -gt 0 ]; then
            dataSelectionSelfcalLoop Cccalibrator
            cat >> \$parset <<EOFINNER
##########
## Shallow source-finding with selavy
##
# The image to be searched
Selavy.image                                    = ${selavyImage}
#
${selavyWeights}
#
# This is how we divide the image up for distributed processing, with
# the number of subdivisions in each direction, and the size of the
#  overlap region in pixels
Selavy.nsubx                                    = ${SELFCAL_SELAVY_NSUBX}
Selavy.nsuby                                    = ${SELFCAL_SELAVY_NSUBY}
Selavy.overlapx                                 = 50
Selavy.overlapy                                 = 50
#
# The search threshold, in units of sigma
Selavy.snrCut                                   = \${SELFCAL_SELAVY_THRESHOLD_ARRAY[\$LOOP-1]}
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
Selavy.Fitter.maxNumGauss = ${SELFCAL_SELAVY_NUM_GAUSSIANS}
# Do not use the number of initial estimates to determine how many Gaussians to fit
Selavy.Fitter.numGaussFromGuess = ${SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS}
# The fit may be a bit poor, so increase the reduced-chisq threshold
Selavy.Fitter.maxReducedChisq = 15.
#
# Allow islands that are slightly separated to be considered a single 'source'
Selavy.flagAdjacent = false
# The separation in pixels for islands to be considered 'joined'
Selavy.threshSpatial = 7
#
${SelavyComponentParset}
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
${CmodelParset}
##########
## Calibration using selavy's component parset
##
# parameters for calibrator
Ccalibrator.dataset                             = ${OUTPUT}/${msSciAv}
\${dataSelectionParams}
Ccalibrator.nAnt                                = ${NUM_ANT}
Ccalibrator.nBeam                               = 1
Ccalibrator.solve                               = antennagains
Ccalibrator.normalisegains                      = \${SELFCAL_NORMALISE_GAINS_ARRAY[\$LOOP-1]}
Ccalibrator.interval                            = \${SELFCAL_INTERVAL_ARRAY[\$LOOP-1]}
#
Ccalibrator.calibaccess                         = table
Ccalibrator.calibaccess.table                   = \${caldata}
Ccalibrator.calibaccess.table.maxant            = ${NUM_ANT}
Ccalibrator.calibaccess.table.maxbeam           = ${maxbeam}
Ccalibrator.calibaccess.table.maxchan           = ${nchanContSci}
Ccalibrator.calibaccess.table.reuse             = false
#
${CalibratorModelDefinition}
#
${CcalibratorReference}
#
Ccalibrator.gridder.snapshotimaging             = ${GRIDDER_SNAPSHOT_IMAGING}
Ccalibrator.gridder.snapshotimaging.wtolerance  = ${GRIDDER_SNAPSHOT_WTOL}
Ccalibrator.gridder.snapshotimaging.longtrack   = ${GRIDDER_SNAPSHOT_LONGTRACK}
Ccalibrator.gridder.snapshotimaging.clipping    = ${GRIDDER_SNAPSHOT_CLIPPING}
Ccalibrator.gridder                             = WProject
Ccalibrator.gridder.WProject.wmax               = ${GRIDDER_WMAX}
Ccalibrator.gridder.WProject.nwplanes           = ${GRIDDER_NWPLANES}
Ccalibrator.gridder.WProject.oversample         = ${GRIDDER_OVERSAMPLE}
Ccalibrator.gridder.WProject.maxsupport         = ${GRIDDER_MAXSUPPORT}
Ccalibrator.gridder.WProject.variablesupport    = true
Ccalibrator.gridder.WProject.offsetsupport      = true
#
Ccalibrator.ncycles                             = 25

EOFINNER
    fi

    # Other than for the first loop, run selavy to extract the
    #  component parset and use it to calibrate
    if [ \${LOOP} -gt 0 ]; then
        cd \${loopdir}

        log=${logs}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}_selavy.log
        echo "--- Source finding with $selavy ---" > \$log
        echo "---    Loop=\$LOOP, Threshold = \${SELFCAL_SELAVY_THRESHOLD_ARRAY[\$LOOP-1]} --" >> \$log
        NCORES=${NPROCS_SELAVY}
        NPPN=${CPUS_PER_CORE_SELFCAL}
        aprun -n \${NCORES} -N \${NPPN} $selavy -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname}_L\${LOOP}_selavy "txt,csv"

        if [ \$err != 0 ]; then
            exit \$err
        fi

        if [ "\${selfcalMethod}" == "Cmodel" ]; then
            log=${logs}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}_cmodel.log
            echo "--- Model creation with $cmodel ---" > \$log
            NCORES=2
            NPPN=2
            aprun -n \${NCORES} -N \${NPPN} $cmodel -c \$parset >> \$log
            err=\$?
            extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname}_L\${LOOP}_cmodel "txt,csv"
            
            if [ \$err != 0 ]; then
                exit \$err
            fi
        fi


        log=${logs}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}_ccalibrator.log
        echo "--- Calibration with $ccalibrator ---" > \$log
        echo "---    Loop \$LOOP, Interval = \${SELFCAL_INTERVAL_ARRAY[\$LOOP-1]} --" >> \$log
        echo "---    Normalise gains = \${SELFCAL_NORMALISE_GAINS_ARRAY[\$LOOP-1]} --" >> \$log
        NCORES=1
        NPPN=1
        aprun -n \${NCORES} -N \${NPPN} $ccalibrator -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname}_L\${LOOP}_ccal "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        fi

        # Remove the previous cal table and copy the new one in its place
        rm -rf ${OUTPUT}/${gainscaltab}
        cp -r \${caldata} ${OUTPUT}/${gainscaltab}

        # Keep a backup of the intermediate images, prior to re-imaging.
        if [ \${copyImages} == true ]; then
            # Use the . with imageBase to get images only, so we don't
            #  move the selfCal directory itself
            mv ${OUTPUT}/*.${imageBase}* .
        fi

        cd $OUTPUT
    fi

    # Run the imager, calibrating if not the first time.
    log=${logs}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP\${LOOP}.log
    echo "--- Imaging with $theimager ---" > \$log
    NCORES=${NUM_CPUS_CONTIMG_SCI}
    NPPN=${CPUS_PER_CORE_CONT_IMAGING}
    aprun -n \${NCORES} -N \${NPPN} $theimager -c \$parset >> \$log
    err=\$?
    rejuvenate *.${imageBase}*
    rejuvenate ${OUTPUT}/${gainscaltab}
    rejuvenate ${OUTPUT}/${msSciAv}
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname}_L\${LOOP} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

done

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
	DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_AVERAGE_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI_AV"`
	ID_CONTIMG_SCI_SC=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_CONTIMG_SCI_SC} "Make a self-calibrated continuum image for beam $BEAM of the science observation, with flags \"$DEP\""
        FLAG_IMAGING_DEP=`addDep "$FLAG_IMAGING_DEP" "$ID_CONTIMG_SCI_SC"`
    else
	echo "Would make a self-calibrated continuum image for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

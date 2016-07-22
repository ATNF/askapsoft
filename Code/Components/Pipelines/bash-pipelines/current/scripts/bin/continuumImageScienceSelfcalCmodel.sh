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
        selavyWeights=${OUTPUT}/weights.${imageBase}
    else
        selavyImage=${OUTPUT}/image.${imageBase}.taylor.0.restored
        selavyWeights=${OUTPUT}/weights.${imageBase}.taylor.0
    fi

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

    sbatchfile=$slurms/science_continuumImageSelfcal_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_CONT_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_SELFCAL}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONT_IMAGING}
#SBATCH --job-name=cleanSC${BEAM}
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

log=${logs}/mslist_for_cmodel_\${SLURM_JOB_ID}.log
NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} $mslist --full ${msSci} 1>& \${log}
freq=\`grep -A1 Ch0 \$log | tail -n 1 | awk '{print \$12}'\`
if [ "${DIRECTION}" != "" ]; then
    modelDirection="${DIRECTION}"
else
    ra=\`grep -A1 RA \$log | tail -1 | awk '{print \$7}'\`
    dec=\`grep -A1 RA \$log | tail -1 | awk '{print \$8}'\`
    eq=\`grep -A1 RA \$log | tail -1 | awk '{print \$9}'\`
    modelDirection="[\${ra}, \${dec}, \${eq}]"
fi

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
## Creation of the model image
##
# The below specifies the GSM source is a selavy output file
Cmodel.gsm.database       = votable
Cmodel.gsm.file           = selavy-results.components.xml
Cmodel.gsm.ref_freq       = \${freq}MHz

# General parameters
Cmodel.bunit              = Jy/pixel
Cmodel.frequency          = \${freq}MHz
Cmodel.increment          = 304MHz
Cmodel.flux_limit         = ${SELFCAL_MODEL_FLUX_LIMIT}
Cmodel.shape              = [${NUM_PIXELS_CONT},${NUM_PIXELS_CONT}]
Cmodel.cellsize           = [${CELLSIZE_CONT}arcsec, ${CELLSIZE_CONT}arcsec]
Cmodel.direction          = \${modelDirection}
Cmodel.stokes             = [I]
Cmodel.nterms             = ${NUM_TAYLOR_TERMS}

# Output specific parameters
Cmodel.output             = casa
Cmodel.filename           = ${modelImage}
##########
## Calibration using selavy's component parset
##
# parameters for calibrator
Ccalibrator.dataset                             = ${OUTPUT}/${msSciAv}
Ccalibrator.nAnt                                = ${NUM_ANT}
Ccalibrator.nBeam                               = 1
Ccalibrator.solve                               = antennagains
Ccalibrator.normalisegains                      = ${SELFCAL_NORMALISE_GAINS}
Ccalibrator.interval                            = ${SELFCAL_INTERVAL}
#
Ccalibrator.calibaccess                         = table
Ccalibrator.calibaccess.table                   = \${caldata}
Ccalibrator.calibaccess.table.maxant            = ${NUM_ANT}
Ccalibrator.calibaccess.table.maxbeam           = ${maxbeam}
Ccalibrator.calibaccess.table.maxchan           = ${nchanContSci}
Ccalibrator.calibaccess.table.reuse             = false
#
# The model definition
Ccalibrator.sources.names                       = [lsm]
Ccalibrator.sources.lsm.direction               = \${modelDirection}
Ccalibrator.sources.lsm.model                   = ${modelImage}
Ccalibrator.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}
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

    echo "=== Continuum imaging with Self-calibration, for beam ${BEAM}, self-cal loop \${LOOP} ===" > \$log

    # Other than for the first loop, run selavy to extract the
    #  component parset and use it to calibrate
    if [ \${LOOP} -gt 0 ]; then
        cd \${loopdir}
        ln -s ${logs} .
        ln -s ${parsets} .

        echo "--- Source finding with $selavy ---" >> \$log
        NCORES=${NPROCS_SELAVY}
        NPPN=${CPUS_PER_CORE_SELFCAL}
        aprun -n \${NCORES} -N \${NPPN} $selavy -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} selavySC_L\${LOOP}_B${BEAM} "txt,csv"

        if [ \$err != 0 ]; then
            exit \$err
        fi

        echo "--- Model creation with $cmodel ---" >> \$log
        NCORES=2
        NPPN=2
        aprun -n \${NCORES} -N \${NPPN} $cmodel -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} cmodelSC_L\${LOOP}_B${BEAM} "txt,csv"

        if [ \$err != 0 ]; then
            exit \$err
        fi

        echo "--- Calibration with $ccalibrator ---" >> \$log
        NCORES=1
        NPPN=1
        aprun -n \${NCORES} -N \${NPPN} $ccalibrator -c \$parset >> \$log
        err=\$?
        extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ccalSC_L\${LOOP}_B${BEAM} "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        fi
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
    echo "--- Imaging with $cimager ---" >> \$log
    NCORES=${NUM_CPUS_CONTIMG_SCI}
    NPPN=${CPUS_PER_CORE_CONT_IMAGING}
    aprun -n \${NCORES} -N \${NPPN} $cimager -c \$parset >> \$log
    err=\$?
    rejuvenate *.${imageBase}*
    rejuvenate ${OUTPUT}/${gainscaltab}
    rejuvenate ${OUTPUT}/${msSciAv}
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} contImagingSC_L\${LOOP}_B${BEAM} "txt,csv"
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
	ID_CONTIMG_SCI_SC=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_CONTIMG_SCI_SC} "Make a self-calibrated continuum image for beam $BEAM of the science observation, with flags \"$DEP\""
        FLAG_IMAGING_DEP=`addDep "$FLAG_IMAGING_DEP" "$ID_CONTIMG_SCI_SC"`
    else
	echo "Would make a self-calibrated continuum image for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi
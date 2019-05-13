#!/bin/bash -l
#
# Launches a job to image the current beam of the science
# observation, using the BasisfunctionMFS solver. This imaging
# involves self-calibration, whereby the image is searched with Selavy
# to produce a component catalogue, which is then used by Ccalibrator
# to calibrate the gains, before running Cimager again. This is done a
# number of times to hopefully converge to a sensible image.
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

# Define the Cimager parset and associated parameters
. "${PIPELINEDIR}/getContinuumCimagerParams.sh"

ID_CONTIMG_SCI_SC=""

DO_IT=$DO_CONT_IMAGING

if [ "${CLOBBER}" != "true" ] && [ -e "${OUTPUT}/${imageName}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Image ${imageName} exists, so not running continuum imaging for beam ${BEAM}"
    fi
    DO_IT=false
fi

if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
    theimager=$altimager
else
    theimager=$cimager
fi

if [ "${DO_IT}" == "true" ] && [ "${DO_SELFCAL}" == "true" ]; then
    
    # How big is the selavy job for selfcal?
    #    NPROCS_SELAVY=$(echo "${SELFCAL_SELAVY_NSUBX}" "${SELFCAL_SELAVY_NSUBY}" | awk '{print $1*$2+1}')
    NPROCS_SELAVY=19
    haveWarnedAboutSelavyNsub=false
    if [ "${SELFCAL_SELAVY_NSUBX}" -ne 6 ] || [ "${SELFCAL_SELAVY_NSUBY}" -ne 3 ]; then
        if [ "${haveWarnedAboutSelavyNsub}" != "true" ]; then
            echo "WARNING - ignoring SELFCAL_SELAVY_NSUBX & SELFCAL_SELAVY_NSUBY for multi-job selfcal. Keeping at 6 & 3"
            haveWarnedAboutSelavyNsub=true
        fi
    fi

    # How many cores per node does the selavy job need?
    #  Default to 20, but need to make smaller if the number of tasks is fewer
    CPUS_PER_CORE_SELFCAL_SELAVY=20
    if [ "${NPROCS_SELAVY}" -lt "${CPUS_PER_CORE_SELFCAL_SELAVY}" ]; then
        CPUS_PER_CORE_SELFCAL_SELAVY=${NPROCS_SELAVY}
    fi

    # Work out number of nodes required, given constraints on number
    # of tasks and tasks per node for each job.
    if [ "${FAT_NODE_CONT_IMG}" == "true" ]; then
        # Need to take account of the master on a node by itself.
        NUM_NODES_IMAGING=$(echo $NUM_CPUS_CONTIMG_SCI $CPUS_PER_CORE_CONT_IMAGING | awk '{nworkers=$1-1; if (nworkers%$2==0) workernodes=nworkers/$2; else workernodes=int(nworkers/$2)+1; print workernodes+1}')
    else
        NUM_NODES_IMAGING=$(echo $NUM_CPUS_CONTIMG_SCI $CPUS_PER_CORE_CONT_IMAGING | awk '{if ($1%$2==0) print $1/$2; else print int($1/$2)+1}')
    fi
    
    # Details for the script to fix the position offsets
    script_location="${ACES_LOCATION}/tools"
    script_args="${RA_POSITION_OFFSET} ${DEC_POSITION_OFFSET}"
    script="${script_location}/fix_position_offsets.py"
    if [ ! -e "${script}" ]; then
        if [ "${DO_POSITION_OFFSET}" == "true" ]; then
            echo "Could not find ACES directory, so turning off DO_POSITION_OFFSET"
            DO_POSITION_OFFSET=false
        fi
    fi
    if [ "${DO_POSITION_OFFSET}" == "true" ]; then
        if [ "${GIVEN_POSITION_OFFSET_WARNING}" != "true" ]; then
            echo "###################"
            echo "# WARNING - you have turned on the DO_POSITION_OFFSET option"
            echo "# This will apply a shift of "
            echo "#    ${RA_POSITION_OFFSET} arcsec in RA"
            echo "#    ${DEC_POSITION_OFFSET} arcsec in Dec"
            echo "#  to the positions of sources in the model used by"
            echo "#  the final self-calibration loop"
            echo "###################"
            GIVEN_POSITION_OFFSET_WARNING=true
        fi
    fi
    
    imageCode=restored
    setImageProperties cont
    selavyImage="${OUTPUT}/${imageName}"
    selavyWeights="${OUTPUT}/${weightsImage}"

    cutWeights=$(echo "${SELFCAL_SELAVY_WEIGHTSCUT}" | awk '{if (($1>0.)&&($1<1.)) print "true"; else print "false";}')
    if [ "${cutWeights}" == "true" ]; then
        selavyWeights="# Use the weights image, with a high cutoff - since we are using
# WProject, everything should be flat. This will reject areas where
# the snapshot warp is reducing the weight.
Selavy.Weights.weightsImage                     = ${selavyWeights}
Selavy.Weights.weightsCutoff                    = ${SELFCAL_SELAVY_WEIGHTSCUT}"
    else
        selavyWeights="# No weights scaling applied in Selavy"
    fi

    modelImage="contmodel.${imageBase}"

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

    caldir=selfCal_${imageBase}
    mkdir -p $caldir

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI")
        DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV")
	if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then 
            DEP=$(addDep "$DEP" "$ID_MSCONCAT_SCI_AV")
	fi
    fi

    for((LOOP=0;LOOP<=SELFCAL_NUM_LOOPS;LOOP++)); do

        loopdir="${caldir}/Loop${LOOP}"
        sources=sources_loop${LOOP}.in
        caldata=caldata_loop${LOOP}.tab

        if [ "${LOOP}" -gt 0 ]; then
            mkdir -p "${loopdir}"
            calparams="# Self-calibration using the recently-generated cal table
Cimager.calibrate                           = true
Cimager.calibrate.ignorebeam                = true
# Allow flagging of vis if inversion of Mueller matrix fails
Cimager.calibrate.allowflag                 = true
# Scale the noise to get correct weighting
Cimager.calibrate.scalenoise                = ${SELFCAL_SCALENOISE}
#
Cimager.calibaccess                         = table
Cimager.calibaccess.table                   = ${loopdir}/${caldata}
Cimager.calibaccess.table.maxant            = ${NUM_ANT}
Cimager.calibaccess.table.maxbeam           = ${maxbeam}
Cimager.calibaccess.table.maxchan           = ${nchanContSci}
Cimager.calibaccess.table.reuse             = false
"
        else
            calparams="# No self-calibration as it is the first time around the loop
Cimager.calibrate                               = false
"                                               

            # Define the calibration parsets
            #  Loop dependent, as could refer to a components file defined by $sources
            
            if [ "${SELFCAL_METHOD}" == "Cmodel" ]; then

                # If no SNR limit has been given, fall back to the flux limit
                if [ "${SELFCAL_COMPONENT_SNR_LIMIT}" == "" ]; then
                    CmodelFluxLimit="Cmodel.flux_limit         = ${SELFCAL_MODEL_FLUX_LIMIT}"
                else
                    CmodelFluxLimit="# flux limit for cmodel determined from image noise - see below"
                fi

                CmodelParset="##########
## Creation of the model image
##
# The below specifies the GSM source is a selavy output file
Cmodel.gsm.database       = votable
Cmodel.gsm.file           = selavy-results.components.xml
Cmodel.gsm.ref_freq       = \${centreFreq}Hz

# General parameters
Cmodel.bunit              = Jy/pixel
Cmodel.frequency          = \${centreFreq}Hz
Cmodel.increment          = \${bandwidth}Hz
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
                if [ "${NUM_TAYLOR_TERMS}" -gt 1 ]; then
                    if [ "$MFS_REF_FREQ" == "" ]; then
                        freq="\${centreFreq}"
                    else
                        freq="${MFS_REF_FREQ}"
                    fi
                    CalibratorModelDefinition="$CalibratorModelDefinition
Ccalibrator.visweights                          = MFS
Ccalibrator.visweights.MFS.reffreq              = ${freq}"
                fi

            elif [ "${SELFCAL_METHOD}" == "CleanModel" ]; then

                imageCode=image
                setImageProperties cont
                modelImage=${imageName%%.fits}
                CalibratorModelDefinition="# The model definition
Ccalibrator.imagetype                           = ${IMAGETYPE_CONT}
Ccalibrator.sources.names                       = [lsm]
Ccalibrator.sources.lsm.direction               = \${modelDirection}
Ccalibrator.sources.lsm.model                   = ${OUTPUT}/${modelImage%%.taylor.0}
Ccalibrator.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}"
                if [ "${NUM_TAYLOR_TERMS}" -gt 1 ]; then
                    if [ "$MFS_REF_FREQ" == "" ]; then
                        freq="\${centreFreq}"
                    else
                        freq=${MFS_REF_FREQ}
                    fi
                    CalibratorModelDefinition="$CalibratorModelDefinition
Ccalibrator.visweights                          = MFS
Ccalibrator.visweights.MFS.reffreq              = ${freq}"
                fi

            elif [ "${SELFCAL_METHOD}" == "Components" ]; then

                CmodelParset="##########
                ## Creation of the model image is not done here"

                SelavyComponentParset="# Saving the fitted components to a parset for use by ccalibrator
Selavy.outputComponentParset                    = true
Selavy.outputComponentParset.filename           = ${sources}
# Reference direction for which component positions should be measured
#  relative to.
Selavy.outputComponentParset.referenceDirection = \${refDirection}
# Only use the brightest components in the parset
Selavy.outputComponentParset.maxNumComponents   = 10"

                CalibratorModelDefinition="# The model definition
Ccalibrator.sources.definition                  = ${sources}"

            else
                echo "ERROR - SELFCAL_METHOD=${SELFCAL_METHOD} - this should not be the case. Exiting!"
                exit 1
            fi

        fi

        # generate the loop-dependant cimager parameters --> loopParams & dataSelectionParams
        cimagerSelfcalLoopParams
        dataSelectionSelfcalLoop

        if [ "$LOOP" -gt 0 ]; then

            setJob science_contSelfcal_L${LOOP} contSC${LOOP}
            cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CONT_SELFCAL}
#SBATCH --nodes=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-contSelfcal-L${LOOP}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

selfcalMethod=${SELFCAL_METHOD}

# Set elements of the metadata based on position & frequency
msMetadata="${MS_METADATA}"
ra=\$(python "\${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=RA)
dec=\$(python "\${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Dec)
epoch=\$(python "\${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Epoch)
bandwidth="\$(python "\${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Bandwidth)"
centreFreq="\$(python "\${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Freq)"

direction="${DIRECTION}"
if [ "\${direction}" != "" ]; then
    modelDirection="\${direction}"
else
    modelDirection="[\${ra}, \${dec}, \${epoch}]"
fi
# Reformat for Selavy's referenceDirection
ra=\$(echo "\$ra" | awk -F':' '{printf "%sh%sm%s",\$1,\$2,\$3}')
refDirection="[\${ra}, \${dec}, \${epoch}]"

parset="${parsets}/science_contSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP${LOOP}.in"
cat > \$parset << EOFINNER
##########
## Shallow source-finding with selavy
##
# The image to be searched
Selavy.image                                    = ${selavyImage}
Selavy.sbid                                     = ${SB_SCIENCE}
Selavy.imageHistory                             = [${imageHistoryString}]
Selavy.imagetype                                = ${IMAGETYPE_CONT}
#
${selavyWeights}
#
# This is how we divide the image up for distributed processing, with
# the number of subdivisions in each direction, and the size of the
#  overlap region in pixels
Selavy.nsubx                                    = 3
Selavy.nsuby                                    = 3
Selavy.overlapx                                 = 50
Selavy.overlapy                                 = 50
#
# The search threshold, in units of sigma
Selavy.snrCut                                   = ${SELFCAL_SELAVY_THRESHOLD_ARRAY[$LOOP]}
# Grow the detections to a secondary threshold
Selavy.flagGrowth                               = true
Selavy.growthCut                                = 5
#
# Turn on the variable threshold option
Selavy.VariableThreshold                        = true
Selavy.VariableThreshold.boxSize                = 50
Selavy.VariableThreshold.ThresholdImageName     = detThresh
Selavy.VariableThreshold.NoiseImageName         = noiseMap
Selavy.VariableThreshold.AverageImageName       = meanMap
Selavy.VariableThreshold.SNRimageName           = snrMap
Selavy.VariableThreshold.imagetype              = ${IMAGETYPE_CONT}
#
# Parameters to switch on and control the Gaussian fitting
Selavy.Fitter.doFit                             = true
# Fit all 6 parameters of the Gaussian
Selavy.Fitter.fitTypes                          = [${SELFCAL_SELAVY_FIT_TYPE}]
# Limit the number of Gaussians to 1
Selavy.Fitter.maxNumGauss = ${SELFCAL_SELAVY_NUM_GAUSSIANS}
# Do not use the number of initial estimates to determine how many Gaussians to fit
Selavy.Fitter.numGaussFromGuess = ${SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS}
# The fit may be a bit poor, so increase the reduced-chisq threshold
Selavy.Fitter.maxReducedChisq = 15.
#
# Require all pixels of an island to be properly contiguous.
# This adds robustness against including and fitting to sidelobes.
Selavy.flagAdjacent = true
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
${dataSelectionParams}
Ccalibrator.nAnt                                = ${NUM_ANT}
Ccalibrator.nBeam                               = 1
Ccalibrator.solve                               = antennagains
Ccalibrator.normalisegains                      = ${SELFCAL_NORMALISE_GAINS_ARRAY[$LOOP]}
Ccalibrator.interval                            = ${SELFCAL_INTERVAL_ARRAY[$LOOP]}
#
Ccalibrator.calibaccess                         = table
Ccalibrator.calibaccess.table                   = ${caldata}
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

cd ${loopdir}

if [ "\${selfcalMethod}" != "CleanModel" ]; then

    log="${logs}/science_contSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP${LOOP}_selavy.log"
    echo "--- Source finding with $selavy ---" > "\$log"
    echo "---    Loop=$LOOP, Threshold = ${SELFCAL_SELAVY_THRESHOLD_ARRAY[$LOOP]} --" >> "\$log"
    NCORES=${NPROCS_SELAVY}
    NPPN=${CPUS_PER_CORE_SELFCAL_SELAVY}
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $selavy -c "\$parset" >> "\$log"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_L${LOOP}_selavy "txt,csv"

    if [ \$err != 0 ]; then
        exit \$err
    fi

    if [ "${DO_POSITION_OFFSET}" == "true" ] && [ -e "${script_location}/fix_position_offsets.py" ]; then
        LOOP=${LOOP}
        if [ \${LOOP} -eq ${SELFCAL_NUM_LOOPS} ]; then
            log="${logs}/fix_position_offsets_\${SLURM_JOB_ID}.log"
            python "${script_location}/fix_position_offsets.py" ${script_args} > "\${log}"
        fi
    fi

    if [ "\${selfcalMethod}" == "Cmodel" ]; then
        fluxLimitSNR="${SELFCAL_COMPONENT_SNR_LIMIT}"
        if [ "\${fluxLimitSNR}" != "" ]; then
            # Get the average noise at location of components, and set a flux limit for cmodel based on it
            avNoise=\$(awk 'BEGIN{sum=0;ct=0;}{if((substr(\$0,1,1)!="#")&&(\$33>0.)){ sum+=\$33; ct++; }}END{print sum/ct}' selavy-results.components.txt)
            fluxLimit=\$(echo \$avNoise \$fluxLimitSNR | awk '{print \$1*\$2}')
            echo "# Flux limit for cmodel determined from image noise & SNR=${SELFCAL_COMPONENT_SNR_LIMIT}
el.flux_limit    = \${fluxLimit}mJy" >> \$parset
        fi
        log="${logs}/science_contSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP${LOOP}_cmodel.log"
        echo "--- Model creation with $cmodel ---" > "\$log"
        NCORES=2
        NPPN=2
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $cmodel -c "\$parset" >> "\$log"
        err=\$?
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_L${LOOP}_cmodel "txt,csv"

        if [ \$err != 0 ]; then
            exit \$err
        fi
    fi

fi

log="${logs}/science_contSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP${LOOP}_ccalibrator.log"
echo "--- Calibration with $ccalibrator ---" > "\$log"
echo "---    Loop $LOOP, Interval = ${SELFCAL_INTERVAL_ARRAY[$LOOP]} --" >> "\$log"
echo "---    Normalise gains = ${SELFCAL_NORMALISE_GAINS_ARRAY[$LOOP]} --" >> "\$log"
NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $ccalibrator -c "\$parset" >> "\$log"
err=\$?
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_L${LOOP}_ccal "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

# Remove the previous cal table and copy the new one in its place
rm -rf "${OUTPUT}/${gainscaltab}"
cp -r "${caldata}" "${OUTPUT}/${gainscaltab}"

# Keep a backup of the intermediate images, prior to re-imaging.
copyImages=${SELFCAL_KEEP_IMAGES}
if [ \${copyImages} == "true" ]; then
    # Use the . with imageBase to get images only, so we don't
    #  move the selfCal directory itself
    mv "${OUTPUT}"/*.${imageBase}* .
fi

EOF

            if [ "${SUBMIT_JOBS}" == "true" ]; then
                thisID=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
                recordJob "${thisID}" "Self-calibration for loop ${LOOP}, based on the continuum image for beam $BEAM of the science observation, with flags \"$DEP\""
                # DEP is dependencies for jobs within this script
                DEP=$(addDep "$DEP" "$thisID")
                # ID_CONTIMG_SCI_SC tracks just this beam
                ID_CONTIMG_SCI_SC=$(addDep "$ID_CONTIMG_SCI_SC" "$thisID")
                # FLAG_IMAGING_DEP runs over all beams
                FLAG_IMAGING_DEP=$(addDep "$FLAG_IMAGING_DEP" "$thisID")
            else
	        echo "Would run self-calibration for loop ${LOOP} based on the continuum image for beam $BEAM of the science observation with slurm file $sbatchfile"
            fi

        fi

        setJob science_continuumImage_L${LOOP} contIm${LOOP}
        cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CONT_IMAGE}
#SBATCH --nodes=${NUM_NODES_IMAGING}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-contImaging-L${LOOP}-%j.out"

${askapsoftModuleCommands}

FAT_NODE_CONT_IMG=${FAT_NODE_CONT_IMG}
nodeDistribution="--ntasks-per-node=${CPUS_PER_CORE_CONT_IMAGING} "
if [ "\${FAT_NODE_CONT_IMG}" == "true" ]; then

    nodelist=\$SLURM_JOB_NODELIST
    
    # Make arrangements to put task 0 (master) of imager on the first node, 
    # and spread the rest out over the other nodes:
    
    newlist=\$(hostname)
    icpu=0
    for node in \$(scontrol show hostnames \$nodelist); do
        if [[ "\$node" != "\$(hostname)" ]]; then
            for proc in \$(seq 1 ${CPUS_PER_CORE_CONT_IMAGING}); do
    	        icpu=\$((icpu+1))
    	        if [[ "\$icpu" -lt "${NUM_CPUS_CONTIMG_SCI}" ]]; then 
    	            newlist=\$newlist,\$node
     	        fi
            done
        fi
    done
    echo "NodeList: "\$nodelist
    echo "NewList: "\$newlist

    nodeDistribution="--nodelist=\$newlist --distribution=arbitrary"

fi

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

parset="${parsets}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP${LOOP}.in"

cat > "\$parset" <<EOFINNER
##########
## Continuum imaging with cimager
##
${cimagerParams}
#
${loopParams}
${dataSelectionParams}
#
${calparams}
#
EOFINNER

# Run the imager, calibrating if not the first time.
log="${logs}/science_imagingSelfcal_${FIELDBEAM}_\${SLURM_JOB_ID}_LOOP${LOOP}.log"
echo "--- Imaging with $theimager ---" > "\$log"
NCORES=${NUM_CPUS_CONTIMG_SCI}
NPPN=${CPUS_PER_CORE_CONT_IMAGING}
srun --export=ALL --ntasks=\${NCORES} \${nodeDistribution} $theimager ${PROFILE_FLAG} -c "\$parset" >> "\$log"
err=\$?

# Handle the profiling files
doProfiling=${USE_PROFILING}
if [ "\${doProfiling}" == "true" ]; then
    dir=Profiling/Beam${BEAM}/LOOP${LOOP}
    mkdir -p \$dir
    mv profile.*.${imageBase}* \${dir}
fi

for im in *.${imageBase}*; do
    rejuvenate "\$im"
done
rejuvenate "${OUTPUT}"/"${gainscaltab}"
rejuvenate "${OUTPUT}"/"${msSciAv}"
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_L${LOOP} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi


EOF

        if [ "${SUBMIT_JOBS}" == "true" ]; then
            thisID=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
            recordJob "${thisID}" "Continuum imaging for beam $BEAM of the science observation, loop ${LOOP} of the self-calibration, with flags \"$DEP\""
            # DEP is dependencies for jobs within this script
            DEP=$(addDep "$DEP" "$thisID")
            # ID_CONTIMG_SCI_SC tracks just this beam
            ID_CONTIMG_SCI_SC=$(addDep "$ID_CONTIMG_SCI_SC" "$thisID")
            # FLAG_IMAGING_DEP runs over all beams
            FLAG_IMAGING_DEP=$(addDep "$FLAG_IMAGING_DEP" "$thisID")
        else
	    echo "Would run continuum imaging for beam $BEAM of the science observation, loop ${LOOP} of the self-calibration, with slurm file $sbatchfile"
        fi

    done
    unset LOOP

    echo " "

fi

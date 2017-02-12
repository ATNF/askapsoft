#!/bin/bash -l
#
# Launches a job to create a catalogue of sources in the continuum image.
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

if [ $DO_SOURCE_FINDING == true ]; then

    # set imageName, weightsImage etc
    imageCode=restored
    setImageProperties cont
    contImage=$imageName
    contWeights=$weightsImage
    setImageProperties contcube
    contCube=$imageName

    # get the text that does the FITS conversion - put in $fitsConvertText
    convertToFITStext

    # This adds L1, L2, etc to the job name when LOOP is defined and
    # >0 -- this means that we are running the sourcefinding on the
    # selfcal loop mosaics, and so we also need to change the image &
    # weights names.
    # We also can't do the RM synthesis on the LOOP images (since the
    # calibrations don't match), so we turn it off if it is on
    doRM=${DO_RM_SYNTHESIS}
    description=selavy
    if [ "$LOOP" != "" ]; then
       if [ $LOOP -gt 0 ]; then
           description=selavyL${LOOP}
           contImage="${contImage}.SelfCalLoop${LOOP}"
           contWeights="${contWeights}.SelfCalLoop${LOOP}"
           doRM=false
       fi
    fi

    # Define the detection thresholds in terms of flux or SNR
    if [ "${SELAVY_FLUX_THRESHOLD}" != "" ]; then
        # Use a direct flux threshold if specified
        thresholdPars="# Detection threshold
Selavy.threshold = ${SELAVY_FLUX_THRESHOLD}"
        if [ ${SELAVY_FLAG_GROWTH} == true ] && 
               [ ${SELAVY_GROWTH_THRESHOLD} != "" ]; then
           thresholdPars="${thresholdPars}
Selavy.flagGrowth =  ${SELAVY_FLAG_GROWTH}
Selavy.growthThreshold = ${SELAVY_GROWTH_THRESHOLD}"
        fi
    else
        # Use a SNR threshold
        thresholdPars="# Detection threshold
Selavy.snrCut = ${SELAVY_SNR_CUT}"
        if [ ${SELAVY_FLAG_GROWTH} == true ] &&
               [ ${SELAVY_GROWTH_CUT} != "" ]; then
           thresholdPars="${thresholdPars}
Selavy.flagGrowth =  ${SELAVY_FLAG_GROWTH}
Selavy.growthThreshold = ${SELAVY_GROWTH_CUT}"
        fi
    fi    

    setJob science_selavy_${contImage} $description
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SOURCEFINDING_CONT}
#SBATCH --ntasks=${NUM_CPUS_SELAVY}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-selavy-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

seldir=selavy_${contImage}
mkdir -p \$seldir

cd \${seldir}

HAVE_IMAGES=true
BEAM=$BEAM
NUM_TAYLOR_TERMS=${NUM_TAYLOR_TERMS}

# List of images to convert to FITS in the Selavy job
imlist=""

image=${OUTPUT}/${contImage}
weights=${OUTPUT}/${contWeights}
contcube=${OUTPUT}/${contCube}

imlist="\${imlist} \${image}"
if [ \$NUM_TAYLOR_TERMS -gt 1 ]; then
    t1im=\`echo \$image | sed -e 's/taylor\.0/taylor\.1/g'\`
    if [ -e \${t1im} ]; then
        imlist="\${imlist} \${t1im}"
    fi
    t2im=\`echo \$image | sed -e 's/taylor\.0/taylor\.2/g'\`
    if [ -e \${t2im} ]; then
        imlist="\${imlist} \${t2im}"
    fi
fi

if [ "\${BEAM}" == "all" ]; then
    imlist="\${imlist} \${weights}"
    weightpars="Selavy.Weights.weightsImage = \${weights##*/}.fits
Selavy.Weights.weightsCutoff = ${LINMOS_CUTOFF}"
else
    weightpars="#"
fi

doRM=${doRM}
if [ \$doRM == true ]; then
    if [ -e \${contcube} ]; then
        imlist="\${imlist} \${contcube}"
    else
        doRM=false
        echo "ERROR - Continuum cube \${contcube} not found. RM Synthesis being turned off."
    fi
fi

echo "Converting to FITS the following images: \${imlist}"
for im in \${imlist}; do 
    casaim="../\${im##*/}"
    fitsim="../\${im##*/}.fits"
    ${fitsConvertText}
    # Make a link so we point to a file in the current directory for
    # Selavy. This gets the referencing correct in the catalogue
    # metadata 
    if [ ! -e \$fitsim ]; then
        HAVE_IMAGES=false
        echo "ERROR - Could not create \${im}.fits"
    else
        ln -s \${im}.fits .
    fi
done

if [ \${HAVE_IMAGES} == true ]; then

    parset=${parsets}/science_selavy_${FIELDBEAM}_\${SLURM_JOB_ID}.in
    log=${logs}/science_selavy_${FIELDBEAM}_\${SLURM_JOB_ID}.log
    
    # Directory for extracted data products
    polDir=PolData
    mkdir -p \$polDir

    if [ \${doRM} == true ]; then
        rmSynthParams="# RM Synthesis on extracted spectra from continuum cube
Selavy.RMSynthesis = \${doRM}
Selavy.RMSynthesis.cube = \$contCube
Selavy.RMSynthesis.outputBase = \${polDir}/${SELAVY_POL_OUTPUT_BASE}
Selavy.RMSynthesis.writeSpectra = ${SELAVY_POL_WRITE_SPECTRA}
Selavy.RMSynthesis.writeComplexFDF = ${SELAVY_POL_WRITE_FDF}
Selavy.RMSynthesis.boxwidth = ${SELAVY_POL_BOX_WIDTH}
Selavy.RMSynthesis.noiseArea = ${SELAVY_POL_NOISE_AREA}
Selavy.RMSynthesis.robust = ${SELAVY_POL_ROBUST_STATS}
Selavy.RMSynthesis.weightType = ${SELAVY_POL_WEIGHT_TYPE}
Selavy.RMSynthesis.modeltype = ${SELAVY_POL_MODEL_TYPE}
Selavy.RMSynthesis.modelPolyOrder = ${SELAVY_POL_MODEL_ORDER}
Selavy.RMSynthesis.polThresholdSNR = ${SELAVY_POL_SNR_THRESHOLD}
Selavy.RMSynthesis.polThresholdDebias = ${SELAVY_POL_DEBIAS_THRESHOLD}
Selavy.RMSynthesis.numPhiChan = ${SELAVY_POL_NUM_PHI_CHAN}
Selavy.RMSynthesis.deltaPhi = ${SELAVY_POL_DELTA_PHI}
Selavy.RMSynthesis.phiZero = ${SELAVY_POL_PHI_ZERO}"
    else
        rmSynthParams="# Not performing RM Synthesis for this case
Selavy.RMSynthesis = \${doRM}"
    fi

    cat > \$parset <<EOFINNER
Selavy.image = \${image##*/}.fits
Selavy.SBid = ${SB_SCIENCE}
Selavy.nsubx = ${SELAVY_NSUBX}
Selavy.nsuby = ${SELAVY_NSUBY}
#
Selavy.resultsFile = selavy-${contImage}.txt
#
Selavy.snrCut = ${SELAVY_SNR_CUT}
Selavy.flagGrowth = ${SELAVY_FLAG_GROWTH}
Selavy.growthCut = ${SELAVY_GROWTH_CUT}
#
Selavy.VariableThreshold = ${SELAVY_VARIABLE_THRESHOLD}
Selavy.VariableThreshold.boxSize = ${SELAVY_BOX_SIZE}
Selavy.VariableThreshold.ThresholdImageName=detThresh.${contImage}.img
Selavy.VariableThreshold.NoiseImageName=noiseMap.${contImage}.img
Selavy.VariableThreshold.AverageImageName=meanMap.${contImage}.img
Selavy.VariableThreshold.SNRimageName=snrMap.${contImage}.img
\${weightpars}
#
Selavy.Fitter.doFit = true
Selavy.Fitter.fitTypes = [full]
Selavy.Fitter.numGaussFromGuess = true
Selavy.Fitter.maxReducedChisq = 10.
#
Selavy.threshSpatial = 5
Selavy.flagAdjacent = false
#
Selavy.minPix = 3
Selavy.minVoxels = 3
Selavy.minChannels = 1
Selavy.sortingParam = -pflux
#
\${rmSynthParams}
EOFINNER

    NCORES=${NUM_CPUS_SELAVY}
    NPPN=${CPUS_PER_CORE_SELAVY}
    aprun -n \${NCORES} -N \${NPPN} $selavy -c \$parset >> \$log
    err=\$?
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

else

    echo "FITS conversion failed, so Selavy did not run"

fi

EOFOUTER

    # Dependencies for the job
    DEP=""
    if [ "$FIELD" == "." ]; then
        DEP=`addDep "$DEP" "$ID_LINMOS_CONT_ALL"`
    elif [ $BEAM == "all" ]; then
        DEP=`addDep "$DEP" "$ID_LINMOS_CONT"`
    else
        if [ $DO_SELFCAL == true ]; then
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI_SC"`
        else
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI"`
        fi
    fi
    if [ ${DO_RM_SYNTHESIS} == true ]; then
        if [ "$FIELD" == "." ]; then
            DEP=`addDep "$DEP" "$ID_LINMOS_CONTCUBE_ALL"`
        elif [ $BEAM == "all" ]; then
            DEP=`addDep "$DEP" "$ID_LINMOS_CONTCUBE"`
        else
            DEP=`addDep "$DEP" "$ID_CONTCUBE_SCI"`
        fi
    fi    
    
    if [ $SUBMIT_JOBS == true ]; then
	ID_SOURCEFINDING_CONT_SCI=`sbatch ${DEP} $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SOURCEFINDING_CONT_SCI} "Run the continuum source-finding on the science image ${contImage} with flags \"$DEP\""
    else
	echo "Would run the continuum source-finding on the science image ${contImage} with slurm file $sbatchfile"
    fi

    echo " "

    
fi

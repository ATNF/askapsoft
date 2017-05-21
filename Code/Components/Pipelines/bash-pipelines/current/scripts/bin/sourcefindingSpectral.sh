#!/bin/bash -l
#
# Launches a job to create a catalogue of sources in the spectral-line
# image cube.
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

DO_IT=$DO_SOURCE_FINDING_SPEC 

# set imageName and selavy directories
imageCode=restored
setImageProperties spectral
beamlog=beamlog.${imageName}.txt

# Dependencies for the job
DEP=""
if [ "$FIELD" == "." ]; then
    DEP=$(addDep "$DEP" "$ID_LINMOS_SPECTRAL_ALL_RESTORED")
elif [ "$BEAM" == "all" ]; then
    DEP=$(addDep "$DEP" "$ID_LINMOS_SPECTRAL_RESTORED")
else
    DEP=$(addDep "$DEP" "$ID_SPECIMG_SCI")
fi

if [ ! -e "${OUTPUT}/${imageName}" ] && [ "${DEP}" == "" ] &&
       [ "${SUBMIT_JOBS}" == "true" ]; then
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then
    
    # Define the detection thresholds in terms of flux or SNR
    if [ "${SELAVY_SPEC_FLUX_THRESHOLD}" != "" ]; then
        # Use a direct flux threshold if specified
        thresholdPars="# Detection threshold
Selavy.threshold = ${SELAVY_SPEC_FLUX_THRESHOLD}"
        if [ "${SELAVY_SPEC_FLAG_GROWTH}" == "true" ] &&
               [ "${SELAVY_SPEC_GROWTH_THRESHOLD}" != "" ]; then
            thresholdPars="${thresholdPars}
Selavy.flagGrowth =  ${SELAVY_SPEC_FLAG_GROWTH}
Selavy.growthThreshold = ${SELAVY_SPEC_GROWTH_THRESHOLD}"
        fi
    else
        # Use a SNR threshold
        thresholdPars="# Detection threshold
Selavy.snrCut = ${SELAVY_SPEC_SNR_CUT}"
        if [ "${SELAVY_SPEC_FLAG_GROWTH}" == true ] && 
               [ "${SELAVY_SPEC_GROWTH_CUT}" != "" ]; then
            thresholdPars="${thresholdPars}
Selavy.flagGrowth =  ${SELAVY_SPEC_FLAG_GROWTH}
Selavy.growthThreshold = ${SELAVY_SPEC_GROWTH_CUT}"
        fi
    fi

    # Pre-processing parameters
    preprocessPars="# No pre-processing done"
    # Smoothing takes precedence over reconstruction
    if [ "${SELAVY_SPEC_FLAG_SMOOTH}" == "true" ]; then
        if [ "${SELAVY_SPEC_SMOOTH_TYPE}" == "spectral" ]; then
            preprocessPars="# Spectral smoothing
Selavy.flagSmooth = ${SELAVY_SPEC_FLAG_SMOOTH}
Selavy.smoothType = ${SELAVY_SPEC_SMOOTH_TYPE}
Selavy.hanningWidth = ${SELAVY_SPEC_HANN_WIDTH}"
        else
            preprocessPars="# Spatial smoothing
Selavy.flagSmooth = ${SELAVY_SPEC_FLAG_SMOOTH}
Selavy.smoothType = ${SELAVY_SPEC_SMOOTH_TYPE}
Selavy.kernMaj = ${SELAVY_SPEC_KERN_MAJ}
Selavy.kernMin = ${SELAVY_SPEC_KERN_MIN}
Selavy.kernPA = ${SELAVY_SPEC_KERN_PA}"
        fi
    elif [ "${SELAVY_SPEC_FLAG_WAVELET}" == "true" ]; then
        preprocessPars="# Multi-resolution wavelet reconstruction
Selavy.flagAtrous = ${SELAVY_SPEC_FLAG_WAVELET}
Selavy.reconDim   = ${SELAVY_SPEC_RECON_DIM}
Selavy.scaleMin   = ${SELAVY_SPEC_RECON_SCALE_MIN}
Selavy.scaleMax   = ${SELAVY_SPEC_RECON_SCALE_MAX}
Selavy.snrRecon   = ${SELAVY_SPEC_RECON_SNR}"        
    fi
    

    setJob "science_selavy_spec_${imageName}" selavySpec
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SOURCEFINDING_SPEC}
#SBATCH --ntasks=${NUM_CPUS_SELAVY_SPEC}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY_SPEC}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-selavy-spec-%j.out

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

# Working directory for the selavy output
seldir=selavy-spectral-${imageName##*/}
mkdir -p "\$seldir"
cd "\$seldir"
    
HAVE_IMAGES=true
BEAM=$BEAM

# List of images to convert to FITS in the Selavy job
imlist=""

# Image to be searched
image="${OUTPUT}/${imageName}"
imlist="\${imlist} \${image}"
if [ "\${BEAM}" == "all" ]; then
    # Weights image - really only useful if primary-beam corrected
    weights=${OUTPUT}/${weightsImage}
    imlist="\${imlist} \${weights}"
    weightpars="Selavy.Weights.weightsImage = \${weights##*/}.fits
Selavy.Weights.weightsCutoff = ${SELAVY_WEIGHTS_CUTOFF}"
else
    weightpars="#"
fi

HAVE_IMAGES=true
echo "Converting to FITS the following images: \${imlist}"
for im in \${imlist}; do 

    casaim="../\${im##*/}"
    fitsim="../\${im##*/}.fits"
    parset=$parsets/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in
    log=$logs/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log
    ${fitsConvertText}
    # Make a link so we point to a file in the current directory for
    # Selavy. This gets the referencing correct in the catalogue
    # metadata 
    if [ ! -e "\$fitsim" ]; then
        HAVE_IMAGES=false
        echo "ERROR - Could not create \${im}.fits"
    else
        ln -s -f "\${im}.fits" .
    fi
done

if [ "\${HAVE_IMAGES}" == "true" ]; then

    parset=${parsets}/science_selavy_spectral_\${SLURM_JOB_ID}.in
    log=${logs}/science_selavy_spectral_\${SLURM_JOB_ID}.log
    
    # Directories for extracted data products
    mkdir -p "${OUTPUT}/$selavySpectraDir"
    mkdir -p "${OUTPUT}/$selavyMomentsDir"
    mkdir -p "${OUTPUT}/$selavyCubeletsDir"
    
    cat > "\$parset" <<EOFINNER
Selavy.image = \${image##*/}.fits
Selavy.nsubx = ${SELAVY_SPEC_NSUBX}
Selavy.nsuby = ${SELAVY_SPEC_NSUBY}
Selavy.nsubz = ${SELAVY_SPEC_NSUBZ}
#
Selavy.resultsFile = selavy-${imageName}.txt
#
\${weightpars}
#
${thresholdPars}
#
Selavy.searchType = ${SELAVY_SPEC_SEARCH_TYPE}
#
${preprocessPars}
#
Selavy.VariableThreshold = ${SELAVY_SPEC_VARIABLE_THRESHOLD}
Selavy.VariableThreshold.boxSize = ${SELAVY_SPEC_BOX_SIZE}
Selavy.VariableThreshold.ThresholdImageName=detThresh.${imageName}.img
Selavy.VariableThreshold.NoiseImageName=noiseMap.${imageName}.img
Selavy.VariableThreshold.AverageImageName=meanMap.${imageName}.img
Selavy.VariableThreshold.SNRimageName=snrMap.${imageName}.img
Selavy.VariableThreshold.reuse = true
#
Selavy.Fitter.doFit = false
#
Selavy.threshSpatial = 5
#Selavy.flagAdjacent = true
Selavy.threshVelocity = 7
#
Selavy.minVoxels = 11
Selavy.minPix = ${SELAVY_SPEC_MIN_PIX}
Selavy.minChannels = ${SELAVY_SPEC_MIN_CHAN}
Selavy.maxChannels = ${SELAVY_SPEC_MAX_CHAN}
Selavy.sortingParam = -pflux
Selavy.spectralUnits = km/s
#
# Emission-line catalogue
Selavy.HiEmissionCatalogue=true
# Extraction
Selavy.extractSpectra = true
Selavy.extractSpectra.spectralCube = \$image
Selavy.extractSpectra.spectralOutputBase = ${OUTPUT}/${selavySpectraDir}/${SELAVY_SPEC_BASE_SPECTRUM}
Selavy.extractSpectra.useDetectedPixels = true
Selavy.extractSpectra.beamLog = ${beamlog}
Selavy.extractNoiseSpectra = true
Selavy.extractNoiseSpectra.spectralCube= \$image
Selavy.extractNoiseSpectra.spectralOutputBase = ${OUTPUT}/${selavySpectraDir}/${SELAVY_SPEC_BASE_NOISE}
Selavy.extractNoiseSpectra.useDetectedPixels = true
Selavy.extractMomentMap = true
Selavy.extractMomentMap.spectralCube = \$image
Selavy.extractMomentMap.momentOutputBase = ${OUTPUT}/${selavyMomentsDir}/${SELAVY_SPEC_BASE_MOMENT}
Selavy.extractMomentMap.moments = [0,1,2]
Selavy.extractCubelet = true
Selavy.extractCubelet.spectralCube = \$image
Selavy.extractCubelet.cubeletOutputBase = ${OUTPUT}/${selavyCubeletsDir}/${SELAVY_SPEC_BASE_CUBELET}
EOFINNER

    NCORES=${NUM_CPUS_SELAVY_SPEC}
    NPPN=${CPUS_PER_CORE_SELAVY_SPEC}
    aprun -n \${NCORES} -N \${NPPN} $selavy -c "\$parset" >> "\$log"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

    # Now convert the extracted spectral & moment-map artefacts to FITS
     parset=temp.in
     log=$logs/convertToFITS_spectralArtefacts_\${SLURM_JOB_ID}.log
     for dir in $OUTPUT/$selavySpectraDir $OUTPUT/$selavyMomentsDir $OUTPUT/$selavyCubeletsDir; do
         cd "\${dir}"
         neterr=0
         for im in ./*; do 
             casaim=\${im}
             fitsim="\${im}.fits"
             echo "Converting \$casaim to \$fitsim" >> "\$log"
             ${fitsConvertText}
             err=\$?
             if [ \$err -ne 0 ]; then
                 neterr=\$err
             fi
         done
         cd -
     done
     extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${neterr} convertFITSspec "txt,csv"
     rm -f \$parset

else

    echo "FITS conversion failed, so Selavy did not run"

fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
	ID_SOURCEFINDING_SPEC_SCI=$(sbatch ${DEP} "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_SOURCEFINDING_SPEC_SCI}" "Run the source-finder on the science cube ${imageName} with flags \"$DEP\""
    else
	echo "Would run the source-finder on the science cube ${imageName} with slurm file $sbatchfile"
    fi

    echo " "

fi

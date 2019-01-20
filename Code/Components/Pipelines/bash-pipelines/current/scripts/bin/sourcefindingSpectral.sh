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

for subband in ${SUBBAND_WRITER_LIST}; do

    DO_IT=$DO_SOURCE_FINDING_SPEC 

    # set the beam log file, based on the restored cube
    imageCode=restored
    setImageProperties spectral
    beamlog=beamlog.${imageName}.txt

    # set imageName and selavy directories based on the imcontsub cube (if we are making that)
    if [ "${DO_SPECTRAL_IMSUB}" == "true" ]; then
        imageCode=contsub
        setImageProperties spectral
    fi

    # Dependencies for the job
    DEP=""
    if [ "${DO_SPECTRAL_IMSUB}" == "true" ]; then
        if [ "$FIELD" == "." ]; then
            DEP=$(addDep "$DEP" "$(echo "${DEP_LINMOS_SPECTRAL_CONTSUB_ALL}" | sed -e 's/-d afterok://g')")
        elif [ "$BEAM" == "all" ]; then
            DEP=$(addDep "$DEP" "$(echo "${DEP_LINMOS_SPECTRAL_CONTSUB}" | sed -e 's/-d afterok://g')")
        fi
    fi
    if [ "$FIELD" == "." ]; then
        DEP=$(addDep "$DEP" "$(echo "${DEP_LINMOS_SPECTRAL_RESTORED_ALL}" | sed -e 's/-d afterok://g')")
    elif [ "$BEAM" == "all" ]; then
        DEP=$(addDep "$DEP" "$(echo "${DEP_LINMOS_SPECTRAL_RESTORED}" | sed -e 's/-d afterok://g')")
    else
        DEP=$(addDep "$DEP" "$ID_SPECIMG_SCI")
    fi
    
    # Define base string for source IDs
    if [ "${FIELD}" == "." ]; then
        sourceIDbase="SB${SB_SCIENCE}"
    elif [ "${BEAM}" == "all" ]; then
        sourceIDbase="SB${SB_SCIENCE}_${FIELD}"
    else
        sourceIDbase="SB${SB_SCIENCE}_${FIELD}_Beam${BEAM}"
    fi
    if [ ${NUM_SPECTRAL_CUBES} -gt 1 ]; then
        sourceIDbase="${sourceIDbase}.sb${subband}"
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
Selavy.threshold                                = ${SELAVY_SPEC_FLUX_THRESHOLD}"
            if [ "${SELAVY_SPEC_FLAG_GROWTH}" == "true" ] &&
                   [ "${SELAVY_SPEC_GROWTH_THRESHOLD}" != "" ]; then
                thresholdPars="${thresholdPars}
Selavy.flagGrowth                               = ${SELAVY_SPEC_FLAG_GROWTH}
Selavy.growthThreshold                          = ${SELAVY_SPEC_GROWTH_THRESHOLD}"
            fi
        else
            # Use a SNR threshold
            thresholdPars="# Detection threshold
Selavy.snrCut                                   = ${SELAVY_SPEC_SNR_CUT}"
            if [ "${SELAVY_SPEC_FLAG_GROWTH}" == true ] && 
                   [ "${SELAVY_SPEC_GROWTH_CUT}" != "" ]; then
                thresholdPars="${thresholdPars}
Selavy.flagGrowth                               = ${SELAVY_SPEC_FLAG_GROWTH}
Selavy.growthThreshold                          = ${SELAVY_SPEC_GROWTH_CUT}"
            fi
        fi

        # Pre-processing parameters
        preprocessPars="# No pre-processing done"
        # Smoothing takes precedence over reconstruction
        if [ "${SELAVY_SPEC_FLAG_SMOOTH}" == "true" ]; then
            if [ "${SELAVY_SPEC_SMOOTH_TYPE}" == "spectral" ]; then
                preprocessPars="# Spectral smoothing
Selavy.flagSmooth                               = ${SELAVY_SPEC_FLAG_SMOOTH}
Selavy.smoothType                               = ${SELAVY_SPEC_SMOOTH_TYPE}
Selavy.hanningWidth                             = ${SELAVY_SPEC_HANN_WIDTH}"
            else
                preprocessPars="# Spatial smoothing
Selavy.flagSmooth                               = ${SELAVY_SPEC_FLAG_SMOOTH}
Selavy.smoothType                               = ${SELAVY_SPEC_SMOOTH_TYPE}
Selavy.kernMaj                                  = ${SELAVY_SPEC_KERN_MAJ}
Selavy.kernMin                                  = ${SELAVY_SPEC_KERN_MIN}
Selavy.kernPA                                   = ${SELAVY_SPEC_KERN_PA}"
            fi
        elif [ "${SELAVY_SPEC_FLAG_WAVELET}" == "true" ]; then
            preprocessPars="# Multi-resolution wavelet reconstruction
Selavy.flagAtrous                               = ${SELAVY_SPEC_FLAG_WAVELET}
Selavy.reconDim                                 = ${SELAVY_SPEC_RECON_DIM}
Selavy.scaleMin                                 = ${SELAVY_SPEC_RECON_SCALE_MIN}
Selavy.scaleMax                                 = ${SELAVY_SPEC_RECON_SCALE_MAX}
Selavy.snrRecon                                 = ${SELAVY_SPEC_RECON_SNR}"        
        fi
        

        if [ ${NUM_SPECTRAL_CUBES} -gt 1 ]; then
            setJob "science_selavy_spec_${imageName}" selavySpec${subband}
        else
            setJob "science_selavy_spec_${imageName}" selavySpec
        fi
        
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SOURCEFINDING_SPEC}
#SBATCH --ntasks=${NUM_CPUS_SELAVY_SPEC}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY_SPEC}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-selavy-spec-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

# Working directory for the selavy output
seldir=${selavyDir}
    
HAVE_IMAGES=true
BEAM=$BEAM

# List of images to convert to FITS in the Selavy job
imlist=""

# Image to be searched
image="${imageName}"
fitsimage="${imageName%%.fits}.fits"
imlist="\${imlist} ${OUTPUT}/\${image}"
if [ "\${BEAM}" == "all" ]; then
    # Weights image - really only useful if primary-beam corrected
    weights=${weightsImage}
    imlist="\${imlist} ${OUTPUT}/\${weights}"
    weightpars="Selavy.Weights.weightsImage                     = \${weights%%.fits}.fits
Selavy.Weights.weightsCutoff                    = ${SELAVY_SPEC_WEIGHTS_CUTOFF}"
else
    weightpars="#"
fi

HAVE_IMAGES=true
echo "Converting to FITS the following images: \${imlist}"
for im in \${imlist}; do 

    casaim="\${im%%.fits}"
    fitsim="\${im%%.fits}.fits"
    parset="${parsets}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in"
    log="${logs}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log"
    ${fitsConvertText}
    # Make a link so we point to a file in the current directory for
    # Selavy. This gets the referencing correct in the catalogue
    # metadata 
    if [ ! -e "\$fitsim" ]; then
        HAVE_IMAGES=false
        echo "ERROR - Could not create \${im}.fits"
    else
        mkdir -p \${seldir}
        cd \${seldir}
        ln -s -f "\${fitsim}" .
        cd ..
    fi
done

if [ "\${HAVE_IMAGES}" == "true" ]; then

    parset="${parsets}/science_selavy_spectral_\${SLURM_JOB_ID}.in"
    log="${logs}/science_selavy_spectral_\${SLURM_JOB_ID}.log"
    
    mkdir -p \${seldir}
    cd \${seldir}

    # Directories for extracted data products
    mkdir -p "${OUTPUT}/$selavySpectraDir"
    mkdir -p "${OUTPUT}/$selavyMomentsDir"
    mkdir -p "${OUTPUT}/$selavyCubeletsDir"
    
    cat > "\$parset" <<EOFINNER
Selavy.image                                    = \${fitsimage}
Selavy.sbid                                     = ${SB_SCIENCE}
Selavy.sourceIdBase                             = ${sourceIDbase}
Selavy.imageHistory                             = [${imageHistoryString}]
#
Selavy.nsubx                                    = ${SELAVY_SPEC_NSUBX}
Selavy.nsuby                                    = ${SELAVY_SPEC_NSUBY}
Selavy.nsubz                                    = ${SELAVY_SPEC_NSUBZ}
Selavy.overlapx                                 = ${SELAVY_SPEC_OVERLAPX}
Selavy.overlapy                                 = ${SELAVY_SPEC_OVERLAPY}
Selavy.overlapz                                 = ${SELAVY_SPEC_OVERLAPZ}
#
Selavy.resultsFile                              = selavy-\${fitsimage%%.fits}.txt
#
\${weightpars}
#
${thresholdPars}
#
Selavy.searchType                               = ${SELAVY_SPEC_SEARCH_TYPE}
#
${preprocessPars}
#
Selavy.VariableThreshold                        = ${SELAVY_SPEC_VARIABLE_THRESHOLD}
Selavy.VariableThreshold.boxSize                = ${SELAVY_SPEC_BOX_SIZE}
Selavy.VariableThreshold.reuse                  = true
#
Selavy.Fitter.doFit                             = false
#
Selavy.threshSpatial                            = 5
#Selavy.flagAdjacent                            = true
Selavy.threshVelocity                           = 7
#
Selavy.minVoxels                                = 11
Selavy.minPix                                   = ${SELAVY_SPEC_MIN_PIX}
Selavy.minChannels                              = ${SELAVY_SPEC_MIN_CHAN}
Selavy.maxChannels                              = ${SELAVY_SPEC_MAX_CHAN}
Selavy.sortingParam                             = -pflux
Selavy.spectralUnits                            = km/s
#
# Emission-line catalogue
Selavy.HiEmissionCatalogue                      = true
Selavy.optimiseMask                             = ${SELAVY_SPEC_OPTIMISE_MASK}
EOFINNER

    NCORES=${NUM_CPUS_SELAVY_SPEC}
    NPPN=${CPUS_PER_CORE_SELAVY_SPEC}
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $selavy -c "\$parset" >> "\$log"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

else

    echo "FITS conversion failed, so Selavy did not run"
    exit 1

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

done

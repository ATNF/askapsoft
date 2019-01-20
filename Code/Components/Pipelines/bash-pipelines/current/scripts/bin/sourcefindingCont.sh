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

DO_IT=$DO_SOURCE_FINDING_CONT

# set imageName, weightsImage etc
imageCode=restored
setImageProperties cont
contImage=$imageName
contWeights=$weightsImage
pol="%p"
setImageProperties contcube
contCube=$imageName
beamlog=beamlog.${imageName}.txt

# lower-case list of polarisations to use
polList=$(echo "${POL_LIST}" | tr '[:upper:]' '[:lower:]')

# Dependencies for the job
DEP=""
if [ "$FIELD" == "." ]; then
    DEP=$(addDep "$DEP" "$ID_LINMOS_CONT_ALL_RESTORED")
elif [ "$BEAM" == "all" ]; then
    DEP=$(addDep "$DEP" "$ID_LINMOS_CONT_RESTORED")
else
    if [ "${DO_SELFCAL}" == "true" ]; then
        DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
    else
        DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI")
    fi
fi
if [ "${USE_CONTCUBE_FOR_SPECTRAL_INDEX}" == "true" ] ||
       [ "${DO_RM_SYNTHESIS}" == "true" ]; then
    if [ "$FIELD" == "." ]; then
        DEP=$(addDep "$DEP" "$ID_LINMOS_CONTCUBE_ALL_RESTORED")
    elif [ "$BEAM" == "all" ]; then
        DEP=$(addDep "$DEP" "$ID_LINMOS_CONTCUBE_RESTORED")
    else
        DEP=$(addDep "$DEP" "$ID_CONTCUBE_SCI")
    fi
fi

# Define base string for source IDs
if [ "${FIELD}" == "." ]; then
    sourceIDbase="SB${SB_SCIENCE}"
elif [ "${BEAM}" == "all" ]; then
    sourceIDbase="SB${SB_SCIENCE}_${FIELD}"
else
    sourceIDbase="SB${SB_SCIENCE}_${FIELD}_Beam${BEAM}"
fi

if [ ! -e "${OUTPUT}/${contImage}" ] && [ "${DEP}" == "" ] &&
       [ "${SUBMIT_JOBS}" == "true" ]; then
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then
    
    # This adds L1, L2, etc to the job name when LOOP is defined and
    # >0 -- this means that we are running the sourcefinding on the
    # selfcal loop mosaics, and so we also need to change the image &
    # weights names.
    # We also can't do the RM synthesis or the
    # spectral-index-from-contcube option on the LOOP images (since
    # the calibrations don't match), so we turn it off if it is on
    doRM=${DO_RM_SYNTHESIS}
    useContCube=${USE_CONTCUBE_FOR_SPECTRAL_INDEX}
    description=selavyCont
    if [ "$LOOP" != "" ]; then
        if [ "$LOOP" -gt 0 ]; then
            description="selavyContL${LOOP}"
            contImage="${contImage%%.fits}.SelfCalLoop${LOOP}"
            contWeights="${contWeights%%.fits}.SelfCalLoop${LOOP}"
            imageName=${contImage}
            setSelavyDirs cont
            noiseMap="${noiseMap%%.fits}.SelfCalLoop${LOOP}"
            thresholdMap="${thresholdMap%%.fits}.SelfCalLoop${LOOP}"
            meanMap="${meanMap%%.fits}.SelfCalLoop${LOOP}"
            snrMap="${snrMap%%.fits}.SelfCalLoop${LOOP}"
            doRM=false
            useContCube=false
            if [ "${IMAGETYPE_CONT}" == "fits" ]; then
                contImage="${contImage}.fits"
                contWeights="${contWeights}"
                imageName="${contImage}"
                noiseMap="${noiseMap%%.fits}.fits"
                thresholdMap="${thresholdMap%%.fits}.fits"
                meanMap="${meanMap%%.fits}.fits"
                snrMap="${snrMap%%.fits}.fits"
            fi
        fi
    fi

    # Define the detection thresholds in terms of flux or SNR
    if [ "${SELAVY_FLUX_THRESHOLD}" != "" ]; then
        # Use a direct flux threshold if specified
        thresholdPars="# Detection threshold
Selavy.threshold                                = ${SELAVY_FLUX_THRESHOLD}"
        if [ "${SELAVY_FLAG_GROWTH}" == "true" ] && 
               [ "${SELAVY_GROWTH_THRESHOLD}" != "" ]; then
            thresholdPars="${thresholdPars}
Selavy.flagGrowth                               = ${SELAVY_FLAG_GROWTH}
Selavy.growthThreshold                          = ${SELAVY_GROWTH_THRESHOLD}"
        fi
    else
        # Use a SNR threshold
        thresholdPars="# Detection threshold
Selavy.snrCut                                   = ${SELAVY_SNR_CUT}"
        if [ "${SELAVY_FLAG_GROWTH}" == "true" ] &&
               [ "${SELAVY_GROWTH_CUT}" != "" ]; then
            thresholdPars="${thresholdPars}
Selavy.flagGrowth                               = ${SELAVY_FLAG_GROWTH}
Selavy.growthThreshold                          = ${SELAVY_GROWTH_CUT}"
        fi
    fi    

    setJob "science_selavy_cont_${contImage}" "$description"
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SOURCEFINDING_CONT}
#SBATCH --ntasks=${NUM_CPUS_SELAVY}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-selavy-cont-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

HAVE_IMAGES=true
BEAM=$BEAM
NUM_TAYLOR_TERMS=${NUM_TAYLOR_TERMS}

# List of images to convert to FITS in the Selavy job
imlist=""

image=${contImage}
fitsimage=${contImage%%.fits}.fits
weights=${contWeights}
contcube=${contCube}

imlist="\${imlist} ${OUTPUT}/\${image}"

for((n=1;n<\${NUM_TAYLOR_TERMS};n++)); do
    sedstr="s/\.taylor\.0/\.taylor\.\$n/g"
    im=\$(echo \$image | sed -e \$sedstr)
    imlist="\${imlist} ${OUTPUT}/\${im}"
done

if [ "\${BEAM}" == "all" ]; then
    imlist="\${imlist} ${OUTPUT}/\${weights}"
    weightpars="Selavy.Weights.weightsImage                     = \${weights%%.fits}.fits
Selavy.Weights.weightsCutoff                    = ${SELAVY_WEIGHTS_CUTOFF}"
else
    weightpars="#"
fi

doRM=${doRM}
useContCube=${useContCube}

if [ "\${useContCube}" == "true" ] || 
      [ "\${doRM}" == "true" ]; then
    polList="${polList}"
    for p in \${polList}; do
        sedstr="s/%p/\$p/g"
        thisim=\$(echo "\$contcube" | sed -e "\$sedstr")
        if [ -e "${OUTPUT}/\${thisim}" ]; then
            imlist="\${imlist} ${OUTPUT}/\${thisim}"
        else
            if [ "\${doRM}" == "true" ]; then
                doRM=false
                echo "ERROR - Continuum cube \${thisim} not found. RM Synthesis being turned off."
            fi
            if [ "\$p" == "i" ] && [ "\${useContCube}" == "true" ]; then
                useContCube=false
                echo "ERROR - Continuum cube \${thisim} not found."
                echo "      - Will not use continuum cube to find spectral indices"
            fi
        fi
    done
fi

if [ "\${imlist}" != "" ]; then
    for im in \${imlist}; do 
        casaim="\${im%%.fits}"
        fitsim="\${im%%.fits}.fits"
        echo "Converting to FITS the image \${im}"
        parset="${parsets}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in"
        log="${logs}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log"
        ${fitsConvertText}
        if [ ! -e "\$fitsim" ]; then
            HAVE_IMAGES=false
            echo "ERROR - Could not create \${fitsim##*/}"
        fi
        # Make a link so we point to a file in the current directory for
        # Selavy. This gets the referencing correct in the catalogue
        # metadata 
        if [ "\${HAVE_IMAGES}" == "true" ]; then
            mkdir -p ${selavyDir}
            cd ${selavyDir}
            ln -s "\${fitsim}" .
            cd ..
        fi
    done
fi

if [ "\${HAVE_IMAGES}" == "true" ]; then

    parset="${parsets}/science_selavy_cont_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
    log="${logs}/science_selavy_cont_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

    # Directory for extracted polarisation data products
    mkdir -p $selavyPolDir

    # Move to the working directory
    cd $selavyDir

    if [ "\${useContCube}" == "true" ]; then
        # Set the parameter for using contcube to measure spectral-index
        SpectralTermUse="Selavy.spectralTermsFromTaylor                  = false
Selavy.spectralTerms.cube                       = ${OUTPUT}/\$contcube
Selavy.spectralTerms.nterms                     = ${SELAVY_NUM_SPECTRAL_TERMS}"
    else
        haveT1=false
        haveT2=false
        if [ "\${NUM_TAYLOR_TERMS}" -gt 1 ]; then
            t1im=\$(echo "\$image" | sed -e 's/taylor\.0/taylor\.1/g')
            if [ -e "${OUTPUT}/\${t1im}" ]; then
                imlist="\${imlist} ${OUTPUT}/\${t1im}"
                haveT1=true
            fi
            t2im=\$(echo "\$image" | sed -e 's/taylor\.0/taylor\.2/g')
            if [ -e "${OUTPUT}/\${t2im}" ] && [ "\${NUM_TAYLOR_TERMS}" -gt 2 ]; then
                imlist="\${imlist} ${OUTPUT}/\${t2im}"
                haveT2=true
            fi
        fi
        # Set the flag indicating whether to measure from Taylor-term images
        SpectralTermUse="Selavy.spectralTermsFromTaylor                  = true
Selavy.findSpectralTerms                        = [\${haveT1}, \${haveT2}]"
    fi
    
    if [ "\${doRM}" == "true" ]; then
        rmSynthParams="# RM Synthesis on extracted spectra from continuum cube
Selavy.RMSynthesis                              = \${doRM}
Selavy.RMSynthesis.cube                         = ${OUTPUT}/\$contcube
Selavy.RMSynthesis.beamLog                      = ${beamlog}
Selavy.RMSynthesis.outputBase                   = ${OUTPUT}/${selavyPolDir}/${SELAVY_POL_OUTPUT_BASE}
Selavy.RMSynthesis.writeSpectra                 = ${SELAVY_POL_WRITE_SPECTRA}
Selavy.RMSynthesis.writeComplexFDF              = ${SELAVY_POL_WRITE_COMPLEX_FDF}
Selavy.RMSynthesis.boxwidth                     = ${SELAVY_POL_BOX_WIDTH}
Selavy.RMSynthesis.noiseArea                    = ${SELAVY_POL_NOISE_AREA}
Selavy.RMSynthesis.robust                       = ${SELAVY_POL_ROBUST_STATS}
Selavy.RMSynthesis.weightType                   = ${SELAVY_POL_WEIGHT_TYPE}
Selavy.RMSynthesis.modeltype                    = ${SELAVY_POL_MODEL_TYPE}
Selavy.RMSynthesis.modelPolyOrder               = ${SELAVY_POL_MODEL_ORDER}
Selavy.RMSynthesis.polThresholdSNR              = ${SELAVY_POL_SNR_THRESHOLD}
Selavy.RMSynthesis.polThresholdDebias           = ${SELAVY_POL_DEBIAS_THRESHOLD}
Selavy.RMSynthesis.numPhiChan                   = ${SELAVY_POL_NUM_PHI_CHAN}
Selavy.RMSynthesis.deltaPhi                     = ${SELAVY_POL_DELTA_PHI}
Selavy.RMSynthesis.phiZero                      = ${SELAVY_POL_PHI_ZERO}"
    else
        rmSynthParams="# Not performing RM Synthesis for this case
Selavy.RMSynthesis                              = \${doRM}"
    fi

    cat > "\$parset" <<EOFINNER
Selavy.image                                    = \${fitsimage}
Selavy.sbid                                     = ${SB_SCIENCE}
Selavy.sourceIdBase                             = ${sourceIDbase}
Selavy.imageHistory                             = [${imageHistoryString}]
Selavy.imagetype                                = ${IMAGETYPE_CONT}
#
\${SpectralTermUse}
Selavy.nsubx                                    = ${SELAVY_NSUBX}
Selavy.nsuby                                    = ${SELAVY_NSUBY}
Selavy.overlapx                                 = ${SELAVY_OVERLAPX}
Selavy.overlapy                                 = ${SELAVY_OVERLAPY}
#
Selavy.resultsFile                              = selavy-\${fitsimage%%.fits}.txt
#
${thresholdPars}
#
Selavy.VariableThreshold                        = ${SELAVY_VARIABLE_THRESHOLD}
Selavy.VariableThreshold.boxSize                = ${SELAVY_BOX_SIZE}
Selavy.VariableThreshold.ThresholdImageName     = ${thresholdMap%%.fits}
Selavy.VariableThreshold.NoiseImageName         = ${noiseMap%%.fits}
Selavy.VariableThreshold.AverageImageName       = ${meanMap%%.fits}
Selavy.VariableThreshold.SNRimageName           = ${snrMap%%.fits}
Selavy.VariableThreshold.imagetype              = ${IMAGETYPE_CONT}
\${weightpars}
#
Selavy.Fitter.doFit                             = true
Selavy.Fitter.fitTypes                          = [full]
Selavy.Fitter.numGaussFromGuess                 = true
Selavy.Fitter.maxReducedChisq                   = 10.
Selavy.Fitter.imagetype                         = ${IMAGETYPE_CONT}
#
Selavy.threshSpatial                            = ${SELAVY_SPATIAL_THRESHOLD}
Selavy.flagAdjacent                             = ${SELAVY_FLAG_ADJACENT}
#
Selavy.minPix                                   = 3
Selavy.minVoxels                                = 3
Selavy.minChannels                              = 1
Selavy.sortingParam                             = -pflux
#
\${rmSynthParams}
EOFINNER

    NCORES=${NUM_CPUS_SELAVY}
    NPPN=${CPUS_PER_CORE_SELAVY}
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $selavy -c "\$parset" >> "\$log"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

    # Convert the noise map to fits, for use with the validation script
    casaim="${noiseMap%%.fits}"
    fitsim="${noiseMap%%.fits}.fits"
    echo "Converting to FITS the image ${noiseMap}"
    parset="${parsets}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in"
    log="${logs}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log"
    ${fitsConvertText}
    if [ ! -e "\$fitsim" ]; then
        echo "ERROR - Could not create \${fitsim}"
    fi

    doValidation=${DO_CONTINUUM_VALIDATION}
    validatePerBeam=${VALIDATE_BEAM_IMAGES}
    if [ "\${doValidation}" == "true" ] && [ "\${BEAM}" != "all" ]; then
        doValidation=\${validatePerBeam}
    fi
    ACES=${ACES_LOCATION}
    scriptname="\${ACES}/UserScripts/col52r/ASKAP_continuum_validation.py"
    if [ "\${doValidation}" == "true" ]; then
        if [ ! -e "\${scriptname}" ]; then
            echo "ERROR - Validation script \${scriptname} not found"
        else
            cd ..
            loadModule continuum_validation_env
            log="${logs}/continuum_validation_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
            validateArgs="\${fitsimage%%.fits}.fits"
            validateArgs="\${validateArgs} -S ${selavyDir}/selavy-\${fitsimage%%.fits}.components.xml"
            validateArgs="\${validateArgs} -N ${selavyDir}/${noiseMap%%.fits}.fits "
            validateArgs="\${validateArgs} -C NVSS_config.txt,SUMSS_config.txt"          
            STARTTIME=\$(date +%FT%T)
            NCORES=1
            NPPN=1
            srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" \${scriptname} \${validateArgs} > "\${log}"
            err=\$?
            unloadModule continuum_validation_env
            echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
            extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "validationCont" "txt,csv"

            validationDir=${validationDir}
            if [ ! -e "\${validationDir}" ]; then
                echo "ERROR - could not create validation directory \${validationDir}"
            fi
            # Place a copy in a standard place on /group
            copyLocation="${VALIDATION_ARCHIVE_DIR}"
            if [ "\${copyLocation}" != "" ] && [ -e "\${copyLocation}" ]; then
                copyLocation="\${copyLocation}/${PROJECT_ID}/SB${SB_SCIENCE}"
                mkdir -p \${copyLocation}
                purgeCSV="${REMOVE_VALIDATION_CSV}"
                validationDirCopy="\${copyLocation}/\${validationDir}__\$(whoami)_${NOW}"
                echo "Copying Validation results to \${validationDirCopy}"
                cp -r \${validationDir} \${validationDirCopy}
                if [ "\${purgeCSV}" == "true" ]; then
                    rm -f \${validationDirCopy}/*.csv
                fi
            fi
            chmod -R g+w \${validationDirCopy}
        fi
    fi

else

    echo "FITS conversion failed, so Selavy did not run"

fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
	ID_SOURCEFINDING_CONT_SCI=$(sbatch ${DEP} "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_SOURCEFINDING_CONT_SCI}" "Run the continuum source-finding on the science image ${contImage} with flags \"$DEP\""
    else
	echo "Would run the continuum source-finding on the science image ${contImage} with slurm file $sbatchfile"
    fi

    echo " "

    
fi

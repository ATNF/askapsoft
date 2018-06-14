#!/bin/bash -l
#
# Sets up the continuum-subtraction job for the case where the
# continuum is represented by a model image created by cmodel from a
# Selavy catalogue
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

# In this bit we use Selavy to make a continuum catalogue, then
# cmodel to create the corresponding continuum model image, which
# is then subtracted from the MS

imageCode=restored
setImageProperties cont
contImage=$imageName
selavyImage="${OUTPUT}/${contImage}"
setImageProperties contcube
contCube="${OUTPUT}/${imageName}"

NPROCS_CONTSUB=$(echo "${CONTSUB_SELAVY_NSUBX}" "${CONTSUB_SELAVY_NSUBY}" | awk '{print $1*$2+1}')
if [ "${NPROCS_CONTSUB}" -eq 1 ]; then
    # If Selavy is just serial, increase nprocs to 2 for cmodel
    NPROCS_CONTSUB=2
fi
if [ "${NPROCS_CONTSUB}" -le 20 ]; then
    CPUS_PER_CORE_CONTSUB=${NPROCS_CONTSUB}
else
    CPUS_PER_CORE_CONTSUB=20
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

ContsubModelDefinition="# The model definition
CContsubtract.sources.names                       = [lsm]
CContsubtract.sources.lsm.direction               = \${modelDirection}
CContsubtract.sources.lsm.model                   = \${contsubDir}/\${contsubCmodelImage}
CContsubtract.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}"
if [ "${NUM_TAYLOR_TERMS}" -gt 1 ]; then
    if [ "$MFS_REF_FREQ" == "" ]; then
        freq=$CENTRE_FREQ
    else
        freq=${MFS_REF_FREQ}
    fi
    ContsubModelDefinition="$ContsubModelDefinition
CContsubtract.visweights                          = MFS
CContsubtract.visweights.MFS.reffreq              = ${freq}"
fi

cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_CONTSUB}
#SBATCH --ntasks=${NPROCS_CONTSUB}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTSUB}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-contsubSLsci-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

log=${logs}/mslist_for_ccontsub_\${SLURM_JOB_ID}.log
NCORES=1
NPPN=1
DIRECTION="$DIRECTION"
if [ "\${DIRECTION}" != "" ]; then
    modelDirection="\${DIRECTION}"
else
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $mslist --full "${msSciSL}" 1>& "\${log}"
    ra=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$log" --val=RA)
    dec=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$log" --val=Dec)
    epoch=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$log" --val=Epoch)
    modelDirection="[\${ra}, \${dec}, \${epoch}]"
fi

setContsubFilenames
mkdir -p \${contsubDir}
cd \${contsubDir}

#################################################
# First, source-finding

if [ "\${useContCube}" == "true" ]; then
    # Set the parameter for using contcube to measure spectral-index
    SpectralTermUse="Selavy.spectralTermsFromTaylor                  = false
Selavy.spectralTerms.cube                       = ${contCube}
Selavy.spectralTerms.nterms                     = ${SELAVY_NUM_SPECTRAL_TERMS}"
else
    haveT1=false
    haveT2=false
    image=${contImage}
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

parset=${parsets}/selavy_for_contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in
log=${logs}/selavy_for_contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log
cat >> "\$parset" <<EOFINNER
##########
## Source-finding with selavy
##
# The image to be searched
Selavy.image                                    = ${selavyImage}
Selavy.sbid                                     = ${SB_SCIENCE}
Selavy.imageHistory                             = [${imageHistoryString}]
#
# This is how we divide it up for distributed processing, with the
#  number of subdivisions in each direction, and the size of the
#  overlap region in pixels
\${SpectralTermUse}
Selavy.nsubx                                    = ${SELAVY_NSUBX}
Selavy.nsuby                                    = ${SELAVY_NSUBY}
#
${thresholdPars}
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
Selavy.Fitter.fitTypes                          = [full]
Selavy.Fitter.numGaussFromGuess                 = true
Selavy.Fitter.maxReducedChisq                   = 10.
# Force the component maps to be casa images for now
Selavy.Fitter.imagetype                         = casa
#
Selavy.threshSpatial                            = 5
Selavy.flagAdjacent                             = false
#
# Size criteria for the final list of detected islands
Selavy.minPix                                   = 3
Selavy.minVoxels                                = 3
Selavy.minChannels                              = 1
#
# How the islands are sorted in the final catalogue - by
#  integrated flux in this case
Selavy.sortingParam                             = -iflux
EOFINNER

NCORES=${NPROCS_CONTSUB}
NPPN=${CPUS_PER_CORE_CONTSUB}
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${selavy} -c "\${parset}" > "\${log}"
err=\$?
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_selavy "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

componentsCatalogue=selavy-results.components.xml
numComp=\$(grep -c "<TR>" "\${componentsCatalogue}")
if [ "\${numComp}" -eq 0 ]; then
    # Nothing detected
    echo "Continuum subtraction : No continuum components found!"
else

    #################################################
    # Next, model image creation

    parset=${parsets}/cmodel_for_contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in
    log=${logs}/cmodel_for_contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log
    cat > "\$parset" <<EOFINNER
# The below specifies the GSM source is a selavy output file
Cmodel.gsm.database       = votable
Cmodel.gsm.file           = \${componentsCatalogue}
Cmodel.gsm.ref_freq       = ${CENTRE_FREQ}Hz

# General parameters
Cmodel.bunit              = Jy/pixel
Cmodel.frequency          = ${CENTRE_FREQ}Hz
Cmodel.increment          = ${BANDWIDTH}Hz
Cmodel.flux_limit         = ${CONTSUB_MODEL_FLUX_LIMIT}
Cmodel.shape              = [${NUM_PIXELS_CONT},${NUM_PIXELS_CONT}]
Cmodel.cellsize           = [${CELLSIZE_CONT}arcsec, ${CELLSIZE_CONT}arcsec]
Cmodel.direction          = \${modelDirection}
Cmodel.stokes             = [I]
Cmodel.nterms             = ${NUM_TAYLOR_TERMS}

# Output specific parameters
Cmodel.output             = casa
Cmodel.filename           = \${contsubCmodelImage}
EOFINNER

    NCORES=2
    NPPN=2
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${cmodel} -c "\${parset}" > "\${log}"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_cmodel "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

    cd ..

    #################################################
    # Then, continuum subtraction

    parset=${parsets}/contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in
    log=${logs}/contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log
    cat > "\$parset" <<EOFINNER
# The measurement set name - this will be overwritten
CContSubtract.dataset                             = ${msSciSL}
${ContsubModelDefinition}
# The gridding parameters
CContSubtract.gridder.snapshotimaging             = ${GRIDDER_SNAPSHOT_IMAGING}
CContSubtract.gridder.snapshotimaging.wtolerance  = ${GRIDDER_SNAPSHOT_WTOL}
CContSubtract.gridder.snapshotimaging.longtrack   = ${GRIDDER_SNAPSHOT_LONGTRACK}
CContSubtract.gridder.snapshotimaging.clipping    = ${GRIDDER_SNAPSHOT_CLIPPING}
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

    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${ccontsubtract} -c "\${parset}" > "\${log}"
    err=\$?
    rejuvenate ${msSciSL}
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch $CONT_SUB_CHECK_FILE
    fi

fi

EOFOUTER

#!/bin/bash -l
#
# Launches a job to image the current beam of the science
# observation, at full spectral resolution, using the spectral-line
# imager 'simager'
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

ID_SPECIMG_SCI=""

DO_IT=$DO_SPECTRAL_IMAGING

if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ]; then
    theImager=$altimager
    Imager="Cimager"
else
    theImager=$simager
    Imager="Simager"
fi

for subband in ${SUBBAND_WRITER_LIST}; do
    imageCode=restored
    setImageProperties spectral
    if [ "${CLOBBER}" != "true" ] && [ -e "${imageName}" ]; then
        if [ "${DO_IT}" == "true" ]; then
            echo "Image ${imageName} exists, so not running spectral-line imaging for beam ${BEAM}"
            echo " "
        fi
        DO_IT=false
    fi
done
unset subband

# Data selection
# UV distance
if [ "${SPECTRAL_IMAGE_MAXUV}" -gt 0 ]; then
    dataSelection="# Apply a maximum UV cutoff
${Imager}.MaxUV                                   = ${SPECTRAL_IMAGE_MAXUV}"
fi
if [ "${SPECTRAL_IMAGE_MINUV}" -gt 0 ]; then
    dataSelection="${dataSelection}
# Apply a minimum UV cutoff
${Imager}.MinUV                                   = ${SPECTRAL_IMAGE_MINUV}"
fi
if [ "${dataSelection}" == "" ]; then
    dataSelection="# No further data selection made in the parset"
fi

# Define the preconditioning
preconditioning="${Imager}.preconditioner.Names                    = ${PRECONDITIONER_LIST_SPECTRAL}"
if [ "$(echo "${PRECONDITIONER_LIST_SPECTRAL}" | grep GaussianTaper)" != "" ]; then
    preconditioning="$preconditioning
${Imager}.preconditioner.GaussianTaper            = ${PRECONDITIONER_SPECTRAL_GAUSS_TAPER}"
fi
if [ "$(echo "${PRECONDITIONER_LIST_SPECTRAL}" | grep Wiener)" != "" ]; then
    # Use the new preservecf preconditioner option, but only for the
    # Wiener filter
    preconditioning="$preconditioning
${Imager}.preconditioner.preservecf               = true"
    if [ "${PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS}" != "" ]; then
	preconditioning="$preconditioning
${Imager}.preconditioner.Wiener.robustness        = ${PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS}"
    fi
    if [ "${PRECONDITIONER_SPECTRAL_WIENER_TAPER}" != "" ]; then
	preconditioning="$preconditioning
${Imager}.preconditioner.Wiener.taper             = ${PRECONDITIONER_SPECTRAL_WIENER_TAPER}"
    fi
fi
shapeDefinition="# Leave shape definition to advise"
if [ "${NUM_PIXELS_SPECTRAL}" != "" ] && [ "${NUM_PIXELS_SPECTRAL}" -gt 0 ]; then
    shapeDefinition="${Imager}.Images.shape                            = [${NUM_PIXELS_SPECTRAL}, ${NUM_PIXELS_SPECTRAL}]"
else
    echo "WARNING - No valid NUM_PIXELS_SPECTRAL parameter given.  Not running spectral imaging."
    DO_IT=false
fi
cellsizeGood=$(echo "${CELLSIZE_SPECTRAL}" | awk '{if($1>0.) print "true"; else print "false";}')
if [ "${CELLSIZE_SPECTRAL}" != "" ] && [ "$cellsizeGood" == "true" ]; then
    cellsizeDefinition="${Imager}.Images.cellsize                         = [${CELLSIZE_SPECTRAL}arcsec, ${CELLSIZE_SPECTRAL}arcsec]"
else
    echo "WARNING - No valid CELLSIZE_SPECTRAL parameter given.  Not running spectral imaging."
    DO_IT=false
fi
restFrequency="# No rest frequency specified"
if [ "${REST_FREQUENCY_SPECTRAL}" != "" ]; then
    restFrequency="${Imager}.Images.restFrequency                    = ${REST_FREQUENCY_SPECTRAL}"
fi

cleaningPars="# These parameters define the clean algorithm
${Imager}.solver                                  = ${SOLVER_SPECTRAL}"
if [ "${SOLVER_SPECTRAL}" == "Clean" ]; then
    cleaningPars="${cleaningPars}
${Imager}.solver.Clean.algorithm                  = ${CLEAN_SPECTRAL_ALGORITHM}
${Imager}.solver.Clean.niter                      = ${CLEAN_SPECTRAL_MINORCYCLE_NITER}
${Imager}.solver.Clean.gain                       = ${CLEAN_SPECTRAL_GAIN}
${Imager}.solver.Clean.scales                     = ${CLEAN_SPECTRAL_SCALES}
${Imager}.solver.Clean.solutiontype               = ${CLEAN_SPECTRAL_SOLUTIONTYPE}
${Imager}.solver.Clean.verbose                    = False
${Imager}.solver.Clean.tolerance                  = 0.01
${Imager}.solver.Clean.weightcutoff               = zero
${Imager}.solver.Clean.weightcutoff.clean         = false
${Imager}.solver.Clean.psfwidth                   = ${CLEAN_SPECTRAL_PSFWIDTH}
${Imager}.solver.Clean.logevery                   = 50
${Imager}.threshold.minorcycle                    = ${CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE}
${Imager}.threshold.majorcycle                    = ${CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE}
${Imager}.ncycles                                 = ${CLEAN_SPECTRAL_NUM_MAJORCYCLES}
${Imager}.Images.writeAtMajorCycle                = ${CLEAN_SPECTRAL_WRITE_AT_MAJOR_CYCLE}
"
fi
if [ "${SOLVER_SPECTRAL}" == "Dirty" ]; then
    cleaningPars="${cleaningPars}
${Imager}.solver.Dirty.tolerance                  = 0.01
${Imager}.solver.Dirty.verbose                    = False
${Imager}.ncycles                                 = 0"
fi

restorePars="# These parameter govern the restoring of the image and the recording of the beam
${Imager}.restore                                 = ${RESTORE_SPECTRAL}"
if [ "${RESTORE_SPECTRAL}" == "true" ]; then
    restorePars="${restorePars}
${Imager}.restore.beam                            = ${RESTORING_BEAM_SPECTRAL}
${Imager}.restore.beam.cutoff                     = ${RESTORING_BEAM_CUTOFF_SPECTRAL}
${Imager}.restore.beamReference                   = ${RESTORING_BEAM_REFERENCE}"
fi


# This is for the new (alt) imager
altImagerParams="# Options for the alternate imager"
if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ]; then


    if [ "${NCHAN_PER_CORE_SL}" == "" ]; then
        nchanpercore=1
    else
        nchanpercore="${NCHAN_PER_CORE_SL}"
    fi
    altImagerParams="${altImagerParams}
Cimager.nchanpercore                           = ${nchanpercore}"
    if [ "${USE_TMPFS}" == "true" ]; then
        usetmpfs="true"
    else
        usetmpfs="false"
    fi
    altImagerParams="${altImagerParams}
Cimager.usetmpfs                               = ${usetmpfs}"
    if [ "${TMPFS}" == "" ]; then
        tmpfs="/dev/shm"
    else
        tmpfs="${TMPFS}"
    fi
    altImagerParams="${altImagerParams}
Cimager.tmpfs                                   = ${tmpfs}"
    altImagerParams="${altImagerParams}
# barycentre and multiple solver mode not supported in continuum imaging (yet)
Cimager.freqframe                               = ${FREQ_FRAME_SL}
Cimager.solverpercore                           = true
Cimager.nwriters                                = ${NUM_SPECTRAL_WRITERS}
Cimager.singleoutputfile                        = ${ALT_IMAGER_SINGLE_FILE}"

    if [ "${OUTPUT_CHANNELS_SL}" != "" ]; then
        altImagerParams="${altImagerParams}
# Output channel specification
Cimager.Frequencies                             = ${OUTPUT_CHANNELS_SL}"
    fi

else
    altImagerParams="${altImagerParams} are not required"
fi

nameDefinition="${Imager}.Images"
if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ]; then
    nameDefinition="${nameDefinition}.Names                           = [image.${imageBase}]"
else
    nameDefinition="${nameDefinition}.name                            = image.${imageBase}"
fi


if [ "${DO_IT}" == "true" ]; then

    echo "Imaging the spectral-line science observation"

    setJob science_spectral_imager spec
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_SPECIMG_SCI}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SPEC_IMAGING}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-spectralImaging-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

direction="${DIRECTION}"
if [ "\${direction}" != "" ]; then
    directionDefinition="${Imager}.Images.direction                       = \${direction}"
else
    msMetadata="${MS_METADATA}"
    ra=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=RA)
    dec=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Dec)
    epoch=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Epoch)
    directionDefinition="${Imager}.Images.direction                       = [\${ra}, \${dec}, \${epoch}]"
fi

parset="${parsets}/science_spectral_imager_${FIELDBEAM}_\${SLURM_JOB_ID}.in"

cat > "\$parset" << EOF
${Imager}.dataset                                 = ${msSciSL}
${Imager}.imagetype                               = ${IMAGETYPE_SPECTRAL}
${dataSelection}
#
${nameDefinition}
${shapeDefinition}
${cellsizeDefinition}
\${directionDefinition}
${restFrequency}
${altImagerParams}
#
# This defines the parameters for the gridding.
${Imager}.gridder.snapshotimaging                 = ${GRIDDER_SPECTRAL_SNAPSHOT_IMAGING}
${Imager}.gridder.snapshotimaging.wtolerance      = ${GRIDDER_SPECTRAL_SNAPSHOT_WTOL}
${Imager}.gridder.snapshotimaging.longtrack       = ${GRIDDER_SPECTRAL_SNAPSHOT_LONGTRACK}
${Imager}.gridder.snapshotimaging.clipping        = ${GRIDDER_SPECTRAL_SNAPSHOT_CLIPPING}
${Imager}.gridder                                 = WProject
${Imager}.gridder.WProject.wmax                   = ${GRIDDER_SPECTRAL_WMAX}
${Imager}.gridder.WProject.nwplanes               = ${GRIDDER_SPECTRAL_NWPLANES}
${Imager}.gridder.WProject.oversample             = ${GRIDDER_SPECTRAL_OVERSAMPLE}
${Imager}.gridder.WProject.maxsupport             = ${GRIDDER_SPECTRAL_MAXSUPPORT}
${Imager}.gridder.WProject.variablesupport        = true
${Imager}.gridder.WProject.offsetsupport          = true
#
${cleaningPars}
#
${preconditioning}
#
${restorePars}
EOF

log="${logs}/science_spectral_imager_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

# Now run the simager
NCORES=${NUM_CPUS_SPECIMG_SCI}
NPPN=${CPUS_PER_CORE_SPEC_IMAGING}
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${theImager} ${PROFILE_FLAG} -c "\$parset" > "\$log"
err=\$?

# Handle the profiling files
doProfiling=${USE_PROFILING}
if [ "\${doProfiling}" == "true" ]; then
    dir=Profiling/Beam${BEAM}
    mkdir -p \$dir
    mv profile.*.${imageBase}* \${dir}
fi

rejuvenate ${msSciSL}
for im in *.${imageBase}*; do
    rejuvenate \$im
done
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"

if [ \${err} -ne 0 ]; then
    echo "Error: ${theImager} returned error code \${err}"
    exit \$err
fi

# Find the cube statistics
loadModule mpi4py
IMAGE_BASE_SPECTRAL=${IMAGE_BASE_SPECTRAL}
BEAM=${BEAM}
FIELD=${FIELD}
IMAGETYPE_SPECTRAL=${IMAGETYPE_SPECTRAL}
DO_ALT_IMAGER_SPECTRAL=${DO_ALT_IMAGER_SPECTRAL}
ALT_IMAGER_SINGLE_FILE=${ALT_IMAGER_SINGLE_FILE}
for subband in ${SUBBAND_WRITER_LIST}; do
    for imageCode in restored residual; do
        setImageProperties spectral
        echo "Finding cube stats for \${imageName}"
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${imageName}
    done
done


EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SL_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CAL_APPLY_SL_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CONT_SUB_SL_SCI_LIST")
	if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then 
                    DEP=$(addDep "$DEP" "$ID_MSCONCAT_SCI_SPECTRAL")
	fi
	ID_SPECIMG_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
        DEP_SPECIMG=$(addDep "$DEP_SPECIMG" "$ID_SPECIMG_SCI")
	recordJob "${ID_SPECIMG_SCI}" "Make a spectral-line cube for beam $BEAM of the science observation, with flags \"$DEP\""
    else
	echo "Would make a spectral-line cube for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

#!/bin/bash -l
#
# Launches a job to image the current beam of the averaged science MS
# to form a "continuum cube". Includes iteration over the list of
# requested polarisations. This uses the $POL_LIST variable, which
# should be something like "I Q U V".
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

ID_CONTCUBE_SCI=""

if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ]; then
    theImager=$altimager
    Imager="Cimager"
else
    theImager=$simager
    Imager="Simager"
fi

for POLN in $POL_LIST; do

    # make a lower-case version of the polarisation, for image name
    pol=$(echo "$POLN" | tr '[:upper:]' '[:lower:]')

    imageCode=residual
    setImageProperties contcube
    residualCube=${imageName}
    imageCode=restored
    setImageProperties contcube

    DO_IT=$DO_CONTCUBE_IMAGING

    if [ "${CLOBBER}" != "true" ] && [ -e "${imageName}" ]; then
        if [ "${DO_IT}" == "true" ]; then
            echo "Image ${imageName} exists, so not running continuum-cube imaging for beam ${BEAM}"
            echo " "
        fi
        DO_IT=false
    fi

    if [ "${DO_APPLY_CAL_CONT}" == "true" ]; then
        msToUse=$msSciAvCal
    else
        msToUse=$msSciAv
    fi

    # Define the image polarisation
    polarisation="${Imager}.Images.polarisation                     = [\"${POLN}\"]"

    # Define the preconditioning
    preconditioning="${Imager}.preconditioner.Names                    = ${PRECONDITIONER_LIST}"
    if [ "$(echo "${PRECONDITIONER_LIST}" | grep GaussianTaper)" != "" ]; then
        preconditioning="$preconditioning
${Imager}.preconditioner.GaussianTaper            = ${PRECONDITIONER_GAUSS_TAPER}"
    fi
    if [ "$(echo "${PRECONDITIONER_LIST}" | grep Wiener)" != "" ]; then
        # Use the new preservecf preconditioner option, but only for the
        # Wiener filter
        preconditioning="$preconditioning
${Imager}.preconditioner.preservecf               = true"
        if [ "${PRECONDITIONER_WIENER_ROBUSTNESS}" != "" ]; then
	    preconditioning="$preconditioning
${Imager}.preconditioner.Wiener.robustness        = ${PRECONDITIONER_WIENER_ROBUSTNESS}"
        fi
        if [ "${PRECONDITIONER_WIENER_TAPER}" != "" ]; then
	    preconditioning="$preconditioning
${Imager}.preconditioner.Wiener.taper             = ${PRECONDITIONER_WIENER_TAPER}"
        fi
    fi
    shapeDefinition="# Leave shape definition to advise"
    if [ "${NUM_PIXELS_CONTCUBE}" != "" ] && [ "${NUM_PIXELS_CONTCUBE}" -gt 0 ]; then
        shapeDefinition="${Imager}.Images.shape                            = [${NUM_PIXELS_CONTCUBE}, ${NUM_PIXELS_CONTCUBE}]"
    else
        echo "WARNING - No valid NUM_PIXELS_CONTCUBE parameter given.  Not running continuum cube imaging."
        DO_IT=false
    fi
    cellsizeGood=$(echo "${CELLSIZE_CONTCUBE}" | awk '{if($1>0.) print "true"; else print "false";}')
    if [ "${CELLSIZE_CONTCUBE}" != "" ] && [ "$cellsizeGood" == "true" ]; then
        cellsizeDefinition="${Imager}.Images.cellsize                         = [${CELLSIZE_CONTCUBE}arcsec, ${CELLSIZE_CONTCUBE}arcsec]"
    else
        echo "WARNING - No valid CELLSIZE_CONTCUBE parameter given.  Not running continuum cube imaging."
        DO_IT=false
    fi
    restFrequency="# No rest frequency specified for continuum cubes"
    if [ "${REST_FREQUENCY_CONTCUBE}" != "" ]; then
        restFrequency="${Imager}.Images.restFrequency                    = ${REST_FREQUENCY_CONTCUBE}"
    fi

    cleaningPars="# These parameters define the clean algorithm
${Imager}.solver                                  = ${SOLVER_CONTCUBE}"
    if [ "${SOLVER_CONTCUBE}" == "Clean" ]; then
        cleaningPars="${cleaningPars}
${Imager}.solver.Clean.algorithm                  = ${CLEAN_CONTCUBE_ALGORITHM}
${Imager}.solver.Clean.niter                      = ${CLEAN_CONTCUBE_MINORCYCLE_NITER}
${Imager}.solver.Clean.gain                       = ${CLEAN_CONTCUBE_GAIN}
${Imager}.solver.Clean.scales                     = ${CLEAN_CONTCUBE_SCALES}
${Imager}.solver.Clean.solutiontype               = ${CLEAN_CONTCUBE_SOLUTIONTYPE}
${Imager}.solver.Clean.verbose                    = False
${Imager}.solver.Clean.tolerance                  = 0.01
${Imager}.solver.Clean.weightcutoff               = zero
${Imager}.solver.Clean.weightcutoff.clean         = false
${Imager}.solver.Clean.psfwidth                   = ${CLEAN_CONTCUBE_PSFWIDTH}
${Imager}.solver.Clean.logevery                   = 50
${Imager}.threshold.minorcycle                    = ${CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE}
${Imager}.threshold.majorcycle                    = ${CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE}
${Imager}.ncycles                                 = ${CLEAN_CONTCUBE_NUM_MAJORCYCLES}
${Imager}.Images.writeAtMajorCycle                = ${CLEAN_CONTCUBE_WRITE_AT_MAJOR_CYCLE}
"
    fi
    if [ "${SOLVER}" == "Dirty" ]; then
        cleaningPars="${cleaningPars}
${Imager}.solver.Dirty.tolerance                  = 0.01
${Imager}.solver.Dirty.verbose                    = False
${Imager}.ncycles                                 = 0"
    fi

    restorePars="# These parameter govern the restoring of the image and the recording of the beam
${Imager}.restore                                 = true
${Imager}.restore.beam                            = ${RESTORING_BEAM_CONTCUBE}
${Imager}.restore.beam.cutoff                     = ${RESTORING_BEAM_CUTOFF_CONTCUBE}
${Imager}.restore.beamReference                   = ${RESTORING_BEAM_CONTCUBE_REFERENCE}"

    # This is for the new (alt) imager
    altImagerParams="# Options for the alternate imager"
    if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ]; then

        altImagerParams="${altImagerParams}
Cimager.nchanpercore                           = ${NCHAN_PER_CORE_CONTCUBE}"
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
Cimager.barycentre                              = false
Cimager.solverpercore                           = true
Cimager.nwriters                                = ${NUM_SPECTRAL_WRITERS_CONTCUBE}
Cimager.singleoutputfile                        = ${ALT_IMAGER_SINGLE_FILE_CONTCUBE}"

        # we also need to change the CPU allocations

    else
        altImagerParams="${altImagerParams} are not required"
    fi

    nameDefinition="${Imager}.Images"
    if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ]; then
        nameDefinition="${nameDefinition}.Names                           = [image.${imageBase}]"
    else
        nameDefinition="${nameDefinition}.name                            = image.${imageBase}"
    fi

    if [ "${DO_IT}" == "true" ]; then

        echo "Imaging the continuum cube, polarisation $POLN, for the science observation"

        setJob "science_contcube_imager_${POLN}" "contcube${POLN}"
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CONTCUBE_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_CONTCUBE_SCI}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTCUBE_IMAGING}
#SBATCH --job-name ${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-contcubeImaging-${BEAM}-${POLN}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

ms=${msToUse}

if [ "${DIRECTION}" != "" ]; then
    directionDefinition="${Imager}.Images.direction                       = ${DIRECTION}"
else
    msMetadata="${MS_METADATA}"
    ra=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=RA)
    dec=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Dec)
    epoch=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Epoch)
    directionDefinition="${Imager}.Images.direction                       = [\${ra}, \${dec}, \${epoch}]"
fi

parset="${parsets}/science_contcube_imager_${FIELDBEAM}_${POLN}_\${SLURM_JOB_ID}.in"
cat > "\$parset" << EOF
${Imager}.dataset                                 = \${ms}
${Imager}.imagetype                               = ${IMAGETYPE_CONTCUBE}
#
${nameDefinition}
${polarisation}
${shapeDefinition}
${cellsizeDefinition}
\${directionDefinition}
${restFrequency}
${altImagerParams}
#
# This defines the parameters for the gridding.
${Imager}.gridder.snapshotimaging                 = ${GRIDDER_SNAPSHOT_IMAGING}
${Imager}.gridder.snapshotimaging.wtolerance      = ${GRIDDER_SNAPSHOT_WTOL}
${Imager}.gridder.snapshotimaging.longtrack       = ${GRIDDER_SNAPSHOT_LONGTRACK}
${Imager}.gridder.snapshotimaging.clipping        = ${GRIDDER_SNAPSHOT_CLIPPING}
${Imager}.gridder                                 = WProject
${Imager}.gridder.WProject.wmax                   = ${GRIDDER_WMAX}
${Imager}.gridder.WProject.nwplanes               = ${GRIDDER_NWPLANES}
${Imager}.gridder.WProject.oversample             = ${GRIDDER_OVERSAMPLE}
${Imager}.gridder.WProject.maxsupport             = ${GRIDDER_MAXSUPPORT}
${Imager}.gridder.WProject.variablesupport        = true
${Imager}.gridder.WProject.offsetsupport          = true
#
${cleaningPars}
#
${preconditioning}
#
${restorePars}
EOF

log="${logs}/science_contcube_imager_${FIELDBEAM}_${POLN}_\${SLURM_JOB_ID}.log"

# Now run the simager
NCORES=${NUM_CPUS_CONTCUBE_SCI}
NPPN=${CPUS_PER_CORE_CONTCUBE_IMAGING}
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${theImager} ${PROFILE_FLAG} -c \$parset > \$log
err=\$?

# Handle the profiling files
doProfiling=${USE_PROFILING}
if [ "\${doProfiling}" == "true" ]; then
    dir=Profiling/Beam${BEAM}
    mkdir -p \$dir
    mv profile.*.${imageBase}* \${dir}
fi

rejuvenate \${ms}
for im in ./*.${imageBase}*; do
    rejuvenate "\$im"
done
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"

if [ \${err} -ne 0 ]; then
    echo "Error: ${theImager} returned error code \${err}"
    exit 1
fi

# Find the cube statistics
loadModule mpi4py
cube=${imageName}
echo "Finding cube stats for \${cube}"
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${cube}
cube=${residualCube}
echo "Finding cube stats for \${cube}"
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${cube}


EOFOUTER

        if [ "${SUBMIT_JOBS}" == "true" ]; then
            DEP=""
            DEP=$(addDep "$DEP" "$DEP_START")
            DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI_LIST")
            DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV_LIST")
            #DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
            DEP=$(addDep "$DEP" "$ID_CAL_APPLY_CONT_SCI")
            ID_CONTCUBE_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
            DEP_CONTCUBE=$(addDep "$DEP_CONTCUBE" "$ID_CONTCUBE_SCI")
	    recordJob "${ID_CONTCUBE_SCI}" "Make a continuum cube in pol $POLN for beam $BEAM of the science observation, with flags \"$DEP\""
        else
	    echo "Would make a continuum cube in pol $POLN for beam $BEAM of the science observation with slurm file $sbatchfile"
        fi

        echo " "

    fi

done

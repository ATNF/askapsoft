#!/bin/bash -l
#
# Launches a job to image the current beam of the science
# observation, at full spectral resolution, using the spectral-line
# imager 'simager'
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

ID_SPECIMG_SCI=""

# set the $imageBase variable
setImageBaseSpectral

DO_IT=$DO_SPECTRAL_IMAGING

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/image.${imageBase}.restored ]; then
    if [ $DO_IT == true ]; then
        echo "Image ${imageBase}.restored exists, so not running spectral-line imaging for beam ${BEAM}"
        echo " "
    fi
    DO_IT=false
fi

# Define the preconditioning
preconditioning="Simager.preconditioner.Names                    = ${PRECONDITIONER_LIST_SPECTRAL}"
if [ "`echo ${PRECONDITIONER_LIST_SPECTRAL} | grep GaussianTaper`" != "" ]; then
    preconditioning="$preconditioning
Simager.preconditioner.GaussianTaper            = ${PRECONDITIONER_SPECTRAL_GAUSS_TAPER}"
fi
if [ "`echo ${PRECONDITIONER_LIST_SPECTRAL} | grep Wiener`" != "" ]; then
    # Use the new preservecf preconditioner option, but only for the
    # Wiener filter
    preconditioning="$preconditioning
Simager.preconditioner.preservecf               = true"
    if [ "${PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS}" != "" ]; then
	preconditioning="$preconditioning
Simager.preconditioner.Wiener.robustness        = ${PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS}"
    fi
    if [ "${PRECONDITIONER_SPECTRAL_WIENER_TAPER}" != "" ]; then
	preconditioning="$preconditioning
Simager.preconditioner.Wiener.taper             = ${PRECONDITIONER_SPECTRAL_WIENER_TAPER}"
    fi
fi
shapeDefinition="# Leave shape definition to advise"
if [ "${NUM_PIXELS_SPECTRAL}" != "" ] && [ $NUM_PIXELS_SPECTRAL -gt 0 ]; then
    shapeDefinition="Simager.Images.shape                            = [${NUM_PIXELS_SPECTRAL}, ${NUM_PIXELS_SPECTRAL}]"
else
    echo "WARNING - No valid NUM_PIXELS_SPECTRAL parameter given.  Not running spectral imaging."
    DO_IT=false
fi
cellsizeGood=`echo ${CELLSIZE_SPECTRAL} | awk '{if($1>0.) print "true"; else print "false";}'`
if [ "${CELLSIZE_SPECTRAL}" != "" ] && [ $cellsizeGood == true ]; then
    cellsizeDefinition="Simager.Images.cellsize                         = [${CELLSIZE_SPECTRAL}arcsec, ${CELLSIZE_SPECTRAL}arcsec]"
else
    echo "WARNING - No valid CELLSIZE_SPECTRAL parameter given.  Not running spectral imaging."
    DO_IT=false
fi
restFrequency="# No rest frequency specified"
if [ "${REST_FREQUENCY_SPECTRAL}" != "" ]; then
    restFrequency="Simager.Images.restFrequency                    = ${REST_FREQUENCY_SPECTRAL}"
fi

cleaningPars="# These parameters define the clean algorithm 
Simager.solver                                  = ${SOLVER_SPECTRAL}"
if [ ${SOLVER_SPECTRAL} == "Clean" ]; then
    cleaningPars="${cleaningPars}
Simager.solver.Clean.algorithm                  = ${CLEAN_SPECTRAL_ALGORITHM}
Simager.solver.Clean.niter                      = ${CLEAN_SPECTRAL_MINORCYCLE_NITER}
Simager.solver.Clean.gain                       = ${CLEAN_SPECTRAL_GAIN}
Simager.solver.Clean.scales                     = ${CLEAN_SPECTRAL_SCALES}
Simager.solver.Clean.verbose                    = False
Simager.solver.Clean.tolerance                  = 0.01
Simager.solver.Clean.weightcutoff               = zero
Simager.solver.Clean.weightcutoff.clean         = false
Simager.solver.Clean.psfwidth                   = 512
Simager.solver.Clean.logevery                   = 50
Simager.threshold.minorcycle                    = ${CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE}
Simager.threshold.majorcycle                    = ${CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE}
Simager.ncycles                                 = ${CLEAN_SPECTRAL_NUM_MAJORCYCLES}
Simager.Images.writeAtMajorCycle                = ${CLEAN_SPECTRAL_WRITE_AT_MAJOR_CYCLE}
"
fi
if [ ${SOLVER_SPECTRAL} == "Dirty" ]; then
    cleaningPars="${cleaningPars}
Simager.solver.Dirty.tolerance                  = 0.01
Simager.solver.Dirty.verbose                    = False
Simager.ncycles                                 = 0"
fi

restorePars="# These parameter govern the restoring of the image and the recording of the beam
Simager.restore                                 = ${RESTORE_SPECTRAL}"
if [ ${RESTORE_SPECTRAL} == "true" ]; then
    restorePars="${restorePars}
Simager.restore.beam                            = ${RESTORING_BEAM_SPECTRAL}
Simager.restore.beamReference                   = ${RESTORING_BEAM_REFERENCE}
Simager.restore.beamLog                         = ${RESTORING_BEAM_LOG}"
fi

if [ $DO_IT == true ]; then

    echo "Imaging the spectral-line science observation"

    sbatchfile=$slurms/science_spectral_imager_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPECTRAL_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_SPECIMG_SCI}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SPEC_IMAGING}
#SBATCH --job-name specimg${BEAM}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-spectralImaging-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

if [ "${DIRECTION_SCI}" != "" ]; then
    directionDefinition="Simager.Images.image.${imageBase}.direction    = ${DIRECTION_SCI}"
else
    log=${logs}/mslist_for_simager_\${SLURM_JOB_ID}.log
    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} $mslist --full ${msSciSL} 1>& \${log}
    ra=\`grep -A1 RA \$log | tail -1 | awk '{print \$7}'\`
    dec=\`grep -A1 RA \$log | tail -1 | awk '{print \$8}'\`
    eq=\`grep -A1 RA \$log | tail -1 | awk '{print \$9}'\`
    directionDefinition="Simager.Images.direction                       = [\${ra}, \${dec}, \${eq}]"
fi

parset=${parsets}/science_spectral_imager_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset << EOF
Simager.dataset                                 = ${msSciSL}
#
Simager.Images.name                             = image.${imageBase}
${shapeDefinition}
${cellsizeDefinition}
\${directionDefinition}
${restFrequency}
#
# This defines the parameters for the gridding.
Simager.gridder.snapshotimaging                 = ${GRIDDER_SPECTRAL_SNAPSHOT_IMAGING}
Simager.gridder.snapshotimaging.wtolerance      = ${GRIDDER_SPECTRAL_SNAPSHOT_WTOL}
Simager.gridder.snapshotimaging.longtrack       = ${GRIDDER_SPECTRAL_SNAPSHOT_LONGTRACK}
Simager.gridder                                 = WProject
Simager.gridder.WProject.wmax                   = ${GRIDDER_SPECTRAL_WMAX}
Simager.gridder.WProject.nwplanes               = ${GRIDDER_SPECTRAL_NWPLANES}
Simager.gridder.WProject.oversample             = ${GRIDDER_SPECTRAL_OVERSAMPLE}
Simager.gridder.WProject.maxsupport             = ${GRIDDER_SPECTRAL_MAXSUPPORT}
Simager.gridder.WProject.variablesupport        = true
Simager.gridder.WProject.offsetsupport          = true
#
${cleaningPars}
#
${preconditioning}
#
${restorePars}
EOF

log=${logs}/science_spectral_imager_beam${BEAM}_\${SLURM_JOB_ID}.log

# Now run the simager
NCORES=${NUM_CPUS_SPECIMG_SCI}
NPPN=${CPUS_PER_CORE_SPEC_IMAGING}
aprun -n \${NCORES} -N \${NPPN} ${simager} -c \$parset > \$log
err=\$?
rejuvenate ${msSciSL}
rejuvenate *.${imageBase}*
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} spectralImaging_B${BEAM} "txt,csv"

if [ \${err} -ne 0 ]; then
    echo "Error: simager returned error code \${err}"
    exit 1
fi


EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SL_SCI"`
        DEP=`addDep "$DEP" "$ID_CAL_APPLY_SL_SCI"`
        DEP=`addDep "$DEP" "$ID_CONT_SUB_SL_SCI"`
	ID_SPECIMG_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SPECIMG_SCI} "Make a spectral-line cube for beam $BEAM of the science observation, with flags \"$DEP\""
    else
	echo "Would make a spectral-line cube for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

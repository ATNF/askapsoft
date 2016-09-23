#!/bin/bash -l
#
# Launches a job to image the current beam of the averaged science MS
# to form a "continuum cube". Includes iteration over the list of
# requested polarisations. This uses the $POL_LIST variable, which
# should be something like "I Q U V".
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

ID_CONTCUBE_SCI=""

if [ $DO_ALT_IMAGER == true ]; then
theimager=$altimager
else
theimager=$cimager
fi

for POLN in $POL_LIST; do

    # make a lower-case version of the polarisation, for image name
    pol=`echo $POLN | tr '[:upper:]' '[:lower:]'`

    # set the $imageBase variable
    setImageBaseContCube

    DO_IT=$DO_CONTCUBE_IMAGING

    if [ $CLOBBER == false ] && [ -e ${OUTPUT}/image.${imageBase}.restored ]; then
        if [ $DO_IT == true ]; then
            echo "Image ${imageBase}.restored exists, so not running continuum-cube imaging for beam ${BEAM}"
            echo " "
        fi
        DO_IT=false
    fi

    if [ ${DO_APPLY_CAL_CONT} == true ]; then
        msToUse=$msSciAvCal
    else
        msToUse=$msSciAv
    fi

    # Define the image polarisation
    polarisation="Simager.Images.polarisation                     = [\"${POLN}\"]"

    # Define the preconditioning
    preconditioning="Simager.preconditioner.Names                    = ${PRECONDITIONER_LIST}"
    if [ "`echo ${PRECONDITIONER_LIST} | grep GaussianTaper`" != "" ]; then
        preconditioning="$preconditioning
Simager.preconditioner.GaussianTaper            = ${PRECONDITIONER_GAUSS_TAPER}"
    fi
    if [ "`echo ${PRECONDITIONER_LIST} | grep Wiener`" != "" ]; then
        # Use the new preservecf preconditioner option, but only for the
        # Wiener filter
        preconditioning="$preconditioning
Simager.preconditioner.preservecf               = true"
        if [ "${PRECONDITIONER_WIENER_ROBUSTNESS}" != "" ]; then
	    preconditioning="$preconditioning
Simager.preconditioner.Wiener.robustness        = ${PRECONDITIONER_WIENER_ROBUSTNESS}"
        fi
        if [ "${PRECONDITIONER_WIENER_TAPER}" != "" ]; then
	    preconditioning="$preconditioning
Simager.preconditioner.Wiener.taper             = ${PRECONDITIONER_WIENER_TAPER}"
        fi
    fi
    shapeDefinition="# Leave shape definition to advise"
    if [ "${NUM_PIXELS_CONT}" != "" ] && [ $NUM_PIXELS_CONT -gt 0 ]; then
        shapeDefinition="Simager.Images.shape                            = [${NUM_PIXELS_CONT}, ${NUM_PIXELS_CONT}]"
    else
        echo "WARNING - No valid NUM_PIXELS_CONT parameter given.  Not running continuum cube imaging."
        DO_IT=false
    fi
    cellsizeGood=`echo ${CELLSIZE_CONT} | awk '{if($1>0.) print "true"; else print "false";}'`
    if [ "${CELLSIZE_CONT}" != "" ] && [ $cellsizeGood == true ]; then
        cellsizeDefinition="Simager.Images.cellsize                         = [${CELLSIZE_CONT}arcsec, ${CELLSIZE_CONT}arcsec]"
    else
        echo "WARNING - No valid CELLSIZE_CONT parameter given.  Not running continuum cube imaging."
        DO_IT=false
    fi
    restFrequency="# No rest frequency specified for continuum cubes"
    if [ "${REST_FREQUENCY_CONTCUBE}" != "" ]; then
        restFrequency="Simager.Images.restFrequency                    = ${REST_FREQUENCY_CONTCUBE}"
    fi

    cleaningPars="# These parameters define the clean algorithm 
Simager.solver                                  = ${SOLVER_CONTCUBE}"
    if [ ${SOLVER_CONTCUBE} == "Clean" ]; then
        cleaningPars="${cleaningPars}
Simager.solver.Clean.algorithm                  = ${CLEAN_CONTCUBE_ALGORITHM}
Simager.solver.Clean.niter                      = ${CLEAN_CONTCUBE_MINORCYCLE_NITER}
Simager.solver.Clean.gain                       = ${CLEAN_CONTCUBE_GAIN}
Simager.solver.Clean.scales                     = ${CLEAN_CONTCUBE_SCALES}
Simager.solver.Clean.verbose                    = False
Simager.solver.Clean.tolerance                  = 0.01
Simager.solver.Clean.weightcutoff               = zero
Simager.solver.Clean.weightcutoff.clean         = false
Simager.solver.Clean.psfwidth                   = ${CLEAN_CONTCUBE_PSFWIDTH}
Simager.solver.Clean.logevery                   = 50
Simager.threshold.minorcycle                    = ${CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE}
Simager.threshold.majorcycle                    = ${CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE}
Simager.ncycles                                 = ${CLEAN_CONTCUBE_NUM_MAJORCYCLES}
Simager.Images.writeAtMajorCycle                = ${CLEAN_CONTCUBE_WRITE_AT_MAJOR_CYCLE}
"
    fi
    if [ ${SOLVER} == "Dirty" ]; then
        cleaningPars="${cleaningPars}
Simager.solver.Dirty.tolerance                  = 0.01
Simager.solver.Dirty.verbose                    = False
Simager.ncycles                                 = 0"
    fi

    restorePars="# These parameter govern the restoring of the image and the recording of the beam
Simager.restore                                 = true
Simager.restore.beam                            = ${RESTORING_BEAM_CONTCUBE}
Simager.restore.beamReference                   = ${RESTORING_BEAM_CONTCUBE_REFERENCE}
Simager.restore.beamLog                         = beamLog.${imageBase}.txt"

    if [ $DO_IT == true ]; then

        echo "Imaging the continuum cube, polarisation $POLN, for the science observation"

        setJob science_contcube_imager_${POLN} contcube${POLN}
        cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_CONTCUBE_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_CONTCUBE_SCI}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTCUBE_IMAGING}
#SBATCH --job-name ${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-contcubeImaging-${BEAM}-${POLN}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

ms=${msToUse}

if [ "${DIRECTION}" != "" ]; then
    directionDefinition="Simager.Images.direction                       = ${DIRECTION}"
else
    log=${logs}/mslist_for_simager_\${SLURM_JOB_ID}.log
    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} $mslist --full \${ms} 2>&1 1> \${log}
    ra=\`python ${PIPELINEDIR}/parseMSlistOutput.py --file=\$log --val=RA\`
    dec=\`python ${PIPELINEDIR}/parseMSlistOutput.py --file=\$log --val=Dec\`
    epoch=\`python ${PIPELINEDIR}/parseMSlistOutput.py --file=\$log --val=Epoch\`
    directionDefinition="Simager.Images.direction                       = [\${ra}, \${dec}, \${epoch}]"
fi

parset=${parsets}/science_contcube_imager_${FIELDBEAM}_${POLN}_\${SLURM_JOB_ID}.in
cat > \$parset << EOF
Simager.dataset                                 = \${ms}
#
Simager.Images.name                             = image.${imageBase}
${polarisation}
${shapeDefinition}
${cellsizeDefinition}
\${directionDefinition}
${restFrequency}
#
# This defines the parameters for the gridding.
Simager.gridder.snapshotimaging                 = ${GRIDDER_SNAPSHOT_IMAGING}
Simager.gridder.snapshotimaging.wtolerance      = ${GRIDDER_SNAPSHOT_WTOL}
Simager.gridder.snapshotimaging.longtrack       = ${GRIDDER_SNAPSHOT_LONGTRACK}
Simager.gridder.snapshotimaging.clipping        = ${GRIDDER_SNAPSHOT_CLIPPING}
Simager.gridder                                 = WProject
Simager.gridder.WProject.wmax                   = ${GRIDDER_WMAX}
Simager.gridder.WProject.nwplanes               = ${GRIDDER_NWPLANES}
Simager.gridder.WProject.oversample             = ${GRIDDER_OVERSAMPLE}
Simager.gridder.WProject.maxsupport             = ${GRIDDER_MAXSUPPORT}
Simager.gridder.WProject.variablesupport        = true
Simager.gridder.WProject.offsetsupport          = true
#
${cleaningPars}
#
${preconditioning}
#
${restorePars}
EOF

log=${logs}/science_contcube_imager_${FIELDBEAM}_${POLN}_\${SLURM_JOB_ID}.log

# Now run the simager
NCORES=${NUM_CPUS_CONTCUBE_SCI}
NPPN=${CPUS_PER_CORE_CONTCUBE_IMAGING}
aprun -n \${NCORES} -N \${NPPN} ${theimager} -c \$parset > \$log
err=\$?
rejuvenate \${ms}
rejuvenate *.${imageBase}*
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"

if [ \${err} -ne 0 ]; then
    echo "Error: simager returned error code \${err}"
    exit 1
fi


EOFOUTER

        if [ $SUBMIT_JOBS == true ]; then
            DEP=""
            DEP=`addDep "$DEP" "$DEP_START"`
            DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
            DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
            DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
            DEP=`addDep "$DEP" "$ID_AVERAGE_SCI"`
            DEP=`addDep "$DEP" "$ID_FLAG_SCI_AV"`
            DEP=`addDep "$DEP" "$ID_CAL_APPLY_CONT_SCI"`
	    ID_CONTCUBE_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	    recordJob ${ID_CONTCUBE_SCI} "Make a continuum cube in pol $POLN for beam $BEAM of the science observation, with flags \"$DEP\""
        else
	    echo "Would make a continuum cube in pol $POLN for beam $BEAM of the science observation with slurm file $sbatchfile"
        fi

        echo " "

    fi

done

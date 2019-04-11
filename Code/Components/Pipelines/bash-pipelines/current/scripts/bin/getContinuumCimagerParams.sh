#!/bin/bash -l
#
# Defines the majority of the parameter set used by Cimager, based on
# the user-defined parameters. This script is called by the two
# continuum imaging scripts to help fill out the parset. Upon return,
# the following environment variables are defined:
#   * cimagerParams - the Cimager parset, excluding any calibrate
#   parameters
#   * imageBase - the base name for the image products, incorporating
#   the current beam
#   * directionDefinition - the direction of the centre of the image, 
#   as a Cimager parameter if 'DIRECTION' is defined
#   * shapeDefinition - the shape of the images, as a Cimager
#   parameter if 'NUM_PIXELS_CONT' is defined
#   * cellsizeDefinition - the shape of the images, as a Cimager
#   parameter if 'CELLSIZE_CONT' is defined
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

# set the image name
imageCode=restored
setImageProperties cont

# Define the shape parameter, or leave to "advise"
shapeDefinition="# Leave shape definition to Cimager to determine from the data"
if [ "${NUM_PIXELS_CONT}" != "" ] && [ "${NUM_PIXELS_CONT}" -gt 0 ]; then
    shapeDefinition="Cimager.Images.shape                            = [${NUM_PIXELS_CONT}, ${NUM_PIXELS_CONT}]"
fi

# Define the cellsize parameter, or leave to "advise"
cellsizeDefinition="# Leave cellsize definition to Cimager to determine from the data"
cellsizeGood=$(echo "${CELLSIZE_CONT}" | awk '{if($1>0.) print "true"; else print "false";}')
if [ "${CELLSIZE_CONT}" != "" ] && [ "$cellsizeGood" == "true" ]; then
    cellsizeDefinition="Cimager.Images.cellsize                         = [${CELLSIZE_CONT}arcsec, ${CELLSIZE_CONT}arcsec]"
fi

# Define the direction parameter, or leave to "advise"
directionDefinition="# Leave direction definition to Cimager to determine from the data"
if [ "${DIRECTION}" != "" ]; then
    directionDefinition="Cimager.Images.image.${imageBase}.direction    = ${DIRECTION}"
fi

# Define the preconditioning
preconditioning="Cimager.preconditioner.Names                    = ${PRECONDITIONER_LIST}"
if [ "$(echo "${PRECONDITIONER_LIST}" | grep GaussianTaper)" != "" ]; then
    preconditioning="$preconditioning
Cimager.preconditioner.GaussianTaper            = ${PRECONDITIONER_GAUSS_TAPER}"
fi
if [ "$(echo "${PRECONDITIONER_LIST}" | grep Wiener)" != "" ]; then
    # Use the new preservecf preconditioner option, but only for the
    # Wiener filter
    preconditioning="$preconditioning
Cimager.preconditioner.preservecf               = true"
    if [ "${PRECONDITIONER_WIENER_ROBUSTNESS}" != "" ]; then
	preconditioning="$preconditioning
Cimager.preconditioner.Wiener.robustness        = ${PRECONDITIONER_WIENER_ROBUSTNESS}"
    fi
    if [ "${PRECONDITIONER_WIENER_TAPER}" != "" ]; then
	preconditioning="$preconditioning
Cimager.preconditioner.Wiener.taper             = ${PRECONDITIONER_WIENER_TAPER}"
    fi
fi

# Define the restore solver
restore="Cimager.restore                                 = true
Cimager.restore.beam                            = ${RESTORING_BEAM_CONT}
Cimager.restore.beam.cutoff                     = ${RESTORING_BEAM_CUTOFF_CONT}"
if [ "${RESTORE_PRECONDITIONER_LIST}" != "" ]; then
    restore="${restore}
Cimager.restore.preconditioner.Names                    = ${RESTORE_PRECONDITIONER_LIST}"
    if [ "$(echo "${RESTORE_PRECONDITIONER_LIST}" | grep GaussianTaper)" != "" ]; then
        restore="$restore
Cimager.restore.preconditioner.GaussianTaper            = ${RESTORE_PRECONDITIONER_GAUSS_TAPER}"
    fi
    if [ "$(echo "${RESTORE_PRECONDITIONER_LIST}" | grep Wiener)" != "" ]; then
        # Use the new preservecf preconditioner option, but only for the
        # Wiener filter
        restore="$restore
Cimager.restore.preconditioner.preservecf               = true"
        if [ "${RESTORE_PRECONDITIONER_WIENER_ROBUSTNESS}" != "" ]; then
	    restore="$restore
Cimager.restore.preconditioner.Wiener.robustness        = ${RESTORE_PRECONDITIONER_WIENER_ROBUSTNESS}"
        fi
        if [ "${RESTORE_PRECONDITIONER_WIENER_TAPER}" != "" ]; then
	    restore="$restore
Cimager.restore.preconditioner.Wiener.taper             = ${RESTORE_PRECONDITIONER_WIENER_TAPER}"
        fi
    fi
fi


#Define the MFS parameters: visweights and reffreq, or leave to advise
mfsParams="# The following are needed for MFS clean
# This one defines the number of Taylor terms
Cimager.Images.image.${imageBase}.nterms       = ${NUM_TAYLOR_TERMS}
# This one assigns one worker for each of the Taylor terms
Cimager.nworkergroups                           = ${nworkergroupsSci}
# Leave 'Cimager.visweights' to be determined by Cimager, based on nterms"

if [ "${MFS_REF_FREQ}" == "" ]; then
    mfsParams="${mfsParams}
# Leave 'Cimager.visweights.MFS.reffreq' to be determined by Cimager"
else
    mfsParams="${mfsParams}
# This is the reference frequency - it should lie in your frequency range (ideally in the middle)
Cimager.visweights.MFS.reffreq                  = ${MFS_REF_FREQ}"
fi

# This is for the new (alt) imager
altImagerParams="# Options for the alternate imager"
if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then

    if [ "${NCHAN_PER_CORE}" == "" ]; then
        nchanpercore=1
    else
        nchanpercore="${NCHAN_PER_CORE}"
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
Cimager.barycentre                              = false
Cimager.solverpercore                           = false
Cimager.nwriters                                = 1"

else
    altImagerParams="${altImagerParams} are not required"
fi

cleaningPars="# These parameters define the clean algorithm
Cimager.solver                                  = ${SOLVER}"
if [ "${SOLVER}" == "Clean" ]; then
    cleaningPars="${cleaningPars}
Cimager.solver.Clean.solutiontype               = ${CLEAN_SOLUTIONTYPE}
Cimager.solver.Clean.verbose                    = false
Cimager.solver.Clean.decoupled                  = true
Cimager.solver.Clean.tolerance                  = 0.01
Cimager.solver.Clean.weightcutoff               = zero
Cimager.solver.Clean.weightcutoff.clean         = false
Cimager.threshold.masking                       = 0.9
Cimager.solver.Clean.logevery                   = 50"
fi
cleaningPars="${cleaningPars}
Cimager.Images.writeAtMajorCycle                = ${CLEAN_WRITE_AT_MAJOR_CYCLE}
"
# threshold.majorcycle and ncycles are defined by the function
# cimagerSelfcalLoopParams in utils.sh. These can vary according to
# the self-calibration loop number, so need to be set from within the
# slurm job


cimagerParams="#Standard Parameter set for Cimager
Cimager.dataset                                 = ${msSciAv}
Cimager.datacolumn                              = ${DATACOLUMN}
Cimager.imagetype                               = ${IMAGETYPE_CONT}
#
${CONTIMG_CHANNEL_SELECTION}
#
Cimager.Images.Names                            = [image.${imageBase}]
${shapeDefinition}
${cellsizeDefinition}
${directionDefinition}
# This is how many channels to write to the image - just a single one for continuum
Cimager.Images.image.${imageBase}.nchan        = 1
#
${mfsParams}
#
${altImagerParams}
#
# This defines the parameters for the gridding.
Cimager.gridder.snapshotimaging                 = ${GRIDDER_SNAPSHOT_IMAGING}
Cimager.gridder.snapshotimaging.wtolerance      = ${GRIDDER_SNAPSHOT_WTOL}
Cimager.gridder.snapshotimaging.longtrack       = ${GRIDDER_SNAPSHOT_LONGTRACK}
Cimager.gridder.snapshotimaging.clipping        = ${GRIDDER_SNAPSHOT_CLIPPING}
Cimager.gridder                                 = WProject
Cimager.gridder.WProject.wmax                   = ${GRIDDER_WMAX}
Cimager.gridder.WProject.nwplanes               = ${GRIDDER_NWPLANES}
Cimager.gridder.WProject.oversample             = ${GRIDDER_OVERSAMPLE}
Cimager.gridder.WProject.maxsupport             = ${GRIDDER_MAXSUPPORT}
Cimager.gridder.WProject.variablesupport        = true
Cimager.gridder.WProject.offsetsupport          = true
#
${cleaningPars}
#
${preconditioning}
#
${restore}
"

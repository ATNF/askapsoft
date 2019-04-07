#!/bin/bash -l
#
# Subtracts the continuum emission from the spectral-line dataset in
# one of two ways: one by finding & fitting components in the restored
# continuum image (using Selavy), making a model image of those
# components with Cmodel, and the subtracting from the measurement
# set; or two by using the continuum clean model to subtract from the
# measurement set.
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

setJob contsub_spectralline contsub

if [ "${CONTSUB_METHOD}" == "Cmodel" ]; then

    . "${PIPELINEDIR}/spectralContinuumSubtraction_Cmodel.sh"
    
elif [ "${CONTSUB_METHOD}" == "Components" ]; then

    . "${PIPELINEDIR}/spectralContinuumSubtraction_Components.sh"

elif [ "${CONTSUB_METHOD}" == "CleanModel" ]; then

    . "${PIPELINEDIR}/spectralContinuumSubtraction_CleanModel.sh"

fi

if [ "${SUBMIT_JOBS}" == "true" ]; then
    DEP=""
    DEP=$(addDep "$DEP" "$DEP_START")
    DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
    DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
    DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
    DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI_LIST")
    DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV_LIST")
    DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI")
    DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
    DEP=$(addDep "$DEP" "$ID_SPLIT_SL_SCI")
    DEP=$(addDep "$DEP" "$ID_CAL_APPLY_SL_SCI")
    ID_CONT_SUB_SL_SCI=$(sbatch $DEP "${sbatchfile}" | awk '{print $4}')
    recordJob "${ID_CONT_SUB_SL_SCI}" "Subtract the continuum model from the spectral-line dataset for imaging beam $BEAM of the science observation, with flags \"$DEP\""
    ID_CONT_SUB_SL_SCI_LIST=$(addDep "$ID_CONT_SUB_SL_SCI" "$ID_CONT_SUB_SL_SCI_LIST")
else
    echo "Would subtract the continuum model from the spectral-line dataset for imaging beam $BEAM of the science observation with slurm file $sbatchfile"
fi

echo " "

#!/bin/bash -l
#
# Process the 1934-638 calibration observations: split out per beam,
# flag, then find the bandpass solution
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

ms1934list=""
FLAG_CBPCAL_DEP=""
FLAG_CBPCAL_DEP=`addDep "$FLAG_CBPCAL_DEP" "$DEP_START"`

ORIGINAL_OUTPUT=${OUTPUT}

#Set the FIELD string to a special one for the bandpass calibrator
FIELD=BPCAL
mkdir -p ${FIELD}
cd ${FIELD}
OUTPUT="${ORIGINAL_OUTPUT}/${FIELD}"
mkdir -p ${OUTPUT}/Checkfiles
lfs setstripe -c 1 ${OUTPUT}/Checkfiles

for BEAM in ${BEAMS_TO_USE}; do
parsets=$parsetsBase
logs=$logsBase
slurms=$slurmsBase
slurmOut=$slurmOutBase

    parsets=$parsetsBase/$FIELD
    mkdir -p $parsets
    logs=$logsBase/$FIELD
    mkdir -p $logs
    slurms=$slurmsBase/$FIELD
    mkdir -p $slurms
    slurmOut=$slurmOutBase/$FIELD
    mkdir -p $slurmOut

    # an empty file that will indicate that the flagging has been done
    FLAG_1934_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_DONE_1934_BEAM${BEAM}"

    FIELDBEAM="${FIELD}_B${BEAM}"
    
    . ${PIPELINEDIR}/split1934.sh
    . ${PIPELINEDIR}/flag1934.sh


done

FIELDBEAM=$FIELD

. ${PIPELINEDIR}/findBandpassCal.sh

cd ..

OUTPUT=${ORIGINAL_OUTPUT}
parsets=$parsetsBase
logs=$logsBase
slurms=$slurmsBase
slurmOut=$slurmOutBase


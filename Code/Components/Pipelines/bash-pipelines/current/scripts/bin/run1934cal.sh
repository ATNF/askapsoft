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

#Set the FIELD string to a special one for the bandpass calibrator
FIELD=BPCAL
mkdir -p ${FIELD}
cd ${FIELD}
OUTPUT="${ORIGINAL_OUTPUT}/${FIELD}"
if [ "${TABLE_BANDPASS%/*}" == "${TABLE_BANDPASS}" ]; then
    # Have just given a filename with no leading path - need to change
    # it to be inside the new OUTPUT directory
    TABLE_BANDPASS=${OUTPUT}/${TABLE_BANDPASS}
fi

mkdir -p ${OUTPUT}/Checkfiles
lfs setstripe -c 1 ${OUTPUT}/Checkfiles

parsets=$parsetsBase/$FIELD
mkdir -p $parsets
logs=$logsBase/$FIELD
mkdir -p $logs
slurms=$slurmsBase/$FIELD
mkdir -p $slurms
slurmOut=$slurmOutBase/$FIELD
mkdir -p $slurmOut

highestBeam=`echo $BEAM_MAX | awk '{printf "%02d",$1}'`
echo "Solving for the bandpass solutions for beams up to beam${highestBeam}"
echo "========================================================="

for((IBEAM=0; IBEAM<=${BEAM_MAX}; IBEAM++)); do
    BEAM=`echo $IBEAM | awk '{printf "%02d",$1}'`
    # Process all beams up to the maximum requested, so that we can
    # give them all to cbpcalibrator.

    # an empty file that will indicate that the flagging has been done
    FLAG_1934_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_DONE_1934_BEAM${BEAM}"

    FIELDBEAM="${FIELD}_B${BEAM}"
    FIELDBEAMJOB=$FIELDBEAM
    
    . ${PIPELINEDIR}/split1934.sh
    . ${PIPELINEDIR}/flag1934.sh


done

FIELDBEAM=$FIELD

. ${PIPELINEDIR}/findBandpassCal.sh

cd ..

# Put all these back to the original values
OUTPUT=${ORIGINAL_OUTPUT}
parsets=$parsetsBase
logs=$logsBase
slurms=$slurmsBase
slurmOut=$slurmOutBase


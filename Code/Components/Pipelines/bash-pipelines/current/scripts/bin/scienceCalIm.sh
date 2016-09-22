#!/bin/bash -l
#
# Process the science field observations: split out per beam, flag,
# apply the bandpass solution, average spectrally, image the continuum
# with or without self-calibration, and image the spectral-line
# data. Finally, mosaic the continuum images.
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

echo "Setting up and calibrating the science observation"

FLAG_IMAGING_DEP=""

ORIGINAL_OUTPUT=${OUTPUT}
FIELD_ID=0

for FIELD in ${FIELD_LIST}; do

    mkdir -p ${FIELD}
    cd ${FIELD}
    OUTPUT="${OUTPUT}/${FIELD}"

    # Get the linmos offsets when we have a common image centre for
    # all beams - store in $LINMOS_BEAM_OFFSETS
    getBeamOffsets

    for BEAM in ${BEAMS_TO_USE}; do

        parsets=$parsetsBase/$FIELD
        mkdir -p $parsets
        logs=$logsBase/$FIELD
        mkdir -p $logs
        slurms=$slurmsBase/$FIELD
        mkdir -p $slurms
        slurmOut=$slurmOutBase/$FIELD
        mkdir -p $slurmOut

        echo "Processing field $FIELD"
        echo "-----------------------"

        
        mkdir -p ${OUTPUT}/Checkfiles
        # an empty file that will indicate that the flagging has been done
        FLAG_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_DONE_BEAM${BEAM}"
        # an empty file that will indicate that the bandpass has been done
        BANDPASS_CHECK_FILE="${OUTPUT}/Checkfiles/BANDPASS_APPLIED_BEAM${BEAM}"
        # an empty file that will indicate the gains have been applied to
        # the averaged (continuum) dataset
        CONT_GAINS_CHECK_FILE="${OUTPUT}/Checkfiles/GAINS_APPLIED_CONT_BEAM${BEAM}"
        # an empty file that will indicate the gains have been applied to
        # the spectral-line dataset
        SL_GAINS_CHECK_FILE="${OUTPUT}/Checkfiles/GAINS_APPLIED_SL_BEAM${BEAM}"
        # an empty file that will indicate the continuum has been
        # subtracted from the spectral-line dataset
        CONT_SUB_CHECK_FILE="${OUTPUT}/Checkfiles/CONT_SUB_SL_BEAM${BEAM}"

        if [ "${DIRECTION_SCI}" != "" ]; then
            # User has requested a particular image direction
            DIRECTION=${DIRECTION_SCI}
        else
            if [ "${IMAGE_AT_BEAM_CENTRES}" == "true" ]; then
                # store the beam centre in $DIRECTION
                getBeamCentre
            fi
            # otherwise we get the phase centre from the MS via advise,
            # and use for all beams
        fi

        
        findScienceMSnames
        FIELDBEAM=`echo $FIELD_ID $BEAM | awk '{printf "F%02d_B%s",$1,$2}'`
        FIELDBEAMJOB=`echo $FIELDBEAM | sed -e 's/_//g'`

        . ${PIPELINEDIR}/splitScience.sh

        . ${PIPELINEDIR}/applyBandpassScience.sh

        msToFlag=$msSci
        . ${PIPELINEDIR}/flagScience.sh
        
        . ${PIPELINEDIR}/averageScience.sh

        if [ $FLAG_AFTER_AVERAGING == true ]; then
            msToFlag=$msSciAv
            . ${PIPELINEDIR}/flagScience.sh
        fi
            
        if [ $DO_SELFCAL == true ]; then
            . ${PIPELINEDIR}/continuumImageScienceSelfcal.sh
        else
	    . ${PIPELINEDIR}/continuumImageScience.sh
        fi

        . ${PIPELINEDIR}/applyCalContinuumScience.sh
        . ${PIPELINEDIR}/sourcefinding.sh

        . ${PIPELINEDIR}/continuumCubeImagingScience.sh

        . ${PIPELINEDIR}/prepareSpectralData.sh

        . ${PIPELINEDIR}/spectralImageScience.sh

        FIELDBEAM=`echo $FIELD_ID | awk '{printf "F%02d",$1}'`
        FIELDBEAMJOB=$FIELDBEAM

        if [ $DO_ALT_IMAGER == true ]; then
            . ${PIPELINEDIR}/altLinmos.sh
        else
            . ${PIPELINEDIR}/linmos.sh
            . ${PIPELINEDIR}/spectralImContSub.sh

        fi




    done



    cd ..

    FIELD_ID=`expr $FIELD_ID + 1`
    

done

OUTPUT=${ORIGINAL_OUTPUT}
parsets=$parsetsBase
logs=$logsBase
slurms=$slurmsBase
slurmOut=$slurmOutBase

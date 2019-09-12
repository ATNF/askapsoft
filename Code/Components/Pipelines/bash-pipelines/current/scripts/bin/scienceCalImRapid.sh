#!/bin/bash -l
#
# Process the science field observations: split out per beam, flag,
# apply the bandpass solution, average spectrally, image the continuum
# with or without self-calibration, and image the spectral-line
# data. Finally, mosaic the continuum images.  This version is
# tailored for the rapid survey mode, assuming just continuum
# processing and no time-splitting. The pre-imaging tasks are bundled
# into a single script that runs at the start of the self-cal job.
#
# @copyright (c) 2019 CSIRO
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
echo "=================================================="

FIELD_ID=0
FULL_LINMOS_CONT_DEP=""
FULL_LINMOS_CONTCUBE_DEP=""
FULL_LINMOS_SPEC_DEP=""
GIVEN_POSITION_OFFSET_WARNING=false

cd "${ORIGINAL_OUTPUT}"

firstBeam=true

IFS="${IFS_FIELDS}"
for FIELD in ${FIELD_LIST}; do

    doField=true
    if [ "${FIELD_SELECTION_SCIENCE}" != "" ] &&
           [ "${FIELD_SELECTION_SCIENCE}" != "$FIELD" ]; then
        # If user has requested a given field, ignore all others
        doField=false
    fi

    if [ "$doField" == "true" ]; then

        parsets="$parsetsBase/$FIELD"
        mkdir -p "$parsets"
        logs="$logsBase/$FIELD"
        mkdir -p "$logs"
        slurms="$slurmsBase/$FIELD"
        mkdir -p "$slurms"
        slurmOut="$slurmOutBase/$FIELD"
        mkdir -p "$slurmOut"

        mkdir -p "${FIELD}"
        cd "${FIELD}"
        OUTPUT="${ORIGINAL_OUTPUT}/${FIELD}"

        if [ "${NEED_BEAM_CENTRES}" == "true" ]; then
            # Get the linmos offsets when we have a common image centre for
            # all beams - store in $LINMOS_BEAM_OFFSETS
            getBeamOffsets
        fi

        FLAG_IMAGING_DEP=""
        FLAG_POLIMAGING_DEP=""
        DEP_CONTCUBE=""
        DEP_CONTPOL_IMG=""
        DEP_SPECIMG=""
        DEP_SPECIMCONTSUB=""

        IFS="${IFS_DEFAULT}"
        for BEAM in ${BEAMS_TO_USE}; do

            echo "Processing field $FIELD, beam $BEAM"
            echo "----------"
            
            mkdir -p "${OUTPUT}/Checkfiles"

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

            # Define the MS metadata file, either the original MS or the new merged one.
            # Store the name in MS_METADATA. 
	    # This is unique for a given beam within a field. 
            findScienceMSmetadataFile

            findScienceMSnames
            
            FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" | awk '{printf "F%02d_B%s",$1,$2}')

            . "${PIPELINEDIR}/makeRapidPreimagingScript.sh"
            DEP_START=$(addDep "$DEP_START" "$ID_CBPCAL")
            # Define base dependencies for following tasks
            setContPreimagingDeps

            if [ "${DO_SELFCAL}" == "true" ]; then
                if [ "${MULTI_JOB_SELFCAL}" == "true" ]; then
                    . "${PIPELINEDIR}/continuumImageScienceSelfcal-multi.sh"
                else
                    . "${PIPELINEDIR}/continuumImageScienceSelfcal.sh"
                fi
                CONT_PREIMAGE_DEPS=$(addDep "$CONT_PREIMAGE_DEPS" "$ID_CONTIMG_SCI_SC")
            else
	        . "${PIPELINEDIR}/continuumImageScience.sh"
                CONT_PREIMAGE_DEPS=$(addDep "$CONT_PREIMAGE_DEPS" "$ID_CONTIMG_SCI")
            fi

            # Find FlagSummary for the continuum dataset: 
	    . "${PIPELINEDIR}/flagSummaryAveraged.sh"
            
            . "${PIPELINEDIR}/applyCalContinuumScience.sh"

#            . "${PIPELINEDIR}/continuumCubeImagingScience.sh"
            . "${PIPELINEDIR}/continuumPolImagingScience.sh"

            if [ "${DO_SOURCE_FINDING_BEAMWISE}" == "true" ]; then
                . "${PIPELINEDIR}/sourcefindingCont.sh"
            fi

            # Run the spectral processing, but only if
            # DO_SPECTRAL_PROCESSING is turned on.
            if [ "${DO_SPECTRAL_PROCESSING}" == "true" ]; then
                echo "WARNING: Rapid-mode processing assumes no spectral processing!"
            fi

            if [ "${firstBeam}" == "true" ]; then
                firstBeam=false
            fi

        done

        FIELDBEAM=$(echo "$FIELD_ID" | awk '{printf "F%02d",$1}')

        . "${PIPELINEDIR}/linmosCont.sh"
#        . "${PIPELINEDIR}/linmosContCube.sh"
        
        # Source-finding on the mosaics created by these jobs
        . "${PIPELINEDIR}/runMosaicSourcefinding.sh"        
        cd ..

    fi

    ((FIELD_ID++))
    
done

# Put all these back to the original values
OUTPUT="${ORIGINAL_OUTPUT}"
parsets="$parsetsBase"
logs="$logsBase"
slurms="$slurmsBase"
slurmOut="$slurmOutBase"
FIELD="."

# Final linmos, combining fields
. "${PIPELINEDIR}/linmosFieldsCont.sh"
#. "${PIPELINEDIR}/linmosFieldsContCube.sh"

# Final source-finding on the mosaics created by these jobs
. "${PIPELINEDIR}/runMosaicSourcefinding.sh"

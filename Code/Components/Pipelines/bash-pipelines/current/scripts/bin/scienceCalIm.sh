#!/bin/bash -l
#
# Process the science field observations: split out per beam, flag,
# apply the bandpass solution, average spectrally, image the continuum
# with or without self-calibration, and image the spectral-line
# data. Finally, mosaic the continuum images.
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
        DEP_CONTCUBE=""
        DEP_SPECIMG=""
        DEP_SPECIMCONTSUB=""

        IFS="${IFS_DEFAULT}"
        for BEAM in ${BEAMS_TO_USE}; do

            echo "Processing field $FIELD, beam $BEAM"
            echo "----------"
            
            mkdir -p "${OUTPUT}/Checkfiles"
            # an empty file that will indicate that the flagging has been done
            FLAG_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_DONE_BEAM${BEAM}"
            # the same, but for the averaged dataset
            FLAG_AV_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_AVERAGED_DATA_DONE_BEAM${BEAM}"
            # an empty file that will indicate that the bandpass has been done
            BANDPASS_CHECK_FILE="${OUTPUT}/Checkfiles/BANDPASS_APPLIED_BEAM${BEAM}"
            # an empty file that will indicate that msconcat has been done for averaged Science
            MSCONCAT_SCI_AV_CHECK_FILE="${OUTPUT}/Checkfiles/MSCONCCAT_SCI_AV_BEAM${BEAM}"
            # an empty file that will indicate that msconcat has been done for spectral Science
            MSCONCAT_SCI_SPECTRAL_CHECK_FILE="${OUTPUT}/Checkfiles/MSCONCCAT_SCI_SPECTRAL_BEAM${BEAM}"
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

            

            # Define the MS metadata file, either the original MS or the new merged one.
            # Store the name in MS_METADATA. 
	    # This is unique for a given beam within a field. 
            findScienceMSmetadataFile

	    # Inititialise JOBID Numbers for a beams here. We will append jobIDs for 
	    # each timeRange: 
	    ID_SPLIT_SCI_LIST=""
	    ID_SPLIT_SL_SCI_LIST=""
	    ID_CCALAPPLY_SCI_LIST=""
	    ID_CCALAPPLY_CONT_SCI_LIST=""
	    ID_CAL_APPLY_SL_SCI_LIST=""
	    ID_FLAG_SCI_LIST=""
	    ID_AVERAGE_SCI_LIST=""
	    ID_FLAG_SCI_AV_LIST=""
	    ID_CONT_SUB_SL_SCI_LIST=""
	    inputs2MSconcat=""
	    inputs2MSconcatSL=""
	    # ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	    # nTimeWindows=0 could be 
	    # used to process an entire beam dataset as a whole. if nTimeWindows>0, 
	    # we will split the beams further into smaller time-ranges. This will 
 	    # be done especially for the pre-imaging parts of the proceessing to 
	    # allow parallellisation on the supercomputer. 

	    if (( ${nTimeWindows} > 0 )) 
	    then 
		echo "Single beams will be split in time and processed..."
		DO_SPLIT_TIMEWISE=true
                for (( itime=1;itime<=$nTimeWindows; itime++ ))
	        do 
		    TimeBegin=$(sed -n $itime\p $timerangefile |awk '{print $1}')
	            TimeEnd=$(sed -n $itime\p $timerangefile |awk '{print $2}')
		    TimeWindow=$(echo ${itime} - 1 |bc)
		    # Output ms: 
                    findScienceMSnames
                    FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" "$TimeWindow" | awk '{printf "F%02d_B%s_TW%02d",$1,$2,$3}')
                    . "${PIPELINEDIR}/prepareScienceData.sh"
		    . "${PIPELINEDIR}/applyBandpassScience.sh"
                    
                    . "${PIPELINEDIR}/flagScience.sh"
                    
                    . "${PIPELINEDIR}/averageScience.sh"
		    if [ "${FLAG_AFTER_AVERAGING}" == "true" ]; then
			. "${PIPELINEDIR}/flagScienceAveraged.sh"
	            fi
		    inputs2MSconcat="${inputs2MSconcat} ${msSciAv}"
	        done
		# Get the name for the msconcat file. The filename should be 
		# the same as for the non-timeSplit case. Hence we call the 
		# findScienceMSnames function by temporarily changing the 
		# DO_SPLIT_TIMEWISE variable to false. 
		DO_SPLIT_TIMEWISE=false
		findScienceMSnames
		DO_SPLIT_TIMEWISE=true
                FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" | awk '{printf "F%02d_B%s",$1,$2}')
		msconcatFile=${msSciAv}
                . "${PIPELINEDIR}/msconcatTimeSplitScienceAveraged.sh"
	    else 
		# Entire Data to be processed at once
		echo "All data from single beams will be processed at once..."
		DO_SPLIT_TIMEWISE=false
                findScienceMSnames
                FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" | awk '{printf "F%02d_B%s",$1,$2}')
                . "${PIPELINEDIR}/prepareScienceData.sh"
                . "${PIPELINEDIR}/applyBandpassScience.sh"
                
		. "${PIPELINEDIR}/flagScience.sh"
		    
		. "${PIPELINEDIR}/averageScience.sh"
		if [ "${FLAG_AFTER_AVERAGING}" == "true" ]; then
		    . "${PIPELINEDIR}/flagScienceAveraged.sh"
	        fi
	    fi
            
	    # ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	    # Redefine FIELDBEAM for imager sbatchfile-naming: timeWin tag not needed.
            FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" | awk '{printf "F%02d_B%s",$1,$2}')
            if [ "${DO_SELFCAL}" == "true" ]; then
                if [ "${MULTI_JOB_SELFCAL}" == "true" ]; then
                    . "${PIPELINEDIR}/continuumImageScienceSelfcal-multi.sh"
                else
                    . "${PIPELINEDIR}/continuumImageScienceSelfcal.sh"
                fi
            else
	        . "${PIPELINEDIR}/continuumImageScience.sh"
            fi

            . "${PIPELINEDIR}/applyCalContinuumScience.sh"

            . "${PIPELINEDIR}/continuumCubeImagingScience.sh"

            if [ "${DO_SOURCE_FINDING_BEAMWISE}" == "true" ]; then
                . "${PIPELINEDIR}/sourcefindingCont.sh"
            fi

            # Run the spectral processing, but only if
            # DO_SPECTRAL_PROCESSING is turned on.
            if [ "${DO_SPECTRAL_PROCESSING}" == "true" ]; then

                # For timeSplit data, we want to apply refined gain solutions from selfcal
	        # to the timeSplit data first, then do the contsub and finally concatenate 
	        # them all before imaging: 
	        if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then 
                    for (( itime=1;itime<=$nTimeWindows; itime++ ))
	            do 
			TimeBegin=$(sed -n $itime\p $timerangefile |awk '{print $1}')
	                TimeEnd=$(sed -n $itime\p $timerangefile |awk '{print $2}')
			TimeWindow=$(echo ${itime} - 1 |bc)
			# Output ms: 
                        findScienceMSnames
                        FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" "$TimeWindow" | awk '{printf "F%02d_B%s_TW%02d",$1,$2,$3}')
		        . "${PIPELINEDIR}/prepareSpectralData.sh"
			inputs2MSconcatSL="${inputs2MSconcatSL} ${msSciSL}"
			# Now msconcat the timeWise split calibrated raw datasets (for each beam)
		    done
		    # Get the name for the msconcat file. The filename should be 
		    # the same as for the non-timeSplit case. Hence we call the 
		    # findScienceMSnames function by temporarily changing the 
		    # DO_SPLIT_TIMEWISE variable to false. 
		    DO_SPLIT_TIMEWISE=false
		    findScienceMSnames
		    DO_SPLIT_TIMEWISE=true
                    FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" | awk '{printf "F%02d_B%s",$1,$2}')
		    msconcatFile=${msSciSL}
                    . "${PIPELINEDIR}/msconcatTimeSplitScienceSpectral.sh"
	        else
		    # DO_SPLIT_TIMEWISE=false
	            # Output ms: 
                    findScienceMSnames
                    FIELDBEAM=$(echo "$FIELD_ID" "$BEAM" | awk '{printf "F%02d_B%s",$1,$2}')
		    DO_SPLIT_TIMEWISE=false
		    . "${PIPELINEDIR}/prepareSpectralData.sh"
	        fi

                . "${PIPELINEDIR}/spectralImageScience.sh"

                . "${PIPELINEDIR}/spectralImContSub.sh"
            
                if [ "${DO_SOURCE_FINDING_BEAMWISE}" == "true" ]; then
                    . "${PIPELINEDIR}/sourcefindingSpectral.sh"
                fi

            fi

            if [ "${firstBeam}" == "true" ]; then
                firstBeam=false
            fi

        done

        FIELDBEAM=$(echo "$FIELD_ID" | awk '{printf "F%02d",$1}')

        . "${PIPELINEDIR}/linmosCont.sh"
        . "${PIPELINEDIR}/linmosContCube.sh"
        . "${PIPELINEDIR}/linmosSpectral.sh"
        
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
. "${PIPELINEDIR}/linmosFieldsContCube.sh"
. "${PIPELINEDIR}/linmosFieldsSpectral.sh"

# Final source-finding on the mosaics created by these jobs
. "${PIPELINEDIR}/runMosaicSourcefinding.sh"

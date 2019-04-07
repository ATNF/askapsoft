#!/bin/bash -l
#
# Launches a job to extract the science observation from the
# appropriate measurement set, selecting the required beam and,
# optionally, a given scan or field
#
# @copyright (c) 2018 CSIRO
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

ID_SPLIT_SCI=""

DO_IT=$DO_SPLIT_SCIENCE

if [ -e "${OUTPUT}/${msSci}" ]; then
    if [ "${CLOBBER}" != "true" ]; then
        # If we aren't clobbering files, don't run anything
        if [ "${DO_IT}" == "true" ]; then
            echo "MS ${msSci} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ "${DO_IT}" == "true" ]; then
            rm -rf "${OUTPUT}/${msSci}"
            rm -f "${FLAG_CHECK_FILE}"
            rm -f "${BANDPASS_CHECK_FILE}"
        fi
    fi
fi

if [ -e "${OUTPUT}/${msSciAv}" ] && [ "${PURGE_FULL_MS}" == "true" ]; then
    # If we are purging the full MS, that means we don't need it.
    # If the averaged one works, we don't need to run the splitting
    DO_IT=false
fi

# Define the input MS. If there is a single MS in the archive
# directory, just use MS_INPUT_SCIENCE. If there is more than one,
# we assume it is split per beam. We compare the number of beams
# with the number of MSs, giving an error if they are
# different. If OK, then we add the beam number to the root MS
# name.

if [ $numMSsci -eq 1 ]; then
    inputMS=${MS_INPUT_SCIENCE}
else
    msroot=$(for ms in ${msnamesSci}; do echo $ms; done | sed -e 's/[0-9]*[0-9].ms//g' | uniq)
    inputMS=$(echo $msroot $BEAM | awk '{printf "%s%d.ms\n",$1,$2}')
fi

if [ ! -e "${inputMS}" ]; then
    echo "ERROR - wanted to use $inputMS as input for beam $BEAM"
    echo "      -  but it does not exist! Not running this beam."
    echo " "
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    if [ "${SCAN_SELECTION_SCIENCE}" == "" ]; then
	scanParam="# No scan selection done"
    else
        if [ "$(echo "${SCAN_SELECTION_SCIENCE}" | awk -F'[' '{print NF}')" -gt 1 ]; then
	    scanParam="scans        = ${SCAN_SELECTION_SCIENCE}"
        else
            scanParam="scans        = [${SCAN_SELECTION_SCIENCE}]"
        fi
    fi

    # Select only the current field
    fieldParam="fieldnames   = \"${FIELD}\""

    # Work out whether we need to run mssplit at all.
    # Four cases here:
    #  1. Selection of channels or scans --> yes
    #  2. No selection, and >1 MS --> no
    #  3. No selection, 1 MS, 1 beam only --> no (that 1 MS only has what we need)
    #  4. No selection, 1 MS, >1 beam --> yes (need to split out our desired beam)
    splitNeeded=true
    if [ ${NUM_FIELDS} -eq 1 ] &&
           [ "${CHAN_RANGE_SCIENCE}" == "" ] &&
           [ "${DO_SPLIT_TIMEWISE}" == "" ] &&
           [ "${SCAN_SELECTION_SCIENCE}" == "" ]; then
        
        splitNeeded=false

        if [ $numMSsci -eq 1 ] && [ ${NUM_BEAMS_FOOTPRINT} -gt 1 ];  then
            splitNeeded=true
        fi
    fi
    
    if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then
        timeParamBeg="timebegin     = ${TimeBegin}"
        timeParamEnd="timeend     = ${TimeEnd}"
    else
	timeParamBeg=""
	timeParamEnd=""
    fi

    # If we have >1 MS and the number differs from the number of
    # beams, we need to do merging. There are different scripts to
    # handle merging or non-merging, so we make that choice here.
    if [ "${NEED_TO_MERGE_SCI}" == "true" ]; then

        if [ "${CHAN_RANGE_SCIENCE}" == "" ]; then
            channelParam="channel     = 1-\${nChan}"
        else
            channelParam="channel     = ${CHAN_RANGE_SCIENCE}"
        fi

        . "${PIPELINEDIR}/mergeSplitSci.sh"

    else

        if [ "${CHAN_RANGE_SCIENCE}" == "" ]; then
            channelParam="channel     = 1-${NUM_CHAN_SCIENCE}"
        else
            channelParam="channel     = ${CHAN_RANGE_SCIENCE}"
        fi


        . "${PIPELINEDIR}/copySplitSci.sh"

    fi
    
    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
	if [[ "$DO_SPLIT_TIMEWISE" == "true" ]] && [[ "$DO_SPLIT_TIMEWISE_SERIAL" == "true" ]]; then
		# Spare poor /astro from a massive read bombardment! 
		DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
	fi
	ID_SPLIT_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_SPLIT_SCI}" "Run ${verb} for beam ${BEAM} of science observation, with flags \"$DEP\""
	ID_SPLIT_SCI_LIST=$(addDep "$ID_SPLIT_SCI" "$ID_SPLIT_SCI_LIST")
    else
	echo "Would run ${verb} for beam ${BEAM} of science observation with slurm file $sbatchfile"
    fi

        
fi

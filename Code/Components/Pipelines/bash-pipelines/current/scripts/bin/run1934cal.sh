#!/usr/bin/env bash
#
# Process the 1934-638 calibration observations: split out per beam,
# flag, then find the bandpass solution
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

ms1934list=""
FLAG_CBPCAL_DEP=""
FLAG_CBPCAL_DEP=`addDep "$FLAG_CBPCAL_DEP" "$DEP_START"`

for((BEAM=${BEAM_MIN}; BEAM<=${BEAM_MAX}; BEAM++)); do

    mkdir -p ${OUTPUT}/Checkfiles
    # an empty file that will indicate that the flagging has been done
    FLAG_1934_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_DONE_1934_BEAM${BEAM}"
    
   . ${SCRIPTDIR}/splitFlag1934.sh

done

. ${SCRIPTDIR}/findBandpassCal.sh



#!/usr/bin/env bash
#
# Process the science field observations: split out per beam, flag,
# apply the bandpass solution, average spectrally, image the continuum
# with or without self-calibration, and image the spectral-line
# data. Finally, mosaic the continuum images.
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

echo "Setting up and calibrating the science observation"

FLAG_IMAGING_DEP=""

for((BEAM=${BEAM_MIN}; BEAM<=${BEAM_MAX}; BEAM++)); do

    mkdir -p ${OUTPUT}/Checkfiles
    # an empty file that will indicate that the flagging has been done
    FLAG_CHECK_FILE="${OUTPUT}/Checkfiles/FLAGGING_DONE_BEAM${BEAM}"
    # an empty file that will indicate that the bandpass has been done
    BANDPASS_CHECK_FILE="${OUTPUT}/Checkfiles/BANDPASS_APPLIED_BEAM${BEAM}"
    # an empty file that will indicate the gains have been applied to
    # the spectral-line dataset
    SL_GAINS_CHECK_FILE="${OUTPUT}/Checkfiles/GAINS_APPLIED_SL_BEAM${BEAM}"
    # an empty file that will indicate the continuum has been
    # subtracted from the spectral-line dataset
    CONT_SUB_CHECK_FILE="${OUTPUT}/Checkfiles/CONT_SUB_SL_BEAM${BEAM}"
    
    findScienceMSnames

    . ${SCRIPTDIR}/splitFlagScience.sh
    
    . ${SCRIPTDIR}/applyBandpassScience.sh

    . ${SCRIPTDIR}/averageScience.sh

    if [ $DO_SELFCAL == true ]; then
	. ${SCRIPTDIR}/continuumImageScienceSelfcal.sh
    else
	. ${SCRIPTDIR}/continuumImageScience.sh
    fi

    . ${SCRIPTDIR}/sourcefinding.sh

    . ${SCRIPTDIR}/prepareSpectralData.sh
    . ${SCRIPTDIR}/spectralImageScience.sh

done

. ${SCRIPTDIR}/linmos.sh


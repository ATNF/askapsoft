#!/usr/bin/env bash
#
# Front-end script to run BETA continuum processing (and eventually
#  spectral-line processing too.) 
# This handles the bandpass calibration from individual-beam
#  observations of 1934-638, and applies them to a "science"
#  observation, that is then averaged and imaged, optionally with 
#  self-calibration. 
# Many of the parameters are governed by the environment variables
#  defined in scripts/defaultConfig.sh. The user needs to define the 
#  scheduling block numbers of the datasets to be used, or to provide
#  the filenames of specific measurement sets.
# The scripts that this front-end makes use of are kept in the 
#  askapsoft/ subdirectory of the ACES subversion repository.
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

USAGE="processBETA.sh -c <config file>"

if [ "${ACES}" == "" ]; then
  
    echo "You need to define the \$ACES environment variable. Loading the ACES module on galaxy will do that."

else 

    SCRIPTDIR=${ACES}/askapsoft/scripts

    . ${SCRIPTDIR}/initialise.sh

    if [ $# == 0 ]; then
	echo $USAGE
    else
	
	userConfig=""
	while getopts ':c:' opt
	do
	    case $opt in
		c) userConfig=$OPTARG;;
		\?) echo "ERROR: Invalid option: $USAGE"
		    exit 1;;
	    esac
	done

	if [ "$userConfig" != "" ]; then
            if [ -e ${userConfig} ]; then
	        echo "Getting extra config from file $userConfig"
	        echo " "
	        . ${userConfig}
            else
                echo "ERROR: Configuration file $userConfig not found"
                exit 1
            fi
	fi

	lfs setstripe -c 8 .

	. ${SCRIPTDIR}/utils.sh	
	. ${SCRIPTDIR}/processDefaults.sh
	
	if [ $DO_1934_CAL == true ]; then
	    
	    . ${SCRIPTDIR}/run1934cal.sh
	    
	fi
	
	if [ $DO_SCIENCE_FIELD == true ]; then
	    
	    . ${SCRIPTDIR}/scienceCalIm.sh
	    
	fi
	
	. ${SCRIPTDIR}/finalise.sh
	
    fi

fi


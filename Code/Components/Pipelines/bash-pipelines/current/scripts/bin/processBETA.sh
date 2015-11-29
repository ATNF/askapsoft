#!/usr/bin/env bash
#
# Front-end script to run BETA processing.
# This handles the bandpass calibration from individual-beam
#  observations of 1934-638, and applies them to a "science"
#  observation, that is then averaged and imaged, optionally with 
#  self-calibration in the case of continuum imaging. 
# Many of the parameters are governed by the environment variables
#  defined in scripts/defaultConfig.sh. The user needs to define the 
#  scheduling block numbers of the datasets to be used, or to provide
#  the filenames of specific measurement sets.
# The scripts that this front-end makes use of are kept in the 
#  askapsoft/ subdirectory of the ACES subversion repository.
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

USAGE="processBETA.sh -c <config file>"

if [ "${PIPELINEDIR}" == "" ]; then
  
    echo "ERROR - the environment variable PIPELINEDIR has not been set. Cannot find the scripts!"

else 

    . ${PIPELINEDIR}/initialise.sh

    if [ $# == 0 ]; then
        echo "No config file provided!"
	echo "Usage: $USAGE"
    else
	
	userConfig=""
	while getopts ':c:h' opt
	do
	    case $opt in
		c) userConfig=$OPTARG;;
                h) echo "Usage: $USAGE"
                   exit 0;;
		\?) echo "ERROR: Invalid option: $USAGE"
		    exit 1;;
	    esac
	done

	. ${PIPELINEDIR}/utils.sh	
        reportVersion | tee -a $JOBLIST

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

	. ${PIPELINEDIR}/processDefaults.sh
	
        if [ "`which lfs`" == "" ]; then
            echo "WARNING: You don't appear to be running this on /scratch2 on galaxy, as 'lfs' is not available."
            if [ $SUBMIT_JOBS == true ]; then
                echo "Setting SUBMIT_JOBS=false, as you won't be able to submit (or you shouldn't submit here)"
                SUBMIT_JOBS=false
            fi
        else
	    lfs setstripe -c 8 .
        fi
        
	if [ $DO_1934_CAL == true ]; then
	    
	    . ${PIPELINEDIR}/run1934cal.sh
	    
	fi
	
	if [ $DO_SCIENCE_FIELD == true ]; then
	    
	    . ${PIPELINEDIR}/scienceCalIm.sh
	    
	fi
	
	. ${PIPELINEDIR}/finalise.sh
	
    fi

fi


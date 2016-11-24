#!/bin/bash -l
#
# Front-end script to run ASKAP processing.
# This handles the bandpass calibration from individual-beam
#  observations of 1934-638, and applies them to a "science"
#  observation, that is then averaged and imaged, optionally with 
#  self-calibration in the case of continuum imaging. 
# Many of the parameters are governed by the environment variables
#  defined in scripts/defaultConfig.sh. The user needs to define the 
#  scheduling block numbers of the datasets to be used, or to provide
#  the filenames of specific measurement sets.
#
# @copyright (c) 2016 CSIRO
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

USAGE="processASKAP.sh -c <config file>"

if [ "${PIPELINEDIR}" == "" ]; then
    
    echo "ERROR - the environment variable PIPELINEDIR has not been set. Cannot find the scripts!"

else 

    if [ "`lfs getstripe -c .`" == "" ]; then
        echo "WARNING: You don't appear to be running this on a Lustre filesystem - lfs does not work."
        exit 1
    fi
    
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
	        . ${userConfig}
                archiveConfig $userConfig
            else
                echo "ERROR: Configuration file $userConfig not found"
                exit 1
            fi
	fi

        . ${PIPELINEDIR}/prepareMetadata.sh
        PROCESS_DEFAULTS_HAS_RUN=false
	. ${PIPELINEDIR}/processDefaults.sh
	
	lfs setstripe -c ${LUSTRE_STRIPING} .
        
	. ${PIPELINEDIR}/run1934cal.sh
	    
        . ${PIPELINEDIR}/scienceCalIm.sh
	    
        . ${PIPELINEDIR}/gatherStats.sh
        
        . ${PIPELINEDIR}/archive.sh
	
	. ${PIPELINEDIR}/finalise.sh

    fi

fi


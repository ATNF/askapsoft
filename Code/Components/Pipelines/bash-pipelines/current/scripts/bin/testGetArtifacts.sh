#!/bin/bash -l

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
    . ${PIPELINEDIR}/defineArtifactsScript.sh

    output=imageFileSummary-${NOW}.txt
    rm -f $output
    expectedImageNames=()

    # Define the lists of image names, types, 
    . "${getArtifacts}"

    expectedImageNames=(${casdaTwoDimImageNames[@]})
    expectedImageNames+=(${casdaOtherDimImageNames[@]})

    for image in ${expectedImageNames[@]}; do
        echo $image >> "$output"
    done

fi

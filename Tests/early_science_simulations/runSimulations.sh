#!/bin/bash -l

simScripts=`pwd`/simulationScripts
configFile="config.sh"

USAGE="runSimulations.sh -c <config file>
    Where <config file> is in the directory $simScripts.
    If no config file given, \"$configFile\" is used."

doSubmit=true
#doSubmit=false
runIt=true

depend=""

while getopts ':c:h' opt
do
    case $opt in
	c) configFile=$OPTARG;;
        h) echo "Usage: $USAGE"
           exit 0;;
	\?) echo "ERROR: Invalid option: $USAGE"
	    exit 1;;
    esac
done


if [ "${ASKAP_ROOT}" == "" ]; then
    echo "You have not set ASKAP_ROOT! Exiting."
    runIt=false
fi

if [ "${AIPSPATH}" == "" ]; then
    echo "You have not set AIPSPATH! Exiting."
    runIt=false
fi

if [ $runIt == true ]; then    

    . ${simScripts}/$configFile
    . ${simScripts}/setup.sh

    # Run the 9-beam calibration observation of 1934-638
    . ${simScripts}/observeCalibrator.sh

    # Make the input sky model
    . ${simScripts}/makeInputModel.sh

    # Run the observation of the science field
    . ${simScripts}/observeScienceField.sh

    cd ..

    if [ ${doSubmit} == true ] && [ "${SBATCH_JOBLIST}" ]; then
	scontrol release $SBATCH_JOBLIST
    fi
fi


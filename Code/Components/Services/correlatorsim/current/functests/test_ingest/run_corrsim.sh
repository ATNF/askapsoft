#!/bin/bash
#
# This script runs correlator simulator
#
# Author: Paulus Lahur <paulus.lahur@csiro.au>
#
echo "Correlator Simulator script"

# Get the number of data streams
if [ -z "$1" ]; then
    echo "This script streams data from Correlator Simulator."
    echo "The number of data streams must be specified."
    echo "Usage: ./run_corrsim.sh <number of streams>"
    exit -1
else
    if [ $1 -lt 1 ]; then
        echo "The number of data streams must be at least 1"
        exit -1
    else
  		NVSTREAM=$1
    fi
fi
echo "The number of data streams: "$NVSTREAM

# Get the amount of time delay in starting (in seconds)
if [ -z "$2" ]; then
  	# By default there is no delay
  	DELAY_START=0
else
  	DELAY_START=$2
fi
echo "Delay before data stream starts: "$DELAY_START" seconds"

# Get the duration (in minutes)
if [ -z "$3" ]; then
  	# Default duration is 5 minutes
  	DURATION=5m
else
  	DURATION=$3
fi
echo "Duration of data streams: "$DURATION" minutes"

# Set environment
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

CORRSIM_DIR=$WORKSPACE/Code/Components/Services/correlatorsim/current/apps

CORRSIM_PARSET=playback.in

let NCORRSIM=$NVSTREAM+1

sleep $DELAY_START

echo "Correlator simulator starts sending metadata and $NVSTREAM visibility streams"

timeout -s 9 $DURATION mpirun -np $NCORRSIM $CORRSIM_DIR/playbackADE.sh -c $CORRSIM_PARSET

echo "Correlator simulator is finished"


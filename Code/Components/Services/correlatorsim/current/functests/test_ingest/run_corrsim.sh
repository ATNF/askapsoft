#!/bin/bash
#
# This script runs correlator simulator
#
# Get the number of visibility streams
if [ -z "$1" ]; then
  NVSTREAM=1
else
  NVSTREAM=$1
fi
echo "Visibility stream in correlator simulator: "$NVSTREAM

# Get the amount of time delay in starting (in seconds)
if [ -z "$2" ]; then
  # By default there is no delay
  DELAY_START=0
else
  DELAY_START=$2
fi
echo "Delay: "$DELAY_START

# Get the duration (in minutes)
if [ -z "$3" ]; then
  # Default duration is 5 minutes
  DURATION=5m
else
  DURATION=$3
fi
echo "Duration: "$DURATION

# Set environment
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

CORRSIM_DIR=$WORKSPACE/Code/Components/Services/correlatorsim/current/apps

CORRSIM_PARSET=playback.in

let NCORRSIM=$NVSTREAM+1

sleep $DELAY_START

echo "Correlator simulator starts sending metadata and $NVSTREAM visibility streams"

timeout -s 9 $DURATION mpirun -np $NCORRSIM $CORRSIM_DIR/playbackADE.sh -c $CORRSIM_PARSET

echo "Correlator simulator is finished"


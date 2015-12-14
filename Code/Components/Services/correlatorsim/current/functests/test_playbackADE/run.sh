#!/bin/bash

###########################################################################
# This script runs all programs for simulated data transmission
# in ADE format:
# - Receiver   : msnoop (metadata) and vsnoopADE (visibility)
# - Sender     : playbackADE (correlator simulator)
# - Facilitator: ICE
# Data is transmitted in parallel in separate UDP ports.
# playbackADE sends data using MPI processes, and vsnoopADE receives it
# using multiple instances of the program.
#
echo "===================================================================="
echo "Functional test for correlator simulator using msnoop and vsnoopADE"
echo "===================================================================="

# USER SETTING
#
# The number of shelves (cards) in correlator simulator.
# Each shelf has its own UDP port number.
# No need to set this anywhere else (eg. in playback.in).
# This number is used to automatically set the number of MPI processes
# for correlator simulator and instances for vsnoopADE.
# Range: 1 ~ 12

NCARD=12
echo "The number of cards (data streams): $NCARD"

# The first port number for visibility transmission 
# (receiver: vsnoopADE)

VPORT1=3001
echo "The first port for visibility data: $VPORT1"

# The port number for metadata transmission

MPORT=4061
echo "The port for metadata             : $MPORT"
echo
###########################################################################

cd `dirname $0`

echo "Setting up the environment"
if [ ! -f ../../init_package_env.sh ]; then
    echo "Error: init_package_env.sh does not exist, please run rbuild in package dir"
    exit 1
fi
source ../../init_package_env.sh
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

echo "Starting the Ice Services"
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
if [ $? -ne 0 ]; then
    echo "Error: Failed to start Ice Services"
    exit 1
fi
sleep 1

echo "Starting the metadata subscriber msnoop" 
# Set the metadata port
sed -i "s/playback.tossim.ice.locator_port.*/playback.tossim.ice.locator_port = $MPORT/" playback.in
sed -i "s/ice.locator_port.*/ice.locator_port = $MPORT/" msnoop.in
../../apps/msnoop -c msnoop.in -v > msnoop.log 2>&1 &
MDPID=$!

echo "Starting the visibility receiver vsnoop" 
COUNTER=1
while [ $COUNTER -le $NCARD ]; do

    # Set visibility port & log file for each data stream
    let VPORT=VPORT1+COUNTER-1
    LOGFILE="vsnoop$COUNTER.log"

    # Execute instances of vsnoop
    ../../apps/vsnoopADE -v -p $VPORT > $LOGFILE 2>&1 &
    VISPID[$COUNTER]=$!

    let COUNTER=COUNTER+1
done

echo "Starting correlator simulator"
# Set its number of MPI processes 
let NMPI=NCARD+1
# Set the reference port in its parameter file
sed -i "s/playback.corrsim.out.port.*/playback.corrsim.out.port = $VPORT1/" playback.in
mpirun -np $NMPI ../../apps/playbackADE.sh -c playback.in
STATUS=$?
echo "playbackADE status: " $STATUS

echo "Giving the subscriber a moment to get the last messages then exit"
sleep 5
kill $MDPID
echo "Killed msnoop"
kill ${VISPID[*]}
echo "Killed all vsnoop instances"
sleep 1
kill -9 $MDPID > /dev/null 2>&1
kill -9 ${VISPID[*]} > /dev/null 2>&1

echo "Stopping the Ice Services"
../stop_ice.sh ../icegridadmin.cfg

exit $STATUS

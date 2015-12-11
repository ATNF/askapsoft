#!/bin/bash

###########################################################################
# Set the number of shelves (cards) in correlator simulator.
# A shelf has its own UDP port number.
# No need to set this anywhere else (eg. in playback.in).
# This number is used to automatically set the number of MPI processes
# for correlator simulator and instances vsnoopADE.
# Range: 1 ~ 12

NCARD=12

# Reference port number of vsnoopADE.
# The first instance of vsnoopADE uses this port number.

REFPORT=3001

# TO CONSIDER
# What about using this number to automatically set the port of both
# the sender (correlator simulator) and the receiver (ingest, vsnoop)?
# This will avoid the possibility of port mismatch.
###########################################################################

cd `dirname $0`

# Setup the environment
if [ ! -f ../../init_package_env.sh ]; then
    echo "Error: init_package_env.sh does not exist, please run rbuild in package dir"
    exit 1
fi
source ../../init_package_env.sh
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
if [ $? -ne 0 ]; then
    echo "Error: Failed to start Ice Services"
    exit 1
fi
sleep 1

# Start the metadata subscriber 
# (don't use the script so this script can kill it)
../../apps/msnoop -c msnoop.in -v > msnoop.log 2>&1 &
MDPID=$!

# Start the visibilities receiver 
COUNTER=1
while [ $COUNTER -le $NCARD ]; do
    let PORT=REFPORT+COUNTER-1
    LOGFILE="vsnoop$COUNTER.log"

    ../../apps/vsnoopADE -v -p $PORT > $LOGFILE 2>&1 &
    VISPID[$COUNTER]=$!

    let COUNTER=COUNTER+1
done

# The number of MPI processes is set automatically
let NMPI=NCARD+1

mpirun -np $NMPI ../../apps/playbackADE.sh -c playback.in
STATUS=$?
echo "playbackADE status: " $STATUS

# Give the subscriber a moment to get the last messages then exit
sleep 5
kill $MDPID
echo "Killed msnoop"
kill ${VISPID[*]}
echo "Killed all vsnoops"
sleep 1
kill -9 $MDPID > /dev/null 2>&1
kill -9 ${VISPID[*]} > /dev/null 2>&1

# Stop the Ice Services
../stop_ice.sh ../icegridadmin.cfg

exit $STATUS

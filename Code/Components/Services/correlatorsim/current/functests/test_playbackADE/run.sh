#!/bin/bash

###############################################################
# Set the number of shelves (cards) in correlator simulator.
# A shelf has its own UDP port number.
# Range: 1 ~ 12
# No need to set this anywhere else (eg. in playback.in)

NCARD=12

###############################################################

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
# (don't use the script so this script can kill it)
../../apps/vsnoopADE -v -p 3001 > vsnoop1.log 2>&1 &
VISPID1=$!
../../apps/vsnoopADE -v -p 3002 > vsnoop2.log 2>&1 &
VISPID2=$!
../../apps/vsnoopADE -v -p 3003 > vsnoop3.log 2>&1 &
VISPID3=$!
../../apps/vsnoopADE -v -p 3004 > vsnoop4.log 2>&1 &
VISPID4=$!
../../apps/vsnoopADE -v -p 3005 > vsnoop5.log 2>&1 &
VISPID5=$!
../../apps/vsnoopADE -v -p 3006 > vsnoop6.log 2>&1 &
VISPID6=$!
../../apps/vsnoopADE -v -p 3007 > vsnoop7.log 2>&1 &
VISPID7=$!
../../apps/vsnoopADE -v -p 3008 > vsnoop8.log 2>&1 &
VISPID8=$!
../../apps/vsnoopADE -v -p 3009 > vsnoop9.log 2>&1 &
VISPID9=$!
../../apps/vsnoopADE -v -p 3010 > vsnoop10.log 2>&1 &
VISPID10=$!
../../apps/vsnoopADE -v -p 3011 > vsnoop11.log 2>&1 &
VISPID11=$!
../../apps/vsnoopADE -v -p 3012 > vsnoop12.log 2>&1 &
VISPID12=$!

# The number of MPI processes is adjusted automatically
let NMPI=$NCARD+1

mpirun -np $NMPI ../../apps/playbackADE.sh -c playback.in
STATUS=$?
echo "playbackADE status: " $STATUS

# Give the subscriber a moment to get the last messages then exit
sleep 5
kill $MDPID
echo "Killed msnoop"
kill $VISPID1 $VISPID2 $VISPID3 $VISPID4 $VISPID5 $VISPID6
kill $VISPID7 $VISPID8 $VISPID9 $VISPID10 $VISPID11 $VISPID12
echo "Killed all vsnoops"
sleep 1
kill -9 $MDPID > /dev/null 2>&1
kill -9 $VISPID1 $VISPID2 $VISPID3 $VISPID4 > /dev/null 2>&1
kill -9 $VISPID5 $VISPID6 $VISPID7 $VISPID8 > /dev/null 2>&1
kill -9 $VISPID9 $VISPID10 $VISPID11 $VISPID12 > /dev/null 2>&1

# Stop the Ice Services
../stop_ice.sh ../icegridadmin.cfg

exit $STATUS

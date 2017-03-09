#!/bin/bash
#
# This script streams data from Correlator Simulator to Ingest.
# Adapted from Tests/hudson/CP/ingest.sh
# 
# Author: Paulus Lahur <paulus.lahur@csiro.au>
#
echo "This script coordinates data streaming from Correlator Simulator to Ingest."

export WORKSPACE=$ASKAP_ROOT

# Max number of blocks, cards and frequency channels
MAX_BLOCK=8
MAX_CARD_IN_BLOCK=12
MAX_CHANNEL_IN_CARD=4
#MAX_CHANNEL_IN_CARD=216
CHANNEL_DIV=54
#CHANNEL_DIV=1

let MAX_FINE_CHANNEL_IN_CARD=$MAX_CHANNEL_IN_CARD*$CHANNEL_DIV
let MAX_CARD=$MAX_BLOCK*$MAX_CARD_IN_BLOCK

# Get total number of cards from argument
# If argument does not exist
if [ -z "$1" ]; then
  	echo "The number of cards used in streaming the data must be specified."
  	echo "Usage: ./run.sh <number of cards = 1~96>"
  	exit -1
else
  	#echo "argument exists: $1"
  	if [ $1 -lt 1 ]; then
    	echo "The number of cards must be at least 1"
    	exit -2
  	elif [ $1 -le $MAX_CARD ]; then
    	NCARD=$1
    	echo "The number of cards: $NCARD"
  	elif [ $1 -gt $MAX_CARD ]; then
    	echo "Too many cards. Maximum is $MAX_CARD"
    	exit -2
  	else
    	echo "Illegal argument"
    	exit -2
  	fi
fi

# Compute the number of coarse channels
let NCHANNEL=$NCARD*$MAX_CHANNEL_IN_CARD
echo "The number of channels: $NCHANNEL"

# Compute the number of visibility streams
NVSTREAM=$NCARD
echo "The number of visibility streams: $NVSTREAM"

# Set environment
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

CORRSIM_DIR=$WORKSPACE/Code/Components/Services/correlatorsim/current/apps

echo "Removing existing output files"
rm -rf ingest_test*.ms
rm -f visplot_*.dat
rm -f spectra*.dat

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
if [ $? -ne 0 ]; then
    echo "Error: Failed to start Ice Services"
    exit 1
fi
sleep 1

#-----------------------------------------------------------------------
# Correlator simulator
echo "Correlator simulator: "`date`

CORRSIM_PARSET=playback.in

echo "Modifying its parset"
# Set the number of channels in correlator simulator
sed -i "s/playback.corrsim.n_coarse_channels.*/playback.corrsim.n_coarse_channels = $NCHANNEL/" $CORRSIM_PARSET

# Set channel division
sed -i "s/playback.corrsim.n_channel_subdivision.*/playback.corrsim.n_channel_subdivision = $CHANNEL_DIV/" $CORRSIM_PARSET

# Delay start in seconds
DELAY_START=10

# Streaming duration in minutes
DURATION=10m

# asynchronous launch of the correlator simulator 
./run_corrsim.sh $NVSTREAM $DELAY_START $DURATION > simcor.out &

echo "Correlator simulator is finished"`date`

#-----------------------------------------------------------------------
# Ingest pipeline
echo "Ingest pipeline: "`date`

INGEST_PARSET=cpingest.in

echo "Modifying its parset"

echo "Setting ingest parameter: task list"
if [ $NVSTREAM -eq 1 ]; then
    # single process
	#sed -i "s/tasks.tasklist.*/tasks.tasklist = [MergedSource, CalcUVWTask, Monitor, MSSink]/" $INGEST_PARSET
	sed -i "s/tasks.tasklist.*/tasks.tasklist = [MergedSource, MSSink]/" $INGEST_PARSET
else
	# parallel
	sed -i "s/tasks.tasklist.*/tasks.tasklist = [MergedSource, Merge, CalcUVWTask, Monitor, MSSink]/" $INGEST_PARSET
	#sed -i "s/tasks.tasklist.*/tasks.tasklist = [MergedSource, CalcUVWTask, Monitor, MSSink]/" $INGEST_PARSET
fi

#if [ $NVSTREAM -lt $MAX_CARD_IN_BLOCK ]; then
#  	NFILE=$NVSTREAM
#else
#  	NFILE=$MAX_CARD_IN_BLOCK
#fi
#let NFILE=1
#echo "Setting ingest parameter: ranks to merge: $NFILE"
#sed -i "s/tasks.Merge.params.ranks2merge.*/tasks.Merge.params.ranks2merge = $NFILE/" $INGEST_PARSET

echo "Removing ingest parameter: channel list"
CHANNEL_COMMENT="# Automatically generated channel list"
sed -i "/$CHANNEL_COMMENT/D" $INGEST_PARSET
CHANNEL_STRING="tasks.MergedSource.params.n_channels."
sed -i "/$CHANNEL_STRING/d" $INGEST_PARSET

CHANMIN=0
let CHANMAX=$NVSTREAM-1

echo "Setting ingest parameter: channel list"
echo $CHANNEL_COMMENT >> $INGEST_PARSET
echo $CHANNEL_STRING$CHANMIN".."$CHANMAX" = "$MAX_FINE_CHANNEL_IN_CARD >> $INGEST_PARSET

#timeout -s 9 10m ./run.sh
#timeout -s 9 10m ./runparallel.sh $NCARD
#./run_ingest.sh $NVSTREAM
timeout -s 9 10m ./run_ingest.sh $NVSTREAM
ERROR=$?

echo "Ingest finished: "`date`

# Stop the Ice Services
../stop_ice.sh ../icegridadmin.cfg

echo "-------------- output of the correlator simulator:"
cat simcor.out
echo "--------------------------------------------------"

if [ $ERROR -ne 0 ]; then
    echo "Ingest returned error $ERROR"
    echo "Failing the test on this condition has been disabled"
    #exit 1
fi

if [ ! -d ingest_test0.ms ]; then
#if [ ! -d $SCRATCHDIR/ingest_test0.ms ]
    echo "Error: ingest_test0.ms was not created"
    exit 1
fi

#SZ=$(set -- `du -sh ingest_test0.ms` ; echo $1)
#
#if [ "$SZ" != "118M" ]; then
#   echo "The size of the output MS ("$SZ") seems to be different from 87M"
#   exit 1
#fi


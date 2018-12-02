#!/bin/bash
# 
# Script to run Ingest.
# Adapted from Tests/hudson/CP/ingest.sh
#
# Author: Paulus Lahur <paulus.lahur@csiro.au>
#
echo "Ingest script"

# Get the number of data streams
if [ -z "$1" ]; then
    echo "This script executes Ingest"
    echo "The number of incoming data streams must be specified."
    echo "Usage: ./run_ingest.sh <number of streams>"
    exit -1
else
	if [ $1 -lt 1 ]; then
		echo "The number of data streams must be at least 1"
		exit -1
	else
		NVSTREAM=$1
	fi
fi
echo "The number of data streams: $NVSTREAM"

PACKAGE_DIR=$ASKAP_ROOT/Code/Components/Services/ingest/current
APP_DIR=$PACKAGE_DIR/apps

# Setup the environment
source $PACKAGE_DIR/init_package_env.sh
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

let NINGEST=$NVSTREAM

# Run the ingest pipeline
#if [ $NINGEST -ge 1 ]; then
#    echo "Running $NINGEST processes of ingest ..."
#    mpirun -np $NINGEST $APP_DIR/cpingest.sh -c cpingest.in
#else
#    echo "Illegal argument"
#    exit -1
#fi
if [ $NINGEST -gt 1 ]; then
    echo "Running a parallel of $NINGEST processes of ingest ..."
    mpirun -np $NINGEST $APP_DIR/cpingest.sh -c cpingest.in
elif [ $NINGEST -eq 1 ]; then
    echo "Running a single process of ingest ..."
    $APP_DIR/cpingest.sh -s -c cpingest.in
else
    echo "Illegal argument"
    exit -1
fi
STATUS=$?
echo "Ingest is finished"

exit $STATUS

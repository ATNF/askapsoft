#!/usr/bin/env bash
#
# A script to set up the working directory prior to starting the job
# submission. This defines all necessary subdirectories and sets the
# date-time stamp.
#
# (c) Matthew Whiting, CSIRO ATNF, 2014

####################
# Define & create directories
CWD=`pwd`
parsets=${CWD}/parsets
logs=${CWD}/logs
slurms=${CWD}/slurmFiles
slurmOut=${CWD}/slurmOutput
tools=${CWD}/tools

mkdir -p $parsets
mkdir -p $logs
mkdir -p $slurms
mkdir -p $slurmOut
mkdir -p $tools

####################
# Date and time stamp
NOW=`date +%F-%H%M`

# File to record list of jobs and descriptions
JOBLIST="${slurmOut}/jobList-${NOW}.txt"

####################
# Define the default

. ${SCRIPTDIR}/defaultConfig.sh


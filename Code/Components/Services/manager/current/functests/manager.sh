#!/usr/bin/env bash

# Setup the environment
source common.sh

# TODO: How do I specify a different config file for each functional test?
echo Starting CP Manager process ...
java askap/cp/manager/CpManager -c config-files/cpmanager.in -l config-files/log4j.properties > cpmanager.log 2>&1 &
PID=$!
trap "kill -15 $PID" TERM KILL INT
wait

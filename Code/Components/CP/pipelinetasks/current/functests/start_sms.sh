#!/bin/bash

# Setup the environment
source `dirname $0`/../init_package_env.sh

# start the SMS
#echo SMS config: $1
#echo SMS logging config: $2
echo -n "Waiting for sky model service to startup ... "
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current
sms -c $1 -l $2 &
PID=$!
echo "${PID}" > sms.pid
echo "STARTED"

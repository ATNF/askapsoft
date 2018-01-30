#!/bin/bash

# Setup the environment
source ../../init_package_env.sh

# start the SMS
echo -n "Waiting for sky model service to startup ... "
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current
sms -c ./sms.in -l ./sms_logging.log_cfg &
PID=$!
echo "${PID}" > sms.pid
echo "STARTED"

#!/bin/bash

# Setup the environment
source ../../init_package_env.sh

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
sleep 2

# start the SMS, passing the full path to the config files
SMS_CONFIG=`readlink -f sms.in`
SMS_LOG_CONFIG=`readlink -f sms_logging.log_cfg`
../start_sms.sh $SMS_CONFIG $SMS_LOG_CONFIG
sleep 2

# use the SMS admin cli to initialise a database
sms_dev_tools --create-schema --config $SMS_CONFIG --log-config $SMS_LOG_CONFIG
if [ $? -ne 0 ]; then exit $?; fi

# use the SMS CLI to ingest a catalog
CATALOG=`readlink -f one_component.xml`
#CATALOG=`readlink -f small_catalog.xml`
sms_tools --config $SMS_CONFIG --log-config $SMS_LOG_CONFIG --ingest-components $CATALOG --sbid 10 --observation-date '2018-01-30T10:13:00'
if [ $? -ne 0 ]; then exit $?; fi

# Run the test harness
echo "Running the testcase..."
../../apps/tDataAccessors.sh -c tDataAccessors.in
STATUS=$?
echo "Testcase finished"

# stop the SMS
../stop_sms.sh

# Stop the Ice Services
../stop_ice.sh ../icegridadmin.cfg

exit $STATUS

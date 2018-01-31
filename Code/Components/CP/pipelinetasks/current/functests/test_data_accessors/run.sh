#!/bin/bash

# Setup the environment
source ../../init_package_env.sh

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
sleep 2

# start the SMS, passing the full path to the config files
../start_sms.sh `readlink -f sms.in` `readlink -f sms_logging.log_cfg`
sleep 2

# get the SMS to ingest a catalog (needs to use the SMS CLI)

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

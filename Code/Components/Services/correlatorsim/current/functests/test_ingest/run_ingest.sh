#!/bin/bash

NMPI=$1

PACKAGE_DIR=$ASKAP_ROOT/Code/Components/Services/ingest/current
APP_DIR=$PACKAGE_DIR/apps
#APP_DIR=/scratch2/astronomy856/lah00b/ASKAPsoft
#ICE_DIR=$PACKAGE_DIR/functests
#Code/Components/Services/ingest/current/functests/test_ingestpipeline

#cd `dirname $0`

# Setup the environment
source $PACKAGE_DIR/init_package_env.sh
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

# Start the Ice Services
#../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
#$ICE_DIR/start_ice.sh $ICE_DIR/iceregistry.cfg $ICE_DIR/icegridadmin.cfg $ICE_DIR/icestorm.cfg
#if [ $? -ne 0 ]; then
#    echo "Error: Failed to start Ice Services"
#    exit 1
#fi
#sleep 1

# Run the ingest pipeline
if [ $NMPI -gt 1 ]; then
    echo "Running a parallel of $NMPI processes of ingest ..."
    mpirun -np $NMPI $APP_DIR/cpingest.sh -c cpingest.in
else
    echo "Running a single process of ingest ..."
    $APP_DIR/cpingest.sh -s -c cpingest.in
fi
STATUS=$?
echo "Ingest is finished"

# Stop the Ice Services
#../stop_ice.sh ../icegridadmin.cfg
#$ICE_DIR/stop_ice.sh $ICE_DIR/icegridadmin.cfg

exit $STATUS

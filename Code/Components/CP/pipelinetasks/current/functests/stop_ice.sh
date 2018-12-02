#!/usr/bin/env bash

BASE_DIR=`dirname $0`
source $BASE_DIR/kill_service.sh

# Check the config file has been passed
if [ $# -ne 1 ]; then
    echo "usage: $0 <config file>"
    exit 1
fi

if [ ! -f $1 ]; then
    echo "Error: Config file $1 not found"
    exit 1
fi

# Setup the environment
source $BASE_DIR/../init_package_env.sh

# Stop IceStorm
kill_service icestorm

echo -n "Terminating service: iceregistry..."
# Request Ice Registry shutdown
icegridadmin --Ice.Config=$1 -u foo -p bar -e "registry shutdown Master"
if [ $? -ne 0 ]; then
    echo "Error stopping registry"
else
    echo "Done"
fi

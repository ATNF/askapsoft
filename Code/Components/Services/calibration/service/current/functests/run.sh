#!/bin/bash

#set -x

waitIceRegistry()
{ # Wait for Ice registry to start, but timeout after 10 seconds
    echo -n Waiting for Ice registry to startup...
    TIMEOUT=10
    STATUS=1
    while [ $STATUS -ne 0 ] && [ $TIMEOUT -ne 0 ]; do
        icegridregistry --Ice.Config=registry.cfg & #> /dev/null 2>&1
        REGISTRYPID=$! 
        STATUS=$?
        TIMEOUT=`expr $TIMEOUT - 1`
        sleep 1
    done

    if [ $TIMEOUT -eq 0 ]; then
        echo FAILED
    else
        echo STARTED
    fi
}

waitCDSServer()
{ # Wait for IceGrid to start, but timeout after 10 seconds
    echo -n Waiting for CDSServer to startup...
    TIMEOUT=10
    STATUS=1
    INACTIVE="<inactive>"
    RESULT=$INACTIVE
    while [ $STATUS -ne 0 ] && [ "${RESULT}" = "${INACTIVE}" ] && [ $TIMEOUT -ne 0 ]; do
        java -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8001 -cp $CLASSPATH askap.services.caldataservice.CdsServer --config server.cfg & #> /dev/null 2>&1
        SERVERPID=$! 
        STATUS=$?
        TIMEOUT=`expr $TIMEOUT - 1`
        sleep 1
    done

    if [ $TIMEOUT -eq 0 ]; then
        echo FAILED
    else
        echo STARTED
    fi
}

# Setup the environment
source `dirname $0`/../init_package_env.sh
echo $CLASSPATH

# Create directories for Ice registry and binary storage
mkdir -p db/data
rm -rf /var/tmp/cdsstoragetest
mkdir -p /var/tmp/cdsstoragetest

# Start the registry

waitIceRegistry

#read -n1 -r -p "Press any key to continue..." key

# Start the server

waitCDSServer
sleep 5s

#read -n1 -r -p "Press any key to start the test..." key

# Run the test
java -Xdebug -Xrunjdwp:transport=dt_socket,server=y,suspend=n,address=8002 -cp $CLASSPATH askap.services.caldataservice.CdsClient client.cfg test
STATUS=$?

#read -n1 -r -p "Press any key to continue..." key

# Request IceGrid shutdown and wait
echo -n "Stopping CDS server and ICE registry..."

kill $REGISTRYPID
kill $SERVERPID

wait $REGISTRYPID
wait $SERVERPID
echo "STOPPED"

# Remove temporary directories
rm -rf db
rm -rf /var/tmp/cdsstoragetest

exit $STATUS

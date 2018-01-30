#!/bin/bash

# Arg1: Process ID
function process_exists {
    ps -p $1 > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        PROC_EXISTS=true
    else
        PROC_EXIST=false
    fi
}

# Arg1: Pidfile name
function kill_service  {
    echo -n "Terminating service: ${1}..."
    if [ -f ${1}.pid ]; then
        PID=`cat ${1}.pid`
        process_exists ${PID}
        if ${PROC_EXISTS} ; then
            kill -SIGTERM ${PID}
            sleep 5
            kill -SIGKILL ${PID} > /dev/null 2>&1
        fi
        rm -f ${1}.pid
    fi
    echo "Done"
}

kill_service sms

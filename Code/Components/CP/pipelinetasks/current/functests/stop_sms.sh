#!/bin/bash

BASE_DIR=`dirname $0`
source $BASE_DIR/kill_service.sh
kill_service sms

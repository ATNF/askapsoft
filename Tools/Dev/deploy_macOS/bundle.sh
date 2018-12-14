#!/usr/bin/env bash 

echo "====================================="
echo "info - bundle script for applications"
echo "====================================="

APP=$1

if [ -z "$APP" ]
then
      echo "Specifiy APP to bundle on the command line"
      exit 1;
else
      echo "will attempt to bundle $APP";
fi


FULL_PATH_TO_APP=`find . -name $APP`;

if [ -z "$FULL_PATH_TO_APP" ]
then
   echo "Cannot find $APP - are you sure?"
   exit 1
else
   echo "Found $FULL_PATH_TO_APP"
fi


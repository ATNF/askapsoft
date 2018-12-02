#!/bin/bash

cd `dirname $0`

INITIALDIR=`pwd`
echo Running test cases...

FAIL=0

cd test_cmodel
echo
echo test_cmodel
echo ----------------------------------------
./run.sh
if [ $? -eq 0 ]; then
    R1="test_cmodel           PASS"
else
    R1="test_cmodel           FAIL"
    FAIL=1
fi
cd $INITIALDIR

cd test_data_accessors
echo
echo test_data_accessors
echo ----------------------------------------
./run.sh
if [ $? -eq 0 ]; then
    R2="test_data_accessors   PASS"
else
    R2="test_data_accessors   FAIL"
    FAIL=1
fi
cd $INITIALDIR
# Print Results
echo
echo Result Summary:
echo ============================
echo $R1
echo $R2

if [ $FAIL -eq 0 ]; then
    exit 0
else
    exit 1
fi

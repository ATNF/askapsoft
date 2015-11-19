#!/bin/bash

cd `dirname $0`

INITIALDIR=`pwd`
echo Running test cases...

FAIL=0

# test_metadatasource testcase
echo "Running test_metadatasource"
netstat -ano | grep 3000
date
cd test_metadatasource
./run.sh
if [ $? -eq 0 ]; then
    R1="test_metadatasource  PASS"
else
    R1="test_metadatasource  FAIL"
    FAIL=1
fi
cd $INITIALDIR


# test_frtmetadatasource testcase
echo "Running test_frtmetadatasource"
netstat -ano | grep 3000
date
cd test_frtmetadatasource
./run.sh
if [ $? -eq 0 ]; then
    R2="test_frtmetadatasource  PASS"
else
    R2="test_frtmetadatasource  FAIL"
    FAIL=1
fi
cd $INITIALDIR


# test_vissource testcase
echo "Running test_vissource"
netstat -ano | grep 3000
date
cd test_vissource
./run.sh
if [ $? -eq 0 ]; then
    R3="test_vissource  PASS"
else
    R3="test_vissource  FAIL"
    FAIL=1
fi
cd $INITIALDIR

# Print Results
echo
echo Result Summary:
echo ===============
echo $R1
echo $R2
echo $R3

if [ $FAIL -eq 0 ]; then
    exit 0
else
    exit 1
fi

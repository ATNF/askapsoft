#!/bin/bash

#
# Phase 1
#

echo "------------- Phase 1 ------------"

unset ASKAP_ROOT
cd $WORKSPACE/trunk
nice /usr/bin/python bootstrap.py -n
if [ $? -ne 0 ]; then
    echo "Error: Bootstrapping failed"
    exit 1
fi

#
# Setup environment
#
source initaskap.sh
if [ $MODULESHOME ]; then
    module load openmpi
fi
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

#
# Build Synthesis
# Note, mpi is needed for the correlator simulator, so build everything with mpi
#
RBUILD_OPTS="-n -p mpi=1"
nice rbuild ${RBUILD_OPTS} Code/Components/Synthesis/synthesis/current
if [ $? -ne 0 ]; then
    echo "Error: rbuild ${RBUILD_OPTS} Code/Components/Synthesis/synthesis/current failed "
    exit 1
fi

#
# Build testdata for the correlator simulator to work
#
nice rbuild ${RBUILD_OPTS} Code/Components/Synthesis/testdata/current
if [ $? -ne 0 ]; then
    echo "rbuild ${RBUILD_OPTS} Code/Components/Synthesis/testdata/current failed"
    exit 1
fi

#
# Run the unit tests
#
cd $WORKSPACE/trunk/Code/Components/Synthesis/synthesis/current
scons test

cd $WORKSPACE/trunk/Code/Base/scimath/current
scons test

cd $WORKSPACE/trunk/Code/Base/accessors/current
scons test

#
# Phase 2
#
#
# Run the solver test
#
cd $WORKSPACE/trunk/Code/Components/Synthesis/testdata/current/simulation/synthregression
python calibratortest.py
ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "calibratortest.py returned errorcode $ERROR"
    exit 1
fi


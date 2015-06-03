#!/bin/bash
#
# Phase 1
#
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
# Build ingest pipeline
#
nice rbuild ${RBUILD_OPTS} Code/Components/Services/ingest/current
if [ $? -ne 0 ]; then
    echo "rbuild ${RBUILD_OPTS} Code/Components/Services/ingest/current failed"
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
# Build the correlator simulator
#
nice rbuild ${RBUILD_OPTS} Code/Components/Services/correlatorsim/current
if [ $? -ne 0 ]; then
    echo "rbuild ${RBUILD_OPTS} Code/Components/Services/correlatorsim/current failed"
    exit 1
fi

#
# Run the unit tests
#
cd $WORKSPACE/trunk/Code/Components/Services/correlatorsim/current
scons test

cd $WORKSPACE/trunk/Code/Components/Services/ingest/current
scons test

#
# Phase 2
#
#
# Simulate dataset for streaming - this is the only function we use from
# synthesis
#
cd $WORKSPACE/trunk/Code/Components/Services/correlatorsim/current/dataset
./sim.sh
ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "correlatorsim/current/dataset/sim.sh returned errorcode $ERROR"
    exit 1
fi

#
# Phase 3
#
#
# Run serial functests which do not require multiple processes running
# and MPI
#

cd $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests
./run.sh
ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "ingest/current/functests/run.sh returned errorcode $ERROR"
    exit 1
fi



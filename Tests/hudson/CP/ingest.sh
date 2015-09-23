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

cd $WORKSPACE/trunk/Code/Components/Services/icewrapper/current
scons test

cd $WORKSPACE/trunk/Code/Components/Services/common/current
scons test

#
# Phase 2
#
#
# Simulate dataset for streaming - this is the only function we use from
# synthesis
#

echo "------------- Phase 2 ------------"

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

echo "------------- Phase 3 ------------"

cd $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests
timeout -s 9 10m ./run.sh
ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "ingest/current/functests/run.sh returned errorcode $ERROR"
    exit 1
fi


#
# Phase 4
#
# Run the main functest which requires MPI and communication with the 
# correlator simulator
#

echo "------------- Phase 4 ------------"

cd $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests/test_ingestpipeline

rm -rf ingest_test0.ms

cat > tmp.simcor.sh <<EOF
#!/bin/sh
cd ../../../../correlatorsim/current/functests/test_playback
sleep 10
timeout -s 9 10m mpirun -np 3 ../../apps/playback.sh -c playback.in
EOF

chmod u+x tmp.simcor.sh

# asynchronous launch of the correlator simulator with a delay set
# in the script
./tmp.simcor.sh > simcor.out &

timeout -s 9 15m ./run.sh

ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "ingest/current/functests/test_ingestpipeline/run.sh returned errorcode $ERROR"
    echo "Failing the test on this condition has been disabled"
    #exit 1
fi

if [ ! -d $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests/test_ingestpipeline/ingest_test0.ms ]; then
    echo "Error: ingest_test0.ms was not created"
    exit 1
fi

SZ=$(set -- `du -sh ingest_test0.ms` ; echo $1)

if [ "$SZ" != "87M" ]; then
   echo "The size of the output MS ("$SZ") seems to be different from 87M"
   exit 1
fi


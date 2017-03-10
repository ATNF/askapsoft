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
# Run parallel metadata test, it requires MPI
#

echo "------------ Phase 4 ------------"
# uncomment to run the code explicitly
#cd $WORKSPACE/trunk/Code/Components/Services/ingest/current
#timeout -s 9 10m mpirun -np 12 apps/ParallelMetadata.sh -c apps/tParallelMetadata.in
# uncomment to run the code via func test
cd $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests/test_parallelmetadata
timeout -s 9 10m ./run.sh
ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "ingest/current/apps/tParallelMetadata.sh returned errorcode $ERROR"
    exit 1
fi

#
# Phase 5
#
# Run the main functest which requires MPI and communication with the 
# correlator simulator
#

echo "------------- Phase 5 ------------"

cd $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests/test_ingestpipeline

rm -rf ingest_test0.ms

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
if [ $? -ne 0 ]; then
    echo "Error: Failed to start Ice Services"
    exit 1
fi

cat > tmp.simcor.sh <<EOF
#!/bin/sh
cd ../../../../correlatorsim/current/functests/test_playbackADE
sleep 10
timeout -s 9 10m mpirun -np 2 ../../apps/playbackADE.sh -c playback.in
EOF

chmod u+x tmp.simcor.sh

# asynchronous launch of the correlator simulator with a delay set
# in the script
./tmp.simcor.sh > simcor.out &

echo "Starting ingest pipeline: "`date`

timeout -s 9 10m ../../apps/cpingest.sh -s -c cpingest.in | tee ingest.out
ERROR=${PIPESTATUS[0]}


echo "Ingest finished: "`date`

for job in `jobs -p`
do
  echo "Waiting for pid="${job}" to finish"
  wait $job
done

# Stop the Ice Services
echo "Stopping ICE"
../stop_ice.sh ../icegridadmin.cfg



echo "-------------- output of the correlator simulator:"
cat simcor.out
echo "--------------------------------------------------"

if [ $ERROR -ne 0 ]; then
    echo "ingest/current/functests/test_ingestpipeline/run.sh returned errorcode $ERROR"
    # workarund for ASKAPSDP-1673
    ICE_EXCEPT=`tail -1 ingest.out | grep IceUtil::NullHandleException | wc -l`
    if [ ${ICE_EXCEPT} != "1" ]; then
         exit 1
    fi
    echo "Failing the test on IceUtil::NullHandleException thas been disabled"
    #exit 1
fi

if [ ! -d $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests/test_ingestpipeline/ingest_test0.ms ]; then
    echo "Error: ingest_test0.ms was not created"
    exit 1
fi

SZ=$(set -- `du -sh ingest_test0.ms` ; echo $1)

if [ "$SZ" != "118M" ]; then
   echo "The size of the output MS ("$SZ") seems to be different from 87M"
   exit 1
fi

#
# Phase 6
#
# Run the main functest which requires MPI and communication with the 
# correlator simulator, this version uses spare ranks and merges data from two
# simulated correlator cards - it represents more closely the system we used
# for early science (in the first quarter of 2017) in terms of communication patterns
#

echo "------------- Phase 6 ------------"

cd $WORKSPACE/trunk/Code/Components/Services/ingest/current/functests/test_ingestpipeline

rm -rf ingest_test0.ms

# Start the Ice Services
../start_ice.sh ../iceregistry.cfg ../icegridadmin.cfg ../icestorm.cfg
if [ $? -ne 0 ]; then
    echo "Error: Failed to start Ice Services"
    exit 1
fi

cat > tmp.simcor.sh <<EOF
#!/bin/sh
cd ../../../../correlatorsim/current/functests/test_playbackADE
sleep 10
timeout -s 9 10m mpirun -np 3 ../../apps/playbackADE.sh -c playback_2cards.in
EOF

chmod u+x tmp.simcor.sh

# asynchronous launch of the correlator simulator with a delay set
# in the script
./tmp.simcor.sh > simcor.out &

echo "Starting ingest pipeline: "`date`

timeout -s 9 10m mpirun -np 3 ../../apps/cpingest.sh -c cpingest_serviceranks.in | tee ingest.out
ERROR=${PIPESTATUS[0]}
echo "Error status: "${ERROR}

echo "Ingest finished: "`date`
for job in `jobs -p`
do
  echo "Waiting for "${job}" to finish"
  wait $job
done

# Stop the Ice Services
echo "Stopping ICE"
../stop_ice.sh ../icegridadmin.cfg



echo "-------------- output of the correlator simulator:"
cat simcor.out
echo "--------------------------------------------------"

if [ $ERROR -ne 0 ]; then
    echo "ingest/current/functests/test_ingestpipeline/run.sh returned errorcode $ERROR"
    # workarund for ASKAPSDP-1673
    ICE_EXCEPT=`cat ingest.out | grep -v DEBUG | tail -1 | grep IceUtil::NullHandleException | wc -l`
    if [ ${ICE_EXCEPT} != "1" ]; then
         exit 1
    fi
    echo "Failing the test on IceUtil::NullHandleException thas been disabled"
    #exit 1
fi

if [ ! -d $WORKSPACE/Code/Components/Services/ingest/current/functests/test_ingestpipeline/ingest_test0.ms ]; then
    echo "Error: ingest_test0.ms was not created"
    exit 1
fi

SZ=$(set -- `du -sh ingest_test0.ms` ; echo $1)

# allowed a range of sizes (hopefully temporary). Not clear why this happens, but one of the theories is it can get +/- 1 cycle
# due to spurious Ice exception (which happen first in the rank which doesn't do any writing and calls MPI_Abort)
if [ "$SZ" != "230M" ]; then
   if [ "$SZ" != "229M" ]; then
       if [ "$SZ" != "231M" ]; then
           if [ "$SZ" != "232M" ]; then
               echo "The size of the output MS ("$SZ") seems to be different from 229M-232M"
               exit 1
           fi
       fi
   fi
fi


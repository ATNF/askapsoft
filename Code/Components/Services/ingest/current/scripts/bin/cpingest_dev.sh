#!/bin/bash

#cd `dirname $0`

# Set a 16GB vmem limit
ulimit -v 16777216

# Setup the environment
export AIPSPATH=/group/askap/askapdata/current
# for now running rom my directory, eventually (when we finish experiments), it will be properly deployed
export ASKAP_ROOT=/group/askap/vor010/ASKAPsdp_ingest
source ${ASKAP_ROOT}/Code/Components/Services/ingest/current/init_package_env.sh
export INGEST_CONFIG=/group/askap/vor010/ingest_config

# loading some stuff necessary to run our stuff - askapops environment doesn't seem to have it
# no need to load thing to compile as this is not the development environment

module use /group/askaprt/software/sles12sp2/modulefiles
module load sandybridge
module load python/2.7.13
module load gcc/4.8.5
module load mvapich/2.3b
module load askapdata

# modules to get mpirun_rsh (based on discussion with Mohsin and ASKAPSDP-2791 work)
#module use /group/pawsey0233/software/sles12sp2/modulefiles 2> err.dbg
#module load sandybridge gcc/4.8.5 mvapich/2.3b 2>> err.dbg


#

function finish {
   # clean up actions
   export TMP_PID=`jobs -p`
   kill ${TMP_PID}
   wait ${TMP_PID}
   # the following line has been commented out while we ingesting itno /group instead of the final location
   #           in this case the DONE flag is set by the copy script
   touch DONE
   #echo "Raising the DONE flag has been temporary disabled - while we capture into /group"
   echo "script termination" >> err.dbg
   date >> err.dbg
}
trap finish EXIT

# it's a bit of the hack, but parameter set is always called cpingest.in
if [ ! -f cpingest.in ]; then
   echo "Error: Unable to find cpingest.in"
   exit -1
fi

export HOSTS_MAP=`cat cpingest.in | grep -v \# | grep hosts_map | awk -F "=" '{print $2;}'| sed -e 's\\ \\\\g'`
if [ ""${HOSTS_MAP} == "" ]; then
    echo "Unable to find cp.ingest.hosts_map FCM parameter"
    exit -1
fi

export WORK_DIR=`pwd`
export HOSTFILE=${WORK_DIR}/config_hostfile.txt

# srun-style hosts file
#echo ${HOSTS_MAP} | sed "s/,/\n/g" | sed "s/ //g" | awk -F ":" '{for (i=0;i<$2;++i) printf("galaxy-ingest%02d\n",$1);}' > ${HOSTFILE}
#export NUM_TASKS=`cat ${HOSTFILE} | wc -l`
#export SLURM_HOSTFILE=${HOSTFILE}

# version for mpirun
echo ${HOSTS_MAP} | sed "s/,/\n/g" | sed "s/ //g" | awk -F ":" '{printf("galaxy-ingest%02d:%i\n",$1,$2);}' > ${HOSTFILE}
export NUM_TASKS=`cat ${HOSTFILE} | awk -F ":" 'BEGIN{s=0;}{s=s+$2;}END{print s;}'`

#export NUM_TASKS=`cat ${INGEST_CONFIG}/num_cards`

echo "Starting ingest with "${NUM_TASKS}" ranks"

echo "Configuration: " > cpingest.log
echo "hosts map: "${HOSTS_MAP}", number of tasks: "${NUM_TASKS} >> cpingest.log
cat cpingest.in | grep tasklist >> cpingest.log
echo " -------------------------- " >> cpingest.log


# Run the ingest pipeline
# it is executed in the SB directory, use stdout instead of logging into file as the latter has problems with
# this highly parallel application and collating log via mpi works much better

#mpirun -verbose -np ${NUM_CARDS} -f ${INGEST_CONFIG}/cpingest_hosts.in -bind-to socket -map-by hwthread ${ASKAP_ROOT}/Code/Components/Services/ingest/current/apps/cpingest.sh "$@" >>cpingest.log 2> err.dbg

export MV2_ENABLE_AFFINITY=0

#srun --export=all -n ${NUM_TASKS} -I --kill-on-bad-exit=1 --exclusive --account=askaprt --partition=ingest --distribution=arbitrary --time=12:00:00 --job-name=ingest --mem=60000M --mem_bind=verbose,local ${ASKAP_ROOT}/Code/Components/Services/ingest/current/apps/cpingest.sh "$@" >>cpingest.log 2> err.dbg

mpirun_rsh -export-all -n ${NUM_TASKS} -hostfile ${HOSTFILE} ${ASKAP_ROOT}/Code/Components/Services/ingest/current/apps/cpingest.sh "$@" >>cpingest.log 2>> err.dbg


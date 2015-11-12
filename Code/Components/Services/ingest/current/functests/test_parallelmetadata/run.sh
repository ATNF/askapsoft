#!/bin/bash

cd `dirname $0`

# Setup the environment
source ../../init_package_env.sh
export AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current

# Run the ingest pipeline
mpirun -np 12 ../../apps/tParallelMetadata.sh -c ./tParallelMetadata.in
ERROR=$?
if [ $ERROR -ne 0 ]; then
    echo "tParallelMetadata.sh returned errorcode $ERROR"
    exit 1
fi


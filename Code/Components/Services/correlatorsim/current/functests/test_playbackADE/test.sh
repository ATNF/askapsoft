#!/bin/bash

# Make the data for correlator
#sbatch /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/dataset/sim.sh

# Receive the data
#sbatch /data/lah00b/ASKAPsoft/Code/Components/Services/ingest/current/functests/test_ingestpipeline/run.sh

# Receive the data and snoop the content
sbatch /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/apps/vsnoopADE.sh -v

# Send the data
export AIPSPATH=/data/lah00b/ASKAPsoft/Code/Base/accessors/current
salloc -N 2 mpirun /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/apps/playbackADE.sh -c /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/functests/test_playbackADE/playback.in


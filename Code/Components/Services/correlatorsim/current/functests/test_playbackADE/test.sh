#!/bin/bash

# Make the data for correlator
#sbatch /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/dataset/sim.sh

# Receive the data
#sbatch /data/lah00b/ASKAPsoft/Code/Components/Services/ingest/current/functests/test_ingestpipeline/run.sh

# Receive the data and snoop the content
sbatch /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/apps/vsnoop.sh -v

# Send the data
export AIPSPATH=/data/lah00b/ASKAPsoft/Code/Base/accessors/current
salloc -N 3 mpirun /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/apps/playback.sh -c /data/lah00b/ASKAPsoft/Code/Components/Services/correlatorsim/current/functests/test_playback/playback.in


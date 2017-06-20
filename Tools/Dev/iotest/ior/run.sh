#!/bin/bash -l
#
# This script is mainly for loading the correct python modules

if [[ $HOSTNAME -ne galaxy-ingest07 ]]; then
    module unload PrgEnv-cray
    module unload gcc

    module load argparse
    module load numpy
    module load matplotlib
fi

python run.py


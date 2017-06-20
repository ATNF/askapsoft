#!/bin/bash -l
#
# This script is mainly for loading the correct python modules

module unload PrgEnv-cray
module unload gcc

module load PrgEnv-gnu
module load argparse
module load numpy
module load matplotlib

python plot.py
#python plot.py >> plot.txt


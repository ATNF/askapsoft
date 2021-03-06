#!/usr/bin/bash

#####
## Makes the working directory, moves to that directory, then makes
## all required subdirectories: parsets, logs, MS, chunks & slices

workdir="run_${now}"

echo "Making working directory $workdir"
mkdir -p ${workdir}
cd ${workdir}
lfs setstripe -c 8 .

mkdir -p ${parsetdir}
mkdir -p ${logdir}
mkdir -p ${slurms}
mkdir -p ${slurmOutput}
mkdir -p ${msdir}
mkdir -p ${chunkdir}
mkdir -p ${slicedir}

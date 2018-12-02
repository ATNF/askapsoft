#!/bin/bash

CPINGEST_HOME=/askap/cp
CPINGEST_NUMCARDS=12

export LD_LIBRARY_PATH=${CPINGEST_HOME}/cpsvcs/default/lib
export AIPSPATH=${CPINGEST_HOME}/measures_data
mpirun -np ${CPINGEST_NUMCARDS} ${CPINGEST_HOME}/cpsvcs/default/bin/cpingest "$@"

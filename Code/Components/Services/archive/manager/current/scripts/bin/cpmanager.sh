#!/usr/bin/env bash

CPSVCS_HOME=/askap/cp/cpsvcs
CPMANAGER_HOME=${CPSVCS_HOME}/default
CPSVCS_LIB=${CPSVCS_HOME}/default/lib
export LD_LIBRARY_PATH=${CPSVCS_LIB}
export JARDIR=${CPSVCS_LIB}


exec java -Xms256m -Xmx1024m -cp "${JARDIR}/*" askap.cp.manager.CpManager -c ${CPMANAGER_HOME}/config/cpmanager.in -l ${CPMANAGER_HOME}/config/cpmanager.log_cfg

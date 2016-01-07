#!/usr/bin/env bash

CPSVCS_HOME=/askap/cp/cpsvcs
CPSVCS_LIB=${CPSVCS_HOME}/default/lib
export LD_LIBRARY_PATH=${CPSVCS_LIB}
export JARDIR=${CPSVCS_LIB}

#exec java -cp ${CPSVCS_LIB}/cpmanager.jar:${CPSVCS_LIB}/java-askap.jar:${CPSVCS_LIB}/atoms.jar:${CPSVCS_LIB}/java-logappenders.jar:${CPSVCS_LIB}/askap-interfaces.jar:${CPSVCS_LIB}/log4j-1.2.15.jar:${CPSVCS_LIB}/Ice-3.5.0.jar:${CPSVCS_LIB}/Freeze.jar:${CPSVCS_LIB}/IceStorm.jar askap/cp/manager/CpManager -c config/cpmanager.in -l config/cpmanager.log_cfg

exec java -Xms256m -Xmx1024m -cp "${JARDIR}/*" askap.cp.manager.CpManager -c config/cpmanager.in -l config/cpmanager.log_cfg

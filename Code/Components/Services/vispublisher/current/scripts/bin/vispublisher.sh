#!/usr/bin/env bash

VISPUBLISHER_HOME=/askap/cp

export LD_LIBRARY_PATH=${VISPUBLISHER_HOME}/cpsvcs/default/lib
export AIPSPATH=${VISPUBLISHER_HOME}/measures_data
exec ${VISPUBLISHER_HOME}/cpsvcs/default/bin/vispublisher -c ${VISPUBLISHER_HOME}/config/vispublisher.in -l ${VISPUBLISHER_HOME}/config/vispublisher.log_cfg

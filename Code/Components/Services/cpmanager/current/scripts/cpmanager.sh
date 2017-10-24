#!/bin/bash

# export ICE_CONFIG="../files/config/ingestmanager.ice_cfg,../files/config/icehost.ice_cfg"
export ICE_CONFIG="../files/config/ingestmanager.ice_cfg"
export PYTHON_LOG_FILE=./cpmanager.log

./ingestservice.py --log-config=../files/config/ingestmanager.pylog_cfg

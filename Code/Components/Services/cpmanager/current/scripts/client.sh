#!/bin/bash

# usage ./client.sh <sbid>
export ICE_CONFIG="../files/config/ingestmanager.ice_cfg"
../functests/Client.py $1


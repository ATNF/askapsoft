#!/bin/bash -l

# Poll the given CASDA upload dir for a DONE file which indicates that the data has been successfully
# uploaded to the archive. Change the SB status to COMPLETE when this happens.
#
# In order to not check too early as the upload does take some time an initial wait time can be specified,
# but if not specified it will default to a reasonable time. After that time has elapsed a check will be
# done every 5 minutes.
#
# usage: poll-casda-done.sh -d <CASDA_UPLOAD_DIR> -s <SBID> [-w <INITIAL_WAIT_TIME>]
#
# Parameters:
# CASDA_UPLOAD_DIR       Required   The directory to check for the presence of a DONE file
# SBID                   Required   The scheduling block ID to update the status for.
# INITIAL_WAIT_TIME      Optional   The amount of time to wait before checking initially, default 1800 (half hour)
#
# This script traps the SIGHUP (hangup) signal and the use of the timeout command could be used to effect a timeout mechanism.
# The trap handler will create a TIMED_OUT file and leave the scheduling block status in the PENDINGARCHIVE state.
# e.g. give up after a 4 hour wait (s seconds and m minutes works too)
#
# timeout --signal=SIGHUP 4h poll-casda-done.sh -d /scratch2/casda/prd/1234 -s 1234 -w 3600
#
# @copyright (c) 2017 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# @author Eric Bastholm <Eric.Bastholm@csiro.au>
#

CASDA_UPLOAD_DIR=""
SBID=""
DEFAULT_INITIAL_WAIT_TIME=1800
INITIAL_WAIT_TIME=0
WAIT_TIME=300

me=$(basename "$0")
USAGE="$me -d <CASDA_UPLOAD_DIR> -s <SBID> [-w <INITIAL_WAIT_TIME>]"

showHelp() {
  printf "Checks for a DONE file in the casda upload directory which indicates a completed and successful upload.\n"
  printf "Then updates the Scheduling Block status to COMPLETE.\n"
  printf "Requires a casda upload directory location and sheduling block ID,\nand optional initial wait time to expire before bothering to check for the DONE file.\n\n"
  printf "usage: $USAGE \n"
}

updateSBStatus() {
  module load askapcli
  schedblock transition -s COMPLETE ${SBID}
  #printf "SB Status updated\n"
  return $?
}

# Trap the hangup signal. This may be sent if we have been running too long. If we are sleeping when this comes in it will be triggered after we wake the next time.
trap "{ touch TIMED_OUT ; exit 255; }" SIGHUP

# Process our options
OPTIND=1
while getopts "h?d:s:w:" opt; do
    case "$opt" in
    h|\?)
        showHelp
        exit 0
        ;;
    d)
        CASDA_UPLOAD_DIR=$OPTARG
        ;;
    s)
        SBID=$OPTARG
        ;;
    w)
        INITIAL_WAIT_TIME=$OPTARG
        ;;
    esac
done         
      
shift $((OPTIND-1))

[ "$1" = "--" ] && shift

# Check for no upload dir
if [ -z "$CASDA_UPLOAD_DIR" ]; then
    printf "Missing CASDA upload directory\n"
    printf "$USAGE\n"
    exit 1
fi

# Check for no SBID
if [ -z "$SBID" ]; then
    printf "Missing scheduling block ID\n"
    printf "$USAGE\n"
    exit 1
fi

# If no initial wait time then default it
if [ "$INITIAL_WAIT_TIME" -eq 0 ]; then
    let INITIAL_WAIT_TIME=DEFAULT_INITIAL_WAIT_TIME
fi

#printf "Waiting %i seconds before begin checking ...\n" $INITIAL_WAIT_TIME
sleep $INITIAL_WAIT_TIME
while true; do
  if [ -e $CASDA_UPLOAD_DIR/DONE ]; then
    updateSBStatus
    exit 0
  else
    #printf "Waiting %i seconds before checking again ...\n" $WAIT_TIME
    sleep $WAIT_TIME
  fi
done

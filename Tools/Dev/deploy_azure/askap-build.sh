#!/bin/bash
# This script will run a long-running-job (if it's not already running)
# and email when it completes.

echo " started running bootstrap.py" >> $logfile
cat "$logfile" | mailx -s "Job starting" stephen.ord@csiro.au

cd $HOME/jacal-dev

python2.7 bootstrap.py -n

echo "done bootstrap" >> $logfile

cat "$logfile" | mailx -s "Bootstrap done .. running rbuild" stephen.ord@csiro.au
echo "running rbuild" >> $logfile

. initaskap.sh

cd Code

rbuild -a

echo "done rbuild" >> $logfile

cat "$logfile" | mailx -s "Job finished" stephen.ord@csiro.au



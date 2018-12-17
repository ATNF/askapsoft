#!/bin/bash
# This script will run a long-running-job (if it's not already running)
# and email when it completes.
lockfile=/var/run/long-job-1.lock
logfile=$(mktemp)
errfile=$(mktemp)
if [[ -f "$lockfile" ]]; then
    echo "This job is already running." 1>&2
    exit 1
else
    echo $$ > "$lockfile"
    trap 'rm -f "$lockfile" "$logfile" "$errfile"' EXIT
fi

echo "running bootstrap.py" >> $logfile
cat "$logfile" | mailx -s "Job succeeded" stephen.ord@csiro.au

cd $HOME/jacal-dev

python2.7 bootstrap.py -n

echo "done bootstrap" >> $logfile

echo "running rbuild" >> $logfile
cat "$logfile" | mailx -s "Job succeeded" stephen.ord@csiro.au

. initaskap.sh

cd Code

rbuild -a

echo "done rbuild" >> $logfile

cat "$logfile" | mailx -s "Job succeeded" stephen.ord@csiro.au



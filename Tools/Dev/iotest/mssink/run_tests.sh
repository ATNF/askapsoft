#!/bin/bash -l
#
# This script runs a test suite via invocations of run.sh
# incorporates some dependency in the SLURM submission to avoid
# simultaneous disk access
#
# Copyright: CSIRO 2017
# Author: Stephen Ord <stephen.ord@csiro.au>
#
# TODO: Think of a way to automate this as part of continuous integration
##############################################################################


submit=1
buildsbatch=1

function show_usage() {
    echo "Usage: $0 [-d DIRECTORY] [-h] [-T] [-B] [-A ASKAP_ROOT]"
    echo "A convenient way to run a suite of tests"
    echo "Required arguments:"
    echo "-A ASKAP_ROOT"
    echo "-d  test directory (there is no default)"
    echo "-h  Help (this usage)"
    echo "Option arguments:"
    echo "-T  Testmode - do not submit the jobs"
    echo "-B  no batch files - do not build the batch files"
    echo "-A ASKAP_ROOT (defaults to environment)"
    exit
}

while getopts A:Bd:hS option; do
    case "${option}" in
        A) ASKAP_ROOT=${OPTARG};;
        B) buildsbatch=0;;
        d) workdir=${OPTARG};;
        h) show_usage;;
        S) submit=0;;
        *) show_usage;;
    esac
done


if [ -z "${ASKAP_ROOT}" ]; then
  echo "ASKAP_ROOT is undefined please set via environment or command line"
  exit
fi

if [ -z "${workdir}" ]; then
  echo "workdir is undefined please set via environment or command line"
  exit
fi

# this is actually a crazy way of doing this so I should change it
#number of antennas

tester="${ASKAP_ROOT}/Tools/Dev/iotest/mssink/run.sh "

antennas=(12 24 36)
channels=(10368 17280)
ranks=(1 2 4 8 40 80 16)
ntasks_per_node=(1 4 8 10 2)
invocations=(1 36)
numberofnodes=8
minutes=1

echo "antenna list          : ${antennas}"
echo "channel list          : ${channels}"
echo "lists of ranks        : ${ranks}"
echo "workdir               : ${workdir}"
echo "tester                : ${tester}"


# these are the defined tests
# (invocations ranks antennas channels minutes)

# Early science split by frequency
ES1=($tester ${invocations[0]} ${ranks[0]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES1" ${ntasks_per_node[0]})
ES2A=($tester ${invocations[0]} ${ranks[1]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2A" ${ntasks_per_node[0]})
ES2B=($tester ${invocations[0]} ${ranks[2]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2B" ${ntasks_per_node[0]})
ES2C=($tester ${invocations[0]} ${ranks[3]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2C" ${ntasks_per_node[0]})

# Early science split by beam
ES3=($tester ${invocations[1]} ${ranks[0]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES3" )

# Early science ++ split by frequency
ES4=($tester ${invocations[0]} ${ranks[0]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES4" ${ntasks_per_node[0]})
ES5A=($tester ${invocations[0]} ${ranks[1]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5A" ${ntasks_per_node[0]})
ES5B=($tester ${invocations[0]} ${ranks[2]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5B" ${ntasks_per_node[0]})
ES5C=($tester ${invocations[0]} ${ranks[3]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5C" ${ntasks_per_node[0]})
let nnodes=${ranks[6]}/${ntasks_per_node[4]}
ES5D=($tester ${invocations[0]} ${nnodes} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5D" ${ntasks_per_node[4]})
# Early science ++ split by beam
ES6=($tester ${invocations[1]} ${ranks[0]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES6")

# Full ASKAP split by frequency
FA1=($tester ${invocations[0]} ${ranks[0]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA1" ${ntasks_per_node[0]})
FA2A=($tester ${invocations[0]} ${ranks[2]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2A" ${ntasks_per_node[0]})
FA2B=($tester ${invocations[0]} ${ranks[3]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2B" ${ntasks_per_node[0]})
# we are using more than 8 nodes for this so:
let nnodes=${ranks[4]}/${ntasks_per_node[2]}
FA2C=($tester ${invocations[0]} ${nnodes} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2C" ${ntasks_per_node[2]})
let nnodes=${ranks[5]}/${ntasks_per_node[3]}
FA2D=($tester ${invocations[0]} ${nnodes} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2D" ${ntasks_per_node[3]})


# Full ASKAP split by beam
FA3=($tester ${invocations[1]} ${ranks[0]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA3")

declare -a TESTARRAY

TESTARRAY=(
    "${ES1[*]}"
    "${ES2A[*]}"
    "${ES2B[*]}"
    "${ES2C[*]}"
#    "${ES3[*]}"
    "${ES4[*]}"
    "${ES5A[*]}"
    "${ES5B[*]}"
    "${ES5C[*]}"
    "${ES5D[*]}"
#    "${ES6[*]}"
    "${FA1[*]}"
    "${FA2A[*]}"
    "${FA2B[*]}"
    "${FA2C[*]}"
    "${FA2D[*]}"
#    "${FA3[*]}"
)

#Split by frequency
cnt=${#TESTARRAY[@]}

echo "Number of tests: $cnt"

#add some checks here and make this a command line argument

mkdir $workdir
cd $workdir

if [[ buildsbatch -ne 0 ]]; then


    for (( i = 0 ; i < cnt ; i++ ))
    do
        # invokes Paulus's tester in test mode
        test=${TESTARRAY[$i]}
        line=`echo $test | awk '{print ""$1" -n "$3" -a "$4" -C "$5" -t "$6" -d "$7" -T -w "$8" \n"}'`
        echo $line
        eval $line

    done

fi
# submit with dependencies so not everyone tries to hit the disk at once
if [[ submit -ne 0 ]]; then

    for (( i = 0 ; i < cnt ; i++ ))
    do
        test=${TESTARRAY[$i]}
        t=`echo $test | awk '{print ""$7""}'`
        if [[ ${i} -lt 1 ]]; then
            sbatch="tMSSink_${t}.sbatch"
            echo "submitting $sbatch"
            jid1=`sbatch ${sbatch} | cut -d " " -f 4`
            post_batch_file="post_${t}.sbatch"
            cmd="sbatch --dependency=afterany:${jid1} ${post_batch_file}"
            echo $cmd
            eval $cmd
        else
            sbatch="tMSSink_${t}.sbatch"
            echo "submitting $sbatch"
            jid1=`sbatch --dependency=afterany:${jid1} ${sbatch} | cut -d " " -f 4`
            post_batch_file="post_${t}.sbatch"
            cmd="sbatch --dependency=afterany:${jid1} ${post_batch_file}"
            echo $cmd
            eval $cmd

        fi
    done
fi

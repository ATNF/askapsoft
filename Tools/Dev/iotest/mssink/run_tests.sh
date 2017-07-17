#!/bin/bash -l
#workdirectory - should make this an argument
module load askapsoft

workdir=/astro/askap/sord

#number of antennas

antennas=(12 24 36)
channels=(10368 17280)
ranks=(1 2 4 8 40)
ntasks_per_node=(1 4 8)
invocations=(1 36)
numberofnodes=8
minutes=1
tester="${ASKAP_ROOT}/Tools/Dev/iotest/mssink/run.sh "

# these are the tests
# (invocations ranks antennas channels minutes)

# Early science split by frequency
ES1=($tester ${invocations[0]} ${ranks[0]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES1" ${ntasks_per_node[0]})
ES2A=($tester ${invocations[0]} ${ranks[1]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2A" ${ntasks_per_node[0]})
ES2B=($tester ${invocations[0]} ${ranks[2]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2B" ${ntasks_per_node[0]}) 
ES2C=($tester ${invocations[0]} ${ranks[3]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2C" ${ntasks_per_node[0]}) 

# Early science split by beam
ES3=($tester ${invocations[1]} ${ranks[0]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES3" )

# Early science ++ split by frequency
ES4=($tester ${invocations[0]} ${ranks[0]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES4") ${ntasks_per_node[0]}
ES5A=($tester ${invocations[0]} ${ranks[1]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5A" ${ntasks_per_node[0]})
ES5B=($tester ${invocations[0]} ${ranks[2]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5B" ${ntasks_per_node[0]}) 
ES5C=($tester ${invocations[0]} ${ranks[3]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5C" ${ntasks_per_node[0]})

# Early science ++ split by beam
ES6=($tester ${invocations[1]} ${ranks[0]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES6")

# Full ASKAP split by frequency
FA1=($tester ${invocations[0]} ${ranks[0]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA1" ${ntasks_per_node[0]})
FA2A=($tester ${invocations[0]} ${ranks[2]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2A" ${ntasks_per_node[0]})
FA2B=($tester ${invocations[0]} ${ranks[3]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2B" ${ntasks_per_node[0]})
FA2C=($tester ${invocations[0]} ${ranks[4]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2C" ${ntasks_per_node[2]})

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
#    "${ES6[*]}"
    "${FA1[*]}"
    "${FA2A[*]}"
    "${FA2B[*]}"
    "${FA2C[*]}"
#    "${FA3[*]}"
)
    
#Split by frequency
cnt=${#TESTARRAY[@]}

echo "Number of tests: $cnt"
cd $workdir

for (( i = 0 ; i < cnt ; i++ ))
do
    test=${TESTARRAY[$i]}   
    line=`echo $test | awk '{print ""$1" -n "$3" -a "$4" -C "$5" -t "$6" -d "$7" -T 1 -w "$8" \n"}'`
    echo $line
    eval $line
    
done
# submit with dependencies so not everyone tries to hit the disk at once
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


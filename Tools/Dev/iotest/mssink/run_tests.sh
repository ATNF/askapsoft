#!/bin/bash -l

#number of antennas

antennas=(12 24 36)
channels=(10368 16200)
ranks=(1 2 4 8 40)
invocations=(1 36)
numberofnodes=8
minutes=1
tester="run_direct.sh "
# these are the tests
# (invocations ranks antennas channels minutes)

# Early science split by frequency
ES1=($tester ${invocations[0]} ${ranks[0]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES1" )
ES2A=($tester ${invocations[0]} ${ranks[1]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2A" )
ES2B=($tester ${invocations[0]} ${ranks[2]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2B") 
ES2C=($tester ${invocations[0]} ${ranks[3]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES2C") 

# Early science split by beam
ES3=($tester ${invocations[1]} ${ranks[0]} ${antennas[0]} ${channels[0]} ${minutes[0]} "ES3")

# Early science ++ split by frequency
ES4=($tester ${invocations[0]} ${ranks[0]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES4")
ES5A=($tester ${invocations[0]} ${ranks[1]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5A")
ES5B=($tester ${invocations[0]} ${ranks[2]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5B") 
ES5C=($tester ${invocations[0]} ${ranks[3]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES5C")

# Early science ++ split by beam
ES6=($tester ${invocations[1]} ${ranks[0]} ${antennas[1]} ${channels[0]} ${minutes[0]} "ES6")

# Full ASKAP split by frequency
FA1=($tester ${invocations[0]} ${ranks[0]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA1")
FA2A=($tester ${invocations[0]} ${ranks[2]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2A")
FA2B=($tester ${invocations[0]} ${ranks[3]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2B")
FA2C=($tester ${invocations[0]} ${ranks[4]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA2C")

# Full ASKAP split by beam
FA3=($tester ${invocations[1]} ${ranks[0]} ${antennas[2]} ${channels[1]} ${minutes[0]} "FA3")

declare -a TESTARRAY

TESTARRAY=(
    "${ES1[*]}"
    "${ES2A[*]}"
    "${ES2B[*]}"
    "${ES2C[*]}"
    "${ES3[*]}"
    "${ES4[*]}"
    "${ES5A[*]}"
    "${ES5B[*]}"
    "${ES5C[*]}"
    "${ES6[*]}"
    "${FA1[*]}"
    "${FA2A[*]}"
    "${FA2B[*]}"
    "${FA2C[*]}"
    "${FA3[*]}"
)
    
#Split by frequency
cnt=${#TESTARRAY[@]}

echo "Number of tests: $cnt"

for (( i = 0 ; i < cnt ; i++ ))
do
    test=${TESTARRAY[$i]}   
    line=`echo $test | awk '{print ""$1" -n "$3" -a "$4" -C "$5" -t "$6" -d "$7" \n"}'`
    echo $line

done

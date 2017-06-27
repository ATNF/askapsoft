#IOR Test Automation

This folder contains scripts to automate a series of IOR tests.

Author: Paulus Lahur (paulus.lahur@csiro.au)

Note: The content of this file is in markdown format.

##What it does

A standard IOR test is executed using inline options or parameter file.
The script "run.py" is executed through "run.sh".
'''
> run.sh
'''
It does the following tasks:
1. Read parameter file "IOR_test_input.txt" as input
2. Expands designated input parameters into individual tests:
'''
transferSize=[1m, 2m, 3m]
'''
3. Creates folder called "script" containing parameter files according to the parameter being expanded:
'''
File "IOR_test_transferSize_1m.txt" containing this parameter:
    transferSize=1m
File "IOR_test_transferSize_2m.txt" containing this parameter:
    transferSize=2m
File "IOR_test_transferSize_3m.txt" containing this parameter:
    transferSize=3m
'''
4. Creates folder called "batch" containing appropriate batch files:
'''
IOR_test_transferSize_1m.sbatch
IOR_test_transferSize_2m.sbatch
IOR_test_transferSize_3m.sbatch
'''
5. Creates folder called "log" that will catch all results in log files:
'''
IOR_test_transferSize_1m.log
IOR_test_transferSize_2m.log
IOR_test_transferSize_3m.log
'''
6. Submit the job to SLURM

The script "post.py" is executed through "post.sh".
'''
> post.sh
'''
It does the following tasks:
1. Gather data from log files in folder "log".
2. Summarize the result in comma-separated format in file "output.csv".


##Files in this folder

IOR_test_input.txt: The input file for the script "run.py"

IOR_user_guide.txt: The official user guide of IOR test

post.py: The script to post process **all** log files in directory "log"

post.sh: The bash script to run "post.py"

README.md: this file

run.py: The main script to automate IOR test series

run.sh: The bash script to run "run.py"


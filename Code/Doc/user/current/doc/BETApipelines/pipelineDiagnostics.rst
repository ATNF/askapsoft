Diagnostics and job management
==============================

Running the processBETA.sh script will also create two scripts that
are available for the user to run. These reside in the tools/
subdirectory, and are tagged with the date and time that
processBETA.sh was called. Symbolic links to them are put in the top
level directory. These scripts are:

* reportProgress – (links to tools/reportProgress-YYYY-MM-DD-HHMM.sh)
  This is a front- end to squeue, showing only those jobs started by
  the most recent call of processBETA.sh. If given a -v option, it
  will also first provide a list of the jobs along with a brief
  description of what that job is doing.
  
* killAll – (links to tools/killAll-YYYY-MM-DD-HHMM.sh) This is a
  front-end to scancel, providing a simple way of cancelling all jobs
  started by the most recent call of processBETA.sh.
  
If you have jobs from more than one call of processBETA.sh running at
once, you can run the individual script in the tools directory, rather
than the symbolic link (which will always point to the most recent
one).

The list of jobs and their descriptions (as used by reportProgress) is
written to a file jobList (which is a symbolic link, linking to
slurmOutput/jobList-YYYY-MM-DD-HHMM.sh). There are symbolic links
created in the top level directory (where the script is run) and in
the output directory.

Each of the individual askapsoft tasks produces a log file that
includes a report of the time taken and the memory used. The
processing scripts extract this information and place them in both
ascii table and csv formatted files in the stats/ directory. These
files are named with the job ID and a description of the task, along
with an indication of whether the job succeeded ('OK') or failed
('FAIL'). Upon completion of all the jobs, these files are combined
into single tables, placed in the top-level directory and labelled
with the date- time stamp of the processBETA.sh call. Here is an
example of the data provided::

  JobID                    Description   Result      Real      User    System    PeakVM   PeakRSS
  145687                flag1934Amp_B0       OK     21.56      7.74       9.1       258        42
  145687            flag1934Dynamic_B0       OK     68.84     52.97     12.42       275        59
  145687                  split1934_B0       OK     46.27       5.1     12.16       282        65
  145696                  findBandpass       OK     10584    601.39   2228.44       405        98
  145697             flagScienceAmp_B0       OK   1267.89    334.51    409.52       264        47
  145697         flagScienceDynamic_B0       OK   3621.49   2396.92    695.65       281        64
  145697               splitScience_B0       OK   1428.65    180.91    419.42       374       153
  145698                   calapply_B0       OK     10709   9357.44    512.95       281        88
  145699                  avScience_B0       OK   1131.21    628.03    157.99       298        81
  145700                contImaging_B0       OK   1162.27   1065.33     80.29     10843      6662

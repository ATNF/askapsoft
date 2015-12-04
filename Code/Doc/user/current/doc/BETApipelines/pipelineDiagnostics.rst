Diagnostics and job management
==============================

Running the processBETA.sh script will also create two scripts that
are available for the user to run. These reside in the tools/
subdirectory, and are tagged with the date and time that
processBETA.sh was called. Symbolic links to them are put in the top
level directory. These scripts are:

* *reportProgress* – (links to *tools/reportProgress-YYYY-MM-DD-HHMM.sh*)
  This is a front-end to squeue, showing only those jobs started by
  the most recent call of *processBETA.sh*. If given a ``-v`` option, it
  will also first provide a list of the jobs along with a brief
  description of what that job is doing.
  
* *killAll* – (links to tools/killAll-YYYY-MM-DD-HHMM.sh) This is a
  front-end to *scancel*, providing a simple way of cancelling all jobs
  started by the most recent call of *processBETA.sh*.
  
In each of these cases, the date-stamp (*YYYY-MM-DD-HHMM*) is the time
at which *processBETA.sh* was run, so you can tie the results and the
jobs down to a particular call of the pipeline.

If you have jobs from more than one call of *processBETA.sh* running
at once, you can run the individual script in the tools directory,
rather than the symbolic link (which will always point to the most
recent one).

The list of jobs and their descriptions (as used by reportProgress) is
written to a file jobList (which is a symbolic link, linking to
*slurmOutput/jobList-YYYY-MM-DD-HHMM.txt*). There are symbolic links
created in the top level directory (where the script is run) and in
the output directory.

Each of the individual askapsoft tasks produces a log file that
includes a report of the time taken and the memory used. The
processing scripts extract this information and place them in both
ascii table and csv formatted files in the stats/ directory. These
files are named with the job ID and a description of the task, along
with an indication of whether the job succeeded ('OK') or failed
('FAIL'). For distributed jobs, a distinction is made between the
master process (rank 0) and the worker processes, and for the workers
both the peak and average memory useage are reported. This is
important for tasks like **cimager** where there are clear differences
between the memory usage on the master & worker nodes.

Upon completion of all the jobs, these files are combined
into single tables, placed in the top-level directory and labelled
with the same time-stamp as above. Here is an example of the data
provided::

    JobID                              Description   Result      Real      User    System    PeakVM   PeakRSS
    917171                            split1934_B0       OK     41.57      5.16      9.54       314        64
    917172                          flag1934Amp_B0       OK     20.13      7.75      8.07       291        42
    917172                      flag1934Dynamic_B0       OK     68.51     50.21     11.14       308        59
    917376                     findBandpass_master       OK   27056.7    481.87    1353.8       434        95
    917376                 findBandpass_workerPeak       OK   27056.7    481.87    1353.8       402        53
    917376                  findBandpass_workerAve       OK   27056.7    481.87    1353.8     395.5      50.3
    917190                         splitScience_B0       OK    1119.3    186.33    291.66       409       154
    917191                       flagScienceAmp_B0       OK    729.76    327.66    294.16       297        48
    917191                   flagScienceDynamic_B0       OK   2799.16   2265.68    463.73       314        65
    919873              contImagingSC_L0_B0_master       OK    598.83    583.32      5.62      1853       954
    919873          contImagingSC_L0_B0_workerPeak       OK    598.83    583.32      5.62      1877       601
    919873           contImagingSC_L0_B0_workerAve       OK    598.83    583.32      5.62     846.1     444.2
    919873              contImagingSC_L1_B0_master       OK    587.22     563.9      5.48      1961       954
    919873          contImagingSC_L1_B0_workerPeak       OK    587.22     563.9      5.48      1769       601
    919873           contImagingSC_L1_B0_workerAve       OK    587.22     563.9      5.48     846.6     444.6
    919874                    calapply_spectral_B0       OK   9685.59    8667.7    411.48       299        73
    919875                     contsub_spectral_B0       OK   10775.2   9494.77    363.58       588       363
    919876               spectralImaging_B0_master       OK   10011.8   2429.51    5606.5      8367       355
    919876           spectralImaging_B0_workerPeak       OK   10011.8   2429.51    5606.5      1271       929
    919876            spectralImaging_B0_workerAve       OK   10011.8   2429.51    5606.5    1258.7     919.6
    

Central Processor Platform
==========================

Overview
--------

The ASKAP Central Processor platform consists of a Cray XC30 supercomputer and a 1.4PB Cray
Sonexion storage system mounted as a Lustre filesystem.

* 2 x Login Nodes:

  - 2 x 2.4 GHz Intel Xeon E5-2665 (Sandy Bridge) CPUs
  - 64 GB RAM
  - 4x QDR Infiniband connectivity to Lustre filesystem

* 472 x Cray XC30 Compute Nodes:

  - 2 x 3.0 GHz Intel Xeon E5-2690 v2 (Ivy Bridge) CPUs
  - 10 Cores per CPU (20 per node)
  - 64GB DDR3-1866Mhz RAM
  - Cray Aries (Dragonfly Topology) Interconnect

* 16 x Ingest Nodes

  - 2 x 2.0 GHz Intel Xeon E5-2650 (Sandy Bridge) CPUs
  - 64GB RAM
  - 10 GbE connectivity to MRO
  - 4x QDR Infiniband connectivity to compute nodes and Lustre filesystem

* 2 x Data Mover Nodes (for external connectivity)

  - 2 x 2.0 GHz Intel Xeon E5-2650 (Sandy Bridge) CPUs
  - 64GB RAM
  - 10 GbE connectivity to Pawsey border router
  - 4x QDR Infiniband connectivity to Lustre filesystem

* High-Performance Storage

  - 1.4PB Lustre Filesystem
  - Approximately 25GB/s I/O performance


Login
------
To login to the ASKAP Central Processor::

   ssh abc123@galaxy.pawsey.org.au

Where abc123 is the login name Pawsey gave you. In that case that your CSIRO login differs
from your Pawsey login, and to avoid having to specify the username each time, you can add
the following to your ~/.ssh/config file::

   Host *.pawsey.org.au
     User abc123

If you intend running the ASKAP pipelines
(see :doc:`../pipelines/DataLocationSelection`), it is best to set up
an SSH key for your Pawsey account. This will allow you to connect to
other Pawsey machines (including hpc-data, which is used within the
pipeline scripts) without having to provide a password. This can be
done by using *ssh-keygen* to create *id_dsa.pub* and *id_dsa* files
in your ~/.ssh directory.

Setting up your account
-------------------------
There are a number of ASKAP-specific environment modules available
(see :doc:`modules` for details). To access them, add the following to
your ~/.bashrc file.

.. code-block:: bash

    # Use the ASKAP environment modules collection
    module use /group/askap/modulefiles

    # Load the ASKAPsoft module
    module load askapsoft

    # Load the ASKAP pipeline scripts module
    module load askappipeline

    # Load the measures data
    module load askapdata

    # Load some general utility functions
    module load askaputils

    # Load the BBCP module for fast external data transfer
    module load bbcp

    # Allow MPICH to fallback to 4k pages if large pages cannot be allocated
    export MPICH_GNI_MALLOC_FALLBACK=enabled

The following was previously suggested, although the **pshell**
utility provided by askaputils is probably better. The ashell module
is kept for backwards-compatibility reasons only, but if you already
have scripts using it then including this in your .bashrc is advised.

.. code-block:: bash

    # Load the "ashell" module for access to the commissioning archive
    module load ashell



Local Filesystems
-----------------

You have two filesystems available to you:

* Your home directory
* The *group* filesystem, where you have a directory ``/group/askap/$USER``

There was formerly a third filesystem, *scratch2*, that was used for
processing. This was removed at the start of 2018 (although is
available for a month from the *hpc-data* nodes in a read-only
fashion, to facilitate further moving of data).

The group filesystem provides the working space for ASKAP users. It is
aimed primarily as a place where you can store data sets or data
products for the medium-term. It has quotas applied at the group
level. e.g. ``askap`` group has a quota of 500TB. 

There is a directory ``/group/askap/scratch`` in which you can create
your own ``$USER`` directory. This has been set up as the preferred
location for running processing jobs (the aim was to apply a purge to
this space, although for technical reasons this will not be done). It
is not essential to run processing here, but it should help you
organise your work.
  
Note that your home directory, while it can be read from the compute nodes, cannot be
written to from the compute nodes. It is mounted read-only on the compute nodes to prevent
users from being able to "clobber" the home directory server with thousands of concurrent I/Os
from compute nodes.

In addition to /group there is another filesystem, /astro, which is reserved for real-time 
use by the askap system. /astro has restricted access and is intended for ASKAP operations to write scheduling 
blocks and for processing pipelines to read them. All non-askap-system write operations are to be to /group.

Submitting a job:
-----------------

This section describes the job execution environment on the ASKAP Central Processor. The
system uses SLURM for Job scheduling, however the below examples use a Cray specific
customisation to declare the resources required. An example slurm file is::

    #!/bin/bash -l
    #SBATCH --time=01:00:00
    #SBATCH --ntasks=80
    #SBATCH --ntasks-per-node=20
    #SBATCH --job-name=myjobname
    #SBATCH --no-requeue
    #SBATCH --export=NONE

    srun --export=ALL ./myprogram

Galaxy now uses native slurm, so note the use of *srun* instead of the
previous *aprun*. The --export=ALL option exports all environment
variables to the launched application (necessary for some cases). 

Specifically, the following part of the above file requests 80 processing
elements (PE) to be created. A PE is just a process. The parameter *ntasks-per-node*
says to execute 20 PEs per node, so this job will require 4 nodes (80/20=4)::

    #SBATCH --ntasks=80
    #SBATCH --ntasks-per-node=20

Then to submit the job::

    sbatch myjob.slurm


Submitting jobs with dependencies
---------------------------------

It may often be the case that you will want to submit a job that
depends on another job for valid input (for instance, you want to
calibrate a measurement set that is being split from a larger
measurement set via mssplit).

The *sbatch* command allows the specification of dependencies, which
act as prior conditions for the job you are submitting to actually run
in the queue. The syntax is::

  sbatch -d afterok:1234 myjob.slurm

The *"-d"* flag indicates a dependency, and the *afterok:* option
indicates that the job being submitted (myjob.qsub) will only be run
after job with ID 1234 completes successfully. There are other options
available - see the man page for sbatch for details.

The ID of a job is available from running squeue. If you are running a
script that involves submitting a string of inter-dependent programs,
you may want to capture the ID string from sbatch's output. When you
run sbatch, you get something like this::

  > sbatch myjob.slurm
  Submitted batch job 1234

which you could parse using something like the following (this would
run in a bash script - adapt accordingly for your scripting language
of choice)::

  JOB_ID=`sbatch myjob.slurm | awk '{print $4}'`

And you would then use that environment variable in the dependency option::

  sbatch -d afterok:${JOB_ID} myjob.slurm


Other example resource specifications
-------------------------------------

The following example launches a job with a number of PEs that is not a multiple of
*ntasks-per-node*, in this case 22 PEs::

    #!/bin/bash -l
    #SBATCH --time=01:00:00
    #SBATCH --ntasks=22
    #SBATCH --ntasks-per-node=20
    #SBATCH --job-name=myjobname
    #SBATCH --no-requeue
    #SBATCH --export=NONE

    srun --ntasks=22 --ntasks-per-node=20 ./myprogram

Note that this explicitly specifies the total number of tasks and the
number per node that the application should use.

**OpenMP Programs:**

The following example launches a job with 20 OpenMP threads per process (although there is only
one process). The *cpus-per-task* option declares the number of threads to be allocated
per process.  The below example starts a single PE with 20 threads::

    #!/bin/bash -l
    #SBATCH --time=00:30:00
    #SBATCH --ntasks=1
    #SBATCH --cpus-per-task=20
    #SBATCH --job-name=myjobname
    #SBATCH --export=NONE

    # Instructs OpenMP to use 20 threads
    export OMP_NUM_THREADS=20

    srun --ntasks=1 --ntasks-per-node=20 ./my_openmp_program


Monitoring job status
---------------------

To see your incomplete jobs::

    squeue -u $USER

Sometimes it is useful to see the entire queue, particularly when your job is queued and you wish
to see how busy the system is. The following commands show running jobs::

    squeue 

And to display accounting information, that includes completed jobs, the following command
can be used::

    sacct

Cancelling a job
----------------

If you wish to cancel a job that is running, or still in the queue,
you use the *scancel*  command together with the job ID::

  scancel 1234

Any jobs that depend on this one (see above) should also get cancelled
at the same time.

Additional Information
----------------------

* `Galaxy User Guide (Pawsey User Portal) <https://support.pawsey.org.au/documentation/display/US/Galaxy+User+Guide>`_
* `Cray XC30 System Documentation <http://docs.cray.com/cgi-bin/craydoc.cgi?mode=SiteMap;f=xc_sitemap>`_
* `SLURM Homepage <http://computing.llnl.gov/linux/slurm>`_
* `Migrating from PBS to SLURM <https://support.pawsey.org.au/documentation/display/US/Migrating+from+PBS+Pro+to+SLURM>`_

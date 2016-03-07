How to run the BETA pipelines
=============================

Loading the pipeline module
---------------------------

The pipeline scripts are now accessed through a specific module on
galaxy - ``askappipeline``. This is separate to the main ``askapsoft``
module, to allow more flexibility in updating the pipeline scripts. To
use, simply run::

  module load askappipeline

Note that for the linear mosaicking, if you want to get the beam
centres based on the beam footprint specification (ie. using the ACES
tool *footprint.py*), you will need to load the ACES module (to get
the correct python packages) and have an up-to-date version of the
ACES subversion repository. If you have not loaded the ACES module, it
is likely the *footprint.py* task will fail, and mosaicking will be
disabled.

Once loaded, the module will set an environment variable
**PIPELINEDIR**, pointing to the directory containing the scripts. It
also defines **PIPELINE_VERSION** to be the version number of the
currently-used module.

Configuration file
------------------

To run the processing, you need to call *processBETA.sh*, providing it
with a user input file that defines all necessary parameters. If
you've loaded the pipelines module as detailed above, then this should
be able to be run directly, like so::

  processBETA.sh -c myInputs.sh

where the user input file myInputs.sh could look something like this::

  #!/usr/bin/env bash
  #
  # Example user input file for BETA processing.
  # Define variables here that will control the processing.
  # Do not put spaces either side of the equals signs!
  # control flags
  SUBMIT_JOBS=true
  DO_SELFCAL=false
  # scheduling blocks for calibrator & data
  SB_1934=507
  SB_SCIENCE=514
  # base names for MS and image data products
  MS_BASE_SCIENCE=B1740_10hr.ms
  IMAGE_BASE_CONT=i.b1740m517.cont
  # beam location information, for mosaicking
  BEAM_FOOTPRINT_NAME=diamond
  FREQ_BAND=1
  BEAM_FOOTPRINT_PA=0
  # other imaging parameters
  NUM_PIXELS_CONT=4096
  NUM_TAYLOR_TERMS=2
  CPUS_PER_CORE_CONT_IMAGING=15

This file should define enough environment variables for the scripts
to run successfully. Mandatory ones, if you are starting from scratch,
are the locations of either the SBs for the observations or the
specific MSs.

When run, this file will be archived in the *slurmOutputs* directory
(see below), marked with an appropriate timestamp so that you'll be
able to keep a record of exactly what you have run.

**Important Note: The input file is a bash script, so formatting
matters. Most importantly in this case, you can not have spaces either
side of the equals sign when defining a variable.**

User-defined Environment Variables
----------------------------------

The user is able to specify environment variables that directly relate
to the parset parameters for the individual ASKAPsoft tasks. The input
parameters are named differently, so that they are tied more obviously
to specific tasks, and to distinguish between the the same parset
parameter used for different jobs (eg. the preconditioning definition
could differ for the continuum & spectral-line imaging).

The input environment variables are all given with upper-case names,
with an underscore separating words (eg. ``VARIABLE_NAME``). This will
distinguish them from the parset parameters.

The following pages list the environment variables defined in
defaultConfig.sh – these can all be redefined in your input file to
set and tweak the processing. The default value of the parameter (if
it has one) is listed in the tables, and in many of the tables the
parset parameter than the environment variable maps to is given.

* :doc:`BETAcontrol`
* :doc:`DataLocationSelection`
* :doc:`BandpassCalibrator`
* :doc:`ScienceFieldPreparation`
* :doc:`ScienceFieldContinuumImaging`
* :doc:`ScienceFieldSelfCalibration`
* :doc:`ScienceFieldContinuumMosaicking`
* :doc:`ContinuumSourcefinding`
* :doc:`ScienceFieldSpectralLineImaging`



What is created and where does it go?
-------------------------------------

Any measurement sets, images and tables that are created are put in an
output directory specified in the input file (if not provided, they go in
the directory in which *processBETA.sh* is run). There will be a file
called *PROCESSED_ON* that holds the timestamp indicating when the
script was run (this timestamp is used in various filenames). Also
created are a number of subdirectories which hold various types of
files. These are:

* *slurmFiles/* – the files in here are the job files that are submitted
  to the queue via the sbatch command. When a job is run, it makes a
  copy of the file that is labelled with the job ID.
* *parsets/* – any parameter sets used by the askapsoft applications
  are written here. These contain the actual parameters that are used
  by the various programs. These are labeled by the job ID.
* *logs/* – the logs that are written by the askapsoft applications
  themselves are put here.
* *slurmOutputs/* – the stdout and stderr from the slurm job itself
  are written to these files. Such files are usually
  *slurm-XXXXXX.out* (XXXXXX being the job ID), but these scripts
  rename the files so that the filename shows what job relates to what
  file (as well as providing the ID).
* *stats/* – diagnostics for each job are written to this
  directory. These report the time taken and the memory usage for each
  job, values which are extracted from the logs. These are combined
  into a single file showing all individual jobs, that is placed in
  the output directory. Both .txt and .csv files are created. The
  output directory also has a symbolic link to the top-level stats
  directory. See :doc:`pipelineDiagnostics` for details.
* *tools/* – utility scripts to show progress and kill all jobs for a
  given run are placed here. See :doc:`pipelineDiagnostics` for
  details.
* *Checkfiles/* - files that indicate progress through stages of the
  pipeline are written here. The pipelien can see these and know to
  skip certain stages, if required by the user.

Measurement sets
----------------

To provide the input data to the scripts, you can provide either the
scheduling blocks (SBs) of the two observations, or provide specific
measurement sets (MSs) for each case.

The measurement sets that will be created should be named in the
configuration file. A wildcard %b should be used to represent the beam
number in the resulting MSs, since the individual beams will be split
into separate files.

Each step detailed below can be switched on or off, and those selected
will run fine (provided any pre- requisites such as measurement sets
or bandpass solutions etc are available). If you have already created
an averaged science MS, you can re-use that with the
``MS_SCIENCE_AVERAGE`` parameter (see :doc:`ScienceFieldPreparation`),
again with the %b wildcard to represent the beam number.

Workflow summary
----------------


Here is a summary of the workflow provided for by these scripts:

* Read in user-defined parameters from the provided configuration
  file, and define further parameters derived from them.
* If bandpass calibration is required and a 1934-638 observation is
  available, we split out the relevant beams with **mssplit**
  (:doc:`../calim/mssplit`) into individual measurement sets (MSs),
  one per beam. Only the scan in which the beam in question was
  pointing at 1934-638 is used - this assumes the beams were pointed
  at it in order (so that beam 0 was pointing at in in scan 0, etc)
* These are flagged using **cflag** (:doc:`../calim/cflag`) in two
  stages: first a dynamic flag is applied (integrating over individual
  spectra), then a straight amplitude cut is applied to remove any
  remaining spikes. The dynamic flagging step can also optionally
  include antenna or baseline flagging.
* The bandpass solution is then determined with **cbpcalibrator**
  (:doc:`../calim/cbpcalibrator`), using all individual MSs and stored
  in a single CASA table.
* The science field data is similarly split and flagged with
  **mssplit** and **cflag**, producing one measurement set per
  beam. You can select particular scans or fields here, but the
  default is to use everything.
* The bandpass solution is then applied to each beam MS with
  **ccalapply** (:doc:`../calim/ccalapply`).
* The science field data are then averaged with **mssplit** to form
  continuum data sets. (Still one per beam).
* Each beam is then imaged individually. This is done in one of two
  ways:
  
  * Basic imaging with **cimager** (:doc:`../calim/cimager`), without
    any self-calibration. A multi-scale, multi-frequency clean is
    used, with major & minor cycles.
  * With self-calibration. First we image the field with **cimager**
    as for the first option. **selavy** (:doc:`../analysis/selavy`) is
    then used to find bright components, which are then used with
    **ccalibrator** (:doc:`../calim/ccalibrator`) to calibrate the
    gains, and we then re-image with **cimager**, using the
    calibration solution. This process is repeated a number of times.
    
* Once the image has been made, the source-finder **selavy** can be run on
  it to produce a deeper catalogue of sources.
* Once all beams have been done, they are all mosaicked together using
  **linmos** (:doc:`../calim/linmos`). This applies a primary-beam
  correction — you need to provide the beam arrangement name and
  (optionally) the position angle (these are used by the
  footprint.py* tool in the ACES svn area) to get the locations of
  the individual beams. Use the logs to find what the beam
  arrangement for your observation was. After mosaicking, **selavy**
  can be run on the final image to create the final source
  catalogue.
* Additionally, spectral-line imaging (that is, imaging at
  full spectral resolution to create a cube) of individual beams can
  be done. There are several optional steps to further prepare the
  spectral-line dataset:

  * A nominated channel range can be copied to a new MS with
    **mssplit**.
  * The gains solution from the continuum self-calibration can be
    applied to the spectral-line MS using **ccalapply**.
  * The continuum can be subtracted from the spectral-line MS (using
    the clean model from the continuum imaging) using
    **ccontsubtract** (:doc:`../calim/ccontsubtract`).

* Once the spectral-line dataset is prepared, **simager**
  (:doc:`../calim/simager`) is used to do the spectral-line
  imaging. This creates a cube using a large number of processors,
  each independently imaging a single channel.

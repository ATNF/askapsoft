How to run the ASKAP pipelines
==============================

Loading the pipeline module
---------------------------

The pipeline scripts are now accessed through a specific module on
galaxy - ``askappipeline``. This is separate to the main ``askapsoft``
module, to allow more flexibility in updating the pipeline scripts. To
use, simply run::

  module load askappipeline

Some parts of the pipeline make use of other modules, which are loaded
at the appropriate time. The beam footprint information is obtained by
using the *schedblock* tool in the askapcli module, while beam
locations are set using the ACES tool *footprint.py*, which requires
the aces module. For the latter, you will need to have an up-to-date
version of the ACES subversion repository (which is not part of a
module itself).

Once loaded, the askappipeline module will set an environment variable
**PIPELINEDIR**, pointing to the directory containing the scripts. It
also defines **PIPELINE_VERSION** to be the version number of the
currently-used module.

Configuration file
------------------

To run the processing, you need to call *processASKAP.sh*, providing it
with a user input file that defines all necessary parameters. If
you've loaded the pipelines module as detailed above, then this should
be able to be run directly, like so::

  processASKAP.sh -c myInputs.sh

where the user input file myInputs.sh could look something like this::

  #!/bin/bash -l
  #
  # Example user input file for ASKAP processing.
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

* :doc:`ControlParameters`
* :doc:`DataLocationSelection`
* :doc:`BandpassCalibrator`
* :doc:`ScienceFieldPreparation`
* :doc:`ScienceFieldContinuumImaging`
* :doc:`ScienceFieldMosaicking`
* :doc:`ContinuumSourcefinding`
* :doc:`SpectralLineSourcefinding`
* :doc:`ScienceFieldSpectralLineImaging`
* :doc:`archiving`



What is created and where does it go?
-------------------------------------

Any measurement sets, images and tables that are created are put in an
output directory specified in the input file (if not provided, they go in
the directory in which *processASKAP.sh* is run). There will be a file
called *PROCESSED_ON* that holds the timestamp indicating when the
script was run (this timestamp is used in various filenames). Also
created are a number of subdirectories which hold various types of
files. These are:

* *slurmFiles/* – the files in here are the job files that are submitted
  to the queue via the sbatch command. When a job is run, it makes a
  copy of the file that is labelled with the job ID.
* *metadata/* – information about the measurement sets and the beam
  footprint are written to files here.
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
* *diagnostics/* - this directory is intended to hold plots and other
  data products that indicate how the processing went. The pipeline only
  produces a few particular types at the moment, but the intention is
  this will expand with time.
* *tools/* – utility scripts to show progress and kill all jobs for a
  given run are placed here. See :doc:`pipelineDiagnostics` for
  details.
* *Checkfiles/* – files that indicate progress through stages of the
  pipeline are written here. The pipeline can see these and know to
  skip certain stages, if required by the user. A version of this
  directory is put in each field directory.

Measurement sets
----------------

To provide the input data to the scripts, you can provide either the
scheduling blocks (SBs) of the two observations, or provide specific
measurement sets (MSs) for each case.

The measurement sets that will be created should be named in the
configuration file. A wildcard %s can be used to represent the
scheduling block ID, and %b should be used to represent the beam
number in the resulting MSs, since the individual beams will be split
into separate files.

Each step detailed below can be switched on or off, and those selected
will run fine (provided any pre- requisites such as measurement sets
or bandpass solutions etc are available). If you have already created
an averaged science MS, you can re-use that with the
``MS_SCIENCE_AVERAGE`` parameter (see :doc:`ScienceFieldPreparation`),
again with the %b wildcard to represent the beam number and %s the
scheduling block ID.

Workflow summary
----------------


Here is a summary of the workflow provided for by these scripts:

* Get observation metadata from the MS and the beam footprint. This
  does the following steps:

  * Use **mslist** to get basic metadata for the observation,
    including number of antennas & channels, and the list of field
    names.
  * Use **schedblock** to determine the footprint specification.
  * Use **footprint.py** (from the ACES tools) to convert that into
    beam centre positions.

* Read in user-defined parameters from the provided configuration
  file, and define further parameters derived from them.
* If bandpass calibration is required and a 1934-638 observation is
  available, we split out the relevant beams with **mssplit**
  (:doc:`../calim/mssplit`) into individual measurement sets (MSs),
  one per beam. Only the scan in which the beam in question was
  pointing at 1934-638 is used - this assumes the beams were pointed
  at it in order (so that beam 0 was pointing at in in scan 0, etc)
* These are flagged using **cflag** (:doc:`../calim/cflag`) in two
  passes: first, selection rules covering antennas, baselines and
  autocorrelations are applied, along with an optional simple flat amplitude
  threshold; then a dynamic amplitude flag that integrates
  individual spectra.
* The bandpass solution is then determined with **cbpcalibrator**
  (:doc:`../calim/cbpcalibrator`), using all individual MSs and stored
  in a single CASA table.
* The science field is processed for each field name - what follows
  describes the steps used for each field.
* The science field data is split with *mssplit**, producing one
  measurement set per beam. You can select particular scans or fields
  here, but the default is to use everything. Each field gets its own
  directory.
* The bandpass solution is then applied to each beam MS with
  **ccalapply** (:doc:`../calim/ccalapply`).
* Flagging is then applied to the bandpass-calibrated dataset. The
  same procedure as for the calibrator is used, with separate user
  parameters to control it.
* The science field data are then averaged with **mssplit** to form
  continuum data sets. (Still one per beam).
* Another round of flagging can be done, this time on the averaged
  dataset.
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
    calibration solution. This process is repeated a number of
    times. The calibration solution can then be applied directly to
    the MS using **ccalapply**, optionally creating a copy in the
    process.

* The continuum dataset can then be optionally imaged as a "continuum
  cube", using **simager** to preserve the full frequency
  sampling. This mode can be run for a range of polarisations,
  creating a cube for each polarisation requested.
* Once the continuum image has been made, the source-finder **selavy**
  can be run on it to produce a deeper catalogue of sources.
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

* There is a new task to remove the continuum from the image by
  fitting a low-order polynomial to each spectrum independently.

* Source-finding with **selavy** can then be run on the
  spectral-cubes.

* Finally a diagnostics script is run to produce QA & related
  plots. This is a prototype script at present, although we will look
  to expand it in the near future.

Staging the processing
----------------------

As described on :doc:`../platform/comm_archive`, many datasets will
not reside on /scratch2, but only on the commissioning archive. They
can be restored by Operations staff if you wish to process (or
re-process) them. It is possible to set up your processing to start
immediately upon completion of the restoration process, by using the
**stage-processing.sh** script in the *askaputils* module. Typical
usage is::

  stage-processing.sh myconfig.sh <jobID>

where <jobID> is the slurm job ID of the restore job and 'myconfig.sh'
can be replaced with your configuration file. Run "stage-processing.sh
-h" for more information.

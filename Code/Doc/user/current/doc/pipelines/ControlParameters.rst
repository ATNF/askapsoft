User Parameters - Pipeline & job control
========================================

Here we detail the input parameters that cover the overall process
control, and the switches to turn on & off different parts of the
pipeline.

Values for parameters that act as flags (ie. those that accept
true/false values) should be given in *lower case only*, to ensure
comparisons work properly.

ASKAPsoft versions
------------------

The default behaviour of the slurm jobs is to use the askapsoft module
that is loaded in your ~/.bashrc file. However, it is possible to run
the pipeline using a different askapsoft module, by setting
``ASKAPSOFT_VERSION`` to the module version (0.19.2, for instance). If
the requested version is not available, the default version is used
instead. 

This behaviour is also robust against there not being an askapsoft
module defined in the ~/.bashrc - in this case the default module is
used, unless ``ASKAPSOFT_VERSION`` is given in the configuration
file.

The pipeline looks in a standard place for the modules, given by the
setup used at the Pawsey Centre. If the pipelines are being run on a
different system, the location of the module files can be given by
``ASKAP_MODULE_DIR`` (this is passed to the *module use* command).

ACES software
-------------

A small number of tasks within the pipeline make use of tools or
scripts developed by the ACES (ASKAP Commissioning & Early Science)
team. These live in a subversion repository that can be checked out by
users (should you have permission) and pointed to by ``$ACES``. The
preferred means of using this is, however, is to use the **acesops**
module, which provides a controlled snapshot of the subversion tree,
allowing processing to be reproducible by recording the revision
number.

Use of the **acesops** module is the default behaviour of the
pipeline, and the user does not need to load it prior to running the
pipeline. To use your own copy of the subversion tree, you need to set
``USE_ACES_OPS=false``. A particular version of the **acesops** module
can be chosen via the ``ACESOPS_VERSION`` config parameter.

Slurm control
-------------

These parameters affect how the slurm jobs are set up and where the
output data products go. To run the jobs, you need to set
``SUBMIT_JOBS=true``. Each job has a time request associated with it -
see the *Slurm time requests* section below for details.

+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| Variable                            | Default                 | Description                                                                     |
+=====================================+=========================+=================================================================================+
| ``SUBMIT_JOBS``                     | false                   |The ultimate switch controlling whether things are run on the galaxy queue or    |
|                                     |                         |not. If false, the slurm files etc will be created but nothing will run (useful  |
|                                     |                         |for checking if things are to your liking).                                      |
|                                     |                         |                                                                                 |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``ASKAPSOFT_VERSION``               | ``""``                  |The version number of the askapsoft module to use for the processing. If not     |
|                                     |                         |given, or if the requested version is not valid, the version defined in the      |
|                                     |                         |~/.bashrc file is used, or the default version should none be defined by the     |
|                                     |                         |~/.bashrc file.                                                                  |
|                                     |                         |                                                                                 |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``ASKAP_MODULE_DIR``                | /group/askap/modulefiles|The location for the modules loaded by the pipeline and its slurm jobs. Change   |
|                                     |                         |this to reflect the setup on the system you are running this on, but it should   |
|                                     |                         |not be changed if running at Pawsey.                                             |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``CLUSTER``                         | galaxy                  |The cluster to which jobs should be submitted. This allows you to submit to the  |
|                                     |                         |galaxy queue from machines such as galaxy-data. Leave as is unless you know      |
|                                     |                         |better.                                                                          |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``QUEUE``                           | workq                   |This should be left as is unless you know better.                                |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``CONSTRAINT``                      | ``""``                  |This allows one to provide slurm with additional constraints. While not needed   |
|                                     |                         |for galaxy, this can be of use in other clusters (particularly those that have a |
|                                     |                         |mix of technologies).                                                            |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``ACCOUNT``                         | ``""``                  |This is the account that the jobs should be charged to. If left blank, then the  |
|                                     |                         |user's default account will be used.                                             |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``RESERVATION``                     | ``""``                  |If there is a reservation you specify the name of it here.  If you don't have a  |
|                                     |                         |reservation, leave this alone and it will be submitted as a regular job.         |
|                                     |                         |                                                                                 |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``JOB_TIME_DEFAULT``                |24:00:00                 |The default time request for the slurm jobs. It is possible to specify a         |
|                                     |                         |different time for individual jobs - see the list below and on the individual    |
|                                     |                         |pages describing the jobs. If those parameters are not given, the time requested |
|                                     |                         |is the value of ``JOB_TIME_DEFAULT``.                                            |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``OUTPUT``                          | .                       |The sub-directory in which to put the images, tables, catalogues, MSs etc. The   |
|                                     |                         |name should be relative to the directory in which the script was run, with the   |
|                                     |                         |default being that directory.                                                    |
|                                     |                         |                                                                                 |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``EMAIL``                           | ``""``                  |An email address to which you want slurm notifications sent (this will be passed |
|                                     |                         |to the ``--mail-user`` option of sbatch).  Leaving it blank will mean no         |
|                                     |                         |notifications are sent.                                                          |
|                                     |                         |                                                                                 |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``EMAIL_TYPE``                      | ALL                     |The types of notifications that are sent (this is passed to the ``--mail-type``  |
|                                     |                         |option of sbatch, and only if ``EMAIL`` is set to something). Options include:   |
|                                     |                         |BEGIN, END, FAIL, REQUEUE, ALL, TIME_LIMIT, TIME_LIMIT_90, TIME_LIMIT_80, &      |
|                                     |                         |TIME_LIMIT_50 (taken from the sbatch man page on galaxy).                        |
|                                     |                         |                                                                                 |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``USE_ACES_OPS``                    | true                    |Whether to use the **acesops** module to access ACES tools within the            |
|                                     |                         |pipeline. Setting to false will force the pipeline to look in the ``$ACES``      |
|                                     |                         |directory defined by your environment. If ``$ACES`` is not set, then             |
|                                     |                         |``USE_ACES_OPS`` will be set back to true.                                       |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+
| ``ACESOPS_VERSION``                 | ``""``                  |The version of the **acesops** module used by the pipeline. Leaving blank will   |
|                                     |                         |make it use the default at the time.                                             |
+-------------------------------------+-------------------------+---------------------------------------------------------------------------------+

Filesystem control
------------------

There are a couple of parameters that affect how files interact with
the Lustre filesystem. We set the striping of the directory at the
start to a value configurable by the user. This only affects the
directories where MSs, images & tables go - parsets, logs, metadata
and the rest are given a stripe count of 1.

There is also a parameter to control the I/O bucketsize of the
measurement sets created by mssplit. This is particularly important in
governing the I/O performance and the splitting run-time. The default,
1MB, matches the stripe size on /group, and has been found to work
well.

There is also a parameter ``PURGE_FULL_MS`` that allows the deletion
of the full-spectral-resolution measurement set once the averaging to
continuum channels has been done. The idea here is that such a dataset
is not needed for some types of processing (continuum & continuum
cube imaging in particular), and so rather than have a large MS left
lying around on the disk, we delete it. This parameter defaults to
true, but is turned off if any of the spectral-line processing tasks
are turned on (``DO_COPY_SL``, ``DO_APPLY_CAL_SL``,
```DO_CONT_SUB_SL`` or ``DO_SPECTRAL_IMAGING``). The deletion is done
in the averaging job, once the averaging has completed
successfully. If the averaging fails it is not removed. 

+---------------------------+---------+-------------------------------------------------------------+
| Variable                  | Default | Description                                                 |
+===========================+=========+=============================================================+
| ``LUSTRE_STRIPING``       | 4       | The stripe count to assign to the data directories          |
+---------------------------+---------+-------------------------------------------------------------+
| ``BUCKET_SIZE``           | 1048576 | The bucketsize passed to mssplit (as "stman.bucketsize") in |
|                           |         | units of bytes.                                             |
+---------------------------+---------+-------------------------------------------------------------+
| ``TILE_NCHAN_SCIENCE``    | 54      | The number of channels in the measurement set tile for the  |
|                           |         | science data, once the local version is created.            |
+---------------------------+---------+-------------------------------------------------------------+
| ``TILE_NCHAN_1934``       | 54      | The number of channels in the measurement set tile for the  |
|                           |         | bandpass calibrator data, once the local version is created.|
+---------------------------+---------+-------------------------------------------------------------+
| ``PURGE_INTERIM_MS_SCI``  | true    | Whether to remove the interim science MSs created when      |
|                           |         | splitting and merging is required.                          |
+---------------------------+---------+-------------------------------------------------------------+
| ``PURGE_INTERIM_MS_1934`` | true    | Whether to remove the interim bandpass calibrator MSs       |
|                           |         | created when splitting and merging is required.             |
+---------------------------+---------+-------------------------------------------------------------+
| ``PURGE_FULL_MS``         | true    | Whether to remove the full-spectral-resolution measurement  |
|                           |         | set once the averaging has been done. See notes above.      |
+---------------------------+---------+-------------------------------------------------------------+


Control of Online Services
--------------------------

The pipeline makes use of two online databases: the scheduling block
service, which provides information about individual scheduling blocks
and their parsets; and the footprint service, which translates
descriptive names of beam footprints into celestial positions.

These are hosted at the MRO, and it may be that the MRO is offline but
Pawsey is still available. If that is the case, use of these can be
turned off via the ``USE_CLI`` parameter (CLI="command line
interface"). If you have previously created the relevant metadata
files, the pipeline will be able to proceed as usual. If the footprint
information is not available, but you know what the footprint name
was, you can use the ``IS_BETA`` option. See
:doc:`ScienceFieldMosaicking` for more information and related
parameters. 

+-------------------------+---------+-------------------------------------------------------------+
| Variable                | Default | Description                                                 |
+=========================+=========+=============================================================+
| ``USE_CLI``             | true    | A parameter that determines whether to use the command-line |
|                         |         | interfaces to the online services, specifically schedblock  |
|                         |         | and footprint.                                              |
+-------------------------+---------+-------------------------------------------------------------+
|  ``IS_BETA``            | false   | A special parameter that, if true, indicates the dataset was|
|                         |         | taken with BETA, and so needs to be treated differently     |
|                         |         | (many of the online services will not work with BETA        |
|                         |         | Scheduling Blocks, and the raw data is in a different       |
|                         |         | place).                                                     |
+-------------------------+---------+-------------------------------------------------------------+


Calibrator switches
-------------------

These parameters control the different types of processing done on the
calibrator observation. The three aspects are splitting by beam/scan,
flagging, and finding the bandpass. The ``DO_1934_CAL`` acts as the
"master switch" for the calibrator processing.

+------------------------------+---------+------------------------------------------------------------+
| Variable                     | Default | Description                                                |
+==============================+=========+============================================================+
| ``DO_1934_CAL``              | true    | Whether to process the 1934-638 calibrator observations. If|
|                              |         | set to ``false`` then all the following switches will be   |
|                              |         | set to ``false``.                                          |
+------------------------------+---------+------------------------------------------------------------+
| ``DO_SPLIT_1934``            | true    | Whether to split a given beam/scan from the input 1934 MS. |
|                              |         | From rev10559 onwards, users can additionally split out    |
|                              |         | bandpass msdata from a specified Time-Range (see below)    |
+------------------------------+---------+------------------------------------------------------------+
| ``DO_FLAG_1934``             | true    | Whether to flag the splitted-out 1934 MS                   |
+------------------------------+-+-------+------------------------------------------------------------+
| ``DO_FIND_BANDPASS``         | true    | Whether to fit for the bandpass using all 1934-638 MSs     |
+------------------------------+---------+------------------------------------------------------------+


Science field switches
----------------------

These parameter control the different types of processing done on the
science field, with ``DO_SCIENCE_FIELD`` acting as a master switch for
the science field processing.

+-----------------------------+---------+-------------------------------------------------------------+
| Variable                    | Default | Description                                                 |
+=============================+=========+=============================================================+
| ``DO_SCIENCE_FIELD``        | true    | Whether to process the science field observations. If set   |
|                             |         | to ``false`` then all the following switches will be set to |
|                             |         | ``false``.                                                  |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_SPLIT_SCIENCE``        | true    | Whether to split out the given beam from the science MS     |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_FLAG_SCIENCE``         | true    | Whether to flag the (splitted) science MS                   |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_APPLY_BANDPASS``       | true    | Whether to apply the bandpass calibration to the science    |
|                             |         | observation                                                 |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_AVERAGE_CHANNELS``     | true    |  Whether to average the science MS to continuum resolution  |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_CONT_IMAGING``         | true    | Whether to image the science MS                             |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_SELFCAL``              | true    | Whether to self-calibrate the science data when imaging     |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_SOURCE_FINDING_CONT``  | ``""``  | Whether to do the continuum source-finding with Selavy. If  |
|                             |         | not given, the default value is that of ``DO_CONT_IMAGING``.|
|                             |         | Source finding on the individual beam images is done by     |
|                             |         | setting the parameter ``DO_SOURCE_FINDING_BEAMWISE`` to     |
|                             |         | ``true`` (the default is ``false``).                        |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_CONTINUUM_VALIDATION`` | true    | Whether to run the continuum validation script upon         |
|                             |         | completion of the source-finding.                           |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_CONTCUBE_IMAGING``     | false   | Whether to image the continuum cube(s), optionally in       |
|                             |         | multiple polarisations.                                     |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_APPLY_CAL_CONT``       | true    | Whether to apply the gains calibration determined from the  |
|                             |         | continuum self-calibration to the averaged MS.              |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_COPY_SL``              | false   | Whether to copy a channel range of the original             |
|                             |         | full-spectral- resolution measurement set into a new MS.    |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_APPLY_CAL_SL``         | false   | Whether to apply the gains calibration determined from the  |
|                             |         | continuum self-calibration to the full-spectral-resolution  |
|                             |         | MS.                                                         |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_CONT_SUB_SL``          | false   | Whether to subtract a continuum model from the              |
|                             |         | spectral-line dataset.                                      |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_SPECTRAL_IMAGING``     | false   | Whether to do the spectral-line imaging                     |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_SPECTRAL_IMSUB``       | false   | Whether to do the image-based continuum subtraction.        |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_SOURCE_FINDING_SPEC``  | ``""``  | Whether to do the spectral-line source-finding with         |
|                             |         | Selavy. If not given the default value is that of           |
|                             |         | ``DO_SPECTRAL_IMAGING``. Source finding on the individual   |
|                             |         | beam cubes is done by setting the parameter                 |
|                             |         | ``DO_SOURCE_FINDING_BEAMWISE`` to ``true`` (default is      |
|                             |         | ``false``).                                                 |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_MOSAIC``               | true    | Whether to mosaic the individual beam images, forming a     |
|                             |         | single, primary-beam-corrected image. Mosaics of each field |
|                             |         | can be done via the ``DO_MOSAIC_FIELDS`` parameter (default |
|                             |         | is ``true``).                                               |
+-----------------------------+---------+-------------------------------------------------------------+
| ``DO_ALT_IMAGER``           | true    | Whether to use the new imager (:doc:`../calim/imager`) for  |
|                             |         | all imaging. Its use for specific modes can be selected by  |
|                             |         | the parameters ``DO_ALT_IMAGER_CONT``,                      |
|                             |         | ``DO_ALT_IMAGER_CONTCUBE``, and ``DO_ALT_IMAGER_SPECTRAL``  |
|                             |         | (which, if not given, default to the value of               |
|                             |         | ``DO_ALT_IMAGER``).                                         |
+-----------------------------+---------+-------------------------------------------------------------+


Post-processing switches
------------------------

After the calibration, imaging and source-finding, there are several
tasks that can be done to prepare the data for archiving in CASDA, and
these tasks are controlled by the following parameters.

+----------------------------+---------+-------------------------------------------------------------+
| Variable                   | Default | Description                                                 |
+============================+=========+=============================================================+
| ``DO_DIAGNOSTICS``         | true    | Whether to run the diagnostic script upon completion of     |
|                            |         | imaging and source-finding. (This is not the continuum      |
|                            |         | validation, but rather other diganostic tasks).             |
+----------------------------+---------+-------------------------------------------------------------+
| ``DO_VALIDATION_SCIENCE``  | true    | Run specific science validation tasks, such as plotting the |
|                            |         | cube statistics.                                            |
+----------------------------+---------+-------------------------------------------------------------+
| ``DO_CONVERT_TO_FITS``     | true    | Whether to convert remaining CASA images and image cubes to |
|                            |         | FITS format (some will have been converted by the           |
|                            |         | source-finding tasks).                                      |
+----------------------------+---------+-------------------------------------------------------------+
| ``DO_MAKE_THUMBNAILS``     | true    | Whether to make the PNG thumbnail images that are used      |
|                            |         | within CASDA to provide previews of the image data products.|
+----------------------------+---------+-------------------------------------------------------------+
| ``DO_STAGE_FOR_CASDA``     | false   | Whether to tun the casda upload script to copy the data to  |
|                            |         | the staging directory for ingest into the archive.          |
+----------------------------+---------+-------------------------------------------------------------+



Slurm time requests
-------------------

Each slurm job has a time request associated with it. These default to
12 hours (24:00:00), given by the user parameter
``JOB_TIME_DEFAULT``. You can use this parameter to set a different
default. Additionally, you can set a different time to the default for
individual jobs, by using the following set of parameters. Acceptable
time formats include (taken from the sbatch man page): "minutes",
"minutes:seconds", "hours:minutes:seconds", "days-hours",
"days-hours:minutes" and "days-hours:minutes:seconds"


+---------------------------------+--------------------------------------------------------------+
| Variable                        | Description                                                  |
+=================================+==============================================================+
| ``JOB_TIME_SPLIT_1934``         | Time request for splitting the calibrator MS                 |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_SPLIT_SCIENCE``      | Time request for splitting the science MS                    |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_FLAG_1934``          | Time request for flagging the calibrator data                |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_FLAG_SCIENCE``       | Time request for flagging the science data                   |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_FIND_BANDPASS``      | Time request for finding the bandpass solution               |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_APPLY_BANDPASS``     | Time request for applying the bandpass to the science data   |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_AVERAGE_MS``         | Time request for averaging the channels of the science data  |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_CONT_IMAGE``         | Time request for imaging the continuum (both types - with and|
|                                 | without self-calibration)                                    |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_SPLIT``     | Time request for splitting out a subset of the spectral data |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_APPLYCAL``  | Time request for applying the gains calibration to the       |
|                                 | spectral data                                                |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_CONTSUB``   | Time request for subtracting the continuum from the spectral |
|                                 | data                                                         |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_IMAGE``     | Time request for imaging the spectral-line data              |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_LINMOS``             | Time request for mosaicking                                  |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_SOURCEFINDING``      | Time request for source-finding jobs                         |
+---------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_VALIDATE``           | Time request for the science validation job.                 |
+---------------------------------+--------------------------------------------------------------+

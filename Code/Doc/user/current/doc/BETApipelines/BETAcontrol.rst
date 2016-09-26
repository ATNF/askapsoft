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
``ASKAPSOFT_VERSION`` to the module version (0.11.2, for instance). If
the requested version is not available, the default version is used
instead. 

This behaviour is also robust against there not being an askapsoft
module defined in the ~/.bashrc - in this case the default module is
used, unless ``ASKAPSOFT_VERSION`` is given in the configuration
file. 


Slurm control
-------------

These parameters affect how the slurm jobs are set up and where the
output data products go. To run the jobs, you need to set
``SUBMIT_JOBS=true``. Each job has a time request associated with it -
see the *Slurm time requests* section below for details.

+-------------------------------------+---------+---------------------------------------------------------------------------------+
| Variable                            | Default | Description                                                                     |
+=====================================+=========+=================================================================================+
| ``SUBMIT_JOBS``                     | false   |The ultimate switch controlling whether things are run on the galaxy queue or    |
|                                     |         |not. If false, the slurm files etc will be created but nothing will run (useful  |
|                                     |         |for checking if things are to your liking).                                      |
|                                     |         |                                                                                 |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``ASKAPSOFT_VERSION``               | ""      |The version number of the askapsoft module to use for the processing. If not     |
|                                     |         |given, or if the requested version is not valid, the version defined in the      |
|                                     |         |~/.bashrc file is used, or the default version should none be defined by the     |
|                                     |         |~/.bashrc file.                                                                  |
|                                     |         |                                                                                 |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``CLUSTER``                         | galaxy  |The cluster to which jobs should be submitted. This allows you to submit to the  |
|                                     |         |galaxy queue from machines such as galaxy-data. Leave as is unless you know      |
|                                     |         |better.                                                                          |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``QUEUE``                           | workq   |This should be left as is unless you know better.                                |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``ACCOUNT``                         | ""      |This is the account that the jobs should be charged to. If left blank, then the  |
|                                     |         |user's default account will be used.                                             |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``RESERVATION``                     | ""      |If there is a reservation you specify the name of it here.  If you don't have a  |
|                                     |         |reservation, leave this alone and it will be submitted as a regular job.         |
|                                     |         |                                                                                 |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``JOB_TIME_DEFAULT``                |12:00:00 |The default time request for the slurm jobs. It is possible to specify a         |
|                                     |         |different time for individual jobs - see the list below and on the individual    |
|                                     |         |pages describing the jobs. If those parameters are not given, the time requested |
|                                     |         |is the value of ``JOB_TIME_DEFAULT``.                                            |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``OUTPUT``                          | .       |The sub-directory in which to put the images, tables, catalogues, MSs etc. The   |
|                                     |         |name should be relative to the directory in which the script was run, with the   |
|                                     |         |default being that directory.                                                    |
|                                     |         |                                                                                 |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``EMAIL``                           | ""      |An email address to which you want slurm notifications sent (this will be passed |
|                                     |         |to the ``--mail-user`` option of sbatch).  Leaving it blank will mean no         |
|                                     |         |notifications are sent.                                                          |
|                                     |         |                                                                                 |
+-------------------------------------+---------+---------------------------------------------------------------------------------+
| ``EMAIL_TYPE``                      | ALL     |The types of notifications that are sent (this is passed to the ``--mail-type``  |
|                                     |         |option of sbatch, and only if ``EMAIL`` is set to something). Options include:   |
|                                     |         |BEGIN, END, FAIL, REQUEUE, ALL, TIME_LIMIT, TIME_LIMIT_90, TIME_LIMIT_80, &      |
|                                     |         |TIME_LIMIT_50 (taken from the sbatch man page on galaxy).                        |
|                                     |         |                                                                                 |
+-------------------------------------+---------+---------------------------------------------------------------------------------+

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
1MB, matches the stripe size on /scratch2, and has been found to work well. 

+----------------------+---------+-------------------------------------------------------------+
| Variable             | Default | Description                                                 |
+======================+=========+=============================================================+
| ``LUSTRE_STRIPING``  | 4       | The stripe count to assign to the data directories          |
+----------------------+---------+-------------------------------------------------------------+
| ``BUCKET_SIZE``      | 1048576 | The bucketsize passed to mssplit (as "stman.bucketsize") in |
|                      |         | units of bytes.                                             |
+----------------------+---------+-------------------------------------------------------------+


Calibrator switches
-------------------

These parameters control the different types of processing done on the
calibrator observation. The three aspects are splitting by beam/scan,
flagging, and finding the bandpass. The ``DO_1934_CAL`` acts as the
"master switch" for the calibrator processing.

+----------------------+---------+------------------------------------------------------------+
| Variable             | Default | Description                                                |
+======================+=========+============================================================+
| ``DO_1934_CAL``      | true    | Whether to process the 1934-638 calibrator observations. If|
|                      |         | set to ``false`` then all the following switches will be   |
|                      |         | set to ``false``.                                          |
+----------------------+---------+------------------------------------------------------------+
| ``DO_SPLIT_1934``    | true    | Whether to split a given beam/scan from the input 1934 MS  |
+----------------------+---------+------------------------------------------------------------+
| ``DO_FLAG_1934``     | true    | Whether to flag the splitted-out 1934 MS                   |
+----------------------+---------+------------------------------------------------------------+
| ``DO_FIND_BANDPASS`` | true    | Whether to fit for the bandpass using all 1934-638 MSs     |
+----------------------+---------+------------------------------------------------------------+


Science field switches
----------------------

These parameter control the different types of processing done on the
science field, with ``DO_SCIENCE_FIELD`` acting as a master switch for
the science field processing.

+-------------------------+---------+-------------------------------------------------------------+
| Variable                | Default | Description                                                 |
+=========================+=========+=============================================================+
| ``DO_SCIENCE_FIELD``    | true    | Whether to process the science field observations. If set   |
|                         |         | to ``false`` then all the following switches will be set to |
|                         |         | ``false``.                                                  |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_SPLIT_SCIENCE``    | true    | Whether to split out the given beam from the science MS     |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_FLAG_SCIENCE``     | true    | Whether to flag the (splitted) science MS                   |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_APPLY_BANDPASS``   | true    | Whether to apply the bandpass calibration to the science    |
|                         |         | observation                                                 |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_AVERAGE_CHANNELS`` | true    |  Whether to average the science MS to continuum resolution  |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_CONT_IMAGING``     | true    | Whether to image the science MS                             |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_SELFCAL``          | false   | Whether to self-calibrate the science data when imaging     |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_SOURCE_FINDING``   | false   | Whether to do the source-finding with Selavy on the         |
|                         |         | individual beam images and the final mosaic.                |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_MOSAIC``           | true    | Whether to mosaic the individual beam images, forming a     |
|                         |         | single, primary-beam-corrected image.                       |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_COPY_SL``          | false   | Whether to copy a channel range of the original             |
|                         |         | full-spectral- resolution measurement set into a new MS.    |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_APPLY_CAL_SL``     | false   | Whether to apply the gains calibration determined from the  |
|                         |         | continuum self-calibration.                                 |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_CONT_SUB_SL``      | false   | Whether to subtract a continuum model from the              |
|                         |         | spectral-line dataset.                                      |
+-------------------------+---------+-------------------------------------------------------------+
| ``DO_SPECTRAL_IMAGING`` | false   | Whether to do the spectral-line imaging                     |
+-------------------------+---------+-------------------------------------------------------------+
|  ``DO_SPECTRAL_IMSUB``  | false   | Whether to do the image-based continuum subtraction.        |
+-------------------------+---------+-------------------------------------------------------------+


Slurm time requests
-------------------

Each slurm job has a time request associated with it. These default to
12 hours (12:00:00), given by the user parameter
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

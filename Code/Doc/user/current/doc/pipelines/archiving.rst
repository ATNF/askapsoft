User parameters - Archiving
===========================

The final stage of the pipeline involves preparing data for storage in
CASDA, the CSIRO ASKAP Science Data Archive. This involves four
steps:

* Diagnostic plots are created. These are intended to be used for
  Quality Analysis & validation. Currently the script that does this
  is only a prototype, producing both greyscale plots of the continuum
  images, with weights contours and the component catalogue overlaid,
  and greyscale plots of the noise maps produced by Selavy. See examples
  on :doc:`validation`.
* All images are converted to FITS format. FITS is the format required
  for storage in CASDA - the ASKAPsoft tasks are able to write
  directly to FITS (``IMAGETYPE_CONT`` etc), but if CASA-format images
  are created, an additional step is required.
  At the same time, the FITS header is given the following keywords:
  PROJECT (the OPAL project code), SBID (the scheduling block ID),
  DATE-OBS (the date/time of the observation), DURATION (the length of
  the observation). Additionally, information on the version of the
  askapsoft, askappipeline and aces software is added to the HISTORY. 
* All two-dimensional images have two "thumbnail" images made. The
  thumbnail image is designed as a "quick-look" image that can be
  incorporated into the CASDA search results. Two sizes are made - a
  small one for easy display, and a larger one to provide a little
  more detail. The greyscale limits are chosen from a robust
  measurement of the overall image noise, and are -10 to +40
  sigma by default. The format is currently PNG.
* The :doc:`../utils/casdaupload` utility is run, copying the relevant
  data products to the staging directory, and producing an XML file
  detailing the contents of this directory.
  The files copied are all FITS files conforming to a particular
  pattern, any Selavy catalogues, measurement sets (just the averaged
  MSs, using the calibrated ones if they have been made), the
  thumbnail images, and the stats summary file detailing how the
  processing tasks went.


The following is a list of files included in the upload to CASDA:

* Images: all FITS files that have been processed (ie. continuum
  images, spectral cubes if requested, continuum cubes if requested)
  that meet the naming system governed by ``IMAGE_LIST``. Model images
  used for continuum subtraction will also be included.
* Additional images produced by Selavy - the noise map, and the
  component map and its associated residual image.
* Measurement sets: all continuum measurement sets are included as
  individual files, along with all spectral measurement sets if
  requested. These will be tarred for access through CASDA. The
  pipeline metadata directory will be copied into the measurement set,
  where it will appear as a directory called ASKAP_METADATA.
* Catalogues: all Selavy catalogues created for the final mosaics, in
  VOTable/XML format, along with any catalogues used for the continuum
  subtraction (if the components method is used).
* Evaluation files: several files are included for use with the
  validation:

  * An XML file listing the validation metrics from the continuum
    source-finding validation.
  * A tar file containing the validation directory
  * The stats files that summarise the resource usage of each job.
  * A tar file containing the processing directory structure, with the
    following:

    * All calibration tables
    * Logs, slurm output and slurm file directories individually tarred.
    * The beam logs for the spectral cubes
    * The metadata directory

There are a number of user parameters that govern aspects of these
scripts, and they are detailed here.

+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| Variable                         |                    Default                     | Parset equivalent               | Description                                                     |
+==================================+================================================+=================================+=================================================================+
| ``DO_DIAGNOSTICS``               | true                                           | none                            | Whether to run the diagnostic script upon completion of imaging |
|                                  |                                                |                                 | and source-finding.                                             |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_DIAGNOSTICS``         | ``JOB_TIME_DEFAULT`` (24:00:00)                | none                            | Time request for the diagnostic script.                         |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``DO_CONVERT_TO_FITS``           | true                                           | none                            | Whether to convert the CASA images to FITS format.              |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_FITS_CONVERT``        | ``JOB_TIME_DEFAULT`` (24:00:00)                | none                            | Time request for the FITS conversion.                           |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``DO_MAKE_THUMBNAILS``           | true                                           | none                            | Whether to make the PNG thumbnail images of the 2D FITS images. |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_THUMBNAILS``          | ``JOB_TIME_DEFAULT`` (24:00:00)                | none                            | Time request for the thumbnail creation.                        |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``DO_STAGE_FOR_CASDA``           | false                                          | none                            | Whether to stage data for ingest into CASDA                     |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_CASDA_UPLOAD``        | ``JOB_TIME_DEFAULT`` (24:00:00)                | none                            | Time request for the CASDA-upload task.                         |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| **General**                      |                                                |                                 |                                                                 |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``IMAGE_LIST``                   | ``"image psf psf.image residual sensitivity"`` | none                            | The list of image prefixes that will be used for generating FITS|
|                                  |                                                |                                 | files and determining the list of images to be uploaded to      |
|                                  |                                                |                                 | CASDA. In addition, the images image.XXX.restored and           |
|                                  |                                                |                                 | image.XXX.alt.restored (in the latter's case, if present) will  |
|                                  |                                                |                                 | also be processed.                                              |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``ARCHIVE_SPECTRAL_MS``          | false                                          | none                            | Whether the individual full-spectral-resolution measurement sets|
|                                  |                                                |                                 | should be included in the archiving.                            |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``ARCHIVE_BEAM_IMAGES``          | false                                          | none                            | Whether the individual beam images should be included in the    |
|                                  |                                                |                                 | archiving (true) or if only the mosaicked image should be       |
|                                  |                                                |                                 | uploaded.                                                       |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``ARCHIVE_SELFCAL_LOOP_MOSAICS`` | false                                          | none                            | Whether to archive the mosaics of the intermediate              |
|                                  |                                                |                                 | self-calibration loop images (see                               |
|                                  |                                                |                                 | :doc:`ScienceFieldContinuumImaging` and                         |
|                                  |                                                |                                 | :doc:`ScienceFieldMosaicking`).                                 |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``ARCHIVE_FIELD_MOSAICS``        | false                                          | none                            | Whether to archive the mosaics for each individual field, as    |
|                                  |                                                |                                 | well as for each tile and the final mosaicked image. See        |
|                                  |                                                |                                 | :doc:`ScienceFieldMosaicking` for a description.                |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``PROJECT_ID``                   | AS033                                          | *<key>*.project                 | The project ID that is written to the FITS header, and used by  |
|                                  |                                                | (:doc:`../utils/casdaupload`)   | the casdaupload script to describe each data product. This is   |
|                                  |                                                |                                 | usually taken from the SB parset, but can be given in the       |
|                                  |                                                |                                 | configuration file in case the SB parset does not have the      |
|                                  |                                                |                                 | information (or the SB parset is not available to the schedblock|
|                                  |                                                |                                 | command-line utility, as will be the case for BETA).            |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| **Thumbnails**                   |                                                |                                 |                                                                 |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_SUFFIX``             | png                                            | none                            | Suffix for thumbnail image files, which in turn determinings the|
|                                  |                                                |                                 | format of these files.                                          |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_GREYSCALE_MIN``      | -10                                            | none                            | Minimum greyscale level fro the thumbnail image colourmap. In   |
|                                  |                                                |                                 | units of the overall image rms noise.                           |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_GREYSCALE_MAX``      | 40                                             | none                            | Maximum greyscale level fro the thumbnail image colourmap. In   |
|                                  |                                                |                                 | units of the overall image rms noise.                           |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_SIZE_INCHES``        | ``"16,5"``                                     | none                            | The sizes (in inches) of the thumbnail images. The sizes        |
|                                  |                                                |                                 | correspond to the size names given below. Don't change unless   |
|                                  |                                                |                                 | you know what you are doing.                                    |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_SIZE_TEXT``          | ``"large,small"``                              | none                            | The labels that go with the thumbnail sizes. These are          |
|                                  |                                                |                                 | incorporated into the thumbnail name, so that image.fits gets a |
|                                  |                                                |                                 | thumbnail image_large.png etc. Don't change unless you know what|
|                                  |                                                |                                 | you are doing.                                                  |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| **CASDA upload**                 |                                                |                                 |                                                                 |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``OBS_PROGRAM``                  | Commissioning                                  | obsprogram                      | The name of the observational program to be associated with this|
|                                  |                                                | (:doc:`../utils/casdaupload`)   | data set.                                                       |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``CASDA_UPLOAD_DIR``             | /group/casda/prd                               | outputdir                       | The output directory to put the staged data. It may be that some|
|                                  |                                                | (:doc:`../utils/casdaupload`)   | users will not have write access to this directory - in this    |
|                                  |                                                |                                 | case the data is written to a local directory and the user must |
|                                  |                                                |                                 | then contact CASDA or Operations staff.                         |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``CASDA_USE_ABSOLUTE_PATHS``     | true                                           | useAbsolutePaths                | If true, refer to filenames in the observation.xml file by their|
|                                  |                                                | (:doc:`../utils/casdaupload`)   | absolute paths. This will mean they remain where they are, and  |
|                                  |                                                |                                 | are not copied to the upload directory. The exceptions are the  |
|                                  |                                                |                                 | XML file itself, and the tarred-up MS files.                    |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``WRITE_CASDA_READY``            | false                                          | writeREADYfile                  | Whether to write the READY file in the staging directory,       |
|                                  |                                                | (:doc:`../utils/casdaupload`)   | indicating that no further changes are to be made and the data  |
|                                  |                                                |                                 | is ready to go into CASDA. Setting this to true will also       |
|                                  |                                                |                                 | transition the scheduling block from PROCESSING to              |
|                                  |                                                |                                 | PENDINGARCHIVE.                                                 |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``TRANSITION_SB``                | false                                          | none                            | If true, the scheduling block status is transitioned from       |
|                                  |                                                |                                 | PROCESSING to PENDINGARCHIVE once the casdaupload task is       |
|                                  |                                                |                                 | complete. This can only be done by the 'askapops' user.         |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``POLLING_DELAY_SEC``            | 1800                                           | none                            | The time, in seconds, between slurm jobs that poll the CASDA    |
|                                  |                                                |                                 | upload directory for the DONE file, indicating ingestion into   |
|                                  |                                                |                                 | CASDA is complete.                                              |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``MAX_POLL_WAIT_TIME``           | 172800                                         | none                            | The maximum time (in seconds) to poll for the DONE file, before |
|                                  |                                                |                                 | timing out and raising an error. (Default is 2 days.)           |
+----------------------------------+------------------------------------------------+---------------------------------+-----------------------------------------------------------------+

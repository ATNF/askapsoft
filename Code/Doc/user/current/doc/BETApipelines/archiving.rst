User parameters - Archiving
===========================

The final stage of the pipeline involves preparing data for storage in
CASDA, the CSIRO ASKAP Science Data Archive. This involves three
steps:

* All images are converted to FITS format. FITS is the format required
  for storage in CASDA - the ASKAPsoft tasks will ultimately write
  directly to FITS, but for now an additional step is required.
  This job is launched as an array job, effectively with a separate
  job for each CASA image.
  At the same time, the FITS header is given a PROJECT keyword,
  linking the image to the appropriate OPAL project. Future versions
  will have a more complete set of metadata.
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
  pattern, any Selavy catalogues, measurement sets, the thumbnail
  images, and the stats summary file detailing how the processing
  tasks went.

There are a number of user parameters that govern aspects of these
scripts, and they are detailed here.

+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| Variable                       |             Default             | Parset equivalent               | Description                                                     |
+================================+=================================+=================================+=================================================================+
| ``DO_CONVERT_TO_FITS``         | false                           | none                            | Whether to convert the CASA images to FITS format.              |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_FITS_CONVERT``      | ``JOB_TIME_DEFAULT`` (12:00:00) | none                            | Time request for the FITS conversion.                           |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``DO_MAKE_THUMBNAILS``         | false                           | none                            | Whether to make the PNG thumbnail images of the 2D FITS images. |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_THUMBNAILS``        | ``JOB_TIME_DEFAULT`` (12:00:00) | none                            | Time request for the thumbnail creation.                        |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``DO_STAGE_FOR_CASDA``         | false                           | none                            | Whether to stage data for ingest into CASDA                     |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``JOB_TIME_CASDA_UPLOAD``      | ``JOB_TIME_DEFAULT`` (12:00:00) | none                            | Time request for the CASDA-upload task.                         |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| **General**                    |                                 |                                 |                                                                 |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``IMAGE_LIST``                 | "image psf psf.image residual   | none                            | The list of image prefixes that will be used for generating FITS|
|                                | sensitivity"                    |                                 | files and determining the list of images to be uploaded to      |
|                                |                                 |                                 | CASDA.                                                          |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``ARCHIVE_BEAM_IMAGES``        | false                           | none                            | Whether the individual beam images should be included in the    |
|                                |                                 |                                 | archiving (true) or if only the mosaicked image should be       |
|                                |                                 |                                 | uploaded.                                                       |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``PROJECT_ID``                 | "AS031"                         | *<key>*.project                 | The project ID that is written to the FITS header, and used by  |
|                                |                                 | (:doc:`../utils/casdaupload`)   | the casdaupload script to describe each data product.           |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| **Thumbnails**                 |                                 |                                 |                                                                 |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_SUFFIX``           | png                             | none                            | Suffix for thumbnail image files, which in turn determinings the|
|                                |                                 |                                 | format of these files.                                          |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_GREYSCALE_MIN``    | -10                             | none                            | Minimum greyscale level fro the thumbnail image colourmap. In   |
|                                |                                 |                                 | units of the overall image rms noise.                           |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_GREYSCALE_MAX``    | 40                              | none                            | Maximum greyscale level fro the thumbnail image colourmap. In   |
|                                |                                 |                                 | units of the overall image rms noise.                           |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_SIZE_INCHES``      | (16 5)                          | none                            | The sizes (in inches) of the thumbnail images. This parameter is|
|                                |                                 |                                 | passed as a bash array, so is surrounded by () with just spaces |
|                                |                                 |                                 | between the entries. The sizes correspond to the size names     |
|                                |                                 |                                 | given below. Don't change unless you know what you are doing.   |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``THUMBNAIL_SIZE_TEXT``        | (large small)                   | none                            | The labels that go with the thumbnail sizes. These are          |
|                                |                                 |                                 | incorporated into the thumbnail name, so that image.fits gets a |
|                                |                                 |                                 | thumbnail image_large.png etc. Don't change unless you know what|
|                                |                                 |                                 | you are doing.                                                  |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| **CASDA upload**               |                                 |                                 |                                                                 |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``OBS_PROGRAM``                | "Commissioning"                 | obsprogram                      | The name of the observational program to be associated with this|
|                                |                                 | (:doc:`../utils/casdaupload`)   | data set.                                                       |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``WRITE_CASDA_READY``          | false                           | writeREADYfile                  | Whether to write the READY file in the staging directory,       |
|                                |                                 | (:doc:`../utils/casdaupload`)   | indicating that no further changes are to be made and the data  |
|                                |                                 |                                 | is ready to go into CASDA.                                      |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+
| ``CASDA_OUTPUT_DIR``           | /scratch2/casda/prd             | outputdir                       | The output directory to put the staged data. It may be that some|
|                                |                                 | (:doc:`../utils/casdaupload`)   | users will not have write access to this directory - in this    |
|                                |                                 |                                 | case the data is written to a local directory and the user must |
|                                |                                 |                                 | then contact CASDA staff.                                       |
+--------------------------------+---------------------------------+---------------------------------+-----------------------------------------------------------------+

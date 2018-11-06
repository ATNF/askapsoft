User Parameters - Spectral-line Imaging
=======================================

There are several steps involved in running the spectral-line imaging,
with several optional pre-processing steps:

1. A nominated channel range can optionally be copied to a new MS with
   mssplit.
2. The gains solution from the continuum self-calibration can be
   applied to the spectral-line MS using ccalapply.
3. The continuum can be subtracted from the spectral-line MS using
   ccontsubtract The continuum is represented by either the clean
   model from the continuum imaging, or -- as the default -- a model
   image constructed by Cmodel from the component catalogue generated
   by Selavy. The Selavy parameters used are those described on
   :doc:`ContinuumSourcefinding`.  

Following this pre-processing, the resulting MS is imaged by either the
simager task (the default), or the new imager, creating a set of
spectral cubes. The new imager provides the ability to image in the
barycentric reference frame, and allows (for efficiency purposes) the
option of writing out multiple sub-cubes (each having a subset of the
full range of channels).

A final task can perform image-based continuum subtraction. There are
two choices for this step, made via the ``SPECTRAL_IMSUB_SCRIPT``
parameter. The first uses the *robust_contsub.py* script in the ACES
directory to fit and subtract a low-order polynomial to each spectrum
in the cube separately. The second uses the *contsub_im.py* script
which uses a Savitzky-Golay filter to find and remove the spectral
baseline, again in each spectrum of the cube separately. These tasks
are intended as demonstrations of this capability - there will be an
ASKAPsoft equivalent to this in a future release. The task assumes
that the requested python script is in *$ACES/tools* - if it is not
found, the task will not run.

The variables presented below work in the same manner as those for the
continuum imaging, albeit with names that clearly refer to the
spectral-imaging.

A note on the imagers and the output formats. The default approach is
to use **simager** to produce the spectral-line cubes. The new imager
application **imager** (:doc:`../calim/imager`) can be used by setting
``DO_ALT_IMAGER_SPECTRAL`` or ``DO_ALT_IMAGER`` to true. The latter is
the switch controlling all types of imaging, but can be overridden by
the former, if provided.

The default output format is CASA images, although FITS files can be
written directly by setting ``IMAGETYPE_SPECTRAL`` to ``fits`` (rather
than ``casa``). This will only work with the new imager, as
**simager** does not yet have this functionality. This mode is still
in development, so may not be completely reliable. The recommended
method for getting images into FITS format is still to use the
``DO_CONVERT_TO_FITS`` flag, which makes use of the
:doc:`../calim/imagetofits` application. A single FITS file can be
produced by setting ``ALT_IMAGER_SINGLE_FILE=true``.



+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| Variable                                      | Default                             | Parset equivalent                  | Description                                                       |
+===============================================+=====================================+====================================+===================================================================+
| ``DO_SPECTRAL_IMAGING``                       | false                               | none                               | Whether to do the spectral-line imaging                           |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_IMAGE``                   | ``JOB_TIME_DEFAULT`` (12:00:00)     | none                               | Time request for imaging the spectral-line data                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``IMAGETYPE_SPECTRAL``                        | fits                                | imagetype (:doc:`../calim/imager`) | Image format to use - can be either 'casa' or 'fits', although    |
|                                               |                                     |                                    | 'fits' can only be given in conjunction with                      |
|                                               |                                     |                                    | ``DO_ALT_IMAGER_SPECTRAL=true``.                                  |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Preparation of spectral dataset**           |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_COPY_SL``                                | false                               | none                               | Whether to copy a channel range of the original                   |
|                                               |                                     |                                    | full-spectral-resolution measurement set into a new MS. If        |
|                                               |                                     |                                    | the original MS is original.ms, this will create original_SL.ms.  |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_SPLIT``                   | ``JOB_TIME_DEFAULT`` (12:00:00)     | none                               | Time request for splitting out a subset of the spectral data      |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CHAN_RANGE_SL_SCIENCE``                     | "1-``NUM_CHAN_SCIENCE``"            | channel (:doc:`../calim/mssplit`)  | The range of channels to copy from the original dataset (1-based).|
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``TILENCHAN_SL``                              | 10                                  | stman.tilenchan                    | The number of channels in the tile size used for the new MS. The  |
|                                               |                                     | (:doc:`../calim/mssplit`)          | tile size defines the minimum amount read at a time. Although the |
|                                               |                                     |                                    | simager will only process single channels, the default is made    |
|                                               |                                     |                                    | larger than 1 (the default for mssplit) so that the mssplit job   |
|                                               |                                     |                                    | completes in a reasonable length of time.                         |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_APPLY_CAL_SL``                           | false                               | none                               | Whether to apply the gains calibration determined from the        |
|                                               |                                     |                                    | continuum self-calibration (see ``GAINS_CAL_TABLE`` in            |
|                                               |                                     |                                    | :doc:`ScienceFieldContinuumImaging`).                             |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_APPLYCAL``                | ``JOB_TIME_DEFAULT`` (12:00:00)     | none                               | Time request for applying the gains calibration to the spectral   |
|                                               |                                     |                                    | data                                                              |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NUM_CORES_CAL_APPLY``                       | 55                                  | none                               | Number of cores for the job to apply the gains calibration to the |
|                                               |                                     |                                    | spectral data.                                                    |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_CONT_SUB_SL``                            | false                               | none                               | Whether to subtract a continuum model from the spectral-line      |
|                                               |                                     |                                    | dataset. If true, the clean model from the continuum imaging will |
|                                               |                                     |                                    | be used to represent the continuum, and this will be subtracted   |
|                                               |                                     |                                    | from the spectral-line dataset (either the original               |
|                                               |                                     |                                    | full-spectral-resolution one, or the reduced-channel-range copy), |
|                                               |                                     |                                    | which gets overwritten.                                           |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_CONTSUB``                 | ``JOB_TIME_DEFAULT`` (12:00:00)     | none                               | Time request for subtracting the continuum from the spectral data |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Continuum subtraction**                     |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_METHOD``                            | Cmodel                              | none                               | This defines which method is used to determine the continuum that |
|                                               |                                     |                                    | is to be subtracted. It can take one of three values: **Cmodel**  |
|                                               |                                     |                                    | (the default), which uses a model image constructed by Cmodel     |
|                                               |                                     |                                    | (:doc:`../calim/cmodel`) from a continuum components catalogue    |
|                                               |                                     |                                    | generated by Selavy (:doc:`../analysis/selavy`); **Components**,  |
|                                               |                                     |                                    | which uses the Selavy catalogue directly by in the form of        |
|                                               |                                     |                                    | components; or **CleanModel**, in which case the clean model from |
|                                               |                                     |                                    | the continuum imaging will be used.                               |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_NSUBX``                      | 6                                   | nsubx (:doc:`../analysis/selavy`)  | Division of image in x-direction for source-finding               |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_NSUBY``                      | 3                                   | nsuby (:doc:`../analysis/selavy`)  | Division of image in y-direction for source-finding               |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_THRESHOLD``                  | 6                                   | snrCut (:doc:`../analysis/selavy`) | SNR threshold for detection with Selavy in determining components |
|                                               |                                     |                                    | to go into the continuum model.                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_MODEL_FLUX_LIMIT``                  | 10uJy                               | flux_limit (:doc:`../calim/cmodel`)| Flux limit applied to component catalogue - only components       |
|                                               |                                     |                                    | brighter than this will be included in the model image. Parameter |
|                                               |                                     |                                    | takes the form of a number+units string.                          |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_FLAG_ADJACENT``              | true                                | flagAdjacent                       | Whether to enforce pixels in islands to be contiguous.            |
|                                               |                                     | (:doc:`../analysis/selavy`)        |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_SPATIAL_THRESHOLD``          | 5                                   | threshSpatial                      | If ``CONTSUB_SELAVY_FLAG_ADJACENT=false``, this is the threshold  |
|                                               |                                     | (:doc:`../analysis/selavy`)        | in pixels within which islands are joined.                        |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Basic variables for imaging**               |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NUM_CPUS_SPECIMG_SCI``                      | 200                                 | none                               | The total number of cores allocated to the spectral-imaging       |
|                                               |                                     |                                    | job. One will be the master, while the rest will be devoted to    |
|                                               |                                     |                                    | imaging individual channels.                                      |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CPUS_PER_CORE_SPEC_IMAGING``                | 20                                  | none                               | The number of cores per node to use (max 20).                     |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``IMAGE_BASE_SPECTRAL``                       | i.SB%s.cube                         | Helps form Images.name             | The base name for image cubes: if ``IMAGE_BASE_SPECTRAL=i.blah``  |
|                                               |                                     | (:doc:`../calim/simager`)          | then we'll get image.i.blah, image.i.blah.restored, psf.i.blah    |
|                                               |                                     |                                    | etc. The %s wildcard will be resolved into the scheduling block   |
|                                               |                                     |                                    | ID.                                                               |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DIRECTION_SCI``                             | none                                | Images.direction                   | The direction parameter for the image cubes, i.e. the central     |
|                                               |                                     | (:doc:`../calim/simager`)          | position. Can be left out, in which case it will be determined    |
|                                               |                                     |                                    | from the measurement set by mslist. This is the same input        |
|                                               |                                     |                                    | parameter as that used for the continuum imaging.                 |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NUM_PIXELS_SPECTRAL``                       | 1536                                | Images.shape                       | The number of spatial pixels along the side for the image cubes.  |
|                                               |                                     | (:doc:`../calim/simager`)          | Needs to be specified (unlike the continuum imaging case).        |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CELLSIZE_SPECTRAL``                         | 4                                   | Images.cellsize                    | The spatial pixel size for the image cubes. Must be specified.    |
|                                               |                                     | (:doc:`../calim/simager`)          |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``REST_FREQUENCY_SPECTRAL``                   | HI                                  | Images.restFrequency               | The rest frequency for the cube. Can be a quantity string (eg.    |
|                                               |                                     | (:doc:`../calim/simager`)          | 1234.567MHz), or the special string 'HI' (which is 1420.405751786 |
|                                               |                                     |                                    | MHz). If blank, no rest frequency will be written to the cube.    |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Gridding**                                  |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_SNAPSHOT_IMAGING``         | true                                | snapshotimaging                    | Whether to use snapshot imaging when gridding.                    |
|                                               |                                     | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_SNAPSHOT_WTOL``            | 2600                                | snapshotimaging.wtolerance         | The wtolerance parameter controlling how frequently to snapshot.  |
|                                               |                                     | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_SNAPSHOT_LONGTRACK``       | true                                | snapshotimaging.longtrack          | The longtrack parameter controlling how the best-fit W plane is   |
|                                               |                                     | (:doc:`../calim/gridder`)          | determined when using snapshots.                                  |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_SNAPSHOT_CLIPPING``        | 0.01                                | snapshotimaging.clipping           | If greater than zero, this fraction of the full image width       |
|                                               |                                     | (:doc:`../calim/gridder`)          | is set to zero. Useful when imaging at high declination as        |
|                                               |                                     |                                    | the edges can generate artefacts.                                 |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_WMAX``                     | 2600                                | WProject.wmax                      | The wmax parameter for the gridder. The default for this depends  |
|                                               | (``GRIDDER_SNAPSHOT_IMAGING=true``) | (:doc:`../calim/gridder`)          | on whether snapshot imaging is invoked or not                     |
|                                               | or 26000                            |                                    | (``GRIDDER_SNAPSHOT_IMAGING``).                                   |
|                                               | (``GRIDDER_SNAPSHOT_IMAGING=false``)|                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_NWPLANES``                 | 99                                  | WProject.nwplanes                  | The nwplanes parameter for the gridder.                           |
|                                               |                                     | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_OVERSAMPLE``               | 4                                   | WProject.oversample                | The oversampling factor for the gridder.                          |
|                                               |                                     | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_MAXSUPPORT``               | 512                                 | WProject.maxsupport                | The maxsupport parameter for the gridder. The default for this    |
|                                               | (``GRIDDER_SNAPSHOT_IMAGING=true``) | (:doc:`../calim/gridder`)          | depends on whether snapshot imaging is invoked or not             |
|                                               | or 1024                             |                                    | (``GRIDDER_SNAPSHOT_IMAGING``).                                   |
|                                               | (``GRIDDER_SNAPSHOT_IMAGING=false``)|                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Cleaning**                                  |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SOLVER_SPECTRAL``                           | Clean                               | solver                             | Which solver to use. You will mostly want to leave this as        |
|                                               |                                     | (:doc:`../calim/solver`)           | 'Clean', but there is a 'Dirty' solver available.                 |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_ALGORITHM``                  | BasisfunctionMFS                    | Clean.algorithm                    | The name of the clean algorithm to use.                           |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_MINORCYCLE_NITER``           | 2000                                | Clean.niter                        | The number of iterations for the minor cycle clean.               |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_GAIN``                       | 0.1                                 | Clean.gain                         | The loop gain (fraction of peak subtracted per minor cycle).      |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_PSFWIDTH``                   | 256                                 | Clean.psfwidth                     | The width of the psf patch used in the minor cycle.               |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_SCALES``                     | "[0,3,10,30]"                       | Clean.scales                       | Set of scales (in pixels) to use with the multi-scale clean.      |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE``       | "[50%, 30mJy, 3.5mJy]"              | threshold.minorcycle               | Threshold for the minor cycle loop.                               |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE``       | 20mJy                               | threshold.majorcycle               | The target peak residual. Major cycles stop if this is reached. A |
|                                               |                                     | (:doc:`../calim/solver`)           | negative number ensures all major cycles requested are done.      |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_NUM_MAJORCYCLES``            | 5                                   | ncycles                            | Number of major cycles.                                           |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_WRITE_AT_MAJOR_CYCLE``                | false                               | Images.writeAtMajorCycle           | If true, the intermediate images will be written (with a .cycle   |
|                                               |                                     | (:doc:`../calim/simager`)          | suffix) after the end of each major cycle.                        |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_SOLUTIONTYPE``               | MAXCHISQ                            | Clean.solutiontype (see discussion | The type of peak finding algorithm to use in the                  |
|                                               |                                     | at :doc:`../recipes/imaging`)      | deconvolution. Choices are MAXCHISQ, MAXTERM0, or MAXBASE.        |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Preconditioning**                           |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_LIST_SPECTRAL``              | "[Wiener, GaussianTaper]"           | preconditioner.Names               | List of preconditioners to apply.                                 |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_SPECTRAL_GAUSS_TAPER``       | "[30arcsec, 30arcsec, 0deg]"        | preconditioner.GaussianTaper       | Size of the Gaussian taper - either single value (for circular    |
|                                               |                                     | (:doc:`../calim/solver`)           | taper) or 3 values giving an elliptical size.                     |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS`` | 0.5                                 | preconditioner.Wiener.robustness   | Robustness value for the Wiener filter.                           |
|                                               |                                     | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_SPECTRAL_WIENER_TAPER``      | ""                                  | preconditioner.Wiener.taper        | Size of gaussian taper applied in image domain to Wiener filter.  |
|                                               |                                     | (:doc:`../calim/solver`)           | Ignored if blank (ie. “”).                                        |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Restoring**                                 |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORE_SPECTRAL``                          | true                                | restore                            | Whether to restore the image cubes.                               |
|                                               |                                     | (:doc:`../calim/simager`)          |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORING_BEAM_SPECTRAL``                   | fit                                 | restore.beam                       | Restoring beam to use: 'fit' will fit the PSF in each channel     |
|                                               |                                     | (:doc:`../calim/simager`)          | separately to determine the appropriate beam for that channel,    |
|                                               |                                     |                                    | else give a size (such as 30arcsec, or                            |
|                                               |                                     |                                    | “[30arcsec, 30arcsec, 0deg]”).                                    |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORING_BEAM_CUTOFF_SPECTRAL``            | 0.5                                 | restore.beam.cutoff                | Cutoff value used in determining the support for the fitting      |
|                                               |                                     | (:doc:`../calim/simager`)          | (ie. the rectangular area given to the fitting routine). Value is |
|                                               |                                     |                                    | a fraction of the peak.                                           |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORING_BEAM_REFERENCE``                  | mid                                 | restore.beamReference              | Which channel to use as the reference when writing the restoring  |
|                                               |                                     | (:doc:`../calim/simager`)          | beam to the image cube. Can be an integer as the channel number   |
|                                               |                                     |                                    | (0-based), or one of 'mid' (the middle channel), 'first' or 'last'|
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **New imager parameters**                     |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_ALT_IMAGER_SPECTRAL``                    | ""                                  | none                               | If true, the spectral-line imaging is done by imager              |
|                                               |                                     |                                    | (:doc:`../calim/imager`). If false, it is done by simager         |
|                                               |                                     |                                    | (:doc:`../calim/simager`). When true, the following parameters are|
|                                               |                                     |                                    | used. If left blank (the default), the value is given by the      |
|                                               |                                     |                                    | overall parameter ``DO_ALT_IMAGER`` (see                          |
|                                               |                                     |                                    | :doc:`ControlParameters`).                                        |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NCHAN_PER_CORE_SL``                         | 9                                   | nchanpercore                       | The number of channels each core will process.                    |
|                                               |                                     | (:doc:`../calim/imager`)           |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``USE_TMPFS``                                 | false                               | usetmpfs (:doc:`../calim/imager`)  | Whether to store the visibilities in shared memory. This will give|
|                                               |                                     |                                    | a performance boost at the expense of memory usage. Better used   |
|                                               |                                     |                                    | for processing continuum data.                                    |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``TMPFS``                                     | /dev/shm                            | tmpfs (:doc:`../calim/imager`)     | Location of the shared memory.                                    |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NUM_SPECTRAL_WRITERS``                      | 16                                  | nwriters (:doc:`../calim/imager`)  | The number of writers used by imager. Unless                      |
|                                               |                                     |                                    | ``ALT_IMAGER_SINGLE_FILE=true``, this will equate to the number of|
|                                               |                                     |                                    | distinct spectral cubes produced. In the case of multiple cubes,  |
|                                               |                                     |                                    | each will be a sub-band of the full bandwidth. No combination of  |
|                                               |                                     |                                    | the sub-cubes is currently done. The number of writers will be    |
|                                               |                                     |                                    | reduced to the number of workers in the job if necessary.         |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``ALT_IMAGER_SINGLE_FILE``                    | true                                | singleoutputfile                   | Whether to write a single cube, even with multiple writers (ie.   |
|                                               |                                     | (:doc:`../calim/imager`)           | ``NUM_SPECTRAL_WRITERS>1``). Only works when                      |
|                                               |                                     |                                    | ``IMAGETYPE_SPECTRAL=fits``                                       |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_BARY``                                   | true                                | barycentre (:doc:`../calim/imager`)| Whether to write the spectral cubes in the Barycentric reference  |
|                                               |                                     |                                    | frame.                                                            |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Image-based continuum subtraction**         |                                     |                                    |                                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_SPECTRAL_IMSUB``                         | false                               | none                               | Whether to run an image-based continuum-subtraction task on the   |
|                                               |                                     |                                    | spectral cube after creation.                                     |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``JOB_TIME_SPECTRAL_IMCONTSUB``               | ``JOB_TIME_DEFAULT`` (12:00:00)     | none                               | Time request for image-based continuum subtraction                |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_SCRIPT``                     | "robust_contsub.py"                 | none                               | The name of the script from the ACES repository to use for        |
|                                               |                                     |                                    | image-based continuum subtraction. The only two accepted values   |
|                                               |                                     |                                    | are "robust_contsub.py" and "contsub_im.py". Anything else reverts|
|                                               |                                     |                                    | to the default.                                                   |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_VERBOSE``                    | true                                | none                               | Whether to use verbose output in the logging for the image-based  |
|                                               |                                     |                                    | continuum subtraction.                                            |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_THRESHOLD``                  | 2.0                                 | none ('threshold' parameter in     | Threshold [sigma] to mask outliers prior to fitting the continuum |
|                                               |                                     | robust_contsub.py)                 | baseline in the "robust_contsub.py" version of the image-based    |
|                                               |                                     |                                    | continuum-subtraction.                                            |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_FIT_ORDER``                  | 2                                   | none ('fit_order' parameter in     | Order of the polynomial to fit to the continuum baseline in the   |
|                                               |                                     | robust_contsub.py)                 | "robust_contsub.py" version of the image-based continuum          |
|                                               |                                     |                                    | subtraction.                                                      |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_CHAN_SAMPLING``              | 1                                   | none ('n_every' parameter in       | If set to n, we use only every nth channel in the polynomial fit  |
|                                               |                                     | robust_contsub.py)                 | (1 uses every channel). Only for "robust_contsub.py"              |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_LOG_SAMPLING``               | 1                                   | none ('log_every' parameter in     | How frequently the log messages from "robust_contsub.py" should be|
|                                               |                                     | robust_contsub.py)                 | written (1 means every channel).                                  |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_SG_FILTERWIDTH``             | 200                                 | none ('filterwidth' parameter in   | The half-width of the Savitzky-Golay filter for baseline smoothing|
|                                               |                                     | contsub_im.py)                     | in the "contsub_im.py" script.                                    |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SPECTRAL_IMSUB_SG_BINWIDTH``                | 4                                   | none ('binwidth' parameter in      | The bin width used for binning the spectrum before continuum      |
|                                               |                                     | contsub_im.py)                     | subtraction ("contsub_im.py" only).                               |
+-----------------------------------------------+-------------------------------------+------------------------------------+-------------------------------------------------------------------+

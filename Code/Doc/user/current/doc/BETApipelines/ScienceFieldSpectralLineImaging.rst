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
   by Selavy.

Following this pre-processing, the resulting MS is imaged by the
simager task, creating a set of spectral cubes. At this point, no
mosaicking of these is done, but this will be added in a future
version of the pipeline.

The variables presented below work in the same manner as those for the
continuum imaging, albeit with names that clearly refer to the
spectral-imaging. 


+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| Variable                                      | Default                       | Parset equivalent                  | Description                                                       |
+===============================================+===============================+====================================+===================================================================+
| ``DO_SPECTRAL_IMAGING``                       | false                         | none                               | Whether to do the spectral-line imaging                           |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Preparation of spectral dataset**           |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_COPY_SL``                                | false                         | none                               | Whether to copy a channel range of the original                   |
|                                               |                               |                                    | full-spectral-resolution measurement set into a new MS. If        |
|                                               |                               |                                    | the original MS is original.ms, this will create original_SL.ms.  |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CHAN_RANGE_SL_SCIENCE``                     | "1-``NUM_CHAN_SCIENCE``"      | channel (:doc:`../calim/mssplit`)  | The range of channels to copy from the original dataset (1-based).|
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``TILENCHAN_SL``                              | 1                             | stman.tilenchan                    | The number of channels in the tile size used for the new MS. The  |
|                                               |                               | (:doc:`../calim/mssplit`)          | tile size defines the minimum amount read at a time. Since the    |
|                                               |                               |                                    | simager will process single channels, making this 1 (the default) |
|                                               |                               |                                    | means the simager workers only read what they need to .           |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_APPLY_CAL_SL``                           | false                         | none                               | Whether to apply the gains calibration determined from the        |
|                                               |                               |                                    | continuum self-calibration (see ``GAINS_CAL_TABLE`` in            |
|                                               |                               |                                    | :doc:`ScienceFieldSelfCalibration`).                              |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DO_CONT_SUB_SL``                            | false                         | none                               | Whether to subtract a continuum model from the spectral-line      |
|                                               |                               |                                    | dataset. If true, the clean model from the continuum imaging will |
|                                               |                               |                                    | be used to represent the continuum, and this will be subtracted   |
|                                               |                               |                                    | from the spectral-line dataset (either the original               |
|                                               |                               |                                    | full-spectral-resolution one, or the reduced-channel-range copy), |
|                                               |                               |                                    | which gets overwritten.                                           |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Continuum subtraction**                     |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_METHOD``                            | Cmodel                        | none                               | This defines which method is used to determine the continuum that |
|                                               |                               |                                    | is to be subtracted. It can take one of three values: **Cmodel**  |
|                                               |                               |                                    | (the default), which uses a model image constructed by Cmodel     |
|                                               |                               |                                    | (:doc:`../calim/cmodel`) from a continuum components catalogue    |
|                                               |                               |                                    | generated by Selavy (:doc:`../analysis/selavy`); **Components**,  |
|                                               |                               |                                    | which uses the Selavy catalogue directly by in the form of        |
|                                               |                               |                                    | components; or **CleanModel**, in which case the clean model from |
|                                               |                               |                                    | the continuum imaging will be used.                               |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_NSUBX``                      | 6                             | nsubx (:doc:`../analysis/selavy`)  | Division of image in x-direction for source-finding               |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_NSUBY``                      | 3                             | nsuby (:doc:`../analysis/selavy`)  | Division of image in y-direction for source-finding               |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_SELAVY_THRESHOLD``                  | 6                             | snrCut (:doc:`../analysis/selavy`) | SNR threshold for detection with Selavy in determining components |
|                                               |                               |                                    | to go into the continuum model.                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CONTSUB_MODEL_FLUX_LIMIT``                  | 0mJy                          | flux_limit (:doc:`../calim/cmodel`)| Flux limit applied to component catalogue - only components       |
|                                               |                               |                                    | brighter than this will be included in the model image. Parameter |
|                                               |                               |                                    | takes the form of a number+units string. Default (0mJy) implies   |
|                                               |                               |                                    | *all* compoennts are used.                                        |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Basic variables for imaging**               |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NUM_CPUS_SPECIMG_SCI``                      | 2000                          | none                               | The total number of processors allocated to the spectral-imaging  |
|                                               |                               |                                    | job. One will be the master, while the rest will be devoted to    |
|                                               |                               |                                    | imaging individual channels.                                      |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CPUS_PER_CORE_SPEC_IMAGING``                | 20                            | none                               | The number of processors per node to use (max 20).                |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``IMAGE_BASE_SPECTRAL``                       | i.cube                        | Helps form                         | The base name for image cubes: if ``IMAGE_BASE_SPECTRAL=i.blah``  |
|                                               |                               | Images.name                        | then we'll get image.i.blah, image.i.blah.restored, psf.i.blah etc|
|                                               |                               | (:doc:`../calim/simager`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``DIRECTION_SCI``                             | none                          | Images.direction                   | The direction parameter for the image cubes, i.e. the central     |
|                                               |                               | (:doc:`../calim/simager`)          | position. Can be left out, in which case it will be determined    |
|                                               |                               |                                    | from the measurement set by mslist. This is the same input        |
|                                               |                               |                                    | parameter as that used for the continuum imaging.                 |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``NUM_PIXELS_SPECTRAL``                       | 2048                          | Images.shape                       | The number of spatial pixels along the side for the image cubes.  |
|                                               |                               | (:doc:`../calim/simager`)          | Needs to be specified (unlike the continuum imaging case).        |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CELLSIZE_SPECTRAL``                         | 10                            | Images.cellsize                    | The spatial pixel size for the image cubes. Must be specified.    |
|                                               |                               | (:doc:`../calim/simager`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``REST_FREQUENCY_SPECTRAL``                   | HI                            | Images.restFrequency               | The rest frequency for the cube. Can be a quantity string (eg.    |
|                                               |                               | (:doc:`../calim/simager`)          | 1234.567MHz), or the special string 'HI' (which is 1420.405751786 |
|                                               |                               |                                    | MHz). If blank, no rest frequency will be written to the cube.    |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Gridding**                                  |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_SNAPSHOT_IMAGING``         | true                          | snapshotimaging                    | Whether to use snapshot imaging when gridding.                    |
|                                               |                               | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_SNAPSHOT_WTOL``            | 2600                          | snapshotimaging.wtolerance         |  The wtolerance parameter controlling how frequently to snapshot. |
|                                               |                               | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_WMAX``                     | 2600                          | WProject.wmax                      | The wmax parameter for the gridder.                               |
|                                               |                               | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_NWPLANES``                 | 99                            | WProject.nwplanes                  | The nwplanes parameter for the gridder.                           |
|                                               |                               | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_OVERSAMPLE``               | 4                             | WProject.oversample                | The oversampling factor for the gridder.                          |
|                                               |                               | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``GRIDDER_SPECTRAL_MAXSUPPORT``               | 512                           | WProject.maxsupport                | The maxsupport parameter for the gridder.                         |
|                                               |                               | (:doc:`../calim/gridder`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Cleaning**                                  |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``SOLVER_SPECTRAL``                           | Clean                         | solver                             | Which solver to use. You will mostly want to leave this as        |
|                                               |                               | (:doc:`../calim/solver`)           | 'Clean', but there is a 'Dirty' solver available.                 |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_ALGORITHM``                  | Basisfunction                 | Clean.algorithm                    | The name of the clean algorithm to use. Note that the default has |
|                                               |                               | (:doc:`../calim/solver`)           | changed to 'Basisfunction', as we don't need the multi-frequency  |
|                                               |                               |                                    | capabilities of 'BasisfunctionMFS'.                               |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_MINORCYCLE_NITER``           | 500                           | Clean.niter                        | The number of iterations for the minor cycle clean.               |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_GAIN``                       | 0.5                           | Clean.gain                         | The loop gain (fraction of peak subtracted per minor cycle).      |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_SCALES``                     | "[0,3,10]"                    | Clean.scales                       | Set of scales (in pixels) to use with the multi-scale clean.      |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE``       | "[30%, 0.9mJy]"               | threshold.minorcycle               | Threshold for the minor cycle loop.                               |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE``       | 1mJy                          | threshold.majorcycle               | The target peak residual. Major cycles stop if this is reached. A |
|                                               |                               | (:doc:`../calim/solver`)           | negative number ensures all major cycles requested are done.      |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_SPECTRAL_NUM_MAJORCYCLES``            | 0                             | ncycles                            | Number of major cycles.                                           |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``CLEAN_WRITE_AT_MAJOR_CYCLE``                | false                         | Images.writeAtMajorCycle           | If true, the intermediate images will be written (with a .cycle   |
|                                               |                               | (:doc:`../calim/simager`)          | suffix) after the end of each major cycle.                        |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Preconditioning**                           |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_LIST_SPECTRAL``              | "[Wiener, GaussianTaper]"     | preconditioner.Names               | List of preconditioners to apply.                                 |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_SPECTRAL_GAUSS_TAPER``       | "[50arcsec, 50arcsec, 0deg]"  | preconditioner.GaussianTaper       | Size of the Gaussian taper - either single value (for circular    |
|                                               |                               | (:doc:`../calim/solver`)           | taper) or 3 values giving an elliptical size.                     |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS`` | 0.5                           | preconditioner.Wiener.robustness   | Robustness value for the Wiener filter.                           |
|                                               |                               | (:doc:`../calim/solver`)           |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``PRECONDITIONER_SPECTRAL_WIENER_TAPER``      | ""                            | preconditioner.Wiener.taper        | Size of gaussian taper applied in image domain to Wiener filter.  |
|                                               |                               | (:doc:`../calim/solver`)           | Ignored if blank (ie. “”).                                        |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| **Restoring**                                 |                               |                                    |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORE_SPECTRAL``                          | true                          | restore                            | Whether to restore the image cubes.                               |
|                                               |                               | (:doc:`../calim/simager`)          |                                                                   |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORING_BEAM_SPECTRAL``                   | fit                           | restore.beam                       | Restoring beam to use: 'fit' will fit the PSF in each channel     |
|                                               |                               | (:doc:`../calim/simager`)          | separately to determine the appropriate beam for that channel,    |
|                                               |                               |                                    | else give a size (such as 30arcsec, or                            |
|                                               |                               |                                    | “[30arcsec, 30arcsec, 0deg]”).                                    |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORING_BEAM_REFERENCE``                  | mid                           | restore.beamReference              | Which channel to use as the reference when writing the restoring  |
|                                               |                               | (:doc:`../calim/simager`)          | beam to the image cube. Can be an integer as the channel number   |
|                                               |                               |                                    | (0-based), or one of 'mid' (the middle channel), 'first' or 'last'|
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+
| ``RESTORING_BEAM_LOG``                        | beamLog.IMAGE.txt (with IMAGE | restore.beamLog                    | The ASCII text file to which will be written the restoring beam   |
|                                               | from ``IMAGE_BASE_SPECTRAL``) | (:doc:`../calim/simager`)          | for each channel. If blank, no such file will be written.         |
+-----------------------------------------------+-------------------------------+------------------------------------+-------------------------------------------------------------------+

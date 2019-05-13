User Parameters - Continuum imaging
===================================

The parameters listed here allow the user to configure the parset and
the slurm job for continuum imaging of the science field. The defaults
have been set based on experience with science commissioning and ASKAP
Early Science, using the 12-antenna array. If you are processing
different data sets, adjusting these defaults may be necessary.

Some of the parameters can be set to blank, to allow cimager to choose
appropriate values based on its "advise" capability (which involves
examining the dataset and setting appropriate values for some
parameters.

For most observations to date, there is a single phase centre that
applies for all beams. Thus, if *advise* is used to determine the
image centre, the same direction will be used for each beam. This is
the way the BETA processing was done, and in this case the image size
must be chosen to be big enough to encompass all beams. This behaviour
is applied when ``IMAGE_AT_BEAM_CENTRES=false``.

The preferred behaviour, however, is to set a different image centre
per beam, as this allows a smaller image to be made when imaging. To
do this, the *footprint.py* tool is used to determine the locations of
the centres of each beam. The footprint specification is determined
preferentially from the scheduling block parset, or (if not available
there) from the parameters described at
:doc:`ScienceFieldMosaicking`. To use this mode, set
``IMAGE_AT_BEAM_CENTRES=true``.

When setting the gridding parameters, note that the gridder is
currently hardcoded to use **WProject**.  If you want to experiment
with other gridders, you need to edit the slurm file or parset
yourself (ie. set ``SUBMIT_JOBS=false`` to produce the slurm files
without submission, then edit and manually submit).

While the default application used for the imaging is **cimager**, it
is possible to use the newer **imager** (:doc:`../calim/imager`). Much
of the functionality will be identical for continuum data (imager was
developed initially as a better spectral-line imaging tool).

An option exists to do the continuum-imaging with self-calibration.
The algorithm here is as follows:

1. Image the data with cimager or imager
2. Run source-finding with Selavy with a relatively large threshold
   (unless we are using option 3c. below)
3. Use the results to calibrate the antenna-based gains by either:

   a. Create a component parset from the resulting component catalogue and use this parset in ccalibrator
   b. Create a model image from the component catalogue, and use in ccalibrator
   c. Use the clean model image from the most recent imaging as the
      model for ccalibrator (no source-finding will be done)

4. Re-run imaging, applying the latest gains table
5. Repeat steps 2-5 for a given number of loops

Each loop gets its own directory, where the intermediate images,
source-finding results, and gains calibration table are stored. At the
end, the final gains calibration table is kept in the main output
directory (as this can then be used by the spectral-line imaging
pipeline).

There are two options for how this is run. The first
(``MULTI_JOB_SELFCAL=true``) launches a separate slurm job for each
imaging task, and each "calibration" task - the calibration job
incorporates the selavy, cmodel and ccalibrator tasks. The latter uses
a single node only. If ``MULTI_JOB_SELFCAL=false``, these are all
incorporated into one slurm job, including all loops.

Some parameters are allowed to vary with the loop number of the
self-calibration. This way, you can decrease, say, the detection
threshold, or increase the number of major cycles, as the calibration
steadily improves. These parameters are for:

* Deconvolution: ``CLEAN_ALGORITHM``, ``CLEAN_GAIN``, ``CLEAN_PSFWIDTH``,
  ``CLEAN_THRESHOLD_MAJORCYCLE``, ``CLEAN_NUM_MAJORCYCLES``,
  ``CLEAN_MINORCYCLE_NITER``, ``CLEAN_THRESHOLD_MINORCYCLE`` and
  ``CLEAN_SCALES``
* Self-calibration: ``SELFCAL_SELAVY_THRESHOLD``, ``SELFCAL_INTERVAL``
  and ``SELFCAL_NORMALISE_GAINS``
* Data selection in imaging: ``CIMAGER_MINUV`` and ``CIMAGER_MAXUV``
* Data selection in calibration: ``CCALIBRATOR_MINUV`` and ``CCALIBRATOR_MAXUV``

To use this mode, the values for these parameters should be given as
an array in the form ``SELFCAL_INTERVAL="[1800,1800,900,300]"``, or,
``CLEAN_SCALES="[0] ; [0,20] ; [0,20,120,240] ; [0,20,120,240,480]"``.
The size of these arrays should be one more than
``SELFCAL_NUM_LOOPS``. This is because loop 0 is just the imaging, and
it is followed by ``SELFCAL_NUM_LOOPS`` loops of
source-find--model--calibrate--image. Only the parameters related to
the imaging (the deconvolution parameters in the list above) have
the first element of their array used. If a single value is given for
these parameters, it is used for every loop.


Once the gains solution has been determined, it can be applied
directly to the continuum measurement set, creating a copy in the
process. This is necessary for continuum cube processing, and for
archiving purposes.
This work is done as a separate slurm job, that starts upon
completion of the self-calibration job.

Following this application of the gains calibration solution, one can
optionally image the continuum dataset as a cube, preserving the
frequency sampling. This task also allows the specification of
multiple polarisations, with a cube created for each polarisation
given. The **simager** task is used for the continuum cube
imaging. Many of the imaging specifications (shape, direction etc, as
well as gridding & preconditioning) given for continuum imaging are
used for the continuum cube, although the cleaning parameters can be
given as different.


A note on the imagers and the output formats. The default approach is
to use **cimager** for the continuum imaging and **simager** for the
continuum cubes. The new imager application **imager**
(:doc:`../calim/imager`) can be used by setting ``DO_ALT_IMAGER_CONT``
or ``DO_ALT_IMAGER_CONTCUBE``, or the main switch ``DO_ALT_IMAGER`` to
true (this is now the default). The latter is the switch controlling
all types of imaging, but can be overridden by the type-specific
versions, if they are provided.

The default output format is CASA images, although FITS files can be
written directly by setting ``IMAGETYPE_CONT`` or
``IMAGETYPE_CONTCUBE`` to ``fits`` (rather than ``casa``). This mode
is still in development, so may not be completely reliable. The
recommended method for getting images into FITS format is still to use
the ``DO_CONVERT_TO_FITS`` flag, which makes use of the
:doc:`../calim/imagetofits` application.


+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| Variable                                   | Default                             | Parset equivalent                                      | Description                                                   |
+============================================+=====================================+========================================================+===============================================================+
| ``DO_CONT_IMAGING``                        | true                                | none                                                   | Whether to image the science MS                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``JOB_TIME_CONT_IMAGE``                    | ``JOB_TIME_DEFAULT`` (24:00:00)     | none                                                   | Time request for imaging the continuum (both types - with and |
|                                            |                                     |                                                        | without self-calibration)                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``IMAGETYPE_CONT``                         | fits                                | imagetype (:doc:`../calim/cimager` and                 | Image format to use - can be either 'casa' or 'fits'.         |
|                                            |                                     | :doc:`../calim/imager`)                                |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``IMAGETYPE_CONTCUBE``                     | fits                                | imagetype (:doc:`../calim/imager`)                     | Image format to use - can be either 'casa' or 'fits',         |
|                                            |                                     |                                                        | although 'fits' can only be given in conjunction with         |
|                                            |                                     |                                                        | ``DO_ALT_IMAGER_CONTCUBE=true``.                              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ```MULTI_JOB_SELFCAL``                     | true                                | none                                                   | Whether to break the selfcal up into separate slurm jobs for  |
|                                            |                                     |                                                        | each imaging and calibration task (``true``) or whether to    |
|                                            |                                     |                                                        | combine them all into a single slurm job.                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``JOB_TIME_CONT_SELFCAL``                  | ``JOB_TIME_DEFAULT`` (24:00:00)     | none                                                   | Time request for the calibration jobs when running with       |
|                                            |                                     |                                                        | ``MULTI_JOB_SELFCAL=true``.                                   |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Basic variables**                        |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``IMAGE_AT_BEAM_CENTRES``                  | true                                | none                                                   | Whether to have each beam's image centred at the centre of    |
|                                            |                                     |                                                        | the beam (IMAGE_AT_BEAM_CENTRES=true), or whether to use a    |
|                                            |                                     |                                                        | single image centre for all beams.                            |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NUM_CPUS_CONTIMG_SCI``                   | ``""``                              | none                                                   | The number of cores in total to use for the continuum         |
|                                            |                                     |                                                        | imaging. If left blank (``""`` - the default), then this is   |
|                                            |                                     |                                                        | calculated based on the number of channels and Taylor terms.  |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CHANNEL_SELECTION_CONTIMG_SCI``          | ``""``                              | Channels (:doc:`../calim/data_selection`)              | If ``NUM_CPUS_CONTIMG_SCI`` is given, the Channels selection  |
|                                            |                                     |                                                        | is provided here. This can be left blank for no selection to  |
|                                            |                                     |                                                        | be applied, or a string (in quotes) conforming to the data    |
|                                            |                                     |                                                        | selection syntax can be provided.                             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CPUS_PER_CORE_CONT_IMAGING``             | 6                                   | Not for parset                                         | Number of cores to use on each node in the continuum imaging. |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``FAT_NODE_CONT_IMG``                      | true                                | Not for parset                                         | Whether the master process for the continuum imaging should be|
|                                            |                                     |                                                        | put on a node of its own (if ```true```), or just treated like|
|                                            |                                     |                                                        | all other processes.                                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DATACOLUMN``                             | DATA                                | datacolumn (:doc:`../calim/cimager`)                   | The column in the measurement set from which to read the      |
|                                            |                                     |                                                        | visibility data. The default, 'DATA', is appropriate for      |
|                                            |                                     |                                                        | datasets processed within askapsoft, but if you are trying to |
|                                            |                                     |                                                        | image data processed, for instance, in CASA, then changing    |
|                                            |                                     |                                                        | this to CORRECTED_DATA may be what you want.                  |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``IMAGE_BASE_CONT``                        | i.SB%s.cont                         | Helps form Images.Names                                | The base name for images: if ``IMAGE_BASE_CONT=i.blah`` then  |
|                                            |                                     | (:doc:`../calim/cimager`)                              | we'll get image.i.blah, image.i.blah.restored, psf.i.blah etc.|
|                                            |                                     |                                                        | The %s wildcard will be resolved into the scheduling block ID.|
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DIRECTION_SCI``                          | none                                | Images.<imagename>.direction                           | The direction parameter for the images, i.e. the central      |
|                                            |                                     | (:doc:`../calim/cimager`)                              | position. Can be left out, in which case Cimager will get it  |
|                                            |                                     |                                                        | from either the beam location (for                            |
|                                            |                                     |                                                        | IMAGE_AT_BEAM_CENTRES=true) or from the measurement set using |
|                                            |                                     |                                                        | the "advise" functionality (for IMAGE_AT_BEAM_CENTRES=false). |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NUM_PIXELS_CONT``                        | 6144                                | Images.shape                                           | The number of pixels on the side of the images to be created. |
|                                            |                                     | (:doc:`../calim/cimager`)                              | If negative, zero, or absent (i.e. ``NUM_PIXELS_CONT=""``),   |
|                                            |                                     |                                                        | this will be set automatically by the Cimager “advise”        |
|                                            |                                     |                                                        | function, based on examination of the MS. Note that this      |
|                                            |                                     |                                                        | default will be suitable for a single beam, but probably not  |
|                                            |                                     |                                                        | for an image to be large enough for the full set of beams     |
|                                            |                                     |                                                        | (when using IMAGE_AT_BEAM_CENTRES=false). The default value,  |
|                                            |                                     |                                                        | combined with the default for the cell size, should be        |
|                                            |                                     |                                                        | sufficient to cover a full field. If you have                 |
|                                            |                                     |                                                        | IMAGE_AT_BEAM_CENTRES=true then this needs only to be big     |
|                                            |                                     |                                                        | enough to fit a single beam.                                  |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CELLSIZE_CONT``                          | 2                                   | Images.cellsize                                        | Size of the pixels in arcsec. If negative, zero or absent,    |
|                                            |                                     | (:doc:`../calim/cimager`)                              | this will be set automatically by the Cimager “advise”        |
|                                            |                                     |                                                        | function, based on examination of the MS. The default is      |
|                                            |                                     |                                                        | chosen together with the default number of pixels to cover a  |
|                                            |                                     |                                                        | typical ASKAP beam with the sidelobes being imaged.           |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NUM_TAYLOR_TERMS``                       | 2                                   | Images.image.${imageBase}.nterms                       | Number of Taylor terms to create in MFS imaging. If more than |
|                                            |                                     | (:doc:`../calim/cimager`)                              | 1, MFS weighting will be used (equivalent to setting          |
|                                            |                                     | linmos.nterms (:doc:`../calim/linmos`)                 | **Cimager.visweights=MFS** in the cimager parset).            |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``MFS_REF_FREQ``                           | no default                          | visweights.MFS.reffreq                                 | Frequency at which continuum image is made [Hz]. This is the  |
|                                            |                                     | (:doc:`../calim/cimager`)                              | reference frequency for the multi-frequency synthesis, which  |
|                                            |                                     |                                                        | should usually be the middle of the band. If negative, zero,  |
|                                            |                                     |                                                        | or absent (the default), this will be set automatically to    |
|                                            |                                     |                                                        | the average of the frequencies being processed.               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORING_BEAM_CONT``                    | fit                                 | restore.beam                                           | Restoring beam to use: 'fit' will fit the PSF to determine    |
|                                            |                                     | (:doc:`../calim/cimager`)                              | the appropriate beam, else give a size (such as               |
|                                            |                                     |                                                        | ``“[30arcsec, 30arcsec, 0deg]”``).                            |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORING_BEAM_CUTOFF_CONT``             | 0.5                                 | restore.beam.cutoff                                    | Cutoff value used in determining the support for the fitting  |
|                                            |                                     | (:doc:`../calim/simager`)                              | (ie. the rectangular area given to the fitting routine).      |
|                                            |                                     |                                                        | Value is a fraction of the peak.                              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CIMAGER_MINUV``                          | 0                                   | MinUV (:doc:`../calim/data_selection`)                 | The minimum UV distance considered in the imaging - used to   |
|                                            |                                     |                                                        | exclude the short baselines. Can be given as an array with    |
|                                            |                                     |                                                        | different values for each self-cal loop                       |
|                                            |                                     |                                                        | (e.g. ``"[200,200,0]"``).                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CIMAGER_MAXUV``                          | 0                                   | MaxUV (:doc:`../calim/data_selection`)                 | The maximum UV distance considered in the imaging. Only used  |
|                                            |                                     |                                                        | if greater than zero. Can be given as an array with different |
|                                            |                                     |                                                        | values for each self-cal loop (e.g. ``"[200,200,0]"``).       |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Gridding parameters**                    |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_IMAGING``               | false                               | snapshotimaging                                        | Whether to use snapshot imaging when gridding.                |
|                                            |                                     | (:doc:`../calim/gridder`)                              |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_WTOL``                  | 2600                                | snapshotimaging.wtolerance                             | The wtolerance parameter controlling how frequently to        |
|                                            |                                     | (:doc:`../calim/gridder`)                              | snapshot.                                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_LONGTRACK``             | true                                | snapshotimaging.longtrack                              | The longtrack parameter controlling how the best-fit W plane  |
|                                            |                                     | (:doc:`../calim/gridder`)                              | is determined when using snapshots.                           |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_CLIPPING``              | 0.01                                | snapshotimaging.clipping                               | If greater than zero, this fraction of the full image width   |
|                                            |                                     | (:doc:`../calim/gridder`)                              | is set to zero. Useful when imaging at high declination as    |
|                                            |                                     |                                                        | the edges can generate artefacts.                             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_WMAX``                           | 2600                                | WProject.wmax                                          | The wmax parameter for the gridder. The default for this      |
|                                            | (``GRIDDER_SNAPSHOT_IMAGING=true``) | (:doc:`../calim/gridder`)                              | depends on whether snapshot imaging is invoked or not         |
|                                            | or 35000                            |                                                        | (``GRIDDER_SNAPSHOT_IMAGING``).                               |
|                                            | (``GRIDDER_SNAPSHOT_IMAGING=false``)|                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_NWPLANES``                       | 99                                  | WProject.nwplanes                                      | The nwplanes parameter for the gridder. The default for this  |
|                                            | (``GRIDDER_SNAPSHOT_IMAGING=true``) | (:doc:`../calim/gridder`)                              | depends on whether snapshot imaging is invoked or not         |
|                                            | or 257                              |                                                        | (``GRIDDER_SNAPSHOT_IMAGING``).                               |
|                                            | (``GRIDDER_SNAPSHOT_IMAGING=false``)|                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_OVERSAMPLE``                     | 5                                   | WProject.oversample                                    | The oversampling factor for the gridder.                      |
|                                            |                                     | (:doc:`../calim/gridder`)                              |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GRIDDER_MAXSUPPORT``                     | 512                                 | WProject.maxsupport                                    | The maxsupport parameter for the gridder. The default for this|
|                                            | (``GRIDDER_SNAPSHOT_IMAGING=true``) | (:doc:`../calim/gridder`)                              | depends on whether snapshot imaging is invoked or not         |
|                                            | or 1024                             |                                                        | (``GRIDDER_SNAPSHOT_IMAGING``).                               |
|                                            | (``GRIDDER_SNAPSHOT_IMAGING=false``)|                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Cleaning parameters**                    |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SOLVER``                                 | Clean                               | solver                                                 | Which solver to use. You will mostly want to leave this as    |
|                                            |                                     | (:doc:`../calim/cimager`)                              | 'Clean', but there is a 'Dirty' solver available.             |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_ALGORITHM``                        |  BasisfunctionMFS                   | Clean.algorithm                                        | The name(s) of clean algorithm(s) to use.                     |
|                                            |                                     | (:doc:`../calim/solver`)                               | To use different algorithms in different selfcal cycles, use: |
|                                            |                                     |                                                        | ``CLEAN_ALGORITHM="Hogbom,BasisfunctionMFS"``                 |
|                                            |                                     |                                                        | If the number of comma-separated algorithms is less than      |
|                                            |                                     |                                                        | ``SELFCAL_NUM_LOOPS + 1``, the first algorithm specified will |
|                                            |                                     |                                                        | be used for ALL selfcal loops.                                |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_MINORCYCLE_NITER``                 | ``"[400,800]"``                     | Clean.niter                                            | The number of iterations for the minor cycle clean. Can be    |
|                                            |                                     | (:doc:`../calim/solver`)                               | varied for each selfcal cycle. (e.g. ``"[200,800,1000]"``)    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_GAIN``                             | 0.2                                 | Clean.gain                                             | The loop gain (fraction of peak subtracted per minor cycle).  |
|                                            |                                     | (:doc:`../calim/solver`)                               | Can be varied for each selfcal                                |
|                                            |                                     |                                                        | cycle. (e.g. ``"[0.1,0.2,0.1]"``)                             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_PSFWIDTH``                         | 256                                 | Clean.psfwidth                                         | The width of the psf patch used in the minor cycle. Can be    |
|                                            |                                     | (:doc:`../calim/solver`)                               | varied for each selfcal cycle. (e.g. ``"[256,512,4096]"``)    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_SCALES``                           | ``"[0,3,10]"``                      | Clean.scales                                           | Set of scales (in pixels) to use with the multi-scale clean.  |
|                                            |                                     | (:doc:`../calim/solver`)                               | Can be varied for each selfcal cycle (e.g. ``"[0] ;           |
|                                            |                                     |                                                        | [0,10]"``) Notice the delimiter ``" ; "`` and the spaces      |
|                                            |                                     |                                                        | around it.                                                    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_THRESHOLD_MINORCYCLE``             | ``"[30%, 0.5mJy, 0.03mJy]"``        | threshold.minorcycle                                   | Threshold for the minor cycle loop. Can be varied for each    |
|                                            |                                     | (:doc:`../calim/cimager`)                              | selfcal cycle. (e.g. ``"[30%,1.8mJy,0.03mJy] ;                |
|                                            |                                     | (:doc:`../calim/solver`)                               | [20%,0.5mJy,0.03mJy]"``) Notice the delimiter ``" ; "`` and   |
|                                            |                                     |                                                        | the spaces around it.                                         |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_THRESHOLD_MAJORCYCLE``             | ``"0.035mJy"``                      | threshold.majorcycle                                   | The target peak residual. Major cycles stop if this is        |
|                                            |                                     | (:doc:`../calim/cimager`)                              | reached. A negative number ensures all major cycles requested |
|                                            |                                     | (:doc:`../calim/solver`)                               | are done. Can be given as an array with different values for  |
|                                            |                                     |                                                        | each self-cal loop (e.g. ``"[3mJy,1mJy,-1mJy]"``).            |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_NUM_MAJORCYCLES``                  | ``"[5,10]"``                        | ncycles                                                | Number of major cycles. Can be given as an array with         |
|                                            |                                     | (:doc:`../calim/cimager`)                              | different values for each self-cal loop (e.g. ``"[2,4,6]"``). |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_WRITE_AT_MAJOR_CYCLE``             | false                               | Images.writeAtMajorCycle                               | If true, the intermediate images will be written (with a      |
|                                            |                                     | (:doc:`../calim/cimager`)                              | .cycle suffix) after the end of each major cycle.             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_SOLUTIONTYPE``                     | MAXBASE                             | Clean.solutiontype (see discussion at                  | The type of peak finding algorithm to use in the              |
|                                            |                                     | :doc:`../recipes/Imaging`)                             | deconvolution. Choices are MAXCHISQ, MAXTERM0, or MAXBASE.    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Preconditioning parameters**             |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``PRECONDITIONER_LIST``                    | ``"[Wiener]"``                      | preconditioner.Names                                   | List of preconditioners to apply.                             |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``PRECONDITIONER_GAUSS_TAPER``             | ``"[10arcsec, 10arcsec, 0deg]"``    | preconditioner.GaussianTaper                           | Size of the Gaussian taper - either single value (for         |
|                                            |                                     | (:doc:`../calim/solver`)                               | circular taper) or 3 values giving an elliptical size.        |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``PRECONDITIONER_WIENER_ROBUSTNESS``       | -0.5                                | preconditioner.Wiener.robustness                       | Robustness value for the Wiener filter.                       |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``PRECONDITIONER_WIENER_TAPER``            | ``""``                              | preconditioner.Wiener.taper                            | Size of gaussian taper applied in image domain to Wiener      |
|                                            |                                     | (:doc:`../calim/solver`)                               | filter. Ignored if blank (ie. “”).                            |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORE_PRECONDITIONER_LIST``            | ``""``                              | restore.preconditioner.Names                           | List of preconditioners to apply at the restore stage, to     |
|                                            |                                     | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | produce an additional restored image.                         |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORE_PRECONDITIONER_GAUSS_TAPER``     | ``"[10arcsec, 10arcsec, 0deg]"``    | restore.preconditioner.GaussianTaper                   | Size of the Gaussian taper for the restore preconditioning -  |
|                                            |                                     | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | either single value (for circular taper) or 3 values giving   |
|                                            |                                     |                                                        | an elliptical size.                                           |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
|``RESTORE_PRECONDITIONER_WIENER_ROBUSTNESS``| -2                                  | restore.preconditioner.Wiener.robustness               | Robustness value for the Wiener filter in the restore         |
|                                            |                                     | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | preconditioning.                                              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORE_PRECONDITIONER_WIENER_TAPER``    | ``""``                              | restore.preconditioner.Wiener.taper                    | Size of gaussian taper applied in image domain to Wiener      |
|                                            |                                     | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | filter in the restore preconditioning. Ignored if blank       |
|                                            |                                     |                                                        | (ie. “”).                                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ***New imager parameters**                 |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DO_ALT_IMAGER_CONT``                     | ``""``                              | none                                                   | If true, the continuum imaging is done by imager              |
|                                            |                                     |                                                        | (:doc:`../calim/imager`). If false, it is done by cimager     |
|                                            |                                     |                                                        | (:doc:`../calim/cimager`). When true, the following           |
|                                            |                                     |                                                        | parameters are used. If left blank (the default), the value   |
|                                            |                                     |                                                        | is given by the overall parameter ``DO_ALT_IMAGER`` (see      |
|                                            |                                     |                                                        | :doc:`ControlParameters`).                                    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DO_ALT_IMAGER_CONTCUBE``                 | ``""``                              | none                                                   | If true, the continuum cube imaging is done by imager         |
|                                            |                                     |                                                        | (:doc:`../calim/imager`). If false, it is done by cimager     |
|                                            |                                     |                                                        | (:doc:`../calim/cimager`). When true, the following           |
|                                            |                                     |                                                        | parameters are used. If left blank (the default), the value   |
|                                            |                                     |                                                        | is given by the overall parameter ``DO_ALT_IMAGER``.          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NCHAN_PER_CORE``                         | 12                                  | nchanpercore                                           | The number of channels each core will process.                |
|                                            |                                     | (:doc:`../calim/imager`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``USE_TMPFS``                              | false                               | usetmpfs (:doc:`../calim/imager`)                      | Whether to store the visibilities in shared memory.This will  |
|                                            |                                     |                                                        | give a performance boost at the expense of memory             |
|                                            |                                     |                                                        | usage. Better used for processing continuum data.             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``TMPFS``                                  | /dev/shm                            | tmpfs (:doc:`../calim/imager`)                         | Location of the shared memory.                                |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NUM_SPECTRAL_WRITERS_CONTCUBE``          | ``""``                              | nwriters (:doc:`../calim/imager`)                      | The number of writers used by imager. Unless                  |
|                                            |                                     |                                                        | ``ALT_IMAGER_SINGLE_FILE_CONTCUBE=true``, this will equate to |
|                                            |                                     |                                                        | the number of distinct spectral cubes produced.In the case of |
|                                            |                                     |                                                        | multiple cubes, each will be a sub-band of the full           |
|                                            |                                     |                                                        | bandwidth. No combination of the sub-cubes is currently       |
|                                            |                                     |                                                        | done. The number of writers will be reduced to the number of  |
|                                            |                                     |                                                        | workers in the job if necessary. If a single image is         |
|                                            |                                     |                                                        | produced, the default is to have the same number of writers as|
|                                            |                                     |                                                        | workers.                                                      |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``ALT_IMAGER_SINGLE_FILE_CONTCUBE``        | true                                | singleoutputfile                                       | Whether to write a single cube, even with multiple writers    |
|                                            |                                     | (:doc:`../calim/imager`)                               | (ie. ``NUM_SPECTRAL_WRITERS_CONTCUBE>1``). Only works when    |
|                                            |                                     |                                                        | ``IMAGETYPE_SPECTRAL=fits``                                   |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Self-calibration**                       |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DO_SELFCAL``                             | true                                | none                                                   | Whether to self-calibrate the science data when imaging.      |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_METHOD``                         | Cmodel                              | none                                                   | How to do the self-calibration. There are three options:      |
|                                            |                                     |                                                        | "Cmodel" means create a model image from the                  |
|                                            |                                     |                                                        | source-finding results; "Components" means use the            |
|                                            |                                     |                                                        | detected components directly through a parset (created by     |
|                                            |                                     |                                                        | Selavy); "CleanModel" means use the clean model image from the|
|                                            |                                     |                                                        | most recent imaging as the model for ccalibrator. Anything    |
|                                            |                                     |                                                        | else will default to "Cmodel".                                |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_NUM_LOOPS``                      | 1                                   | none                                                   | Number of loops of self-calibration.                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_INTERVAL``                       | ``"[200,200]"``                     | interval                                               | Interval [sec] over which to solve for self-calibration. Can  |
|                                            |                                     | (:doc:`../calim/ccalibrator`)                          | be given as an array with different values for each self-cal  |
|                                            |                                     |                                                        | loop, as for the default, or a single value that applies to   |
|                                            |                                     |                                                        | each loop.                                                    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_KEEP_IMAGES``                    | true                                | none                                                   | Should we keep the images from the intermediate selfcal       |
|                                            |                                     |                                                        | loops?                                                        |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``MOSAIC_SELFCAL_LOOPS``                   | false                               | none                                                   | Should we make full-field mosaics for each loop of the        |
|                                            |                                     |                                                        | self-calibration? This is done for each field separately.     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_THRESHOLD``               | 8                                   | snrCut                                                 | SNR threshold for detection with Selavy in determining        |
|                                            |                                     | (:doc:`../analysis/selavy`)                            | selfcal sources. Can be given as an array with different      |
|                                            |                                     |                                                        | values for each self-cal loop (e.g. ``"[15,10,8]"``).         |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBX``                   | 6                                   | nsubx                                                  | Division of image in x-direction for source-finding in        |
|                                            |                                     | (:doc:`../analysis/selavy`)                            | selfcal.                                                      |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBY``                   | 3                                   | nsuby                                                  | Division of image in y-direction for source-finding in        |
|                                            |                                     | (:doc:`../analysis/selavy`)                            | selfcal.                                                      |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS``    | true                                | Selavy.Fitter.numGaussFromGuess                        | Whether to fit the number of Gaussians given by the initial   |
|                                            |                                     | (:doc:`../analysis/postprocessing`)                    | estimate (true), or to only fit a fixed number (false). The   |
|                                            |                                     |                                                        | number is given by ``SELFCAL_SELAVY_NUM_GAUSSIANS``.          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_NUM_GAUSSIANS``           | 1                                   | Selavy.Fitter.maxNumGauss                              | The number of Gaussians to fit to each island when            |
|                                            |                                     | (:doc:`../analysis/postprocessing`)                    | ``SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS=false``.                |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_FIT_TYPE``                | full                                | Selavy.Fitter.fitTypes                                 | The type of fit to be used in the Selavy job. The possible    |
|                                            |                                     | (:doc:`../analysis/postprocessing`)                    | options are 'full', 'psf', 'shape', or 'height'.              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SELAVY_WEIGHTSCUT``              | 0.95                                | Selavy.Weights.weightsCutoff                           | Pixels with weight less than this fraction of the peak        |
|                                            |                                     | (:doc:`../analysis/thresholds`)                        | weight will not be considered by the source-finding. If       |
|                                            |                                     |                                                        | the value is negative, or more than one, no consideration     |
|                                            |                                     |                                                        | of the weight is made.                                        |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_COMPONENT_SNR_LIMIT``            | 10                                  | Used to create Cmodel.flux_limit                       | The signal-to-noise level used to set the flux limit for      |
|                                            |                                     | (:doc:`../calim/cmodel`)                               | components that are used by Cmodel. The image noise values    |
|                                            |                                     |                                                        | reported for all components are averaged, then multiplied by  |
|                                            |                                     |                                                        | this value to form the Cmodel flux limit. If left blank       |
|                                            |                                     |                                                        | (``""``), the flux limit is determined by                     |
|                                            |                                     |                                                        | ``SELFCAL_MODEL_FLUX_LIMIT``.                                 |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_MODEL_FLUX_LIMIT``               | 10uJy                               | Cmodel.flux_limit (:doc:`../calim/cmodel`)             | The minimum integrated flux for components to be included in  |
|                                            |                                     |                                                        | the model used for self-calibration.                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_NORMALISE_GAINS``                | true                                | normalisegains                                         | Whether to normalise the amplitudes of the gains to 1,        |
|                                            |                                     | (:doc:`../calim/ccalibrator`)                          | approximating the phase-only self-calibration approach. Can   |
|                                            |                                     |                                                        | be given as an array with different values for each self-cal  |
|                                            |                                     |                                                        | loop (e.g. "[true,true,false]").                              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_REF_ANTENNA``                    | ``""``                              | refantenna (:doc:`../calim/ccalibrator`)               | Reference antenna to use in the calibration. Should be        |
|                                            |                                     |                                                        | antenna number, 0 - nAnt-1, that matches the antenna          |
|                                            |                                     |                                                        | numbering in the MS.                                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_REF_GAINS``                      | ``""``                              | refgains (:doc:`../calim/ccalibrator`)                 | Reference gains to use in the calibration - something like    |
|                                            |                                     |                                                        | gain.g11.0.0.                                                 |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SELFCAL_SCALENOISE``                     | false                               | calibrate.scalenoise                                   | Whether the noise estimate will be scaled in accordance       |
|                                            |                                     | (:doc:`../calim/cimager`)                              | with the applied calibrator factor to achieve proper          |
|                                            |                                     |                                                        | weighting.                                                    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``GAINS_CAL_TABLE``                        | cont_gains_cal_SB%s_%b.tab          | none (directly)                                        | The table name to hold the final gains solution. Once         |
|                                            |                                     |                                                        | the self-cal loops have completed, the cal table in the       |
|                                            |                                     |                                                        | final loop is copied to a table of this name in the base      |
|                                            |                                     |                                                        | directory. This can then be used for the spectral-line        |
|                                            |                                     |                                                        | imaging if need be. If this is blank, both ``DO_SELFCAL``     |
|                                            |                                     |                                                        | and ``DO_APPLY_CAL_SL`` will be set to false. The %s wildcard |
|                                            |                                     |                                                        | will be resolved into the scehduling block ID, and the %b will|
|                                            |                                     |                                                        | be replaced with "FIELD_beamBB", where FIELD is the field id, |
|                                            |                                     |                                                        | and BB the (zero-based) beam number.                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CCALIBRATOR_MINUV``                      | 0                                   | MinUV (:doc:`../calim/data_selection`)                 | The minimum UV distance considered in the calibration - used  |
|                                            |                                     |                                                        | to exclude the short baselines. Can be given as an array with |
|                                            |                                     |                                                        | different values for each self-cal loop                       |
|                                            |                                     |                                                        | (e.g. ``"[200,200,0]"``).                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CCALIBRATOR_MAXUV``                      | 0                                   | MaxUV (:doc:`../calim/data_selection`)                 | The maximum UV distance considered in the calibration. Only   |
|                                            |                                     |                                                        | used if greater than zero. Can be given as an array with      |
|                                            |                                     |                                                        | different values for each self-cal loop                       |
|                                            |                                     |                                                        | (e.g. ``"[200,200,0]"``).                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DO_POSITION_OFFSET``                     | false                               | none                                                   | Whether to add a fixed RA & Dec offset to the positions of    |
|                                            |                                     |                                                        | sources in the final self-calibration catalogue (prior to it  |
|                                            |                                     |                                                        | being used to calibrate the data). This has been implemented  |
|                                            |                                     |                                                        | to help with commissioning - do not use unless you understand |
|                                            |                                     |                                                        | what it is doing! This makes use of the ACES script           |
|                                            |                                     |                                                        | *tools/fix_position_offsets.py*.                              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RA_POSITION_OFFSET``                     | 0.                                  | none                                                   | The offset in position in the RA direction, in arcsec. This is|
|                                            |                                     |                                                        | taken from the **offset_pipeline_params.txt** file produced by|
|                                            |                                     |                                                        | the continuum validation script, where the sense of the offset|
|                                            |                                     |                                                        | is **REFERENCE-ASKAP**.                                       |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DEC_POSITION_OFFSET``                    | 0.                                  | none                                                   | The offset in position in the DEC direction, in arcsec. This  |
|                                            |                                     |                                                        | is taken from the **offset_pipeline_params.txt** file produced|
|                                            |                                     |                                                        | by the continuum validation script, where the sense of the    |
|                                            |                                     |                                                        | offset is **REFERENCE-ASKAP**.                                |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Application of gains calibration**       |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DO_APPLY_CAL_CONT``                      | true                                | none                                                   | Whether to apply the calibration to the averaged              |
|                                            |                                     |                                                        | ("continuum") dataset.                                        |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``JOB_TIME_CONT_APPLYCAL``                 | ``JOB_TIME_DEFAULT`` (24:00:00)     | none                                                   | Time request for applying the calibration                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``KEEP_RAW_AV_MS``                         | true                                | none                                                   | Whether to make a copy of the averaged MS before applying     |
|                                            |                                     |                                                        | the gains calibration (true), or to just overwrite with       |
|                                            |                                     |                                                        | the calibrated data (false).                                  |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Continuum cube imaging**                 |                                     |                                                        |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``DO_CONTCUBE_IMAGING``                    | false                               | none                                                   | Whether to create continuum cubes                             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``JOB_TIME_CONTCUBE_IMAGE``                | ``JOB_TIME_DEFAULT`` (24:00:00)     | none                                                   | Time request for individual continuum cube jobs               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``IMAGE_BASE_CONTCUBE``                    | i.SB%s.contcube                     | Helps form Images.name (:doc:`../calim/simager`)       | Base name for the continuum cubes. It should include "i.", as |
|                                            |                                     |                                                        | the actual base name will include the correct polarisation    |
|                                            |                                     |                                                        | ('I' will produce i.contcube, Q will produce q.contcube and   |
|                                            |                                     |                                                        | so on).  The %s wildcard will be resolved into the scheduling |
|                                            |                                     |                                                        | block ID.                                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NUM_PIXELS_CONTCUBE``                    | 4096                                | Images.shape (:doc:`../calim/simager`)                 | Number of pixels on the spatial dimension for the continuum   |
|                                            |                                     |                                                        | cubes.                                                        |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CELLSIZE_CONTCUBE``                      | ``""``                              | Images.cellsize (:doc:`../calim/simager`)              | Angular size of spatial pixels for the continuum cubes. If not|
|                                            |                                     |                                                        | provided, it defaults to the value of ``CELLSIZE_CONT``.      |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CONTCUBE_POLARISATIONS``                 | ``"I"``                             | Images.polarisation (:doc:`../calim/simager`)          | List of polarisations to create cubes for. This should be a   |
|                                            |                                     |                                                        | comma-separated list of (upper-case) polarisations. Separate  |
|                                            |                                     |                                                        | jobs will be launched for each polarisation given.            |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``REST_FREQUENCY_CONTCUBE``                | ``""``                              | Images.restFrequency (:doc:`../calim/simager`)         | Rest frequency to be written to the continuum cube. If left   |
|                                            |                                     |                                                        | blank, no rest frequency is written.                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORING_BEAM_CONTCUBE``                | fit                                 | restore.beam (:doc:`../calim/simager`)                 | Restoring beam to use: 'fit' will fit the PSF in each channel |
|                                            |                                     |                                                        | separately to determine the appropriate beam for that         |
|                                            |                                     |                                                        | channel, else give a size (such as ``“[30arcsec,              |
|                                            |                                     |                                                        | 30arcsec, 0deg]”``).                                          |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORING_BEAM_CUTOFF_CONTCUBE``         | 0.5                                 | restore.beam.cutoff                                    | Cutoff value used in determining the support for the fitting  |
|                                            |                                     | (:doc:`../calim/simager`)                              | (ie. the rectangular area given to the fitting routine).      |
|                                            |                                     |                                                        | Value is a fraction of the peak.                              |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``RESTORING_BEAM_CONTCUBE_REFERENCE``      | mid                                 | restore.beamReference (:doc:`../calim/simager`)        | Which channel to use as the reference when writing the        |
|                                            |                                     |                                                        | restoring beam to the image cube. Can be an integer as the    |
|                                            |                                     |                                                        | channel number (0-based), or one of 'mid' (the middle         |
|                                            |                                     |                                                        | channel), 'first' or 'last'                                   |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NUM_CPUS_CONTCUBE_SCI``                  | ``""``                              | none                                                   | Total number of cores to use for the continuum cube job. If   |
|                                            |                                     |                                                        | left blank, this will be chosen to match the number of        |
|                                            |                                     |                                                        | channels (taking into account ``NCHAN_PER_CORE_CONTCUBE`` if  |
|                                            |                                     |                                                        | necessary), plus an additional core for the master process.   |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``NCHAN_PER_CORE_CONTCUBE``                | 3                                   | nchanpercore (:doc:`../calim/imager`)                  | If imager (:doc:`../calim/imager`) is used, this determines   |
|                                            |                                     |                                                        | how many channels each *worker* will process.                 |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CPUS_PER_CORE_CONTCUBE_IMAGING``         | 8                                   | none                                                   | How many of the cores on each node to use.                    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| **Continuum cube cleaning**                |                                     |                                                        | Different cleaning parameters used for the continuum cubes    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``SOLVER_CONTCUBE``                        | Clean                               | solver                                                 | Which solver to use. You will mostly want to leave this as    |
|                                            |                                     | (:doc:`../calim/cimager`)                              | 'Clean', but there is a 'Dirty' solver available.             |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_ALGORITHM``               | BasisfunctionMFS                    | Clean.algorithm                                        | The name of the clean algorithm to use.                       |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_MINORCYCLE_NITER``        | 600                                 | Clean.niter                                            | The number of iterations for the minor cycle clean.           |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_GAIN``                    | 0.2                                 | Clean.gain                                             | The loop gain (fraction of peak subtracted per minor cycle).  |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_PSFWIDTH``                | 256                                 | Clean.psfwidth                                         | The width of the psf patch used in the minor cycle.           |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_SCALES``                  | ``"[0,3,10]"``                      | Clean.scales                                           | Set of scales (in pixels) to use with the multi-scale clean.  |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE``    | ``"[40%, 0.5mJy, 0.05mJy]"``        | threshold.minorcycle                                   | Threshold for the minor cycle loop.                           |
|                                            |                                     | (:doc:`../calim/solver`)                               |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE``    | 0.06mJy                             | threshold.majorcycle                                   | The target peak residual. Major cycles stop if this is        |
|                                            |                                     | (:doc:`../calim/solver`)                               | reached. A negative number ensures all major cycles requested |
|                                            |                                     |                                                        | are done.                                                     |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_NUM_MAJORCYCLES``         | 3                                   | ncycles                                                | Number of major cycles.                                       |
|                                            |                                     | (:doc:`../calim/cimager`)                              |                                                               |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_WRITE_AT_MAJOR_CYCLE``    | false                               | Images.writeAtMajorCycle                               | If true, the intermediate images will be written (with a      |
|                                            |                                     | (:doc:`../calim/cimager`)                              | .cycle suffix) after the end of each major cycle.             |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+
| ``CLEAN_CONTCUBE_SOLUTIONTYPE``            | MAXCHISQ                            | Clean.solutiontype (see discussion at                  | The type of peak finding algorithm to use in the              |
|                                            |                                     | :doc:`../recipes/Imaging`)                             | deconvolution. Choices are MAXCHISQ, MAXTERM0, or MAXBASE.    |
+--------------------------------------------+-------------------------------------+--------------------------------------------------------+---------------------------------------------------------------+

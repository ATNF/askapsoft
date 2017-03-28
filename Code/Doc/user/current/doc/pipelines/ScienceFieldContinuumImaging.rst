User Parameters - Continuum imaging
===================================

The parameters listed here allow the user to configure the parset and
the slurm job for continuum imaging of the science field.

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
3. Use the results to calibrate the antenna-based gains by either:
   
   a. Create a component parset from the resulting component catalogue and use this parset in ccalibrator, or
   b. Create a model image from the component catalogue, and use in ccalibrator
      
4. Re-run imaging, applying the latest gains table
5. Repeat steps 2-5 for a given number of loops

Each loop gets its own directory, where the intermediate images,
source-finding results, and gains calibration table are stored. At the
end, the final gains calibration table is kept in the main output
directory (as this can then be used by the spectral-line imaging
pipeline).

Some parameters are allowed to vary with the loop number of the
self-calibration. This way, you can decrease, say, the detection
threshold, or increase the number of major cycles, as the calibration
steadily improves. These parameters are for:

* Deconvolution: ``CLEAN_THRESHOLD_MAJORCYCLE`` and ``CLEAN_NUM_MAJORCYCLES``
* Self-calibration: ``SELFCAL_SELAVY_THRESHOLD``, ``SELFCAL_INTERVAL``
  and ``SELFCAL_NORMALISE_GAINS``
* Data selection: ``CIMAGER_MINUV`` and ``CCALIBRATOR_MINUV``

To use this mode, the values for these parameters should be given as
an array in the form ``SELFCAL_INTERVAL="[1800,1800,900,300]"``. The
size of these arrays should be one more than
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
given as different (the default Solver, for instance, is Basisfunction
instead of BasisfunctionMFS).

+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| Variable                                   | Default                         | Parset equivalent                                      | Description                                                  |
+============================================+=================================+========================================================+==============================================================+
| ``DO_CONT_IMAGING``                        | true                            | none                                                   | Whether to image the science MS                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_CONT_IMAGE``                    | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                                   | Time request for imaging the continuum (both types - with and|
|                                            |                                 |                                                        | without self-calibration)                                    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Basic variables**                        |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``IMAGE_AT_BEAM_CENTRES``                  | true                            | none                                                   | Whether to have each beam's image centred at the centre of   |
|                                            |                                 |                                                        | the beam (IMAGE_AT_BEAM_CENTRES=true), or whether to use a   |
|                                            |                                 |                                                        | single image centre for all beams.                           |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_CPUS_CONTIMG_SCI``                   | ""                              | none                                                   | The number of cores in total to use for the continuum        |
|                                            |                                 |                                                        | imaging. If left blank ("" - the default), then this is      |
|                                            |                                 |                                                        | calculated based on the number of channels and Taylor terms. |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CPUS_PER_CORE_CONT_IMAGING``             | 16                              | Not for parset                                         |Number of cores to use on each node in the continuum imaging. |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DATACOLUMN``                             | DATA                            | datacolumn (:doc:`../calim/cimager`)                   | The column in the measurement set from which to read the     |
|                                            |                                 |                                                        | visibility data. The default, 'DATA', is appropriate for     |
|                                            |                                 |                                                        | datasets processed within askapsoft, but if you are trying to|
|                                            |                                 |                                                        | image data processed, for instance, in CASA, then changing   |
|                                            |                                 |                                                        | this to CORRECTED_DATA may be what you want.                 |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``IMAGE_BASE_CONT``                        | i.cont                          | Helps form Images.Names                                | The base name for images: if ``IMAGE_BASE_CONT=i.blah`` then |
|                                            |                                 | (:doc:`../calim/cimager`)                              | we'll get image.i.blah, image.i.blah.restored, psf.i.blah etc|
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DIRECTION_SCI``                          | none                            | Images.<imagename>.direction                           | The direction parameter for the images, i.e. the central     |
|                                            |                                 | (:doc:`../calim/cimager`)                              | position. Can be left out, in which case Cimager will get it |
|                                            |                                 |                                                        | from either the beam location (for                           |
|                                            |                                 |                                                        | IMAGE_AT_BEAM_CENTRES=true) or from the measurement set using|
|                                            |                                 |                                                        | the "advise" functionality (for IMAGE_AT_BEAM_CENTRES=false).|
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_PIXELS_CONT``                        | 4096                            | Images.shape                                           | The number of pixels on the side of the images to be created.|
|                                            |                                 | (:doc:`../calim/cimager`)                              | If negative, zero, or absent (i.e. ``NUM_PIXELS_CONT=""``),  |
|                                            |                                 |                                                        | this will be set automatically by the Cimager “advise”       |
|                                            |                                 |                                                        | function, based on examination of the MS. Note that this     |
|                                            |                                 |                                                        | default will be suitable for a single beam, but probably not |
|                                            |                                 |                                                        | for an image to be large enough for the full set of beams    |
|                                            |                                 |                                                        | (when using IMAGE_AT_BEAM_CENTRES=false). The default value, |
|                                            |                                 |                                                        | combined with the default for the cell size, should be       |
|                                            |                                 |                                                        | sufficient to cover a full field. If you have                |
|                                            |                                 |                                                        | IMAGE_AT_BEAM_CENTRES=true then this needs only to be big    |
|                                            |                                 |                                                        | enough to fit a single beam.                                 |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CELLSIZE_CONT``                          | 10                              | Images.cellsize                                        | Size of the pixels in arcsec. If negative, zero or absent,   |
|                                            |                                 | (:doc:`../calim/cimager`)                              | this will be set automatically by the Cimager “advise”       |
|                                            |                                 |                                                        | function, based on examination of the MS. The default is     |
|                                            |                                 |                                                        | chosen together with the default number of pixels to cover a |
|                                            |                                 |                                                        | typical full ASKAP field.                                    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_TAYLOR_TERMS``                       | 2                               | Images.image.${imageBase}.nterms                       | Number of Taylor terms to create in MFS imaging. If more than|
|                                            |                                 | (:doc:`../calim/cimager`)                              | 1, MFS weighting will be used (equivalent to setting         |
|                                            |                                 | linmos.nterms (:doc:`../calim/linmos`)                 | **Cimager.visweights=MFS** in the cimager parset).           |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``MFS_REF_FREQ``                           | no default                      | visweights.MFS.reffreq                                 | Frequency at which continuum image is made [Hz]. This is the |
|                                            |                                 | (:doc:`../calim/cimager`)                              | reference frequency for the multi-frequency synthesis, which |
|                                            |                                 |                                                        | should usually be the middle of the band. If negative, zero, |
|                                            |                                 |                                                        | or absent (the default), this will be set automatically to   |
|                                            |                                 |                                                        | the average of the frequencies being processed.              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORING_BEAM_CONT``                    | fit                             | restore.beam                                           | Restoring beam to use: 'fit' will fit the PSF to determine   |
|                                            |                                 | (:doc:`../calim/cimager`)                              | the appropriate beam, else give a size (such as 30arcsec, or |
|                                            |                                 |                                                        | “[30arcsec, 30arcsec, 0deg]”).                               |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORING_BEAM_CUTOFF_CONT``             | 0.05                            | restore.beam.cutoff                                    | Cutoff value used in determining the support for the fitting |
|                                            |                                 | (:doc:`../calim/simager`)                              | (ie. the rectangular area given to the fitting routine).     |
|                                            |                                 |                                                        | Value is a fraction of the peak.                             |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CIMAGER_MINUV``                          | 0                               | MinUV (:doc:`../calim/data_selection`)                 | The minimum UV distance considered in the imaging - used to  |
|                                            |                                 |                                                        | exclude the short baselines. Can be given as an array with   |
|                                            |                                 |                                                        | different values for each self-cal loop (e.g. "[200,200,0]").|
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Gridding parameters**                    |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_IMAGING``               | true                            | snapshotimaging                                        | Whether to use snapshot imaging when gridding.               |
|                                            |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_WTOL``                  | 2600                            | snapshotimaging.wtolerance                             | The wtolerance parameter controlling how frequently to       |
|                                            |                                 | (:doc:`../calim/gridder`)                              | snapshot.                                                    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_LONGTRACK``             | true                            | snapshotimaging.longtrack                              | The longtrack parameter controlling how the best-fit W plane |
|                                            |                                 | (:doc:`../calim/gridder`)                              | is determined when using snapshots.                          |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_CLIPPING``              | 0                               | snapshotimaging.clipping                               | If greater than zero, this fraction of the full image width  |
|                                            |                                 | (:doc:`../calim/gridder`)                              | is set to zero. Useful when imaging at high declination as   |
|                                            |                                 |                                                        | the edges can generate artefacts.                            |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_WMAX``                           | 2600                            | WProject.wmax                                          | The wmax parameter for the gridder.                          |
|                                            |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_NWPLANES``                       | 99                              | WProject.nwplanes                                      | The nwplanes parameter for the gridder.                      | 
|                                            |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_OVERSAMPLE``                     | 4                               | WProject.oversample                                    | The oversampling factor for the gridder.                     |
|                                            |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_MAXSUPPORT``                     | 512                             | WProject.maxsupport                                    | The maxsupport parameter for the gridder.                    |
|                                            |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Cleaning parameters**                    |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SOLVER``                                 | Clean                           | solver                                                 | Which solver to use. You will mostly want to leave this as   |
|                                            |                                 | (:doc:`../calim/cimager`)                              | 'Clean', but there is a 'Dirty' solver available.            |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_ALGORITHM``                        | BasisfunctionMFS                | Clean.algorithm                                        | The name of the clean algorithm to use.                      |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_MINORCYCLE_NITER``                 | 500                             | Clean.niter                                            | The number of iterations for the minor cycle clean.          |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_GAIN``                             | 0.5                             | Clean.gain                                             | The loop gain (fraction of peak subtracted per minor cycle). |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_PSFWIDTH``                         | 512                             | Clean.psfwidth                                         | The width of the psf patch used in the minor cycle.          |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_SCALES``                           | "[0,3,10]"                      | Clean.scales                                           | Set of scales (in pixels) to use with the multi-scale clean. |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_THRESHOLD_MINORCYCLE``             | "[30%, 0.9mJy]"                 | threshold.minorcycle                                   | Threshold for the minor cycle loop.                          |
|                                            |                                 | (:doc:`../calim/cimager`)                              |                                                              |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_THRESHOLD_MAJORCYCLE``             | 1mJy                            | threshold.majorcycle                                   | The target peak residual. Major cycles stop if this is       |
|                                            |                                 | (:doc:`../calim/cimager`)                              | reached. A negative number ensures all major cycles requested|
|                                            |                                 | (:doc:`../calim/solver`)                               | are done. Can be given as an array with different values for |
|                                            |                                 |                                                        | each self-cal loop (e.g. "[3mJy,1mJy,-1mJy]").               |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_NUM_MAJORCYCLES``                  | 2                               | ncycles                                                | Number of major cycles. Can be given as an array with        |
|                                            |                                 | (:doc:`../calim/cimager`)                              | different values for each self-cal loop (e.g. "[2,4,6]").    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_WRITE_AT_MAJOR_CYCLE``             | false                           | Images.writeAtMajorCycle                               | If true, the intermediate images will be written (with a     |
|                                            |                                 | (:doc:`../calim/cimager`)                              | .cycle suffix) after the end of each major cycle.            |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Preconditioning parameters**             |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_LIST``                    | "[Wiener, GaussianTaper]"       | preconditioner.Names                                   | List of preconditioners to apply.                            |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_GAUSS_TAPER``             |  "[30arcsec, 30arcsec, 0deg]"   | preconditioner.GaussianTaper                           | Size of the Gaussian taper - either single value (for        |
|                                            |                                 | (:doc:`../calim/solver`)                               | circular taper) or 3 values giving an elliptical size.       |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_WIENER_ROBUSTNESS``       | 0.5                             | preconditioner.Wiener.robustness                       | Robustness value for the Wiener filter.                      |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_WIENER_TAPER``            | ""                              | preconditioner.Wiener.taper                            | Size of gaussian taper applied in image domain to Wiener     |
|                                            |                                 | (:doc:`../calim/solver`)                               | filter. Ignored if blank (ie. “”).                           |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORE_PRECONDITIONER_LIST``            | "[Wiener, GaussianTaper]"       | restore.preconditioner.Names                           | List of preconditioners to apply at the restore stage, to    |
|                                            |                                 | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | produce an additional restored image.                        |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORE_PRECONDITIONER_GAUSS_TAPER``     |  "[30arcsec, 30arcsec, 0deg]"   | restore.preconditioner.GaussianTaper                   | Size of the Gaussian taper for the restore preconditioning - |
|                                            |                                 | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | either single value (for circular taper) or 3 values giving  |
|                                            |                                 |                                                        | an elliptical size.                                          |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
|``RESTORE_PRECONDITIONER_WIENER_ROBUSTNESS``| -1.                             | restore.preconditioner.Wiener.robustness               | Robustness value for the Wiener filter in the restore        |
|                                            |                                 | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | preconditioning.                                             |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORE_PRECONDITIONER_WIENER_TAPER``    | ""                              | restore.preconditioner.Wiener.taper                    | Size of gaussian taper applied in image domain to Wiener     |
|                                            |                                 | (:doc:`../calim/cimager` & :doc:`../calim/solver`)     | filter in the restore preconditioning. Ignored if blank      |
|                                            |                                 |                                                        | (ie. “”).                                                    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ***New imager parameters**                 |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DO_ALT_IMAGER_CONT``                     | ""                              | none                                                   | If true, the continuum imaging is done by imager             |
|                                            |                                 |                                                        | (:doc:`../calim/imager`). If false, it is done by cimager    |
|                                            |                                 |                                                        | (:doc:`../calim/cimager`). When true, the following          |
|                                            |                                 |                                                        | parameters are used. If left blank (the default), the value  |
|                                            |                                 |                                                        | is given by the overall parameter ``DO_ALT_IMAGER``.         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DO_ALT_IMAGER_CONTCUBE``                 | ""                              | none                                                   | If true, the continuum cube imaging is done by imager        |
|                                            |                                 |                                                        | (:doc:`../calim/imager`). If false, it is done by cimager    |
|                                            |                                 |                                                        | (:doc:`../calim/cimager`). When true, the following          |
|                                            |                                 |                                                        | parameters are used. If left blank (the default), the value  |
|                                            |                                 |                                                        | is given by the overall parameter ``DO_ALT_IMAGER``.         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NCHAN_PER_CORE``                         | 1                               | nchanpercore                                           | The number of channels each core will process.               |
|                                            |                                 | (:doc:`../calim/imager`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``USE_TMPFS``                              | false                           | usetmpfs (:doc:`../calim/imager`)                      | Whether to store the visibilities in shared memory.This will |
|                                            |                                 |                                                        | give a performance boost at the expense of memory            |
|                                            |                                 |                                                        | usage. Better used for processing continuum data.            |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``TMPFS``                                  | /dev/shm                        | tmpfs (:doc:`../calim/imager`)                         | Location of the shared memory.                               |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_SPECTRAL_CUBES_CONTCUBE``            | 1                               | nwriters (:doc:`../calim/imager`)                      | Number of spectral cubes to be produced. This actually       |
|                                            |                                 |                                                        | configures the number of writers employed by imager, each of |
|                                            |                                 |                                                        | which writes a sub-band. No combination of the sub-cubes is  |
|                                            |                                 |                                                        | currently done. Note that this defaults to a single cube, as |
|                                            |                                 |                                                        | the continuum cubes are not as I/O intensive as the          |
|                                            |                                 |                                                        | spectral-line cubes.                                         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Self-calibration**                       |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DO_SELFCAL``                             | true                            | none                                                   | Whether to self-calibrate the science data when imaging.     |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_METHOD``                         | Cmodel                          | none                                                   | How to do the self-calibration. There are two options:       |
|                                            |                                 |                                                        | "Cmodel" means create a model image from the                 |
|                                            |                                 |                                                        | source-finding results; "Components" means use the           |
|                                            |                                 |                                                        | detected components directly through a parset (created by    |
|                                            |                                 |                                                        | Selavy). Anything else will default to "Cmodel".             |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_NUM_LOOPS``                      | 5                               | none                                                   | Number of loops of self-calibration.                         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_INTERVAL``                       | 300                             | interval                                               | Interval [sec] over which to solve for self-calibration. Can |
|                                            |                                 | (:doc:`../calim/ccalibrator`)                          | be given as an array with different values for each self-cal |
|                                            |                                 |                                                        | loop (e.g. "[1800,900,300]")                                 |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_KEEP_IMAGES``                    | true                            | none                                                   | Should we keep the images from the intermediate selfcal      |
|                                            |                                 |                                                        | loops?                                                       |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``MOSAIC_SELFCAL_LOOPS``                   | true                            | none                                                   | Should we make full-field mosaics for each loop of the       |
|                                            |                                 |                                                        | self-calibration? This is done for each field separately.    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SELAVY_THRESHOLD``               | 15                              | snrCut                                                 | SNR threshold for detection with Selavy in determining       |
|                                            |                                 | (:doc:`../analysis/selavy`)                            | selfcal sources. Can be given as an array with different     |
|                                            |                                 |                                                        | values for each self-cal loop (e.g. "[15,10,8]").            |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBX``                   | 6                               | nsubx                                                  | Division of image in x-direction for source-finding in       |
|                                            |                                 | (:doc:`../analysis/selavy`)                            | selfcal.                                                     |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBY``                   | 3                               | nsuby                                                  | Division of image in y-direction for source-finding in       |
|                                            |                                 | (:doc:`../analysis/selavy`)                            | selfcal.                                                     |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS``    | true                            | Selavy.Fitter.numGaussFromGuess                        | Whether to fit the number of Gaussians given by the initial  |
|                                            |                                 | (:doc:`../analysis/postprocessing`)                    | estimate (true), or to only fit a fixed number (false). The  |
|                                            |                                 |                                                        | number is given by ``SELFCAL_SELAVY_NUM_GAUSSIANS``.         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SELAVY_NUM_GAUSSIANS``           | 1                               | Selavy.Fitter.maxNumGauss                              | The number of Gaussians to fit to each island when           |
|                                            |                                 | (:doc:`../analysis/postprocessing`)                    | ``SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS=false``.               |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SELAVY_WEIGHTSCUT``              | 0.95                            | Selavy.Weights.weightsCutoff                           | Pixels with weight less than this fraction of the peak       |
|                                            |                                 | (:doc:`../analysis/thresholds`)                        | weight will not be considered by the source-finding. If      |
|                                            |                                 |                                                        | the value is negative, or more than one, no consideration    |
|                                            |                                 |                                                        | of the weight is made.                                       |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_MODEL_FLUX_LIMIT``               | 10uJy                           | Cmodel.flux_limit (:doc:`../calim/cmodel`)             | The minimum integrated flux for components to be included in |
|                                            |                                 |                                                        | the model used for self-calibration.                         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_NORMALISE_GAINS``                | true                            | normalisegains                                         | Whether to normalise the amplitudes of the gains to 1,       |
|                                            |                                 | (:doc:`../calim/ccalibrator`)                          | approximating the phase-only self-calibration approach. Can  |
|                                            |                                 |                                                        | be given as an array with different values for each self-cal |
|                                            |                                 |                                                        | loop (e.g. "[true,true,false]").                             |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_REF_ANTENNA``                    | ""                              | refantenna (:doc:`../calim/ccalibrator`)               | Reference antenna to use in the calibration. Should be       |
|                                            |                                 |                                                        | antenna number, 0 - nAnt-1, that matches the antenna         |
|                                            |                                 |                                                        | numbering in the MS.                                         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_REF_GAINS``                      | ""                              | refgains (:doc:`../calim/ccalibrator`)                 | Reference gains to use in the calibration - something like   |
|                                            |                                 |                                                        | gain.g11.0.0.                                                |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SELFCAL_SCALENOISE``                     | false                           | calibrate.scalenoise                                   | Whether the noise estimate will be scaled in accordance      |
|                                            |                                 | (:doc:`../calim/cimager`)                              | with the applied calibrator factor to achieve proper         |
|                                            |                                 |                                                        | weighting.                                                   |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GAINS_CAL_TABLE``                        | cont_gains_cal_beam%b.tab       | none (directly)                                        | The table name to hold the final gains solution. Once        |
|                                            |                                 |                                                        | the self-cal loops have completed, the cal table in the      |
|                                            |                                 |                                                        | final loop is copied to a table of this name in the base     |
|                                            |                                 |                                                        | directory. This can then be used for the spectral-line       |
|                                            |                                 |                                                        | imaging if need be. If this is blank, both ``DO_SELFCAL``    |
|                                            |                                 |                                                        | and ``DO_APPLY_CAL_SL`` will be set to false.                |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CCALIBRATOR_MINUV``                      | 0                               | MinUV (:doc:`../calim/data_selection`)                 | The minimum UV distance considered in the calibration - used |
|                                            |                                 |                                                        | to exclude the short baselines. Can be given as an array with|
|                                            |                                 |                                                        | different values for each self-cal loop (e.g. "[200,200,0]").|
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Application of gains calibration**       |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DO_APPLY_CAL_CONT``                      | true                            | none                                                   | Whether to apply the calibration to the averaged             |
|                                            |                                 |                                                        | ("continuum") dataset.                                       |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_CONT_APPLYCAL``                 | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                                   | Time request for applying the calibration                    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``KEEP_RAW_AV_MS``                         | true                            | none                                                   | Whether to make a copy of the averaged MS before applying    |
|                                            |                                 |                                                        | the gains calibration (true), or to just overwrite with      |
|                                            |                                 |                                                        | the calibrated data (false).                                 |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Continuum cube imaging**                 |                                 |                                                        |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DO_CONTCUBE_IMAGING``                    | false                           | none                                                   | Whether to create continuum cubes                            |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_CONTCUBE_IMAGE``                | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                                   | Time request for individual continuum cube jobs              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``IMAGE_BASE_CONTCUBE``                    | i.contcube                      | Helps form Images.name (:doc:`../calim/simager`)       | Base name for the continuum cubes. It should include "i.", as|
|                                            |                                 |                                                        | the actual base name will include the correct polarisation   |
|                                            |                                 |                                                        | ('I' will produce i.contcube, Q will produce q.contcube and  |
|                                            |                                 |                                                        | so on).                                                      |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CONTCUBE_POLARISATIONS``                 | "I,Q,U,V"                       | Images.polarisation (:doc:`../calim/simager`)          | List of polarisations to create cubes for. This should be a  |
|                                            |                                 |                                                        | comma-separated list of (upper-case) polarisations. Separate |
|                                            |                                 |                                                        | jobs will be launched for each polarisation given.           |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``REST_FREQUENCY_CONTCUBE``                | ""                              | Images.restFrequency (:doc:`../calim/simager`)         | Rest frequency to be written to the continuum cube. If left  |
|                                            |                                 |                                                        | blank, no rest frequency is written.                         |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORING_BEAM_CONTCUBE``                | fit                             | restore.beam (:doc:`../calim/simager`)                 | Restoring beam to use: 'fit' will fit the PSF in each channel|
|                                            |                                 |                                                        | separately to determine the appropriate beam for that        |
|                                            |                                 |                                                        | channel, else give a size (such as 30arcsec, or “[30arcsec,  |
|                                            |                                 |                                                        | 30arcsec, 0deg]”).                                           |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORING_BEAM_CUTOFF_CONTCUBE``         | 0.05                            | restore.beam.cutoff                                    | Cutoff value used in determining the support for the fitting |
|                                            |                                 | (:doc:`../calim/simager`)                              | (ie. the rectangular area given to the fitting routine).     |
|                                            |                                 |                                                        | Value is a fraction of the peak.                             |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORING_BEAM_CONTCUBE_REFERENCE``      | mid                             | restore.beamReference (:doc:`../calim/simager`)        | Which channel to use as the reference when writing the       |
|                                            |                                 |                                                        | restoring beam to the image cube. Can be an integer as the   |
|                                            |                                 |                                                        | channel number (0-based), or one of 'mid' (the middle        |
|                                            |                                 |                                                        | channel), 'first' or 'last'                                  |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_CPUS_CONTCUBE_SCI``                  | ""                              | none                                                   | Total number of cores to use fo the continuum cube job. If   |
|                                            |                                 |                                                        | left blank, this will be chosen to match the number of       |
|                                            |                                 |                                                        | channels, plus an additional core for the master process.    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CPUS_PER_CORE_CONTCUBE_IMAGING``         | 20                              | none                                                   | How many of the cores on each node to use.                   |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Continuum cube cleaning**                |                                 |                                                        | Different cleaning parameters used for the continuum cubes   |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SOLVER_CONTCUBE``                        | Clean                           | solver                                                 | Which solver to use. You will mostly want to leave this as   |
|                                            |                                 | (:doc:`../calim/cimager`)                              | 'Clean', but there is a 'Dirty' solver available.            |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_ALGORITHM``               | Basisfunction                   | Clean.algorithm                                        | The name of the clean algorithm to use.                      |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_MINORCYCLE_NITER``        | 500                             | Clean.niter                                            | The number of iterations for the minor cycle clean.          |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_GAIN``                    | 0.5                             | Clean.gain                                             | The loop gain (fraction of peak subtracted per minor cycle). |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_PSFWIDTH``                | 512                             | Clean.psfwidth                                         | The width of the psf patch used in the minor cycle.          |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_SCALES``                  | "[0,3,10]"                      | Clean.scales                                           | Set of scales (in pixels) to use with the multi-scale clean. |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE``    | "[30%, 0.9mJy]"                 | threshold.minorcycle                                   | Threshold for the minor cycle loop.                          |
|                                            |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE``    | 1mJy                            | threshold.majorcycle                                   | The target peak residual. Major cycles stop if this is       |
|                                            |                                 | (:doc:`../calim/solver`)                               | reached. A negative number ensures all major cycles requested|
|                                            |                                 |                                                        | are done.                                                    |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_NUM_MAJORCYCLES``         | 2                               | ncycles                                                | Number of major cycles.                                      |
|                                            |                                 | (:doc:`../calim/cimager`)                              |                                                              |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_CONTCUBE_WRITE_AT_MAJOR_CYCLE``    | false                           | Images.writeAtMajorCycle                               | If true, the intermediate images will be written (with a     |
|                                            |                                 | (:doc:`../calim/cimager`)                              | .cycle suffix) after the end of each major cycle.            |
+--------------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+

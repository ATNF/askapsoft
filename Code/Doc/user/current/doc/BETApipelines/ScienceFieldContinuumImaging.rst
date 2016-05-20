User Parameters - Continuum imaging
===================================

The parameters listed here allow the user to configure the parset and
the slurm job for continuum imaging of the science field.

Some of the parameters can be set to blank, to allow cimager to choose
appropriate values based on its "advise" capability (which involves
examining the dataset and setting appropriate values for some
parameters.

When setting the gridding parameters, note that the gridder is
currently hardcoded to use **WProject**.  If you want to experiment
with other gridders, you need to edit the slurm file or parset
yourself (ie. set ``SUBMIT_JOBS=false`` to produce the slurm files
without submission, then edit and manually submit).

+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| Variable                             | Default                         | Parset equivalent                                      | Description                                                  |
+======================================+=================================+========================================================+==============================================================+
| ``DO_CONT_IMAGING``                  | true                            | none                                                   | Whether to image the science MS                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``JOB_TIME_CONT_IMAGE``              | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                                   | Time request for imaging the continuum (both types - with and|
|                                      |                                 |                                                        | without self-calibration)                                    |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Basic variables**                  |                                 |                                                        |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CPUS_PER_CORE_CONT_IMAGING``       | 16                              | Not for parset                                         | Number of CPUs to use on each core in the continuum imaging. |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DATACOLUMN``                       | DATA                            | datacolumn (:doc:`../calim/cimager`)                   | The column in the measurement set from which to read the     |
|                                      |                                 |                                                        | visibility data. The default, 'DATA', is appropriate for     |
|                                      |                                 |                                                        | datasets processed within askapsoft, but if you are trying to|
|                                      |                                 |                                                        | image data processed, for instance, in CASA, then changing   |
|                                      |                                 |                                                        | this to CORRECTED_DATA may be what you want.                 |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``IMAGE_BASE_CONT``                  | i.cont                          | Helps form Images.Names                                | The base name for images: if ``IMAGE_BASE_CONT=i.blah`` then |
|                                      |                                 | (:doc:`../calim/cimager`)                              | we'll get image.i.blah, image.i.blah.restored, psf.i.blah etc|
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``DIRECTION_SCI``                    | none                            | Images.<imagename>.direction                           | The direction parameter for the images, i.e. the central     |
|                                      |                                 | (:doc:`../calim/cimager`)                              | position. Can be left out, in which case Cimager will get it |
|                                      |                                 |                                                        | from the measurement set using the "advise" functionality.   |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_PIXELS_CONT``                  | 4096                            | Images.shape                                           | The number of pixels on the side of the images to be created.|
|                                      |                                 | (:doc:`../calim/cimager`)                              | If negative, zero, or absent (i.e. ``NUM_PIXELS_CONT=""``),  |
|                                      |                                 |                                                        | this will be set automatically by the Cimager “advise”       |
|                                      |                                 |                                                        | function, based on examination of the MS. Note that this     |
|                                      |                                 |                                                        | default will be suitable for a single beam, but probably not |
|                                      |                                 |                                                        | for an image to be large enough for the full 9 beams. The    |
|                                      |                                 |                                                        | default value, combined with the default for the cell size,  |
|                                      |                                 |                                                        | should be sufficient to cover a full field.                  |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CELLSIZE_CONT``                    | 10                              | Images.cellsize                                        | Size of the pixels in arcsec. If negative, zero or absent,   |
|                                      |                                 | (:doc:`../calim/cimager`)                              | this will be set automatically by the Cimager “advise”       |
|                                      |                                 |                                                        | function, based on examination of the MS. The default is     |
|                                      |                                 |                                                        | chosen together with the default number of pixels to cover a |
|                                      |                                 |                                                        | typical full BETA field.                                     |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``NUM_TAYLOR_TERMS``                 | 2                               | Images.image.${imageBase}.nterms                       | Number of Taylor terms to create in MFS imaging. If more than|
|                                      |                                 | (:doc:`../calim/cimager`)                              | 1, MFS weighting will be used (equivalent to setting         |
|                                      |                                 | linmos.nterms (:doc:`../calim/linmos`)                 | **Cimager.visweights=MFS** in the cimager parset).           |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``MFS_REF_FREQ``                     | no default                      | visweights.MFS.reffreq                                 | Frequency at which continuum image is made [Hz]. This is the |
|                                      |                                 | (:doc:`../calim/cimager`)                              | reference frequency for the multi-frequency synthesis, which |
|                                      |                                 |                                                        | should usually be the middle of the band. If negative, zero, |
|                                      |                                 |                                                        | or absent (the default), this will be set automatically to   |
|                                      |                                 |                                                        | the average of the frequencies being processed.              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``RESTORING_BEAM_CONT``              | fit                             | restore.beam                                           | Restoring beam to use: 'fit' will fit the PSF to determine   |
|                                      |                                 | (:doc:`../calim/cimager`)                              | the appropriate beam, else give a size (such as 30arcsec, or |
|                                      |                                 |                                                        | “[30arcsec, 30arcsec, 0deg]”).                               |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Gridding parameters**              |                                 |                                                        |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_IMAGING``         | true                            | snapshotimaging                                        | Whether to use snapshot imaging when gridding.               |
|                                      |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_WTOL``            | 2600                            | snapshotimaging.wtolerance                             | The wtolerance parameter controlling how frequently to       |
|                                      |                                 | (:doc:`../calim/gridder`)                              | snapshot.                                                    |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_SNAPSHOT_LONGTRACK``       | true                            | snapshotimaging.longtrack                              | The longtrack parameter controlling how the best-fit W plane |
|                                      |                                 | (:doc:`../calim/gridder`)                              | is determined when using snapshots.                          |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_WMAX``                     | 2600                            | WProject.wmax                                          | The wmax parameter for the gridder.                          |
|                                      |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+ 
| ``GRIDDER_NWPLANES``                 | 99                              | WProject.nwplanes                                      | The nwplanes parameter for the gridder.                      |
|                                      |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_OVERSAMPLE``               | 4                               | WProject.oversample                                    | The oversampling factor for the gridder.                     |
|                                      |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``GRIDDER_MAXSUPPORT``               | 512                             | WProject.maxsupport                                    | The maxsupport parameter for the gridder.                    |
|                                      |                                 | (:doc:`../calim/gridder`)                              |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Cleaning parameters**              |                                 |                                                        |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``SOLVER``                           | Clean                           | solver                                                 | Which solver to use. You will mostly want to leave this as   |
|                                      |                                 | (:doc:`../calim/cimager`)                              | 'Clean', but there is a 'Dirty' solver available.            |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_ALGORITHM``                  | BasisfunctionMFS                | Clean.algorithm                                        | The name of the clean algorithm to use.                      |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_MINORCYCLE_NITER``           | 500                             | Clean.niter                                            | The number of iterations for the minor cycle clean.          |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_GAIN``                       | 0.5                             | Clean.gain                                             | The loop gain (fraction of peak subtracted per minor cycle). |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+ 
| ``CLEAN_SCALES``                     | "[0,3,10]"                      | Clean.scales                                           | Set of scales (in pixels) to use with the multi-scale clean. |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_THRESHOLD_MINORCYCLE``       | "[30%, 0.9mJy]"                 | threshold.minorcycle                                   | Threshold for the minor cycle loop.                          |
|                                      |                                 | (:doc:`../calim/cimager`)                              |                                                              |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_THRESHOLD_MAJORCYCLE``       | 1mJy                            | threshold.majorcycle                                   | The target peak residual. Major cycles stop if this is       |
|                                      |                                 | (:doc:`../calim/cimager`)                              | reached. A negative number ensures all major cycles requested|
|                                      |                                 | (:doc:`../calim/solver`)                               | are done.                                                    |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_NUM_MAJORCYCLES``            | 2                               | ncycles                                                | Number of major cycles.                                      |
|                                      |                                 | (:doc:`../calim/cimager`)                              |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``CLEAN_WRITE_AT_MAJOR_CYCLE``       | false                           | Images.writeAtMajorCycle                               | If true, the intermediate images will be written (with a     |
|                                      |                                 | (:doc:`../calim/cimager`)                              | .cycle suffix) after the end of each major cycle.            |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| **Preconditioning parameters**       |                                 |                                                        |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_LIST``              | "[Wiener, GaussianTaper]"       | preconditioner.Names                                   | List of preconditioners to apply.                            |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_GAUSS_TAPER``       |  "[30arcsec, 30arcsec, 0deg]"   | preconditioner.GaussianTaper                           | Size of the Gaussian taper - either single value (for        |
|                                      |                                 | (:doc:`../calim/solver`)                               | circular taper) or 3 values giving an elliptical size.       |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_WIENER_ROBUSTNESS`` | 0.5                             | preconditioner.Wiener.robustness                       | Robustness value for the Wiener filter.                      |
|                                      |                                 | (:doc:`../calim/solver`)                               |                                                              |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+
| ``PRECONDITIONER_WIENER_TAPER``      | ""                              | preconditioner.Wiener.taper                            | Size of gaussian taper applied in image domain to Wiener     |
|                                      |                                 | (:doc:`../calim/solver`)                               | filter. Ignored if blank (ie. “”).                           |
+--------------------------------------+---------------------------------+--------------------------------------------------------+--------------------------------------------------------------+

User Parameters - Continuum Source-finding
==========================================

Source-finding is run in two ways within these scripts. It can be run
after each individual beam is imaged, and then again after the
mosaicking has completed.

Only continuum source-finding is done at this point - spectral-line
source-finding will be added in the future.

The source-finding applies a spatially-varying signal-to-noise
threshold, and fits 2D Gaussians to detected islands to create a
component catalogue.

At this point, only a limited set of Selavy's possible parameters are
made available through these scripts. This will likely increase, but
in the interim you can edit the sbatch files that are created and
re-run it yourself.

+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| Variable                       | Default | Parset equivalent               | Description                                                 |
+================================+=========+=================================+=============================================================+
| ``DO_SOURCE_FINDING``          | false   | none                            | Whether to do the source-finding with Selavy on the         |
|                                |         |                                 | individual beam images and the final mosaic.                |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_NSUBX``               | 6       | nsubx                           | Number of divisions in the x-direction that divide the image|
|                                |         | (:doc:`../analysis/selavy`)     | up, allowing parallel processing in the source-detection.   |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_NSUBY``               | 3       | nsuby                           | Number of divisions in the y-direction that divide the image|
|                                |         | (:doc:`../analysis/selavy`)     | up, allowing parallel processing in the source-detection.   |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_SNR_CUT``             | 5.0     | snrcut                          | The signal-to-noise ratio threshold to use in the           |
|                                |         | (:doc:`../analysis/selavy`)     | source-detection.                                           | 
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_FLAG_GROWTH``         | true    | flagGrowth                      | A flag indicating whether to grow detections down to a      |
|                                |         | (:doc:`../analysis/selavy`)     | lower threshold.                                            |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_GROWTH_CUT``          | 3.0     | growthCut                       | The secondary signal-to-noise threshold to which detections |
|                                |         | (:doc:`../analysis/selavy`)     | should be grown.                                            |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_VARIABLE_THRESHOLD``  | true    | VariableThreshold               | A flag indicating whether to determine the signal-to-noise  |
|                                |         | (:doc:`../analysis/thresholds`) | threshold on a pixel-by-pixel basis based on local          |
|                                |         |                                 | statistics (that is, the statistics within a relatively     |
|                                |         |                                 | small box centred on the pixel in question).                |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+
| ``SELAVY_BOX_SIZE``            | 50      | VariableThreshold.boxSize       | The half-width of the sliding box used to determine the     |
|                                |         | (:doc:`../analysis/thresholds`) | local statistics.                                           |
+--------------------------------+---------+---------------------------------+-------------------------------------------------------------+

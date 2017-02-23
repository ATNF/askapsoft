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

The RM Synthesis capabilities of Selavy have been made available. Full
control over this mode is provided through the pipeilne configuration,
so that RM Synthesis can be performed on the full-Stokes spectra of
continuum components. This is designed to work with the continuum cube
imaging (:doc:`ScienceFieldContinuumImaging`), and checks are made for
the existence of the required cubes at run-time for this mode to be
switched on.

+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| Variable                         |             Default             | Parset equivalent                   | Description                                                 |
+==================================+=================================+=====================================+=============================================================+
| ``DO_SOURCE_FINDING_CONT``       | ""                              | none                                | Whether to do the source-finding with Selavy on the         |
|                                  |                                 |                                     | final mosaic continuum images. If not given in the config   |
|                                  |                                 |                                     | file, it takes on the value of ``DO_CONT_IMAGING``.         |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
|  ``DO_SOURCE_FINDING_BEAMWISE``  | false                           | none                                | If true, the source-finding will be run on the individual   |
|                                  |                                 |                                     | beam images as well.                                        |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``JOB_TIME_SOURCEFINDING``       | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                | Time request for source-finding jobs.                       |
|                                  |                                 |                                     |                                                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| **Basic sourcefinding**          |                                 |                                     |                                                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+ 
| ``SELAVY_NSUBX``                 | 6                               | nsubx                               | Number of divisions in the x-direction that divide the image|
|                                  |                                 | (:doc:`../analysis/selavy`)         | up, allowing parallel processing in the source-detection.   |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_NSUBY``                 | 3                               | nsuby                               | Number of divisions in the y-direction that divide the image|
|                                  |                                 | (:doc:`../analysis/selavy`)         | up, allowing parallel processing in the source-detection.   |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SNR_CUT``               | 5.0                             | snrcut                              | The signal-to-noise ratio threshold to use in the           |
|                                  |                                 | (:doc:`../analysis/selavy`)         | source-detection.                                           |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_FLAG_GROWTH``           | true                            | flagGrowth                          | A flag indicating whether to grow detections down to a      |
|                                  |                                 | (:doc:`../analysis/selavy`)         | lower threshold.                                            |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_GROWTH_CUT``            | 3.0                             | growthCut                           | The secondary signal-to-noise threshold to which detections |
|                                  |                                 | (:doc:`../analysis/selavy`)         | should be grown.                                            |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_FLUX_THRESHOLD``        | ""                              | threshold                           | The flux threshold to use in the source-detection. If left  |
|                                  |                                 | (:doc:`../analysis/selavy`)         | blank, we use the SNR threshold ``SELAVY_SNR_CUT``.         |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_GROWTH_THRESHOLD``      | ""                              | growthCut                           | The secondary signal-to-noise threshold to which detections |
|                                  |                                 | (:doc:`../analysis/selavy`)         | should be grown. Only used if ``SELAVY_FLUX_THRESHOLD`` is  |
|                                  |                                 |                                     | given.                                                      |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
|  ``SELAVY_VARIABLE_THRESHOLD``   | true                            | VariableThreshold                   | A flag indicating whether to determine the signal-to-noise  |
|                                  |                                 | (:doc:`../analysis/thresholds`)     | threshold on a pixel-by-pixel basis based on local          |
|                                  |                                 |                                     | statistics (that is, the statistics within a relatively     |
|                                  |                                 |                                     | small box centred on the pixel in question).                |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_BOX_SIZE``              | 50                              | VariableThreshold.boxSize           | The half-width of the sliding box used to determine the     |
|                                  |                                 | (:doc:`../analysis/thresholds`)     | local statistics.                                           |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| **RM Synthesis**                 | 50                              | VariableThreshold.boxSize           | The half-width of the sliding box used to determine the     |
|                                  |                                 | (:doc:`../analysis/thresholds`)     | local statistics.                                           |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+ 
| ``DO_RM_SYNTHESIS``              | true                            | none                                | Whether to perform RM Synthesis after continuum             |
|                                  |                                 |                                     | source-finding.                                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_OUTPUT_BASE``       | pol                             | Forms part of                       | Base part of the filenames of extracted spectra and Faraday | 
|                                  |                                 | RMSynthesis.outputBase              | Dispersion function. All files will go in a directory       |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | PolData within the Selavy directory, and will be called     |
|                                  |                                 |                                     | "<outputBase>_<imageBase>_spec" or similar.                 |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+ 
| ``SELAVY_POL_WRITE_SPECTRA``     | true                            | RMSynthesis.writeSpectra            | Whether to write the extracted Stokes spectra to individual |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | files.                                                      |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_WRITE_COMPLEX_FDF`` | false                           | RMSynthesis.writeComplexFDF         | Whether to write the Faraday Dispersion Function for each   | 
|                                  |                                 | (:doc:`../analysis/postprocessing`) | source as a single complex-valued spectrum (true) or as a   |
|                                  |                                 |                                     | pair of real-valued spectra containing amplitude & phase    |
|                                  |                                 |                                     | (false).                                                    |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_BOX_WIDTH``         | 5                               | RMSynthesis.boxWidth                | The width (N) of the NxN box to be applied in the extraction|
|                                  |                                 | (:doc:`../analysis/postprocessing`) | of Stokes spectra.                                          | 
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_NOISE_AREA``        | 50                              | RMSynthesis.noiseArea               | The number of beam areas over which to measure the noise in |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | each channel.                                               |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+ 
| ``SELAVY_POL_ROBUST_STATS``      | true                            | RMSynthesis.robust                  | Whether to use robust statistics in the calculation of the  |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | noise spectra.                                              |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_WEIGHT_TYPE``       | variance                        | RMSynthesis.weightType              | The type of weighting to be used in the RM Synthesis -      |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | either "variance" or "uniform".                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_MODEL_TYPE``        | taylor                          | RMSynthesis.modelType               | The type of Stokes-I model to use. Either "taylor"          |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | (Taylor-term decomposition from the MFS imaging), or "poly" | 
|                                  |                                 |                                     | (polynomial fit to the Stokes-I spectrum".                  |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_MODEL_ORDER``       | 3                               | RMSynthesis.modelPolyOrder          | When ``SELAVY_POL_MODEL_TYPE=poly``, this gives the order of|
|                                  |                                 | (:doc:`../analysis/postprocessing`) | the polynomial that is fit to the Stokes-I spectrum.        |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_SNR_THRESHOLD``     | 8                               | RMSynthesis.polThresholdSNR         | Signal-to-noise threshold (in the FDF) for a valid          |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | detection.                                                  |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_DEBIAS_THRESHOLD``  | 5                               | RMSynthesis.polThresholdDebias      | Signal-to-noise threshold (in the FDF) above which to       |
|                                  |                                 | (:doc:`../analysis/postprocessing`) | perform debiasing.                                          |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_NUM_PHI_CHAN``      | 30                              | RMSynthesis.numPhiChan              | Number of Faraday Depth channels used in RM Synthesis.      |
|                                  |                                 | (:doc:`../analysis/postprocessing`) |                                                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_DELTA_PHI``         | 5                               | RMSynthesis.deltaPhi                | Spacing between the Faraday depth channels [rad/m2].        |
|                                  |                                 | (:doc:`../analysis/postprocessing`) |                                                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+
| ``SELAVY_POL_PHI_ZERO``          | 0                               | RMSynthesis.phiZero                 | Faraday depth [rad/m2] of the central channel of the FDF.   |
|                                  |                                 | (:doc:`../analysis/postprocessing`) |                                                             |
+----------------------------------+---------------------------------+-------------------------------------+-------------------------------------------------------------+

User Parameters - Spectral-line Source-finding
==============================================

It is possible to run source-finding with Selavy on the created
spectral-line cubes. Through the pipeline, it is possible to specify
either flux or SNR thresholds, apply a variable SNR threshold, and
pre-process the cube with smoothing or multi-resolution wavelet
reconstruction (to enhance the signal-to-noise of real sources).

+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| Variable                           |             Default             | Parset equivalent                            | Description                                                 |
+====================================+=================================+==============================================+=============================================================+
| ``DO_SOURCE_FINDING_SPEC``         | ""                              | none                                         | Whether to do the source-finding with Selavy on the         |
|                                    |                                 |                                              | final mosaic images. If not given in the config file, it    |
|                                    |                                 |                                              | takes on the value of ``DO_SPECTRAL_IMAGING``.              |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
|   ``DO_SOURCE_FINDING_BEAMWISE``   | false                           | none                                         | If true, the source-finding will be run on the individual   |
|                                    |                                 |                                              | beam images as well.                                        |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
|  ``JOB_TIME_SOURCEFINDING_SPEC``   | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                         | Time request for source-finding jobs                        |
|                                    |                                 |                                              |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``CPUS_PER_CORE_SELAVY_SPEC``      | ""                              | none                                         | Number of cores used on each node. If not provided, it will |
|                                    |                                 |                                              | be the lower of the number of cores requested or the maximum|
|                                    |                                 |                                              | number of cores available per node.                         | 
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_NSUBX``              | 6                               | nsubx (:doc:`../analysis/selavy`)            | Number of divisions in the x-direction that divide the image|
|                                    |                                 |                                              | up, allowing parallel processing in the source-detection.   |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_NSUBY``              | 3                               | nsuby (:doc:`../analysis/selavy`)            | Number of divisions in the y-direction that divide the image|
|                                    |                                 |                                              | up, allowing parallel processing in the source-detection.   |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_NSUBZ``              | 1                               | nsubz (:doc:`../analysis/selavy`)            | Number of divisions in the z-direction that divide the image|
|                                    |                                 |                                              | up, allowing parallel processing in the source-detection.   |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| **Searching**                      |                                 |                                              |                                                             |
|                                    |                                 |                                              |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_SNR_CUT``            | 5.0                             | snrcut (:doc:`../analysis/selavy`)           | The signal-to-noise ratio threshold to use in the           |
|                                    |                                 |                                              | source-detection.                                           |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_FLAG_GROWTH``        | true                            | flagGrowth (:doc:`../analysis/selavy`)       | A flag indicating whether to grow detections down to a      |
|                                    |                                 |                                              | lower threshold.                                            |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_GROWTH_CUT``         | 3.0                             | growthCut (:doc:`../analysis/selavy`)        | The secondary signal-to-noise threshold to which detections |
|                                    |                                 |                                              | should be grown.                                            |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_FLUX_THRESHOLD``     | ""                              | threshold (:doc:`../analysis/selavy`)        | The flux threshold to use in the source-detection. If left  |
|                                    |                                 |                                              | blank, we use the SNR threshold ``SELAVY_SNR_CUT``.         |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_GROWTH_THRESHOLD``   | ""                              | growthCut (:doc:`../analysis/selavy`)        | The secondary signal-to-noise threshold to which detections |
|                                    |                                 |                                              | should be grown. Only used if ``SELAVY_FLUX_THRESHOLD`` is  |
|                                    |                                 |                                              | given.                                                      |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_SEARCH_TYPE``        | spectral                        | searchType (:doc:`../analysis/selavy`)       | Type of searching to be performed: either 'spectral'        |
|                                    |                                 |                                              | (searches are done in each 1D spectrum) or 'spatial'        |
|                                    |                                 |                                              | (searches are done in each 2D channel image). Anything else |
|                                    |                                 |                                              | defaults to spectral.                                       |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_VARIABLE_THRESHOLD`` | true                            | VariableThreshold                            | A flag indicating whether to determine the signal-to-noise  |
|                                    |                                 | (:doc:`../analysis/thresholds`)              | threshold on a pixel-by-pixel basis based on local          |
|                                    |                                 |                                              | statistics (that is, the statistics within a relatively     |
|                                    |                                 |                                              | small box centred on the pixel in question). The dimensions |
|                                    |                                 |                                              | of the box are governed by the search type - if 'spectral'  |
|                                    |                                 |                                              | then it will be a one-dimensional box slid along each       |
|                                    |                                 |                                              | spectrum, else if 'spatial' it will be a 2D box done on each|
|                                    |                                 |                                              | channel image.                                              |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_BOX_SIZE``           | 50                              | VariableThreshold.boxSize                    | The half-width of the sliding box used to determine the     |
|                                    |                                 | (:doc:`../analysis/thresholds`)              | local statistics.                                           |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_MIN_PIX``            | 5                               | minPix (:doc:`../analysis/selavy`)           | Minimum number of (spatial) pixels allowed in a detection   |
|                                    |                                 |                                              |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_MIN_CHAN``           | 5                               | minChan (:doc:`../analysis/selavy`)          | Minimum number of channels allowed in a detection           |
|                                    |                                 |                                              |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_MAX_CHAN``           | 2592                            | maxChan (:doc:`../analysis/selavy`)          | Maximum number of channels allowed in a detection           |
|                                    |                                 |                                              |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| **Pre-processing**                 |                                 |                                              |                                                             |
|                                    |                                 |                                              |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_FLAG_SMOOTH``        | false                           | flagSmooth                                   | Whether to smooth the input cube prior to searching.        |
|                                    |                                 | (:doc:`../analysis/preprocessing`)           |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_SMOOTH_TYPE``        | spectral                        | smoothType                                   | Type of smoothing to perform - either 'spectral' or         |
|                                    |                                 | (:doc:`../analysis/preprocessing`)           | 'spatial'. Anything else defaults to spectral.              |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_HANN_WIDTH``         | 5                               | hanningWidth                                 | The width of the Hanning spectral smoothing kernel.         |
|                                    |                                 | (:doc:`../analysis/preprocessing`)           |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_SPATIAL_KERNEL``     | 3                               | kernMaj, kernMin, kernPA                     | The specs for the spatial Gaussian smoothing kernel. Either |
|                                    |                                 | (:doc:`../analysis/preprocessing`)           | a single number, which is interpreted as a circular Gaussian|
|                                    |                                 |                                              | (kernMaj=kernMin, kernPA=0), or a string with three values  |
|                                    |                                 |                                              | enclosed by square brackets (eg. "[4,3,45]"), interpreted as|
|                                    |                                 |                                              | "[kernMaj,kernMin,kernPA]".                                 |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_FLAG_WAVELET``       | false                           | flagAtrous                                   | Whether to use the multi-resolution wavelet reconstruction. |
|                                    |                                 | (:doc:`../analysis/preprocessing`)           |                                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_RECON_DIM``          | 1                               | reconDim (:doc:`../analysis/preprocessing`)  | The number of dimensions in which to perform the            |
|                                    |                                 |                                              | reconstruction. 1 means reconstruct each spectrum           |
|                                    |                                 |                                              | separately, 2 means each channel map is done separately, and|
|                                    |                                 |                                              | 3 means do the whole cube in one go.                        |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_RECON_SNR``          | 4                               | snrRecon (:doc:`../analysis/preprocessing`)  | Signal-to-noise threshold applied to wavelet arrays prior to|
|                                    |                                 |                                              | reconstruction.                                             |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_RECON_SCALE_MIN``    | 1                               | scaleMin (:doc:`../analysis/preprocessing`)  | Minimum wavelet scale to include in reconstruction. A value |
|                                    |                                 |                                              | of 1 means "use all scales”.                                |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+
| ``SELAVY_SPEC_RECON_SCALE_MAX``    | 0                               | scaleMax (:doc:`../analysis/preprocessing`)  | Maximum wavelet scale to use in the reconstruction. If 0 or |
|                                    |                                 |                                              | negative, then the maximum scale is calculated from the size|
|                                    |                                 |                                              | of the array.                                               |
+------------------------------------+---------------------------------+----------------------------------------------+-------------------------------------------------------------+

User Parameters - Continuum Self-calibration
============================================

An option for continuum imaging is to run self-calibration at the same
time. The algorithm here is as follows:

1. Image the data with cimager
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

+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| Variable                      | Default                   | Parset equivalent              | Description                                              |
+===============================+===========================+================================+==========================================================+
| ``DO_SELFCAL``                | true                      | none                           | Whether to self-calibrate the science data when imaging. |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_METHOD``            | Cmodel                    | none                           | How to do the self-calibration. There are two options:   |
|                               |                           |                                | "Cmodel" means create a model image from the             |
|                               |                           |                                | source-finding results; "Components" means use the       |
|                               |                           |                                | detected components directly through a parset (created by|
|                               |                           |                                | Selavy). Anything else will default to "Cmodel".         |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_INTERVAL``          | 10                        | interval                       | Interval [sec] over which to solve for self-calibration. |
|                               |                           | (:doc:`../calim/ccalibrator`)  |                                                          |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_NUM_LOOPS``         | 5                         | none                           | Number of loops of self-calibration.                     |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_KEEP_IMAGES``       | true                      | none                           | Should we keep the images from the intermediate selfcal  |
|                               |                           |                                | loops?                                                   |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_THRESHOLD``  | 15                        | snrCut                         | SNR threshold for detection with Selavy in determining   |
|                               |                           | (:doc:`../analysis/selavy`)    | selfcal sources.                                         |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBX``      | 6                         | nsubx                          | Division of image in x-direction for source-finding in   |
|                               |                           | (:doc:`../analysis/selavy`)    | selfcal.                                                 |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_WEIGHTSCUT`` | 0.95                      | Selavy.Weights.weightsCutoff   | Pixels with weight less than this fraction of the peak   |
|                               |                           | (:doc:`../analysis/thresholds`)| weight will not be considered by the source-finding. If  |
|                               |                           |                                | the value is negative, or more than one, no consideration|
|                               |                           |                                | of the weight is made.                                   |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBY``      | 3                         | nsuby                          | Division of image in y-direction for source-finding in   |
|                               |                           | (:doc:`../analysis/selavy`)    | selfcal.                                                 |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
|  ``SELFCAL_NORMALISE_GAINS``  | true                      | normalisegains                 | Whether to normalise the amplitudes of the gains to 1,   |
|                               |                           | (:doc:`../calim/ccalibrator`)  | approximating the phase-only self-calibration approach.  |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``SELFCAL_SCALENOISE``        | false                     | calibrate.scalenoise           | Whether the noise estimate will be scaled in accordance  |
|                               |                           | (:doc:`../calim/cimager`)      | with the applied calibrator factor to achieve proper     |
|                               |                           |                                | weighting.                                               |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+
| ``GAINS_CAL_TABLE``           | cont_gains_cal_beam%b.tab | none (directly)                | The table name to hold the final gains solution. Once    |
|                               |                           |                                | the self-cal loops have completed, the cal table in the  |
|                               |                           |                                | final loop is copied to a table of this name in the base |
|                               |                           |                                | directory. This can then be used for the spectral-line   |
|                               |                           |                                | imaging if need be. If this is blank, both ``DO_SELFCAL``|
|                               |                           |                                | and ``DO_APPLY_CAL_SL`` will be set to false.            |
|                               |                           |                                |                                                          |
+-------------------------------+---------------------------+--------------------------------+----------------------------------------------------------+

Once the gains solution has been determined, it can be applied
directly to the continuum measurement set, creating a copy in the
process. This is necessary for continuum cube processing, and for
archiving purposes.
This work is done as a separate slurm job, that starts upon
completion of the self-calibration job.

+-------------------------------+-----------------------------------+--------------------------------+----------------------------------------------------------+
| Variable                      | Default                           | Parset equivalent              | Description                                              |
+===============================+===================================+================================+==========================================================+
| ``DO_APPLY_CAL_CONT``         | true                              | none                           | Whether to apply the calibration to the averaged         |
|                               |                                   |                                | ("continuum") dataset.                                   |
+-------------------------------+-----------------------------------+--------------------------------+----------------------------------------------------------+
| ``JOB_TIME_CONT_APPLYCAL``    | ``JOB_TIME_DEFAULT`` (12:00:00)   | none                           | Time request for applying the calibration                |
+-------------------------------+-----------------------------------+--------------------------------+----------------------------------------------------------+
| ``KEEP_RAW_AV_MS``            | true                              | none                           | Whether to make a copy of the averaged MS before applying|
|                               |                                   |                                | the gains calibration (true), or to just overwrite with  |
|                               |                                   |                                | the calibrated data (false).                             |
+-------------------------------+-----------------------------------+--------------------------------+----------------------------------------------------------+
| ``DO_SELFCAL``                | false                             |                                | Whether to self-calibrate the science data when imaging. |
+-------------------------------+-----------------------------------+--------------------------------+----------------------------------------------------------+

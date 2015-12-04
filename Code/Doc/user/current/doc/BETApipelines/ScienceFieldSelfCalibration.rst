User Parameters - Continuum Self-calibration
============================================

An option for continuum imaging is to run self-calibration at the same
time. The algorithm here is as follows:

1. Image the data with cimager
2. Run source-finding with Selavy with a relatively large threshold
3. Create a component parset from the resulting component catalogue
4. Use this parset in ccalibrator to calibrate the antenna-based gains
5. Re-run imaging, applying the latest gains table
6. Repeat steps 2-5 for a given number of loops

Each loop gets its own directory, where the intermediate images,
source-finding results, and gains calibration table are stored. At the
end, the final gains calibration table is kept in the main output
directory (as this can then be used by the spectral-line imaging
pipeline). 

+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| Variable                     | Default                   | Parset equivalent            | Description                                              |
+==============================+===========================+==============================+==========================================================+
| ``DO_SELFCAL``               | false                     |                              | Whether to self-calibrate the science data when imaging. |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``SELFCAL_INTERVAL``         | 10                        | interval                     | Interval [sec] over which to solve for self-calibration. |
|                              |                           | (:doc:`../calim/ccalibrator`)|                                                          |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``SELFCAL_NUM_LOOPS``        | 5                         | none                         | Number of loops of self-calibration.                     |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``SELFCAL_KEEP_IMAGES``      | true                      | none                         | Should we keep the images from the intermediate selfcal  |
|                              |                           |                              | loops?                                                   |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_THRESHOLD`` | 15                        | snrCut                       | SNR threshold for detection with Selavy in determining   |
|                              |                           | (:doc:`../analysis/selavy`)  | selfcal sources.                                         |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBX``     | 6                         | nsubx                        | Division of image in x-direction for source-finding in   |
|                              |                           | (:doc:`../analysis/selavy`)  | selfcal.                                                 |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``SELFCAL_SELAVY_NSUBY``     | 3                         | nsuby                        | Division of image in y-direction for source-finding in   |
|                              |                           | (:doc:`../analysis/selavy`)  | selfcal.                                                 |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+
| ``GAINS_CAL_TABLE``          | cont_gains_cal_beam%b.tab | none (directly)              | The table name to hold the final gains solution. Once    |
|                              |                           |                              | the self-cal loops have completed, the cal table in the  |
|                              |                           |                              | final loop is copied to a table of this name in the base |
|                              |                           |                              | directory. This can then be used for the spectral-line   |
|                              |                           |                              | imaging if need be. If this is blank, both ``DO_SELFCAL``|
|                              |                           |                              | and ``DO_APPLY_CAL_SL`` will be set to false.            |
|                              |                           |                              |                                                          |
+------------------------------+---------------------------+------------------------------+----------------------------------------------------------+

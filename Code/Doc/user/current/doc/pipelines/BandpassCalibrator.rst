User Parameters - Bandpass Calibration
======================================

These parameters govern all processing used for the calibrator
observation. The requested measurement set is split up by beam and
scan, assuming that a given beam points at 1934-638 in the
correspondingly-numbered scan. While it is possible to use the
``BEAM_MIN`` and ``BEAM_MAX`` parameters to specify a given range of
beams to process for the science field, only the ``BEAM_MAX``
parameter is applied to the bandpass calibrator processing. All beams
up to ``BEAM_MAX`` are split & flagged, and have their bandpass solved
for. This is due to the particular requirements of
:doc:`../calim/cbpcalibrator`.

The MS is flagged in two passes. First, a combination of
selection rules (allowing flagging of antennas & baselines, and
autocorrelations) and (optionally) a simple flat amplitude threshold are
applied. Then dynamic flagging of amplitudes is done, integrating over
individual spectra. Each of these steps is selectable via input
parameters. 

Then the bandpass table is calculated with
:doc:`../calim/cbpcalibrator`, which requires MSs for all beams to be
given. This is a parallel job, the size of which is configurable
through ``NUM_CPUS_CBPCAL``.

If a second processing job is run with a higher ``BEAM_MAX`` (and
hence wanting to use beams not included in a previous bandpass
solution), the bandpass table is deleted and re-made once the new
beams are split and flagged.

The cbpcalibrator job can make use of a script from ACES to both plot
the bandpass solutions (as a function of antenna, beam and
polarisation), and to smooth the bandpass table so that outlying
points are interpolated over. This produces a second table (with
".smooth" appended to the name), which will then be applied to the
science data instead of the original. There are a number of parameters
that may be used to tweak the smoothing - this is intended to be an
interim solution until this functionality is implemented in
ASKAPsoft. 

+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| Variable                                | Default                               | Parset equivalent                                      | Description                                               |
+=========================================+=======================================+========================================================+===========================================================+
| ``DO_SPLIT_1934``                       | true                                  | none                                                   | Whether to split a given beam/scan from the input 1934 MS |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_SPLIT_1934``                 | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                                   | Time request for splitting the calibrator MS              |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_FLAG_1934``                        | true                                  | none                                                   | Whether to flag the splitted-out 1934 MS                  |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_FLAG_1934``                  | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                                   | Time request for flagging the calibrator MS               |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_FIND_BANDPASS``                    | true                                  | none                                                   | Whether to fit for the bandpass using all 1934-638 MSs    |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_FIND_BANDPASS``              | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                                   | Time request for finding the bandpass solution            |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Preparing the calibrator datasets**   |                                       |                                                        |                                                           |
|                                         |                                       |                                                        |                                                           |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``MS_BASE_1934``                        | 1934_SB%s_%b.ms                       | none                                                   | Base name for the 1934 measurement sets after splitting.  |
|                                         |                                       |                                                        | The wildcard %b will be replaced with the string "beamBB",|
|                                         |                                       |                                                        | where BB is the (zero-based) beam number, and             |
|                                         |                                       |                                                        | the %s will be replaced by the calibration scheduling     |
|                                         |                                       |                                                        | block ID.                                                 |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``CHAN_RANGE_1934``                     | ""                                    | channel (:doc:`../calim/mssplit`)                      | Channel range for splitting (1-based!). This range also   |
|                                         |                                       |                                                        | defines the internal variable ``NUM_CHAN_1934`` (which    |
|                                         |                                       |                                                        | replaces the previously-available parameter NUM_CHAN). The|
|                                         |                                       |                                                        | default is to use all available channels in the MS.       |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_DYNAMIC_AMPLITUDE_1934``      | true                                  | none                                                   | Whether to do the dynamic flagging, after the rule-based  |
|                                         |                                       |                                                        | and simple flat-amplitude flagging is done.               |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_1934``         |  4.0                                  | amplitude_flagger.threshold                            | Dynamic threshold applied to amplitudes when flagging 1934|
|                                         |                                       |  (:doc:`../calim/cflag`)                               | data [sigma]                                              |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DYNAMIC_1934_INTEGRATE_SPECTRA`` | true                                  | amplitude_flagger.integrateSpectra                     | Whether to integrate the spectra in time and flag channels|
|                                         |                                       | (:doc:`../calim/cflag`)                                | during the dynamic flagging task.                         |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_1934_SPECTRA`` |  4.0                                  | amplitude_flagger.integrateSpectra.threshold           | Dynamic threshold applied to amplitudes when flagging 1934|
|                                         |                                       | (:doc:`../calim/cflag`)                                | data in integrateSpectra mode [sigma]                     |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_DYNAMIC_1934_INTEGRATE_TIMES``  | false                                 | amplitude_flagger.integrateTimes                       | Whether to integrate across spectra and flag time samples |
|                                         |                                       | (:doc:`../calim/cflag`)                                | during the dynamic flagging task.                         |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_THRESHOLD_DYNAMIC_1934_TIMES``  |  4.0                                  | amplitude_flagger.integrateTimes.threshold             | Dynamic threshold applied to amplitudes when flagging 1934|
|                                         |                                       | (:doc:`../calim/cflag`)                                | data in integrateTimes mode [sigma]                       |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_STOKESV_1934``                | true                                  | none                                                   | Whether to do Stokes-V flagging, after the rule-based     |
|                                         |                                       |                                                        | and simple flat-amplitude flagging is done.               |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_1934``         |  4.0                                  | stokesv_flagger.threshold                              | Threshold applied to amplitudes when flagging Stokes-V in |
|                                         |                                       |  (:doc:`../calim/cflag`)                               | 1934 data [sigma]                                         |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_STOKESV_1934_INTEGRATE_SPECTRA`` | true                                  | stokesv_flagger.integrateSpectra                       | Whether to integrate the spectra in time and flag channels|
|                                         |                                       | (:doc:`../calim/cflag`)                                | during the Stokes-V flagging task.                        |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_1934_SPECTRA`` |  4.0                                  | stokesv_flagger.integrateSpectra.threshold             | Threshold applied to amplitudes when flagging Stokes-V    |
|                                         |                                       | (:doc:`../calim/cflag`)                                | in 1934 data in integrateSpectra mode [sigma]             |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_STOKESV_1934_INTEGRATE_TIMES``  | false                                 | stokesv_flagger.integrateTimes                         | Whether to integrate across spectra and flag time samples |
|                                         |                                       | (:doc:`../calim/cflag`)                                | during the Stokes-V flagging task.                        |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_THRESHOLD_STOKESV_1934_TIMES``  |  4.0                                  | stokesv_flagger.integrateTimes.threshold               | Threshold applied to amplitudes when flagging Stokes-V in |
|                                         |                                       | (:doc:`../calim/cflag`)                                | 1934 data in integrateTimes mode [sigma]                  |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_FLAT_AMPLITUDE_1934``         | false                                 | none                                                   | Whether to apply a simple ("flat") amplitude threshold to |
|                                         |                                       |                                                        | the 1934 data.                                            |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|    ``FLAG_THRESHOLD_AMPLITUDE_1934``    | 0.2                                   | amplitude_flagger.high (:doc:`../calim/cflag`)         | Simple amplitude threshold applied when flagging 1934     |
|                                         |                                       |                                                        | data.                                                     |
|                                         |                                       |                                                        | If set to blank (``FLAG_THRESHOLD_AMPLITUDE_1934=""``),   |
|                                         |                                       |                                                        | then no minimum value is applied.                         |
|                                         |                                       |                                                        | [value in hardware units - before calibration]            |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_THRESHOLD_AMPLITUDE_1934_LOW``  | 0.                                    | amplitude_flagger.low (:doc:`../calim/cflag`)          | Lower threshold for the simple amplitude flagging. If set |
|                                         |                                       |                                                        | to blank (``FLAG_THRESHOLD_AMPLITUDE_1934_LOW=""``), then |
|                                         |                                       |                                                        | no minimum value is applied.                              |
|                                         |                                       |                                                        | [value in hardware units - before calibration]            |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``ANTENNA_FLAG_1934``                   | ""                                    | selection_flagger.<rule>.antenna                       | Allows flagging of antennas or baselines. For example, to |
|                                         |                                       | (:doc:`../calim/cflag`)                                | flag out the 1-3 baseline, set this to "ak01&&ak03" (with |
|                                         |                                       |                                                        | the quote marks). See documentation for further details on|
|                                         |                                       |                                                        | format.                                                   |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_AUTOCORRELATION_1934``           | false                                 | selection_flagger.<rule>.autocorr                      | If true, then autocorrelations will be flagged.           |
|                                         |                                       |                                                        |                                                           |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Solving for the bandpass**            |                                       |                                                        |                                                           |
|                                         |                                       |                                                        |                                                           |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DIRECTION_1934``                      | "[19h39m25.036, -63.42.45.63, J2000]" | sources.field1.direction                               | Location of 1934-638, formatted for use in cbpcalibrator. |
|                                         |                                       | (:doc:`../calim/cbpcalibrator`)                        |                                                           |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``TABLE_BANDPASS``                      | calparameters_1934_bp_SB%s.tab        | calibaccess.table                                      | Name of the CASA table used for the bandpass calibration  |
|                                         |                                       | (:doc:`../calim/cbpcalibrator` and                     | parameters. If no leading directory is given, the table   |
|                                         |                                       | :doc:`../calim/ccalapply`)                             | will be put in the BPCAL directory. Otherwise, the table  |
|                                         |                                       |                                                        | is left where it is (this allows the user to specify a    |
|                                         |                                       |                                                        | previously-created table for use with the science         |
|                                         |                                       |                                                        | field). The %s will be replaced by the calibration        |
|                                         |                                       |                                                        | scheduling block ID.                                      |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SCALENOISE``                 | false                                 | calibrate.scalenoise (:doc:`../calim/ccalapply`)       | Whether the noise estimate will be scaled in accordance   |
|                                         |                                       |                                                        | with the applied calibrator factor to achieve proper      |
|                                         |                                       |                                                        | weighting.                                                |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``NCYCLES_BANDPASS_CAL``                | 50                                    | ncycles (:doc:`../calim/cbpcalibrator`)                | Number of cycles used in cbpcalibrator.                   |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``NUM_CPUS_CBPCAL``                     | 216                                   | none                                                   | The number of cpus allocated to the cbpcalibrator job. The|
|                                         |                                       |                                                        | job will use all 20 cpus on each node (the memory         |
|                                         |                                       |                                                        | footprint is small enough to allow this).                 |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_MINUV``                      | 200                                   | MinUV (:doc:`../calim/data_selection`)                 | Minimum UV distance [m] applied to data prior to solving  |
|                                         |                                       |                                                        | for the bandpass (used to exclude the short baselines).   |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Smoothing and plotting the bandpass** |                                       |                                                        |                                                           |
|                                         |                                       |                                                        |                                                           |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_BANDPASS_SMOOTH``                  | true                                  | none                                                   | Whether to produce a smoothed version of the bandpass     |
|                                         |                                       |                                                        | table, which will be applied to the science data.         |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_BANDPASS_PLOT``                    | true                                  | none                                                   | Whether to produce plots of the bandpass                  |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_AMP``                 | true                                  | none                                                   | Whether to smooth the amplitudes (if false, smoothing is  |
|                                         |                                       |                                                        | done on the real and imaginary values).                   |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_OUTLIER``             | true                                  | none                                                   | If true, only smooth/interpolate over outlier points      |
|                                         |                                       |                                                        | (based on the inter-quartile range).                      |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_FIT``                 | 0                                     | none                                                   | The order of the polynomial (if >=0) or the window size   |
|                                         |                                       |                                                        | (if <0) used in the smoothing.                            |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_THRESHOLD``           | 3.0                                   | none                                                   | The threshold level used for fitting to the bandpass.     |
+-----------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+



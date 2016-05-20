User Parameters - Bandpass Calibration
======================================

These parameters govern all processing used for the calibrator
observation. The requested measurement set is split up by beam and
scan, assuming that a given beam points at 1934-638 in the
correspondingly-numbered scan.

The MS is then flagged in two passes. First, a combination of
selection rules (allowing flagging of antennas & baselines, and
autocorrelations) and a simple flat amplitude threshold are
applied. Then dynamic flagging of amplitudes is done, integrating over
individual spectra. Each of these steps is selectable via input
parameters. 

Then the bandpass table is calculated with
:doc:`../calim/cbpcalibrator`, which requires MSs for all beams to be
given. This is a parallel job, the size of which is configurable
through ``NUM_CPUS_CBPCAL``.

+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| Variable                              | Default                               | Parset equivalent                                  | Description                                               |
+=======================================+=======================================+====================================================+===========================================================+
| ``DO_SPLIT_1934``                     | true                                  | none                                               | Whether to split a given beam/scan from the input 1934 MS |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_SPLIT_1934``               | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                               | Time request for splitting the calibrator MS              |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``DO_FLAG_1934``                      | true                                  | none                                               | Whether to flag the splitted-out 1934 MS                  |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_FLAG_1934``                | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                               | Time request for flagging the calibrator MS               |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``DO_FIND_BANDPASS``                  | true                                  | none                                               | Whether to fit for the bandpass using all 1934-638 MSs    |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_FIND_BANDPASS``            | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                               | Time request for finding the bandpass solution            |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``MS_BASE_1934``                      | 1934_beam%b.ms                        | none                                               | Base name for the 1934 measurement sets after splitting.  |
|                                       |                                       |                                                    | The wildcard %b will be replaced with the beam number.    |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``CHAN_RANGE_1934``                   | "1-16416"                             | channel (:doc:`../calim/mssplit`)                  | Channel range for splitting (1-based!). This range also   |
|                                       |                                       |                                                    | defines the internal variable ``NUM_CHAN_1934`` (which    |
|                                       |                                       |                                                    | replaces the previously-available parameter NUM_CHAN)     |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_DYNAMIC_AMPLITUDE_1934``    | true                                  | none                                               | Whether to do the dynamic flagging, after the rule-based  |
|                                       |                                       |                                                    | and simple flat-amplitude flagging is done.               |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_1934``       |  4.0                                  | amplitude_flagger.threshold                        | Dynamic threshold applied to amplitudes when flagging 1934|
|                                       |                                       | amplitude_flagger.integrateSpectra.threshold       | data [sigma]                                              |
|                                       |                                       | (:doc:`../calim/cflag`)                            |                                                           |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_FLAT_AMPLITUDE_1934``       | true                                  | none                                               | Whether to apply a simple ("flat") amplitude threshold to |
|                                       |                                       |                                                    | the 1934 data.                                            |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
|   ``FLAG_THRESHOLD_AMPLITUDE_1934``   | 0.2                                   | amplitude_flagger.high (:doc:`../calim/cflag`)     | Simple amplitude threshold applied when flagging 1934     |
|                                       |                                       |                                                    | data.                                                     |
|                                       |                                       |                                                    | If set to blank (``FLAG_THRESHOLD_AMPLITUDE_1934=""``),   |
|                                       |                                       |                                                    | then no minimum value is applied.                         |
|                                       |                                       |                                                    | [value in hardware units - before calibration]            |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_AMPLITUDE_1934_LOW`` | 0.                                    | amplitude_flagger.low (:doc:`../calim/cflag`)      | Lower threshold for the simple amplitude flagging. If set |
|                                       |                                       |                                                    | to blank (``FLAG_THRESHOLD_AMPLITUDE_1934_LOW=""``), then |
|                                       |                                       |                                                    | no minimum value is applied.                              |
|                                       |                                       |                                                    | [value in hardware units - before calibration]            |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``ANTENNA_FLAG_1934``                 | ""                                    | selection_flagger.<rule>.antenna                   | Allows flagging of antennas or baselines. For example, to |
|                                       |                                       | (:doc:`../calim/cflag`)                            | flag out the 1-3 baseline, set this to "ak01&&ak03" (with |
|                                       |                                       |                                                    | the quote marks). See documentation for further details on|
|                                       |                                       |                                                    | format.                                                   |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_AUTOCORRELATION_1934``         | false                                 | selection_flagger.<rule>.autocorr                  | If true, then autocorrelations will be flagged.           |
|                                       |                                       |                                                    |                                                           |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``DIRECTION_1934``                    | "[19h39m25.036, -63.42.45.63, J2000]" | sources.field1.direction                           | Location of 1934-638, formatted for use in cbpcalibrator. |
|                                       |                                       | (:doc:`../calim/cbpcalibrator`)                    |                                                           |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``TABLE_BANDPASS``                    | calparameters_1934_bp.tab             | calibaccess.table                                  | Name of the CASA table used for the bandpass calibration  |
|                                       |                                       | (:doc:`../calim/cbpcalibrator` and                 | parameters.                                               |
|                                       |                                       | :doc:`../calim/ccalapply`)                         |                                                           |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SCALENOISE``               | false                                 | calibrate.scalenoise (:doc:`../calim/ccalapply`)   | Whether the noise estimate will be scaled in accordance   |
|                                       |                                       |                                                    | with the applied calibrator factor to achieve proper      |
|                                       |                                       |                                                    | weighting.                                                |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``NCYCLES_BANDPASS_CAL``              | 25                                    | ncycles (:doc:`../calim/cbpcalibrator`)            | Number of cycles used in cbpcalibrator.                   |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``NUM_CPUS_CBPCAL``                   | 400                                   | none                                               | The number of cpus allocated to the cbpcalibrator job. The|
|                                       |                                       |                                                    | job will use all 20 cpus on each node (the memory         |
|                                       |                                       |                                                    | footprint is small enough to allow this).                 |
+---------------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+



User Parameters - Bandpass Calibration
======================================

These parameters govern all processing used for the calibrator
observation. The requested measurement set is split up by beam and
scan, assuming that a given beam points at 1934-638 in the
correspondingly-numbered scan.

The MS is then flagged twices: first a dynamic flag is applied
(integrating over individual spectra), then a straight amplitude cut
is applied to remove any remaining spikes. The dynamic flagging step
can also optionally include antenna or baseline flagging.

Then the bandpass table is calculated with
:doc:`../calim/cbpcalibrator`, which requires MSs for all beams to be
given. This is a parallel job, the size of which is configurable
through ``NUM_CPUS_CBPCAL``.

+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| Variable                          | Default                               | Parset equivalent                                  | Description                                               |
+===================================+=======================================+====================================================+===========================================================+
| ``DO_SPLIT_1934``                 | true                                  | none                                               | Whether to split a given beam/scan from the input 1934 MS |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``DO_FLAG_1934``                  | true                                  | none                                               | Whether to flag the splitted-out 1934 MS                  |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``DO_FIND_BANDPASS``              | true                                  | none                                               | Whether to fit for the bandpass using all 1934-638 MSs    |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``MS_BASE_1934``                  | 1934_beam%b.ms                        | none                                               | Base name for the 1934 measurement sets after splitting.  |
|                                   |                                       |                                                    | The wildcard %b will be replaced with the beam number.    |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``CHAN_RANGE_1934``               | "1-16416"                             | channel (:doc:`../calim/mssplit`)                  | Channel range for splitting (1-based!). This range also   |
|                                   |                                       |                                                    | defines the internal variable ``NUM_CHAN_1934`` (which    |
|                                   |                                       |                                                    | replaces the previously-available parameter NUM_CHAN)     |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_1934``   |  4.0                                  | amplitude_flagger.threshold                        | Dynamic threshold applied to amplitudes when flagging 1934|
|                                   |                                       | amplitude_flagger.integrateSpectra.threshold       | data [sigma]                                              |
|                                   |                                       | (:doc:`../calim/cflag`)                            |                                                           |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_AMPLITUDE_1934`` | 0.2                                   | amplitude_flagger.high (:doc:`../calim/cflag`)     | Second amplitude threshold applied when flagging 1934 data|
|                                   |                                       |                                                    | [hardware units - before calibration]                     |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``ANTENNA_FLAG_1934``             | ""                                    | selection_flagger.<rule>.antenna                   | Allows flagging of antennas or baselines. For example, to |
|                                   |                                       | (:doc:`../calim/cflag`)                            | flag out the 1-3 baseline, set this to "ak01&&ak03" (with |
|                                   |                                       |                                                    | the quote marks). See documentation for further details on|
|                                   |                                       |                                                    | format.                                                   |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``DIRECTION_1934``                | "[19h39m25.036, -63.42.45.63, J2000]" | sources.field1.direction                           | Location of 1934-638, formatted for use in cbpcalibrator. |
|                                   |                                       | (:doc:`../calim/cbpcalibrator`)                    |                                                           |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``TABLE_BANDPASS``                | calparameters_1934_bp.tab             | calibaccess.table                                  | Name of the CASA table used for the bandpass calibration  |
|                                   |                                       | (:doc:`../calim/cbpcalibrator` and                 | parameters.                                               |
|                                   |                                       | :doc:`../calim/ccalapply`)                         |                                                           |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``NCYCLES_BANDPASS_CAL``          | 25                                    | ncycles (:doc:`../calim/cbpcalibrator`)            | Number of cycles used in cbpcalibrator.                   |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+
| ``NUM_CPUS_CBPCAL``               | 400                                   | none                                               | The number of cpus allocated to the cbpcalibrator job. The|
|                                   |                                       |                                                    | job will use all 20 cpus on each node (the memory         |
|                                   |                                       |                                                    | footprint is small enough to allow this).                 |
+-----------------------------------+---------------------------------------+----------------------------------------------------+-----------------------------------------------------------+



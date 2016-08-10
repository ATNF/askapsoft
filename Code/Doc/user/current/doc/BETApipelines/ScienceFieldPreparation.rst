User Parameters - Preparation of the Science field data
=======================================================

These parameters govern the pre-processing needed for the science
field, to split by beam, flag, apply the bandpass (although none of
the parameters listed here relate to that), and average to
continuum resolution.

The splitting is done by beam, and optionally by particular scans
and/or fields (where the latter are selected on the basis of the FIELD
NAME in the MS). The flagging is done in the same manner as for the
calibrator observation.

The averaging scale defaults to 54 channels, resulting in a
1MHz-resolution MS that can be imaged with cimager, although this
averaging scale can be changed by the user. 


+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| Variable                                  | Default                         | Parset equivalent                               | Description                                                           |
+===========================================+=================================+=================================================+=======================================================================+
| ``DO_SPLIT_SCIENCE``                      | true                            | none                                            | Whether to split out the given beam from the science MS               |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_SPLIT_SCIENCE``                | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for splitting the science MS                             |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_FLAG_SCIENCE``                       | true                            | none                                            | Whether to flag the (splitted) science MS                             |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_FLAG_SCIENCE``                 | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for flagging the science MS                              |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_APPLY_BANDPASS``                     | true                            | none                                            | Whether to apply the bandpass calibration to the science              |
|                                           |                                 |                                                 | observation                                                           |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_APPLY_BANDPASS``               | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for applying the bandpass to the science data            |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_AVERAGE_CHANNELS``                   | true                            | none                                            | Whether to average the science MS to continuum resolution             |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_AVERAGE_MS``                   | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for averaging the channels of the science data           |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``SCAN_SELECTION_SCIENCE``                |  no default (see description)   | scans (:doc:`../calim/mssplit`)                 | This allows selection of particular scans from the science            |
|                                           |                                 |                                                 | observation. If not provided, no scan selection is done (all scans are|
|                                           |                                 |                                                 | included in the output MS).                                           |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FIELD_SELECTION_SCIENCE``               |  no default (see description)   | fields (:doc:`../calim/mssplit`)                | This allows selection of particular FIELD NAMEs from the science      |
|                                           |                                 |                                                 | observation. If not provided, no field selection is done.             |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``MS_BASE_SCIENCE``                       |  scienceObservation_beam%b.ms   | none                                            | Base name for the science observation measurement set after           |
|                                           |                                 |                                                 | splitting. The wildcard %b will be replaced by the                    |
|                                           |                                 |                                                 | beam number (scienceObservation_beam0.ms etc).                        |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``MS_SCIENCE_AVERAGE``                    |  no default (see description)   | dataset (:doc:`../calim/cimager`)               | The name of the averaged measurement set that will be                 |
|                                           |                                 |                                                 | imaged by the continuum imager. Provide this if you want              |
|                                           |                                 |                                                 | to skip the bandpass calibration and averaging steps                  |
|                                           |                                 |                                                 | (perhaps you've already done them). The wildcard %b, if               |
|                                           |                                 |                                                 | present, will be replaced with the beam number. If not                |
|                                           |                                 |                                                 | provided, the averaged MS name will be derived from                   |
|                                           |                                 |                                                 | ``MS_BASE_SCIENCE``, with ".ms" replaced with                         |
|                                           |                                 |                                                 | "_averaged.ms".                                                       |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``CHAN_RANGE_SCIENCE``                    | ""                              | channel (:doc:`../calim/mssplit`)               | Range of channels in science observation (used in splitting and       |
|                                           |                                 |                                                 | averaging). This must (for now) be the same as                        |
|                                           |                                 |                                                 | ``CHAN_RANGE_1934``. The default is to use all available channels from|
|                                           |                                 |                                                 | the MS.                                                               |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``NUM_CHAN_TO_AVERAGE``                   | 54                              | stman.tilenchan (:doc:`../calim/mssplit`)       | Number of channels to be averaged to create continuum                 |
|                                           |                                 |                                                 | measurement set. Also determines the tile size when                   |
|                                           |                                 |                                                 | creating the MS.                                                      |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE``     | true                            | none                                            | Whether to do the dynamic flagging, after the rule-based              |
|                                           |                                 |                                                 | and simple flat-amplitude flagging is done                            |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_SCIENCE``        | 4.0                             | amplitude_flagger.threshold                     |                                                                       |
|                                           |                                 | amplitude_flagger.integrateSpectra.threshold    | Dynamic threshold applied to amplitudes when flagging                 |
|                                           |                                 | (:doc:`../calim/cflag`)                         | science field data [sigma]                                            |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_FLAT_AMPLITUDE_SCIENCE``        | true                            | none                                            |                                                                       |
|                                           |                                 |                                                 |                                                                       |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
|   ``FLAG_THRESHOLD_AMPLITUDE_SCIENCE``    | 0.2                             | amplitude_flagger.high (:doc:`../calim/cflag`)  | Simple amplitude threshold applied when flagging science field data.  |
|                                           |                                 |                                                 | If set to blank (``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""``),        |
|                                           |                                 |                                                 | then no minimum value is applied.                                     |
|                                           |                                 |                                                 | [hardware units - before calibration]                                 |
|                                           |                                 |                                                 |                                                                       |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ```FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW`` | 0.                              | amplitude_flagger.low (:doc:`../calim/cflag`)   | Lower threshold for the simple amplitude flagging. If set             |
|                                           |                                 |                                                 | to blank (``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""``),               |
|                                           |                                 |                                                 | then no minimum value is applied.                                     |
|                                           |                                 |                                                 | [value in hardware units - before calibration]                        |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``ANTENNA_FLAG_SCIENCE``                  | ""                              | selection_flagger.<rule>.antenna                | Allows flagging of antennas or baselines. For example, to             |
|                                           |                                 | (:doc:`../calim/cflag`)                         | flag out the 1-3 baseline, set this to "ak01&&ak03" (with             |
|                                           |                                 |                                                 | the quote marks). See documentation for further details on            |
|                                           |                                 |                                                 | format.                                                               |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_AUTOCORRELATION_SCIENCE``          | false                           | selection_flagger.<rule>.autocorr               | If true, then autocorrelations will be flagged.                       |
+-------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+

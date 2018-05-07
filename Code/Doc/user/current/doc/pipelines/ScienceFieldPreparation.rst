User Parameters - Preparation of the Science field data
=======================================================

These parameters govern the pre-processing needed for the science
field, to split by beam, apply the bandpass (although none of
the parameters listed here relate to that), flag, and average to
continuum resolution.

The splitting is done by beam, and optionally by particular scans
and/or fields (where the latter are selected on the basis of the FIELD
NAME in the MS).

As noted in :doc:`DataLocationSelection`, when an observation was
taken in one-field-per-beam mode, and no selection on scans or
channels is done, and there is only a single beam in the MS, then the
beam MSs are copied instead of using *mssplit*. This will run much
faster.

Once copied or split, the raw measurement set is first calibrated with
the bandpass calibration table derived previously (see
:doc:`BandpassCalibrator`). Once calibrated, the dataset will be
flagged to remove interference.

As is the case for the bandpass calibrator dataset, the MS is flagged in two
passes. First, a combination of selection rules (allowing flagging of
channels, time ranges, antennas & baselines, and autocorrelations) and (optionally)
a simple flat amplitude threshold are applied. Then a sequence of
Stokes-V flagging and dynamic flagging of amplitudes is done,
optionally integrating over or across individual spectra. Each of
these steps is selectable via input parameters.

Again, there is an option to use the
AOFlagger tool (written by Andre Offringa) to do the flagging. This
can be turned on by ``FLAG_WITH_AOFLAGGER``, or
``FLAG_SCIENCE_WITH_AOFLAGGER`` & ``FLAG_SCIENCE_AV_WITH_AOFLAGGER``
(to just do it for the full-spectral-resolution or averaged science
data respectively). You can provide a strategy file via
``AOFLAGGER_STRATEGY`` or ``AOFLAGGER_STRATEGY_SCIENCE`` &
``AOFLAGGER_STRATEGY_SCIENCE_AV``, with access to some of the
aoflagger parameters provided - see the table below. These strategy
files need to be created prior to running the pipeline.

The dataset used for continuum imaging is created by averaging the
frequency channels. The averaging scale defaults to 54 channels, resulting in a
1MHz-resolution MS that can be imaged with cimager, although this
averaging scale can be changed by the user.

Once the averaged dataset has been created, a second round of flagging
can be done on it, to flag any additional features that the averaging
process may have enhanced.

The default behaviour is to process all fields within the science MS
(interleaving, for instance, makes use of multiple fields), with each
field being processed in its own sub-directory. The field selection is
done in the splitting task, at the same time as the beam selection. It
is possible, however, to select a single field to process via the
``FIELD_SELECTION_SCIENCE`` parameter (by giving the field **name**). 


+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| Variable                                      | Default                         | Parset equivalent                               | Description                                                           |
+===============================================+=================================+=================================================+=======================================================================+
| **Job selection**                             |                                 |                                                 |                                                                       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_SPLIT_SCIENCE``                          | true                            | none                                            | Whether to split out the given beam from the science MS               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_SPLIT_SCIENCE``                    | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for splitting the science MS                             |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_FLAG_SCIENCE``                           | true                            | none                                            | Whether to flag the (splitted) science MS                             |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_FLAG_SCIENCE``                     | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for flagging the science MS                              |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_APPLY_BANDPASS``                         | true                            | none                                            | Whether to apply the bandpass calibration to the science              |
|                                               |                                 |                                                 | observation                                                           |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_APPLY_BANDPASS``                   | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for applying the bandpass to the science data            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``DO_AVERAGE_CHANNELS``                       | true                            | none                                            | Whether to average the science MS to continuum resolution             |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``JOB_TIME_AVERAGE_MS``                       | ``JOB_TIME_DEFAULT`` (12:00:00) | none                                            | Time request for averaging the channels of the science data           |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| **Data selection**                            |                                 |                                                 |                                                                       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``SCAN_SELECTION_SCIENCE``                    |  no default (see description)   | scans (:doc:`../calim/mssplit`)                 | This allows selection of particular scans from the science            |
|                                               |                                 |                                                 | observation. If not provided, no scan selection is done (all scans are|
|                                               |                                 |                                                 | included in the output MS).                                           |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FIELD_SELECTION_SCIENCE``                   |  no default (see description)   | fields (:doc:`../calim/mssplit`)                | This allows selection of particular FIELD NAMEs from the science      |
|                                               |                                 |                                                 | observation. If not provided, all fields are done. The value must be  |
|                                               |                                 |                                                 | just the field name - not surrounded by square brackets (which is a   |
|                                               |                                 |                                                 | possible format for mssplit.fields). This is because the value iwll be|
|                                               |                                 |                                                 | matched to field names from the measurement set.                      |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``MS_BASE_SCIENCE``                           |     scienceData_SB%s_%b.ms      | none                                            | Base name for the science observation measurement set after           |
|                                               |                                 |                                                 | splitting. The wildcard %b will be replaced by the string             |
|                                               |                                 |                                                 | "FIELD_beamBB", where FIELD represents the FIELD id, and BB the       |
|                                               |                                 |                                                 | (zero-based) beam number (scienceData_SB1234_LMC_beam00.ms etc), and  |
|                                               |                                 |                                                 | the %s will be replaced by the scheduling block ID.                   |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``MS_SCIENCE_AVERAGE``                        |  no default (see description)   | dataset (:doc:`../calim/cimager`)               | The name of the averaged measurement set that will be                 |
|                                               |                                 |                                                 | imaged by the continuum imager. Provide this if you want              |
|                                               |                                 |                                                 | to skip the bandpass calibration and averaging steps                  |
|                                               |                                 |                                                 | (perhaps you've already done them). The wildcard %b, if               |
|                                               |                                 |                                                 | present, will be replaced with "FIELD_beamBB", as described above. If |
|                                               |                                 |                                                 | not provided, the averaged MS name will be derived from               |
|                                               |                                 |                                                 | ``MS_BASE_SCIENCE``, with ".ms" replaced with "_averaged.ms".         |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``CHAN_RANGE_SCIENCE``                        | ""                              | channel (:doc:`../calim/mssplit`)               | Range of channels in science observation (used in splitting and       |
|                                               |                                 |                                                 | averaging). This must (for now) be the same as                        |
|                                               |                                 |                                                 | ``CHAN_RANGE_1934``. The default is to use all available channels from|
|                                               |                                 |                                                 | the MS.                                                               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``NUM_CHAN_TO_AVERAGE``                       | 54                              | stman.tilenchan (:doc:`../calim/mssplit`)       | Number of channels to be averaged to create continuum                 |
|                                               |                                 |                                                 | measurement set. Also determines the tile size when                   |
|                                               |                                 |                                                 | creating the MS.                                                      |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| **Initial flagging**                          |                                 |                                                 |                                                                       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE``         | true                            | none                                            | Whether to do the dynamic flagging, after the rule-based              |
|                                               |                                 |                                                 | and simple flat-amplitude flagging is done                            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_SCIENCE``            | 4.0                             | amplitude_flagger.threshold                     | Dynamic threshold applied to amplitudes when flagging science field   |
|                                               |                                 | (:doc:`../calim/cflag`)                         | data [sigma]                                                          |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DYNAMIC_INTEGRATE_SPECTRA``            | true                            | amplitude_flagger.integrateSpectra              | Whether to integrate the spectra in time and flag channels during the |
|                                               |                                 | (:doc:`../calim/cflag`)                         | dynamic flagging task.                                                |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
|  ``FLAG_THRESHOLD_DYNAMIC_SCIENCE_SPECTRA``   | 4.0                             | amplitude_flagger.integrateSpectra.threshold    | Dynamic threshold applied to amplitudes when flagging science field   |
|                                               |                                 | (:doc:`../calim/cflag`)                         | data in integrateSpectra mode [sigma]                                 |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DYNAMIC_INTEGRATE_TIMES``              | false                           | amplitude_flagger.integrateTimes                | Whether to integrate across spectra and flag time samples during the  |
|                                               |                                 | (:doc:`../calim/cflag`)                         | dynamic flagging task.                                                |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
|   ``FLAG_THRESHOLD_DYNAMIC_SCIENCE_TIMES``    | 4.0                             | amplitude_flagger.integrateTimes.threshold      | Dynamic threshold applied to amplitudes when flagging science field   |
|                                               |                                 | (:doc:`../calim/cflag`)                         | data in integrateTimes mode [sigma]                                   |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_STOKESV_SCIENCE``                   | true                            | none                                            | Whether to do the Stokes-V flagging on the science data, after the    |
|                                               |                                 |                                                 | rule-based and simple flat-amplitude flagging is done                 |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE``     | true                            | stokesv_flagger.useRobustStatistics             | Whether to use robust statistics (median and inter-quartile range) in |
|                                               |                                 | (:doc:`../calim/cflag`)                         | computing the Stokes-V statistics.                                    |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_SCIENCE``            | 4.0                             | stokesv_flagger.threshold                       | Threshold applied to amplitudes when flagging the Stokes-V for the    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | science field data [sigma]                                            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_STOKESV_INTEGRATE_SPECTRA``            | true                            | stokesv_flagger.integrateSpectra                | Whether to integrate the spectra in time and flag channels during the |
|                                               |                                 | (:doc:`../calim/cflag`)                         | Stokes-V flagging task.                                               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
|  ``FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA``   | 4.0                             | stokesv_flagger.integrateSpectra.threshold      | Threshold applied to amplitudes when flagging the Stokes-V for the    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | science field data in integrateSpectra mode [sigma]                   |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_STOKESV_INTEGRATE_TIMES``              | false                           | stokesv_flagger.integrateTimes                  | Whether to integrate across spectra and flag time samples during the  |
|                                               |                                 | (:doc:`../calim/cflag`)                         | Stokes-V flagging task.                                               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES``      | 4.0                             | stokesv_flagger.integrateTimes.threshold        | Threshold applied to amplitudes when flagging the Stokes-V for the    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | science field data in integrateTimes mode [sigma]                     |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_FLAT_AMPLITUDE_SCIENCE``            | false                           | none                                            | Whether to apply a flag amplitude flux threshold to the data.         |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_AMPLITUDE_SCIENCE``          | 0.2                             | amplitude_flagger.high (:doc:`../calim/cflag`)  | Simple amplitude threshold applied when flagging science field data.  |
|                                               |                                 |                                                 | If set to blank (``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""``),        |
|                                               |                                 |                                                 | then no minimum value is applied.                                     |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ```FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW``     | ""                              | amplitude_flagger.low (:doc:`../calim/cflag`)   | Lower threshold for the simple amplitude flagging. If set             |
|                                               |                                 |                                                 | to blank (``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""``),               |
|                                               |                                 |                                                 | then no minimum value is applied.                                     |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``ANTENNA_FLAG_SCIENCE``                      | ""                              | selection_flagger.<rule>.antenna                | Allows flagging of antennas or baselines. For example, to             |
|                                               |                                 | (:doc:`../calim/cflag`)                         | flag out the 1-3 baseline, set this to "ak01&&ak03" (with             |
|                                               |                                 |                                                 | the quote marks). See documentation for further details on            |
|                                               |                                 |                                                 | format.                                                               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``CHANNEL_FLAG_SCIENCE``                      | ""                              | selection_flagger.<rule>.spw                    | Allows flagging of a specified range of channels. For example, to flag|
|                                               |                                 | (:doc:`../calim/cflag`)                         | out the first 100 channnels, use "0:0~16" (with the quote marks). See |
|                                               |                                 |                                                 | the documentation for further details on the format.                  |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``TIME_FLAG_SCIENCE``                         | ""                              | selection_flagger.<rule>.timerange              | Allows flagging of a specified time range(s). The string given is     |
|                                               |                                 | (:doc:`../calim/cflag`)                         | passed directly to the ``timerange`` option of cflag's selection      |
|                                               |                                 |                                                 | flagger. For details on the possible syntax, consult the `MS          |
|                                               |                                 |                                                 | selection`_ documentation.                                            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_AUTOCORRELATION_SCIENCE``              | false                           | selection_flagger.<rule>.autocorr               | If true, then autocorrelations will be flagged.                       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| **Flagging of averaged data**                 |                                 |                                                 |                                                                       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_AFTER_AVERAGING``                      | true                            | none                                            | Whether to do an additional step of flagging on the channel-averaged  |
|                                               |                                 |                                                 | MS proior to imaging.                                                 |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE_AV``      | true                            | none                                            | Whether to do the dynamic flagging on the averaged science data, after|
|                                               |                                 |                                                 | the simple flat-amplitude flagging is done                            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_SCIENCE_AV``         | 4.0                             | amplitude_flagger.threshold                     | Dynamic threshold applied to amplitudes when flagging the averaged    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | science field data [sigma]                                            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DYNAMIC_INTEGRATE_SPECTRA_AV``         | true                            | amplitude_flagger.integrateSpectra              | Whether to integrate the spectra in time and flag channels during the |
|                                               |                                 | (:doc:`../calim/cflag`)                         | dynamic flagging task.                                                |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_SCIENCE_SPECTRA_AV`` | 4.0                             | amplitude_flagger.integrateSpectra.threshold    | Dynamic threshold applied to amplitudes when flagging the averaged    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | science field data in integrateSpectra mode [sigma]                   |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DYNAMIC_INTEGRATE_TIMES_AV``           | false                           | amplitude_flagger.integrateTimes                | Whether to integrate across spectra and flag time samples during the  |
|                                               |                                 | (:doc:`../calim/cflag`)                         | dynamic flagging task.                                                |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_SCIENCE_TIMES_AV``   | 4.0                             | amplitude_flagger.integrateTimes.threshold      | Dynamic threshold applied to amplitudes when flagging the averaged    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | science field data in integrateTimes mode [sigma]                     |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_STOKESV_SCIENCE_AV``                | true                            | none                                            | Whether to do the Stokes-V flagging on the averaged science data,     |
|                                               |                                 |                                                 | after the rule-based and simple flat-amplitude flagging is done       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE_AV``  | true                            | stokesv_flagger.useRobustStatistics             | Whether to use robust statistics (median and inter-quartile range) in |
|                                               |                                 | (:doc:`../calim/cflag`)                         | computing the Stokes-V statistics.                                    |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_SCIENCE_AV``         | 4.0                             | stokesv_flagger.threshold                       | Threshold applied to amplitudes when flagging the Stokes-V for the    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | averaged science field data [sigma]                                   |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_STOKESV_INTEGRATE_SPECTRA_AV``         | true                            | stokesv_flagger.integrateSpectra                | Whether to integrate the spectra in time and flag channels during the |
|                                               |                                 | (:doc:`../calim/cflag`)                         | Stokes-V flagging task.                                               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA_AV`` | 4.0                             | stokesv_flagger.integrateSpectra.threshold      | Threshold applied to amplitudes when flagging the Stokes-V for the    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | averaged science field data in integrateSpectra mode [sigma]          |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_STOKESV_INTEGRATE_TIMES_AV``           | false                           | stokesv_flagger.integrateTimes                  | Whether to integrate across spectra and flag time samples during the  |
|                                               |                                 | (:doc:`../calim/cflag`)                         | Stokes-V flagging task.                                               |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
|  ``FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES_AV``  | 4.0                             | stokesv_flagger.integrateTimes.threshold        | Threshold applied to amplitudes when flagging the Stokes-V for the    |
|                                               |                                 | (:doc:`../calim/cflag`)                         | averaged science field data in integrateTimes mode [sigma]            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_DO_FLAT_AMPLITUDE_SCIENCE_AV``         | false                           | none                                            | Whether to apply a flag amplitude flux threshold to the averaged      |
|                                               |                                 |                                                 | science data.                                                         |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_AV``       | 0.2                             | amplitude_flagger.high (:doc:`../calim/cflag`)  | Simple amplitude threshold applied when flagging the averaged science |
|                                               |                                 |                                                 | field data. If set to blank                                           |
|                                               |                                 |                                                 | (``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""``),                        |
|                                               |                                 |                                                 | then no minimum value is applied. [value in flux-calibrated units]    |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV``   | ""                              | amplitude_flagger.low (:doc:`../calim/cflag`)   | Lower threshold for the simple amplitude flagging on the averaged     |
|                                               |                                 |                                                 | data. If set to blank (``FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""``),  |
|                                               |                                 |                                                 | then no minimum value is applied. [value in flux-calibrated units]    |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``CHANNEL_FLAG_SCIENCE_AV``                   | ""                              | selection_flagger.<rule>.spw                    | Allows flagging of a specified range of channels. For example, to flag|
|                                               |                                 | (:doc:`../calim/cflag`)                         | out the first 100 channnels, use "0:0~16" (with the quote marks). See |
|                                               |                                 |                                                 | the docuemntation for further details on the format.                  |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``TIME_FLAG_SCIENCE_AV``                      | ""                              | selection_flagger.<rule>.timerange              | Allows flagging of a specified time range(s). The string given is     |
|                                               |                                 | (:doc:`../calim/cflag`)                         | passed directly to the ``timerange`` option of cflag's selection      |
|                                               |                                 |                                                 | flagger. For details on the possible syntax, consult the `MS          |
|                                               |                                 |                                                 | selection`_ documentation.                                            |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| **Using AOFlagger for flagging**              |                                 |                                                 |                                                                       |
|                                               |                                 |                                                 |                                                                       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_WITH_AOFLAGGER``                       | false                           | none                                            | Use AOFlagger for all flagging tasks in the pipeline. This overrides  |
|                                               |                                 |                                                 | the individual task level switches.                                   |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_SCIENCE_WITH_AOFLAGGER``               | false                           | none                                            | Use AOFlagger for the flagging of the full-spectral-resolution science|
|                                               |                                 |                                                 | dataset. This and the next parameter allows differentiation between   |
|                                               |                                 |                                                 | the different flagging tasks in the pipeline.                         |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``FLAG_SCIENCE_AV_WITH_AOFLAGGER``            | false                           | none                                            | Use AOFlagger for the flagging of the averaged science dataset.       |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``AOFLAGGER_STRATEGY``                        | ""                              | none                                            | The strategy file to use for all AOFlagger tasks in the               |
|                                               |                                 |                                                 | pipeline. Giving this a value will apply this one strategy file to all|
|                                               |                                 |                                                 | flagging jobs. The strategy file needs to be provided by the user.    |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``AOFLAGGER_STRATEGY_SCIENCE``                | ""                              | none                                            | The strategy file to be used for the full-spectral-resolution science |
|                                               |                                 |                                                 | dataset. This will be overridden by ``AOFLAGGER_STRATEGY``.           |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``AOFLAGGER_STRATEGY_SCIENCE_AV``             | ""                              | none                                            | The strategy file to be used for the averaged science dataset. This   |
|                                               |                                 |                                                 | will be overridden by ``AOFLAGGER_STRATEGY``.                         |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``AOFLAGGER_VERBOSE``                         | true                            | none                                            | Verbose output for AOFlagger                                          |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``AOFLAGGER_READ_MODE``                       | auto                            | none                                            | Read mode for AOflagger. This can take the value of one of "auto",    |
|                                               |                                 |                                                 | "direct", "indirect", or "memory". These trigger the following        |
|                                               |                                 |                                                 | respective command-line options for AOflagger: "-auto-read-mode",     |
|                                               |                                 |                                                 | "-direct-read", "-indirect-read", "-memory-read".                     |
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+
| ``AOFLAGGER_UVW``                             | false                           | none                                            | When true, the command-line argument "-uvw" is added to the AOFlagger |
|                                               |                                 |                                                 | command. This reads uvw values (some exotic strategies require these).|
+-----------------------------------------------+---------------------------------+-------------------------------------------------+-----------------------------------------------------------------------+


 .. _MS selection :  http://www.aoc.nrao.edu/~sbhatnag/misc/msselection/msselection.html
 

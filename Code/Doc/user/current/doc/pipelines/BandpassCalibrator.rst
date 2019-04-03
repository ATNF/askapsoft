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

If the bandpass calibration observation is long, and you just wish to
use a portion of it for bandpass calibration, then you can specify a
time range through ``SPLIT_TIME_START_1934`` and
``SPLIT_TIME_END_1934``. If either is not given, the start or end time
are taken to be the start or end of the observation respectively.

The default behaviour is to flag the data with
:doc:`../calim/cflag`. In this case, the MS is flagged in two
passes. First, a combination of selection rules (allowing flagging of
channels, time ranges, antennas & baselines, and autocorrelations) and
(optionally) a simple flat amplitude threshold are applied. Then a
sequence of Stokes-V flagging and dynamic flagging of amplitudes is
done, integrating over individual spectra. Each of these steps is
selectable via input parameters.

There is an option to use the AOFlagger tool (written by Andre
Offringa) to do the flagging. This can be turned on by
``FLAG_WITH_AOFLAGGER``, or ``FLAG_1934_WITH_AOFLAGGER`` (to just do
it for the bandpass calibrator). You can provide a strategy file via
``AOFLAGGER_STRATEGY`` or ``AOFLAGGER_STRATEGY_1934``, with access to
some of the aoflagger parameters provided - see the table below.

Then the bandpass table is calculated with
:doc:`../calim/cbpcalibrator`, which requires MSs for all beams to be
given. This is a parallel job, the size of which is configurable
through ``NUM_CPUS_CBPCAL``.

If a second processing job is run with a higher ``BEAM_MAX`` (and
hence wanting to use beams not included in a previous bandpass
solution), the bandpass table is deleted and re-made once the new
beams are split and flagged.

The cbpcalibrator job can make use of one of two scripts developed in
the commissioning team. The first, ``plot_caltable.py`` from ACES both plots
the bandpass solutions (as a function of antenna, beam and
polarisation), and smooths the bandpass table so that outlying
points are interpolated over. This produces a second table (with
".smooth" appended to the name), which will then be applied to the
science data instead of the original. There are a number of parameters
that may be used to tweak the smoothing - this is intended to be an
interim solution until this functionality is implemented in
ASKAPsoft.

The second script, ``smooth_bandpass.py``, does the smoothing of the
bandpass via harmonic fitting, in a way that is robust to portions of
the spectra being flagged.

If either of these options are used, a bandpass validation script is
run, producing plots that can describe the quality of the bandpass
solutions. 

Finally, the bandpass solutions can be applied back to the 1934
datasets themselves. This will permit possible diagnostic analysis of
the quality of the bandpass solution.

+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| Variable                                      | Default                               | Parset equivalent                                      | Description                                               |
+===============================================+=======================================+========================================================+===========================================================+
| ``DO_SPLIT_1934``                             | true                                  | none                                                   | Whether to split a given beam/scan from the input 1934 MS |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_SPLIT_1934``                       | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                                   | Time request for splitting the calibrator MS              |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_FLAG_1934``                              | true                                  | none                                                   | Whether to flag the splitted-out 1934 MS                  |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_FLAG_1934``                        | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                                   | Time request for flagging the calibrator MS               |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_FIND_BANDPASS``                          | true                                  | none                                                   | Whether to fit for the bandpass using all 1934-638 MSs    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``JOB_TIME_FIND_BANDPASS``                    | ``JOB_TIME_DEFAULT`` (12:00:00)       | none                                                   | Time request for finding the bandpass solution            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Preparing the calibrator datasets**         |                                       |                                                        |                                                           |
|                                               |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``MS_BASE_1934``                              | 1934_SB%s_%b.ms                       | none                                                   | Base name for the 1934 measurement sets after splitting.  |
|                                               |                                       |                                                        | The wildcard %b will be replaced with the string "beamBB",|
|                                               |                                       |                                                        | where BB is the (zero-based) beam number, and             |
|                                               |                                       |                                                        | the %s will be replaced by the calibration scheduling     |
|                                               |                                       |                                                        | block ID.                                                 |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``CHAN_RANGE_1934``                           | ""                                    | channel (:doc:`../calim/mssplit`)                      | Channel range for splitting (1-based!). This range also   |
|                                               |                                       |                                                        | defines the internal variable ``NUM_CHAN_1934`` (which    |
|                                               |                                       |                                                        | replaces the previously-available parameter NUM_CHAN). The|
|                                               |                                       |                                                        | default is to use all available channels in the MS.       |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_DYNAMIC_AMPLITUDE_1934``            | true                                  | none                                                   | Whether to do the dynamic flagging, after the rule-based  |
|                                               |                                       |                                                        | and simple flat-amplitude flagging is done.               |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_1934``               |  4.0                                  | amplitude_flagger.threshold                            | Dynamic threshold applied to amplitudes when flagging 1934|
|                                               |                                       |  (:doc:`../calim/cflag`)                               | data [sigma]                                              |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DYNAMIC_1934_INTEGRATE_SPECTRA``       | true                                  | amplitude_flagger.integrateSpectra                     | Whether to integrate the spectra in time and flag channels|
|                                               |                                       | (:doc:`../calim/cflag`)                                | during the dynamic flagging task.                         |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_DYNAMIC_1934_SPECTRA``       |  4.0                                  | amplitude_flagger.integrateSpectra.threshold           | Dynamic threshold applied to amplitudes when flagging 1934|
|                                               |                                       | (:doc:`../calim/cflag`)                                | data in integrateSpectra mode [sigma]                     |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_DYNAMIC_1934_INTEGRATE_TIMES``        | false                                 | amplitude_flagger.integrateTimes                       | Whether to integrate across spectra and flag time samples |
|                                               |                                       | (:doc:`../calim/cflag`)                                | during the dynamic flagging task.                         |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_THRESHOLD_DYNAMIC_1934_TIMES``        |  4.0                                  | amplitude_flagger.integrateTimes.threshold             | Dynamic threshold applied to amplitudes when flagging 1934|
|                                               |                                       | (:doc:`../calim/cflag`)                                | data in integrateTimes mode [sigma]                       |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_STOKESV_1934``                      | true                                  | none                                                   | Whether to do Stokes-V flagging, after the rule-based     |
|                                               |                                       |                                                        | and simple flat-amplitude flagging is done.               |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_USE_ROBUST_STATS_STOKESV_1934``        | true                                  | stokesv_flagger.useRobustStatistics                    | Whether to use robust statistics (median and              |
|                                               |                                       | (:doc:`../calim/cflag`)                                | inter-quartile range) in computing the Stokes-V           |
|                                               |                                       |                                                        | statistics.                                               |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_1934``               |  4.0                                  | stokesv_flagger.threshold                              | Threshold applied to amplitudes when flagging Stokes-V in |
|                                               |                                       |  (:doc:`../calim/cflag`)                               | 1934 data [sigma]                                         |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_STOKESV_1934_INTEGRATE_SPECTRA``       | true                                  | stokesv_flagger.integrateSpectra                       | Whether to integrate the spectra in time and flag channels|
|                                               |                                       | (:doc:`../calim/cflag`)                                | during the Stokes-V flagging task.                        |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_STOKESV_1934_SPECTRA``       |  4.0                                  | stokesv_flagger.integrateSpectra.threshold             | Threshold applied to amplitudes when flagging Stokes-V    |
|                                               |                                       | (:doc:`../calim/cflag`)                                | in 1934 data in integrateSpectra mode [sigma]             |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_STOKESV_1934_INTEGRATE_TIMES``        | false                                 | stokesv_flagger.integrateTimes                         | Whether to integrate across spectra and flag time samples |
|                                               |                                       | (:doc:`../calim/cflag`)                                | during the Stokes-V flagging task.                        |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_THRESHOLD_STOKESV_1934_TIMES``        |  4.0                                  | stokesv_flagger.integrateTimes.threshold               | Threshold applied to amplitudes when flagging Stokes-V in |
|                                               |                                       | (:doc:`../calim/cflag`)                                | 1934 data in integrateTimes mode [sigma]                  |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_DO_FLAT_AMPLITUDE_1934``               | false                                 | none                                                   | Whether to apply a simple ("flat") amplitude threshold to |
|                                               |                                       |                                                        | the 1934 data.                                            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_THRESHOLD_AMPLITUDE_1934``             | 0.2                                   | amplitude_flagger.high (:doc:`../calim/cflag`)         | Simple amplitude threshold applied when flagging 1934     |
|                                               |                                       |                                                        | data.                                                     |
|                                               |                                       |                                                        | If set to blank (``FLAG_THRESHOLD_AMPLITUDE_1934=""``),   |
|                                               |                                       |                                                        | then no minimum value is applied.                         |
|                                               |                                       |                                                        | [value in hardware units - before calibration]            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
|  ``FLAG_THRESHOLD_AMPLITUDE_1934_LOW``        | ""                                    | amplitude_flagger.low (:doc:`../calim/cflag`)          | Lower threshold for the simple amplitude flagging. If set |
|                                               |                                       |                                                        | to blank (``FLAG_THRESHOLD_AMPLITUDE_1934_LOW=""``), then |
|                                               |                                       |                                                        | no minimum value is applied.                              |
|                                               |                                       |                                                        | [value in hardware units - before calibration]            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``ANTENNA_FLAG_1934``                         | ""                                    | selection_flagger.<rule>.antenna                       | Allows flagging of antennas or baselines. For example, to |
|                                               |                                       | (:doc:`../calim/cflag`)                                | flag out the 1-3 baseline, set this to "ak01&&ak03" (with |
|                                               |                                       |                                                        | the quote marks). See the documentation for further       |
|                                               |                                       |                                                        | details on the format.                                    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``CHANNEL_FLAG_1934``                         | ""                                    | selection_flagger.<rule>.spw (:doc:`../calim/cflag`)   | Allows flagging of a specified range of channels. For     |
|                                               |                                       |                                                        | example, to flag out the first 100 channnels, use "0:0~16"|
|                                               |                                       |                                                        | (with the quote marks). See the docuemntation for further |
|                                               |                                       |                                                        | details on the format.                                    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``TIME_FLAG_1934``                            | ""                                    | selection_flagger.<rule>.timerange                     | Allows flagging of a specified time range(s). The string  |
|                                               |                                       | (:doc:`../calim/cflag`)                                | given is passed directly to the ``timerange`` option of   |
|                                               |                                       |                                                        | cflag's selection flagger. For details on the possible    |
|                                               |                                       |                                                        | syntax, consult the `MS selection`_ documentation.        |
|                                               |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_AUTOCORRELATION_1934``                 | false                                 | selection_flagger.<rule>.autocorr                      | If true, then autocorrelations will be flagged.           |
|                                               |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``SPLIT_TIME_START_1934``                     | ""                                    | timebegin (:doc:`../calim/mssplit`)                    | The starting time for data to be used from the 1934 SB. If|
|                                               |                                       |                                                        | blank, the start of the observation will be used. The     |
|                                               |                                       |                                                        | formatting must conform to requirements of                |
|                                               |                                       |                                                        | :doc:`../calim/mssplit`.                                  |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``SPLIT_TIME_END_1934``                       | ""                                    | timeend (:doc:`../calim/mssplit`)                      | The starting time for data to be used from the 1934 SB. If|
|                                               |                                       |                                                        | blank, the start of the observation will be used. The     |
|                                               |                                       |                                                        | formatting must conform to requirements of                |
|                                               |                                       |                                                        | :doc:`../calim/mssplit`.                                  |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Using AOFlagger for flagging**              |                                       |                                                        |                                                           |
|                                               |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_WITH_AOFLAGGER``                       | false                                 | none                                                   | Use AOFlagger for all flagging tasks in the pipeline. This|
|                                               |                                       |                                                        | overrides the individual task level switches.             |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``FLAG_1934_WITH_AOFLAGGER``                  | false                                 | none                                                   | Use AOFlagger for the flagging of the bandpass calibrator.|
|                                               |                                       |                                                        | This allows differentiation between the different flagging|
|                                               |                                       |                                                        | tasks in the pipeline.                                    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``AOFLAGGER_STRATEGY``                        | ""                                    | none                                                   | The strategy file to use for all AOFlagger tasks in the   |
|                                               |                                       |                                                        | pipeline. Giving this a value will apply this one strategy|
|                                               |                                       |                                                        | file to all flagging jobs. The strategy file needs to be  |
|                                               |                                       |                                                        | provided by the user.                                     |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``AOFLAGGER_STRATEGY_1934``                   | ""                                    | none                                                   | The strategy file to be used for the bandpass             |
|                                               |                                       |                                                        | calibrator. This will be overridden by                    |
|                                               |                                       |                                                        | ``AOFLAGGER_STRATEGY``.                                   |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``AOFLAGGER_VERBOSE``                         | true                                  | none                                                   | Verbose output for AOFlagger                              |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``AOFLAGGER_READ_MODE``                       | auto                                  | none                                                   | Read mode for AOflagger. This can take the value of one of|
|                                               |                                       |                                                        | "auto", "direct", "indirect", or "memory". These trigger  |
|                                               |                                       |                                                        | the following respective command-line options for         |
|                                               |                                       |                                                        | AOflagger: "-auto-read-mode", "-direct-read",             |
|                                               |                                       |                                                        | "-indirect-read", "-memory-read".                         |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``AOFLAGGER_UVW``                             | false                                 | none                                                   | When true, the command-line argument "-uvw" is added to   |
|                                               |                                       |                                                        | the AOFlagger command. This reads uvw values (some exotic |
|                                               |                                       |                                                        | strategies require these).                                |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Solving for the bandpass**                  |                                       |                                                        |                                                           |
|                                               |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DIRECTION_1934``                            | "[19h39m25.036, -63.42.45.63, J2000]" | sources.field1.direction                               | Location of 1934-638, formatted for use in cbpcalibrator. |
|                                               |                                       | (:doc:`../calim/cbpcalibrator`)                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``TABLE_BANDPASS``                            | calparameters_1934_bp_SB%s.tab        | calibaccess.table                                      | Name of the CASA table used for the bandpass calibration  |
|                                               |                                       | (:doc:`../calim/cbpcalibrator` and                     | parameters. If no leading directory is given, the table   |
|                                               |                                       | :doc:`../calim/ccalapply`)                             | will be put in the BPCAL directory. Otherwise, the table  |
|                                               |                                       |                                                        | is left where it is (this allows the user to specify a    |
|                                               |                                       |                                                        | previously-created table for use with the science         |
|                                               |                                       |                                                        | field). The %s will be replaced by the calibration        |
|                                               |                                       |                                                        | scheduling block ID.                                      |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SCALENOISE``                       | false                                 | calibrate.scalenoise (:doc:`../calim/ccalapply`)       | Whether the noise estimate will be scaled in accordance   |
|                                               |                                       |                                                        | with the applied calibrator factor to achieve proper      |
|                                               |                                       |                                                        | weighting.                                                |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``NCYCLES_BANDPASS_CAL``                      | 50                                    | ncycles (:doc:`../calim/cbpcalibrator`)                | Number of cycles used in cbpcalibrator.                   |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``NUM_CPUS_CBPCAL``                           | 216                                   | none                                                   | The number of cpus allocated to the cbpcalibrator job. The|
|                                               |                                       |                                                        | job will use all 20 cpus on each node (the memory         |
|                                               |                                       |                                                        | footprint is small enough to allow this).                 |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_MINUV``                            | 200                                   | MinUV (:doc:`../calim/data_selection`)                 | Minimum UV distance [m] applied to data prior to solving  |
|                                               |                                       |                                                        | for the bandpass (used to exclude the short baselines).   |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_REFANTENNA``                       | 1                                     | refantenna (:doc:`../calim/cbpcalibrator`)             | Antenna number to be used as reference in the bandpass    |
|                                               |                                       |                                                        | calibration. Ignored if negative, or if provided as a     |
|                                               |                                       |                                                        | blank string (``BANDPASS_REFANTENNA=""``).                |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Smoothing and plotting the bandpass**       |                                       |                                                        |                                                           |
|                                               |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_BANDPASS_SMOOTH``                        | true                                  | none                                                   | Whether to produce a smoothed version of the bandpass     |
|                                               |                                       |                                                        | table, which will be applied to the science data.         |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_TOOL``                      | plot_caltable                         | none                                                   | Which tool to use. Possible values here are               |
|                                               |                                       |                                                        | "plot_caltable" (the default) or                          |
|                                               |                                       |                                                        | "smooth_bandpass". Relevant parameters for each tool      |
|                                               |                                       |                                                        | follow.                                                   |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| *plot_caltable*                               |                                       |                                                        | Options for the script "plot_caltable.py"                 |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_BANDPASS_PLOT``                          | true                                  | none                                                   | Whether to produce plots of the bandpass                  |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_AMP``                       | true                                  | none                                                   | Whether to smooth the amplitudes (if false, smoothing is  |
|                                               |                                       |                                                        | done on the real and imaginary values).                   |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_OUTLIER``                   | true                                  | none                                                   | If true, only smooth/interpolate over outlier points      |
|                                               |                                       |                                                        | (based on the inter-quartile range).                      |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_FIT``                       | 1                                     | none                                                   | The order of the polynomial (if >=0) or the window size   |
|                                               |                                       |                                                        | (if <0) used in the smoothing.                            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_THRESHOLD``                 | 1.0                                   | none                                                   | The threshold level used for fitting to the bandpass.     |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| *smooth_bandpass*                             |                                       |                                                        | Options for the script "smooth_bandpass.py"               |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_POLY_ORDER``                | ""                                    | none                                                   | The polynomial order for the fit - the value for the      |
|                                               |                                       |                                                        | ``-np`` option. Ignored if left blank.                    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_HARM_ORDER``                | ""                                    | none                                                   | The harmonic order for the fit - the value for the        |
|                                               |                                       |                                                        | ``-nh`` option. Ignored if left blank.                    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_N_WIN``                     | ""                                    | none                                                   | The number of windows to divide the spectrum into for the |
|                                               |                                       |                                                        | moving fit - the value for the ``-nwin`` option. Ignored  |
|                                               |                                       |                                                        | if left blank.                                            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_N_TAPER``                   | ""                                    | none                                                   | The width (in channels) of the Gaussian Taper function to |
|                                               |                                       |                                                        | remove high-frequency components - the value for the      |
|                                               |                                       |                                                        | ``-nT`` option. Ignored if left blank.                    |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``BANDPASS_SMOOTH_N_ITER``                    | ""                                    | none                                                   | The number of iterations for Fourier-interpolation across |
|                                               |                                       |                                                        | flagged points - the value for the ``-nI`` option. Ignored|
|                                               |                                       |                                                        | if left blank.                                            |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| **Applying the bandpass solution**            |                                       |                                                        |                                                           |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``DO_APPLY_BANDPASS_1934``                    | true                                  | none                                                   | Whether to apply the bandpass solution to the 1934        |
|                                               |                                       |                                                        | datasets.                                                 |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+
| ``KEEP_RAW_1934_DATA``                        | true                                  | none                                                   | If true, the 1934 MSs will be copied prior to having the  |
|                                               |                                       |                                                        | bandpass solution applied. This means you will have copies|
|                                               |                                       |                                                        | of both the raw and calibrated datasets.                  |
+-----------------------------------------------+---------------------------------------+--------------------------------------------------------+-----------------------------------------------------------+


 .. _MS selection :  http://www.aoc.nrao.edu/~sbhatnag/misc/msselection/msselection.html
 

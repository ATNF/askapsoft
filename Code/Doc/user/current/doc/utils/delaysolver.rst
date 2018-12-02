delaysolver
============

Usage::

    delaysolver -c delaysolver.parset [-s scheduling_block] [-f measurement_set]

This utility is intended to assist initial delay calibration of the array after
maintenance or power cycle. It processes a short observation of some dominant 
continuum calibrator (e.g. Virgo) and produces a file directly suitable for 
uploading into the FCM. The tool also supports a number of useful options to
assist commissioning. It is intended to be run on Galaxy, but will work, albeit 
a bit slow, on akingest01 (which can be handy to avoid waiting for the data transfer).

The measurement set can be given either in the command line, or in the parset 
(see below). If the parset contains *cp.ingest.tasks.FringeRotationTask.params.fixeddelays*
keyword, the delay corrections solved for by this tool will be added to the delays
specified in this parameter (the name is deliberately chosen to match the old fcm key
and assist copy-pasting). The result is reported to the log and is written into
an ascii file called **corrected_fixeddelay.parset** in the format accepted by
**fcm put**. It is also possible to export delays in the old format, i.e. with all
delays given in a single *cp.ingest.tasks.FringeRotationTask.params.fixeddelays* key.
However, by default, the current FCM format (i.e. separate delay key per antenna) is assumed.
Another useful alternative, which is intended to be the main
method used in operations, is to specify the scheduling block ID via the **-s**
command line option. In this case, both previous delay values and the measurement set
file name are taken from the scheduling block. The most recent FCM delay key format is assumed
and a warning is given if the old style key is present in the parset.

With the correctly set cutoff, the tool was expected to deal with birdies automatically
and provide a delay solution automatically. However, it pays to inspect that the system
is working well and doesn't have other correlator artefacts in addition to occasional 
spikes. Otherwise, an incorrect delay could be inserted which can make things worse.
As an additional diagnostics, the tool produces an ASCII file called **avgspectrum.dat**
which contains phase spectra for all baselines before they are passed to the delay solver
(i.e. with flagging and averaging applied). In the case of troubleshooting, it is handy
to inspect these spectra which should show a reasonable phase slope (note, phase wraps
are normal - they are handled well by the delay solution algorithm). The format of this
file contains 4 columns::
 
  antenna1_id (from 0 to 5)
  antenna2_id (from 0 to 5)
  channel_id (0 based, after averaging; 0 corresponds to 0 in the original dataset)
  phase_in_degrees

Parameters understood by delaysolverr are given in the following table
and should not contain any prefix:

+------------------------------+---------------+-----------+-----------------------------------------+
|*Parameter*                   |*Type*         |*Default*  |*Description*                            |
+==============================+===============+===========+=========================================+
|stokes                        |string         |"XX"       |Polarisation product to use. The code    |
|                              |               |           |does no conversion, so the parameter     |
|                              |               |           |should correspond to a product which     |
|                              |               |           |actually has been observed.              |
+------------------------------+---------------+-----------+-----------------------------------------+
|resolution                    |double         |1e6        |Spectral resolution in Hz to average data|
|                              |               |           |to before solving for delays. Averaging  |
|                              |               |           |of integer number of channels is done to |
|                              |               |           |match the requested resolution as close  |
|                              |               |           |as possible. If the dataset has already  |
|                              |               |           |equal or more coarse resolution, nothing |
|                              |               |           |is done to the data prior to solving     |
+------------------------------+---------------+-----------+-----------------------------------------+
|beam                          |int            |0          |Beam to work with                        |
+------------------------------+---------------+-----------+-----------------------------------------+
|cutoff                        |double         |-1         |If positive, the spectral channels which |
|                              |               |           |have the amplitude greater than the      |
|                              |               |           |cutoff value are flagged. This is done   |
|                              |               |           |before any averaging.                    |
+------------------------------+---------------+-----------+-----------------------------------------+
|exclude13                     |bool           |false      |If true, AK01-AK03 baseline is excluded  |
|                              |               |           |from the solution                        |
+------------------------------+---------------+-----------+-----------------------------------------+
|sbpath                        |string         |"./"       |Path to the directory which contains     |
|                              |               |           |scheduling blocks (only used with -s     |
|                              |               |           |command line option                      |
+------------------------------+---------------+-----------+-----------------------------------------+
|ms                            |string         |""         |A full path to the measurement set to use|
|                              |               |           |(required if ms is not given in the      |
|                              |               |           |command line or via the scheduling block)|
+------------------------------+---------------+-----------+-----------------------------------------+
|cp.ingest.tasks.FringeRotatio\|vector<double> |None       |Current fixed delays (can only be used if|
|nTask.params.fixeddelays      |               |           |no scheduling block is given). Note, this|
|                              |               |           |is an obsolete format for specifying     |
|                              |               |           |delays. Specify scheduling block to work |
|                              |               |           |with the current format.                 |
+------------------------------+---------------+-----------+-----------------------------------------+
|oldfcmformat                  |bool           |false      |If true, the output is given in the old  |
|                              |               |           |FCM format, i.e. as a single vector for  |
|                              |               |           |all defined antennas. May be handy for   |
|                              |               |           |analysis scripts, but not intended to be |
|                              |               |           |used in normal operations.               |
+------------------------------+---------------+-----------+-----------------------------------------+
|refant                        |unsigned int   |1          |Index of the reference antenna (zero     |
|                              |               |           |delay correction is assumed). Should be  |
|                              |               |           |present in the measurement set, otherwise|
|                              |               |           |the result may have degeneracies.        |
+------------------------------+---------------+-----------+-----------------------------------------+

Examples
--------

**Typical parset for use on Galaxy:**

The tool is used with **-s** command line option and takes the measurement set name and the
fixed delay setting used during observations from the supplied scheduling block

.. code-block:: bash

    stokes = XX
    resolution = 1e6
    beam = 0
    # the cutoff may need an adjustment if beams are formed with different normalisation
    # the following value seems to be good for single port beams we currently use
    cutoff = 0.23
    # We exclude AK01-AK03 baseline due to cross-talk and interference, otherwise it
    # can skew the solution
    exclude13 = true
    sbpath = /astro/askaprt/askapops/askap-scheduling-blocks


User Parameters - Data Location & Beam Selection
================================================

The following parameters determine where the measurement sets are that
hold the data to be processed. The default behaviour is to read the MS
from the given Scheduling Block (SB) directory. If SB numbers are
given, this is where to look. The location for these SB directories is
given by ``DIR_SB``. If SB numbers are not provided, however,
the ``MS_INPUT`` variables give the filename for the relevant MS. The
full path must be specified here.

The measurement sets presented in the scheduling block directories
have a variety of forms:

1. The earliest observations had all beams in a single measurement
   set. For these, splitting with mssplit is required to get a
   single-beam MS used for processing.
2. Many observations have one beam per MS. If no selection of channels
   or fields or scans is required, these can be copied rather than
   split. Note that splitting of the bandpass observations are
   necessary, to isolate the relevant scan.
3. Recent large observations have more than one MS per beam, split in
   frequency chunks. The pipeline will merge these to form a single
   local MS for each beam. If splitting (of channels, fields or scans)
   is required, this is done first, before merging the local subsets.


In the multiple-file modes, the metadata for the observation is taken
from a single MS (although they should all be the same for a given
observation).

If no channel or scan selection is being made (that is,
``CHANNEL_RANGE_SCIENCE`` or ``SCAN_SELECTION_SCIENCE`` are not
given), there is only a single field in the dataset, and the
observation used the MS-per-beam mode, then instead of splitting the
dataset the pipeline simply copies the archive MS to the output
directory. This should run faster than using *mssplit*.

The bandpass calibration datasets are treated the same way, although
*mssplit* is always used (since the relevant scan needs to be split
out). 

It may be that the SB you wish to process has been removed from /astro.
You can follow the instructions on :doc:`../platform/comm_archive` to request
it be reinstated. If you have already started processing it and wish to
continue, you can specify the measurement set name it would have (this forms
the **metadata/mslist_** filename), using the parameters ``MS_INPUT_1934`` or
``MS_INPUT_SCIENCE``,  and the scripts will recognise the metadata
file and continue. For anything to be done, it requires you to have already
run the splitting, so that there is a local copy of the individual beam data.

+------------------------+---------------------------------------------------------+------------------------------------------------------------+
| Variable               | Default                                                 | Description                                                |
+========================+=========================================================+============================================================+
| ``DIR_SB``             | /astro/askaprt/askapops/askap-scheduling-blocks         | Location (on galaxy) of the scheduling blocks. This is used|
|                        |                                                         | when specifying SB numbers - the default is the standard   |
|                        |                                                         | location for ASKAP operation. If blank (``""``), the MS    |
|                        |                                                         | used will be given by ``MS_INPUT_SCIENCE`` and             |
|                        |                                                         | ``MS_INPUT_1934``.                                         |
+------------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``SB_1934``            | no default                                              | SB number for the 1934-638 calibration observation.        |
+------------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``MS_INPUT_1934``      | no default                                              | MS for the 1934-638 observation. Ignored if the SB number  |
|                        |                                                         | is provided.                                               |
+------------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``SB_SCIENCE``         | no default                                              | SB number for the science field observation.               |
+------------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``MS_INPUT_SCIENCE``   | no default                                              | MS for the science field. Ignored if the SB number is      |
|                        |                                                         | provided.                                                  |
+------------------------+---------------------------------------------------------+------------------------------------------------------------+

These parameters determine which beams in the data to process. The
beams can be listed explicitly via the ``BEAMLIST`` parameter, given
as a comma-separated list of beams and beam ranges, such as
``"0,1,4,7,8-10"``.
Alternatively (and if ``BEAMLIST`` is not provided), the ``BEAM_MIN``
and ``BEAM_MAX`` parameters are used to specify an inclusive
range. Note that in all cases the beams are numbered from zero
(ie. beam 0 is the first beam in the measurement set).

The beams used for the bandpass calibrator will be all beams up to the
maximum beam requested for the science dataset (either ``BEAM_MAX`` or
the largest number in ``BEAMLIST``).

The beam list will be limited by the number of beams available in the
footprints for both the science and bandpass calibrator datasets. If
more beams are requested via ``BEAMLIST`` or ``BEAM_MAX``, then a
warning message is given and the list trimmed to match the limits from
the relevant scheduling blocks. 

+----------------+-----------+--------------------------------------------------+
| Variable       | Default   | Description                                      |
+================+===========+==================================================+
| ``BEAMLIST``   | ``""``    | A comma-separated list of beams and beam ranges  |
|                |           | (for instance ``"0,1,4,7,8-10"``), to specify the|
|                |           | set of beams than should be processed from the   |
|                |           | science observation. Beam numbers are 0-based.   |
+----------------+-----------+--------------------------------------------------+
| ``BEAM_MIN``   | 0         | First beam number (0-based). Used when           |
|                |           | ``BEAMLIST`` is not given. All beams from        |
|                |           | ``BEAM_MIN`` to ``BEAM_MAX`` *inclusive* are used|
|                |           | for the science processing.                      |
+----------------+-----------+--------------------------------------------------+
| ``BEAM_MAX``   | 35        | Final beam number (0-based), to go with          |
|                |           | ``BEAM_MIN``.                                    |
+----------------+-----------+--------------------------------------------------+

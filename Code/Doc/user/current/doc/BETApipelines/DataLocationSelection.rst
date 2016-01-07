User Parameters - Data Location & Beam Selection
================================================

The following parameters determine where the measurement sets are that
hold the data to be processed. The default behaviour is to read the MS
from the given Scheduling Block (SB) directory. If SB numbers are
given, this is where to look. If SB numbers are not provided, however,
the ``MS_INPUT`` variables give the filename for the relevant MS.

+----------------------+---------------------------------------------------------+------------------------------------------------------------+
| Variable             | Default                                                 | Description                                                |
+======================+=========================================================+============================================================+
| ``DIR_SB``           |/scratch2/askap/askapops/beta-scheduling-blocks          |Location (on galaxy) of the scheduling blocks. This is used |
|                      |                                                         |when specifying SB numbers - the default is the standard    |
|                      |                                                         |location for BETA operation.                                |
+----------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``SB_1934``          | no default                                              |SB number for the 1934-638 calibration observation.         |
+----------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``MS_INPUT_1934``    | no default                                              |MS for the 1934-638 observation. Ignored if the SB number   |
|                      |                                                         |is provided.                                                |
+----------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``SB_SCIENCE``       | no default                                              |SB number for the science field observation.                |
+----------------------+---------------------------------------------------------+------------------------------------------------------------+
| ``MS_INPUT_SCIENCE`` | no default                                              |MS for the science field. Ignored if the SB number is       |
|                      |                                                         |provided.                                                   |
+----------------------+---------------------------------------------------------+------------------------------------------------------------+

These parameters determine which beams in the data to process. This
applies to both the calibrator and science data. The ranges are
*inclusive*, and 0-based (as this is how the beams are recorded in the
MS). 

+----------------+-----------+--------------------------------------------------+
| Variable       | Default   | Description                                      |
+================+===========+==================================================+
| ``BEAM_MIN``   | 0         |First beam number (0-based). All beams from       |
|                |           |``BEAM_MIN`` to ``BEAM_MAX`` *inclusive* are used.|
+----------------+-----------+--------------------------------------------------+
| ``BEAM_MAX``   | 8         | Final beam number (0-based)                      |
+----------------+-----------+--------------------------------------------------+

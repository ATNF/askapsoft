ShadowFlagTask
=========

ShadowFlagTask is an experimental task to detect and flag shadowed antennas on the fly. It supports only basic interferometry mode.
For example, cases where off-axis beams are shadowed during offset pointings of the holography run will not be treated correctly. 
Even in the standard mode there are limitations with regard to off-axis beams: the algorithm uses the phase tracking centre and,
therefore, would treat all beams the same if no fringe tracking per beam is done and if executed without distribution by beam
(the latter limitation may be lifted in the future, if necessary). Note, antennas excluded from data recording may cause shadowing,
but would not be detected. Antennas which ingest pipeline knows about (i.e. data are recorded) are treated as tracking the same source
regardless whether they are participating in the observations at all or point somewhere else. The task requires uvw information to be
present and, therefore, should be in the appropriate position in the task chain.

Configuration Parameters
------------------------

The program requires a configuration file be provided on the command line. This
section describes the valid parameters. In addition to mandatory parameters which are
always required, individual tasks often have specific parameters which need to be
defined only if a particular task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters). The type of
the task defined by tasks.\ **name**\ .type should be set to *ShadowFlagTask*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|dry_run                     |boolean            |false       |If true, the task will only report when antennas are shadowed |
|                            |                   |            |or unshadowed without actually flagging affected baselines.   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|dish_diamter                |float              |12          |Dish diamter in metres for shadowing calculations. Note, ante\|
|                            |                   |            |nnas sesne each other a bit earlier before geometrical shadow\|
|                            |                   |            |ing occurs. In addition, increasing it by about a metre ensur\|
|                            |                   |            |es the most offset beams are also flagged for the shortest    |
|                            |                   |            |separation between antennas if fringe tracking is not done per|
|                            |                   |            |beam.                                                         |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Example
~~~~~~~

.. code-block:: bash

    ########################## FlagTask ##############################

    tasks.tasklist = [MergedSource, Merge, FringeRotationTask, ShadowGlagger, MSSink, FlagTask, TCPSink]

    # dish diameter for flagging calculations
    tasks.ShadowFlagger.params.dish_diamter = 15
    # type of the task
    tasks.ShadowFlagger.type = ShadowFlagTask

    


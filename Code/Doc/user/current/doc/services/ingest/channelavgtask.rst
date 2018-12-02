ChannelAvgTask
==============

ChannelAvgTask averages given number of consecutive spectral channels to reduce the spectral resolution of the
processed data chunk. Flagging is taken into account. The resulting data point is only flagged if all contributing
spectral channels are flagged. Otherwise, the sample is considered to be valid. The resulting frequency is always
the average of contibuting frequencies regardless of the flag status. No handling of noise estimate is done
at this stage.

Configuration Parameters
------------------------

The program requires a configuration file be provided on the command line. This
section describes the valid parameters. In addition to mandatory parameters which are
always required, individual tasks often have specific parameters which need to be
defined only if a particular task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters. The type of
the task defined by tasks.\ **name**\ .type should be set to *ChannelAvgTask*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|averaging                   |unsigned int       |None        |Averaging factor, i.e. the number of consecutive spectral     |
|                            |                   |            |channels to average. The total number of channels in the chunk|
|                            |                   |            |should be integral multiple of this number.                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## ChannelAvgTask ##############################

    tasks.tasklist = [MergedSource, Merge, CalcUVWTask, FringeRotationTask, ChanAvgTask, MSSink, AvgForMonitoring, TCPSink]

    # note we use two different task names for the same task to enable averaging with different factors.

    # number of channels to average, if input is 16416 we get 304 channels after this task (will be passed to MSSink)
    tasks.ChanAvgTask.params.averaging = 54
    # type of the task
    tasks.ChanAvgTask.type = ChannelAvgTask

    # number of channels to average, if input is 304 we get 19 channels after this task (will be passed to TCPSink)
    tasks.AvgForMonitoring.params.averaging = 16
    # type of the task
    tasks.AvgForMonitoring.type = ChannelAvgTask
    


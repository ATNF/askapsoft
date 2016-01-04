ChannelSelTask
==============

ChannelSelTask selects a subset of channel space. Data outside of selected range of channels are
discarded. Only contiguous band of channels is supported. This is a legacy task which was used for
early BETA debugging with notably contaminated spectrum. It is unlikely that we are going to use
it with the production system, but it can still be useful for some tests. The task has not been
tested or used with parallel data streams, but is expected to perform identical operation based
on local channel numbers for each stream.

Configuration Parameters
------------------------

Ingest pipeline requires a configuration file be provided on the command line. This
section describes the valid parameters applicable to this particular task.
These parameters need to be defined only if this task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters). The type of
the task defined by tasks.\ **name**\ .type should be set to *ChannelSelTask*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|start                       |unsigned int       |None        |The first channel of selected subset of channels. The number  |
|                            |                   |            |is zero based. If the selection exceeds the bound of available|
|                            |                   |            |channel space, a warning is given and all existing data are   |
|                            |                   |            |flagged, but processing is not stopped. This, however, may    |
|                            |                   |            |create problems for tasks later in the processing chain.      |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|nchan                       |unsigned int       |None        |The number of channels selected for further processing.       |
|                            |                   |            |If the selection exceeds the bounds of available channel      |
|                            |                   |            |space, a warning is given and all existing data are flagged,  |
|                            |                   |            |but processing is not stopped. This, however, may create      |
|                            |                   |            |problems for tasks later in the processing chain.             |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Example
~~~~~~~

.. code-block:: bash

    ########################## ChannelSelTask ##############################

    tasks.tasklist = [MergedSource, CalcUVWTask, MSSink, ChanSel, TCPSink]

    # select the first coarse channel for monitoring
    # (we did exactly this for one of the commissioning tests when
    # we turned LNAs on and off to figure out port mapping)                         
    tasks.ChanSel.params.start = 0
    tasks.ChanSel.params.nchan = 54
    # type of the task
    tasks.ChanSel.type = ChannelSelTask

    


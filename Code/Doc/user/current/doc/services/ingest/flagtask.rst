FlagTask
=========

FlagTask provides basic threshold-based on the fly flagging. Thresholds can be specified separately for
auto-correlations and cross-correlations. No threshold specified means no flagging done based on that 
threshold. Flagged data can be optionally zeroed (handy, if flagging is done to regularise monitoring of
spectrum-averaged visibilities).

Configuration Parameters
------------------------

The program requires a configuration file be provided on the command line. This
section describes the valid parameters. In addition to mandatory parameters which are
always required, individual tasks often have specific parameters which need to be
defined only if a particular task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters). The type of
the task defined by tasks.\ **name**\ .type should be set to *FlagTask*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|threshold.crosscorr         |float              |None        |Threshold for cross-correlations. Any cross-correlation       |
|                            |                   |            |amplitude  exceeding this threshold will be flagged. If this  |
|                            |                   |            |parameter is undefined, no flagging of cross-correlations is  |
|                            |                   |            |done                                                          |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|threshold.autocorr          |float              |None        |Threshold for auto-correlations. Any auto-correlation         |
|                            |                   |            |visibility exceeding this threshold will be flagged. If this  |
|                            |                   |            |parameter is undefined, no flagging of auto-correlations is   |
|                            |                   |            |done                                                          |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|zeroflagged                 |boolean            |false       |If true, the flagged visibilites are also zeroed. This is     |
|                            |                   |            |handy if averaging in spectral domain follows (e.g. for       |
|                            |                   |            |monitoring).                                                  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Example
~~~~~~~

.. code-block:: bash

    ########################## FlagTask ##############################

    tasks.tasklist = [MergedSource, Merge, CalcUVWTask, FringeRotationTask, MSSink, FlagTask, TCPSink]

    # note the order of tasks in tasklist, flagging only affects monitoring (i.e. data distributed via TCPSink)

    # threshold for cross-correlations, no flagging based on auto-correlations
    tasks.FlagTask.params.threshold.crosscorr = 10
    # zero flagged visibilities
    task.FlagTask.params.zeroflagged = true
    # type of the task
    tasks.FlagTask.type = FlagTask

    


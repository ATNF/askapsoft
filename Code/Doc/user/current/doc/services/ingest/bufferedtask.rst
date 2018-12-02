BufferedTask
============

BufferedTask is an adapter which executes another task (called child task) in an asynchronous fashion.
The data are sent to ingest pipeline asynchronously and it takes a significant fraction of a cycle to
just assemble the data structure. Some tasks require all data to be present (e.g. writing the data to
disk by :doc:`mssink`) but do not alter the data in any way. This task creates a copy of the data and
buffers them for the service thread to execute the child task on these data. Provided the child 
fundamentally keeps up with data rate and there is no issues related to memory bandwidth, etc slowing
down the rest of the ingest pipeline, this approach gives more time for the child task to finish.
In particular, it is found useful to wrap the MS writing this way. Any task which does not alter 
data or data distribution pattern (except on the first iteration which is executed in the main thread
as normal to allow automatic distribution patterns) can be used. If the first condition
is not the case and the data are altered by the child task, the modifications are simply lost.
If the latter condition is not fulfilled - the result is unpredictable and can cause MPI
lock-ups or crashes. It, therefore, requires an expert user to make the decision about ingest 
configuration when it comes to buffering. 

Configuration Parameters
------------------------

Ingest pipeline requires a configuration file be provided on the command line. This
section describes the valid parameters applicable to this particular task.
These parameters need to be defined only if this task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters). The type of
the task defined by tasks.\ **name**\ .type should be set to *BufferedTask*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|child                       |string             |None        |The (logical) name of the child task used with this instance  |
|                            |                   |            |of buffered task (there could be more than one buffered task  |
|                            |                   |            |in the *tasklist* wrapping around different tasks, e.g.       |
|                            |                   |            |:doc:`mssink` and :doc:`tcpsink`). Any name which can be used |
|                            |                   |            |in *tasklist* is acceptable.                                  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|lossless                    |boolean            |true        |Flag showing what to do if the child task is found to not     |
|                            |                   |            |keep up and the timeout defined by *maxwait* has expired.     |
|                            |                   |            |If true, an exception is thrown and data acquision is stopped.|
|                            |                   |            |Otherwise, the new data chunk is ingored and processing       |
|                            |                   |            |continues (with an error message in the log).                 |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|size                        |unsigned int       |1           |Size of the buffer. Although, in principle, having a longer   |
|                            |                   |            |buffer might help in the case of very variable execution times|
|                            |                   |            |for the child, if it fundamentally keeps up, the general      |
|                            |                   |            |buffering mechanism of ingest pipeline's Source task should be|
|                            |                   |            |sufficient.                                                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|maxwait                     |unsigned int       |30          |Maximum waiting time before triggering the timeout condition  |
|                            |                   |            |(see also *lossless*), if the buffer is full.                 |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Example
~~~~~~~

.. code-block:: bash

    ########################## BufferedTask ##############################

    # the following line would force ingest to use orginary unbuffered MSSink
    #tasks.tasklist = [MergedSource, CalcUVWTask, MSSink]
    # and this task list triggers buffering
    tasks.tasklist = [MergedSource, CalcUVWTask, BufferedMSSink]

     
    # type of the task
    tasks.BufferedMSSink.type = BufferedMSSink
    # child to be wrapped around - MSSink in this instance
    tasks.BufferedMSSink.params.child = MSSink

    # ordinary setup of MSSink task

    # the measurement sets will be created in the current directory and named like 2015-12-22_162617.ms
    tasks.MSSink.params.filename = %d_%t.ms
    # pointing table will be written
    tasks.MSSink.params.pointingtable.enable = true
    # type of the task
    tasks.MSSink.type = MSSink
    


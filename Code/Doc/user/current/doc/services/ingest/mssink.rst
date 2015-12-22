MSSink     
======

MSSink task writes all active data streams into a separate measurement set, so a number
of measurement sets are written in the parallel case (and name should be chosen
accordingly to avoid conflicts).

Configuration Parameters
------------------------

The program requires a configuration file be provided on the command line. This
section describes the valid parameters. In addition to mandatory parameters which are
always required, individual tasks often have specific parameters which need to be
defined only if a particular task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters, i.e. writing full resolution
measurement set with one name and reduced resolution into another file). The type of
the task defined by tasks.\ **name**\ .type should be set to *MSSink*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|filename                    |string             |None        |Name of the measurement set to be written. The name can have  |
|                            |                   |            |wildcards which are substituted at the run time. **%d** is    |
|                            |                   |            |substituted with the UT date in the "YYYY-MM-DD" format,      |
|                            |                   |            |**%t** is substituted with the UT time in the "HHMMSS" format,|
|                            |                   |            |**%w** is substituted by the rank (worker) number, **%s** is  |
|                            |                   |            |substituted with the stream number (i.e. inactive ranks are   |
|                            |                   |            |not counted). Note, the result of substitution are made       |
|                            |                   |            |consistent across all ranks, i.e. it is guaranteed that the   |
|                            |                   |            |date-stamped name will be the same for all streams. In the    |
|                            |                   |            |parallel case, the date and time are taken at the moment when |
|                            |                   |            |first record is written to the measurement set (to be able to |
|                            |                   |            |count active ranks, which is a run time information). In the  |
|                            |                   |            |serial mode, the date and time are taken up front when the    |
|                            |                   |            |ingest pipeline is initialised with all its tasks.            |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|stman,bucketsize            |int                |131072      |Bucket size parameter of the storage manager associated with  |
|                            |                   |            |the measurement set (i.e. buffer size)                        |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|stman.tilencorr             |int                |4           |Tiling of the visibility cube in the polarisation dimension.  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|stman.tilenchan             |int                |1           |Number of channels in a single cube tile (affects caching).   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|pointingtable.enable        |boolean            |false       |If true, pointing table will be written for each integration. |
|                            |                   |            |Note, it contains non-standard columns to get raw azimuth,    |
|                            |                   |            |elevation and third axis position.                            | 
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## MSSink ##############################

    tasks.tasklist = [MergedSource, Merge, CalcUVWTask, FringeRotationTask, MSSink, TCPSink]

    # the measurement sets will be created in the current directory and named like 2015-12-22_162617.ms
    tasks.MSSink.params.filename = %d_%t.ms
    # pointing table will be written
    tasks.MSSink.params.pointingtable.enable = true
    # storage manager parameters (caching fine tuning)
    tasks.MSSink.params.stman.bucketsize = 131072
    tasks.MSSink.params.stman.tilenchan = 216
    tasks.MSSink.params.stman.tilencorr = 4
    # type of the task
    tasks.MSSink.type = MSSink


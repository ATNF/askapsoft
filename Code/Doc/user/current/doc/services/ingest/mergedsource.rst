MergedSource  
============

MergedSource task is the main source task intended to be used in the production system. It is named
such because it merges visibility stream (received by VisSource) and metadata stream (received by 
MetadataSource) by aligning them using time. The user does not need to setup VisSource and MetadataSource
explicitly, they are instantiated inside this task. The metadata access requires appropriate *Ice*
configuration, see the main :doc:`index` page for details.

Configuration Parameters
------------------------

Ingest pipeline requires a configuration file be provided on the command line. This
section describes the valid parameters applicable to this particular task.
These parameters need to be defined only if this task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist*\ .
The type of the task defined by tasks.\ **name**\ .type should be set to *MergedSource*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|n_channels.\ **N**          |unsigned int       |None        |The number of channels handled by the **N** rank. The full    |
|                            |                   |            |bandwidth (or channel space) does not have to be sliced in    |
|                            |                   |            |equal chunks for parallel processing. These keywords define   |
|                            |                   |            |the number of channels handled by each rank. This is the      |
|                            |                   |            |number of spectral channels the source task is expected to    |
|                            |                   |            |receive for each integration. Note, **N** may not only be a   |
|                            |                   |            |number, but also a string in the form **start..end**, in this |
|                            |                   |            |case ranks from **start** to **end** inclusive will be set up |
|                            |                   |            |the same number of channels                                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|maxbeams                    |unsigned int       |0           |Number of beams to allocate for the data chunk passed through |
|                            |                   |            |the task chain. If data are written to the disk, this is the  |
|                            |                   |            |number of beams in the measurement set to be created. The     |
|                            |                   |            |default is zero which means the smallest number of beams      |
|                            |                   |            |covering the selected beam mapping configuration (see beammap)|
|                            |                   |            |and rejection at source criterion (see vis_source.max_beamid) |
|                            |                   |            |as well as the total number defined in the beam configuration.|
|                            |                   |            |If explicit **beammap** is given, the number cannot be less   |
|                            |                   |            |than the number of beams required by the map (but can be more,|
|                            |                   |            |if desired, although it would bloat the size of the output    |
|                            |                   |            |measurement set). If no explicit map is given, this number    |
|                            |                   |            |can be less than the total number of beams configured, i.e.   |
|                            |                   |            |sent by the hardware and passed through VisSource-based       |
|                            |                   |            |rejection criterion. Use this if you want to record only a    |
|                            |                   |            |small number of beams.                                        |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|beammap                     |string             |""          |Explicit beam map between hardware beam indices and the       |
|                            |                   |            |0-based indices used in the measurement set (and throughout   |
|                            |                   |            |the ingest pipeline). By default (i.e. an empty string), the  |
|                            |                   |            |continuous 1-based to 0-based mapping is set up up to either  |
|                            |                   |            |**maxbeams** or the number of beams expected to be received   |
|                            |                   |            |by VisSource (i.e. number of beams defined in the main        |
|                            |                   |            |configuration subject to vis_source.max_beamid filtering),    |
|                            |                   |            |whichever is less. Otherwise, the string is expected to have  |
|                            |                   |            |comma-separated pairs of "hw_index:ms_index" in any order.    |
|                            |                   |            |The non-zero **maxbeams** parameter should be large enough to |
|                            |                   |            |include the explicit beam map. Use this keyword if you want   |
|                            |                   |            |to record a subset of non-contiguous physical beams or        |
|                            |                   |            |physical beams not starting from the beginning. For example,  |
|                            |                   |            |"1:0,3:1,5:2" maps hardware beam IDs of 1, 3 and 5 to         |
|                            |                   |            |measurement set indices 0, 1 and 2. In the absense of         |
|                            |                   |            |**maxbeams**, the measurement set will contain only 3 beams.  |
|                            |                   |            |Note, however, that the main beam configuration of the        |
|                            |                   |            |:doc:`index` should contain at least 5 beams of which the     |
|                            |                   |            |first three should match physical beams 1, 3 and 5 and the    |
|                            |                   |            |other two are ignored.                                        |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|buffer_size                 |unsigned int       |89856       |The circular buffer size for visibility datagrams. Ideally,   |
|                            |                   |            |this number should be large enough to include two complete    |
|                            |                   |            |integrations. The units are datagrams. The default value is   |
|                            |                   |            |two integrations of BETA datagrams.                           |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
| the following parameters have an additional **vis_source** prefix                                                          |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|receive_buffer_size         |unsigned int       |16777216    |The size of the asio receive buffer in bytes. Passed to the   |
|                            |                   |            |library as is.                                                |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|port                        |unsigned int       |None        |Port number listened by the rank 0 instance. Other ranks      |
|                            |                   |            |listen **port**\ +rank.                                       |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|max_beamid                  |unsigned int       |9           |Datagrams with hardware beam ID exceeding this number are     |
|                            |                   |            |rejected at the VisSource without buffering or checking the   |
|                            |                   |            |beam configuration/map. This relaxes performance requirements |
|                            |                   |            |and is the option which is required for the current BETA setup|
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|max_slice                   |unsigned int       |15          |Datagrams with slice ID exceeding this number are rejected at |
|                            |                   |            |the VisSource without buffering or further processing. The    |
|                            |                   |            |default value doesn't reject anything for either ASKAP or BETA|
|                            |                   |            |(note the meaning of the slice changed between BETA and       |
|                            |                   |            |ASKAP). Use this parameter if performance is limited, and     |
|                            |                   |            |slices with higher numbers are not used anyway).              |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## MergedSource ##############################

    tasks.tasklist = [MergedSource, CalcUVWTask, TCPSink]

    # record only 9 beams, discard the rest (determined by the
    # beam configuration (see the main ingest pipeline documentation page) 
    tasks.MergedSource.params.maxbeams = 9
    # channel distribution for each rank
    tasks.MergedSource.params.n_channels.0 = 216
    tasks.MergedSource.params.n_channels.1 = 216
    tasks.MergedSource.params.n_channels.10 = 216
    tasks.MergedSource.params.n_channels.11 = 216
    tasks.MergedSource.params.n_channels.2 = 216
    tasks.MergedSource.params.n_channels.3 = 216
    tasks.MergedSource.params.n_channels.4 = 216
    tasks.MergedSource.params.n_channels.5 = 216
    tasks.MergedSource.params.n_channels.6 = 216
    tasks.MergedSource.params.n_channels.7 = 216
    tasks.MergedSource.params.n_channels.8 = 216
    tasks.MergedSource.params.n_channels.9 = 216
    # visibility source details
    # do not reject any beams
    tasks.MergedSource.params.vis_source.max_beamid = 36
    # reject slices with ID of 1 and above, for ASKAP it means
    # baselines up to antenna 16. We use this for tests at MRO
    tasks.MergedSource.params.vis_source.max_slice = 0
    # port to receive visibility data from (for rank 0, other ranks listen
    # port number equal to this parameter + rank)
    tasks.MergedSource.params.vis_source.port = 16384
    # UDP receive buffer size in bytes (the value we used for ASKAP6 as in Nov2015)
    tasks.MergedSource.params.vis_source.receive_buffer_size = 67108864
    # type of the task
    tasks.MergedSource.type = MergedSource

    


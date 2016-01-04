NoMetadataSource  
================

NoMetadataSource is a source task which does not depend on the metadata stream from the 
Telescope Operating System (TOS). It uses the same classes to receive visibility datagrams
and decode the content as the main :doc:`mergedsource` task, but takes additional information,
which is normally supplied via the metadata, from the configuration parameters. This is often handy for
debugging because one could have the ingest pipeline loosely coupled to the rest of the system, and,
therefore, being started and stopped manually at any time (as opposed to being started by TOS when
the observations start with the data ingest commencing only when the appropriate metadata are received).
As this task does not use the metadata stream at all, the metadata-specific  *Ice* configuration 
(see the main :doc:`index` page for details) is ignored. Note, the metadata supplied via the parset 
essentially lead to a static configuration with a single scan which cannot be changed without stopping
the ingest pipeline. This unlike :doc:`mergedsource`\ , which receives the same information via the
metadata and, therefore, can start new scans dynamically. Another consequence of this is inability
to stop based on the special **end of observation** flag in the metadata. If **NoMetadataSource** task
is used, the ingest pipeline has to be stopped via **Ctrl+C** or a signal.

Configuration Parameters
------------------------

Ingest pipeline requires a configuration file to be provided on the command line. This
section describes the valid parameters applicable to this particular task.
These parameters need to be defined only if this task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist*\ .
The type of the task defined by tasks.\ **name**\ .type should be set to *NoMetadataSource*.
As mentioned above, this task is different from :doc:`mergedsource` in the way the information normally
supplied in the metadata stream is obtained. Therefore, all configuration parameters of the :doc:`mergedsource`
are understood by **NoMetadataSource**\ . The table below contains additional parameters understood by this
task which fill in for the metadata content. 


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|centre_freq                 |quantity string    |None        |The central frequency of the observed band. This is the same  |
|                            |                   |            |value which is specified in the scheduling block parset and   |
|                            |                   |            |distributed via the metadata when :doc:`mergedsource` is used.|
|                            |                   |            |Note, in the parallel mode, this is the centre of the band    |
|                            |                   |            |corresponding to the first correlator card (rather than the   |
|                            |                   |            |centre of the combined band).                                 |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|target_name                 |string             |None        |Name of the target. This information is used just to fill the |
|                            |                   |            |appropriate field in the measurement set.                     |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|target_direction            |quantity string    |None        |Direction to the observed field. It is used to fill the       |
|                            |                   |            |appropriate field in the measurement set and can be treated as|
|                            |                   |            |a phase centre if ingest pipeline does fringe rotation.       |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|correlator_mode             |string             |None        |Correlator mode (see main :doc:`index` configuration) used for|
|                            |                   |            |observations.                                                 |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## NoMetadataSource ##############################

    tasks.tasklist = [NoMetadataSource, CalcUVWTask, TCPSink]

    # record only 9 beams, discard the rest (determined by the
    # beam configuration (see the main ingest pipeline documentation page) 
    tasks.NoMetadataSource.params.maxbeams = 9
    # channel distribution - assuming serial mode, i.e. only value for 
    # rank zero is defined 
    tasks.NoMetadataSource.params.n_channels.0 = 216
    # visibility source details
    # do not reject any beams
    tasks.NoMetadataSource.params.vis_source.max_beamid = 36
    # reject slices with ID of 1 and above, for ASKAP it means
    # baselines up to antenna 16. We use this for tests at MRO
    tasks.NoMetadataSource.params.vis_source.max_slice = 0
    # port to receive visibility data from (for rank 0, other ranks would listen
    # port number equal to this parameter + rank, if we used this setup in the parallel mode)
    tasks.NoMetadataSource.params.vis_source.port = 16384
    # UDP receive buffer size in bytes (the value we used for ASKAP6 as in Nov2015)
    tasks.NoMetadataSource.params.vis_source.receive_buffer_size = 67108864
    # frequency of the band centre
    tasks.NoMetadataSource.params.centre_freq = 0.9175GHz
    # field name, this is just written to the FIELD table
    tasks.NoMetadataSource.params.target_name = test-field1
    # field centre/phase centre: Virgo
    # in the task configuration as above (i.e. without ingest controlled phase tracking),
    # this is just a piece of metadata to be written into the FIELD table
    tasks.NoMetadataSource.params.target_direction = [12h30m49.43, +12d23m28.100, J2000]
    # correlator mode
    tasks.NoMetadataSource.params.correlator_mode = standard
    # circular buffer size (in datagrams). We used this value in August 2014 commissioning run
    tasks.NoMetadataSource.params.buffer_size = 15552
    # type of the task
    tasks.NoMetadataSource.type = NoMetadataSource

    

